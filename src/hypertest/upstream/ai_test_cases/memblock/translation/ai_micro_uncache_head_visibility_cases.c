#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_INVALIDATE_CACHE_ALIAS(cache_vaddr, message) \
    do { \
        fence_iorw_iorw(); \
        TEST_SETUP_EXCEPT(); \
        cbo_inval(cache_vaddr); \
        AI_ASSERT_NO_EXCEPT_AND(message, true); \
    } while (0)

#define AI_MICRO_EXPECT_MMIO_CBO_INVAL_SAF(cache_vaddr, message) \
    do { \
        fence_iorw_iorw(); \
        TEST_SETUP_EXCEPT(); \
        cbo_inval(cache_vaddr); \
        TEST_ASSERT(message, \
            excpt.triggered == true && \
            excpt.cause == CAUSE_SAF && \
            excpt.tval == (cache_vaddr)); \
    } while (0)

#define AI_MICRO_HEAD_CACHE_ALIAS_PAGE  VSRWXPbmt0_GURWXPbmt0
#define AI_MICRO_HEAD_NC_ALIAS_PAGE     VSRWXPbmt1_GURWXPbmt1
#define AI_MICRO_HEAD_MMIO_ALIAS_PAGE   VSRWX_GRWX
#define AI_MICRO_HEAD_MMIO_BACKING_PAGE SCRATCHPAD
#define AI_MICRO_HEAD_MMIO_MEM_BASE     0x30000000000ULL

static inline uintptr_t ai_micro_uncache_head_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_uncache_head_mmio_backing_paddr(void)
{
    return phys_page_base_offset(AI_MICRO_HEAD_MMIO_BACKING_PAGE,
        AI_MICRO_HEAD_MMIO_MEM_BASE);
}

static inline void ai_micro_uncache_head_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0x0fULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_uncache_head_reset_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    ai_micro_uncache_head_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_uncache_head_reset_pbmt_only(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_prepare_nc_cache_pair(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x(AI_MICRO_HEAD_CACHE_ALIAS_PAGE);
    pbmt_hspt_to_x(AI_MICRO_HEAD_NC_ALIAS_PAGE);
    sfence_vma();

    return hs_page_base(AI_MICRO_HEAD_NC_ALIAS_PAGE);
}

static inline uintptr_t ai_micro_prepare_mmio_cache_pair(void)
{
    uintptr_t backing_paddr = ai_micro_uncache_head_mmio_backing_paddr();

    ai_micro_uncache_head_reset_env();

    /*
     * Keep this as a real PMA/IO-region probe: LinkNan reset PMA maps the
     * 3T window as responsive non-cacheable IO memory.  Reprogramming custom
     * PMA CSRs here changes the platform setup and is not modeled by Spike.
     */
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    hspt_init();
    hspt_leaf_change_base_paddr(AI_MICRO_HEAD_CACHE_ALIAS_PAGE,
        ai_micro_uncache_head_anchor(AI_MICRO_HEAD_CACHE_ALIAS_PAGE, backing_paddr));
    hspt_leaf_change_base_paddr(AI_MICRO_HEAD_MMIO_ALIAS_PAGE,
        ai_micro_uncache_head_anchor(AI_MICRO_HEAD_MMIO_ALIAS_PAGE, backing_paddr));
    sfence_vma();

    return hs_page_base(AI_MICRO_HEAD_MMIO_ALIAS_PAGE);
}

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

static inline void ai_micro_uncache_probe_ld_x0(uintptr_t vaddr)
{
    asm volatile("ld x0, 0(%0)" :: "r"(vaddr) : "memory");
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

bool ai_micro_hs_load_uncache_replay_head_data_visible()
{
    TEST_START();

    uintptr_t cache_vaddr = hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE);
    uintptr_t nc_paddr = phys_page_base(X);
    uintptr_t mmio_paddr = ai_micro_uncache_head_mmio_backing_paddr();
    const uint64_t nc_initial = 0x1122334455667788ULL;
    const uint64_t nc_updated = 0x8877665544332211ULL;
    const uint64_t mmio_initial = 0x1021324354657687ULL;
    const uint64_t mmio_updated = 0xa1b2c3d4e5f60718ULL;

    uintptr_t nc_vaddr = ai_micro_prepare_nc_cache_pair();

    goto_priv(PRIV_M);
    write64(nc_paddr, nc_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_nc = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable alias warms with the original payload before the replay-head NC load probe",
        warmed_nc == nc_initial
    );

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, nc_updated);
    AI_ASSERT_NO_EXCEPT_AND("the setup NC store refreshes backing memory without surfacing an exception before the replay-head NC load probe",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t stale_nc = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable alias still observes stale data before the replay-head NC load is issued",
        stale_nc == nc_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_nc = ld(nc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS replay-head load through the NC path fetches the refreshed backing payload",
        observed_nc == nc_updated
    );

    uintptr_t mmio_vaddr = ai_micro_prepare_mmio_cache_pair();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(mmio_vaddr, mmio_initial);
    AI_ASSERT_NO_EXCEPT_AND("the setup store through the default 3T PMA/IO alias completes before the replay-head MMIO-style load probe",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_mmio = ld(mmio_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a load through the default 3T PMA/IO alias observes the initial payload before the refresh store",
        warmed_mmio == mmio_initial
    );

    TEST_SETUP_EXCEPT();
    sd(mmio_vaddr, mmio_updated);
    AI_ASSERT_NO_EXCEPT_AND("the refresh store through the default 3T PMA/IO alias completes before the replay-head MMIO-style load probe",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_mmio = ld(mmio_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS replay-head load through the default 3T PMA/IO path fetches the refreshed backing payload",
        observed_mmio == mmio_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the replay-head MMIO load probe leaves backing memory at the expected refreshed payload",
        read64(mmio_paddr) == mmio_updated
    );
    ai_micro_uncache_head_reset_env();

    TEST_END();
}

static bool ai_micro_uncache_redirect_adjacent_uncache_load_window(bool uncache_first,
    bool repeat_rounds)
{
    const enum test_page survivor_first_page = SWITCH1;
    const enum test_page survivor_second_page = VSRW_GRW;
    const uintptr_t cache_alias_vaddr = hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE);
    const uintptr_t bad_load_base_vaddr = hs_page_base(VSI_GI) + 0x180U;
    const uintptr_t survivor_first_offset = 0x120U;
    const uintptr_t survivor_second_offset = 0x1a0U;
    const uintptr_t survivor_first_vaddr =
        hs_page_base(survivor_first_page) + survivor_first_offset;
    const uintptr_t survivor_second_vaddr =
        hs_page_base(survivor_second_page) + survivor_second_offset;
    const uintptr_t survivor_first_paddr =
        phys_page_base(survivor_first_page) + survivor_first_offset;
    const uintptr_t survivor_second_paddr =
        phys_page_base(survivor_second_page) + survivor_second_offset;
    const uintptr_t uncache_paddr = phys_page_base(X);
    const unsigned rounds = repeat_rounds ? 3U : 1U;
    static const uint64_t survivor_first_values[] = {
        0x1021324354657687ULL,
        0x2031425364758697ULL,
        0x30415263748596a7ULL,
    };
    static const uint64_t survivor_second_values[] = {
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
        0xaabbccddeeff0011ULL,
    };
    static const uint64_t uncache_initial_values[] = {
        0x0112233445566778ULL,
        0x1223344556677889ULL,
        0x233445566778899aULL,
    };
    static const uint64_t uncache_updated_values[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    bool test_status = true;

    for (unsigned round = 0; round < rounds; ++round) {
        const uintptr_t bad_load_vaddr =
            bad_load_base_vaddr + (uintptr_t)round * sizeof(uint64_t);
        const uint64_t survivor_first_value = survivor_first_values[round];
        const uint64_t survivor_second_value = survivor_second_values[round];
        const uint64_t uncache_initial_value = uncache_initial_values[round];
        const uint64_t uncache_updated_value = uncache_updated_values[round];
        uintptr_t uncache_vaddr = ai_micro_prepare_nc_cache_pair();

        goto_priv(PRIV_M);
        write64(survivor_first_paddr, survivor_first_value);
        write64(survivor_second_paddr, survivor_second_value);

        goto_priv(PRIV_HS);
        AI_MICRO_INVALIDATE_CACHE_ALIAS(cache_alias_vaddr,
            "before seeding the redirect-adjacent VLQ uncache-late-paddr window, invalidating the cacheable alias line leaves the probe setup exception-free");

        TEST_SETUP_EXCEPT();
        sd(uncache_vaddr, uncache_initial_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias seeds the original payload before the redirect-adjacent VLQ uncache-late-paddr window",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_survivor_first = ld(survivor_first_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent VLQ/MMIO window starts, the first surviving cacheable load target reads back its seeded payload",
            warmed_survivor_first == survivor_first_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_survivor_second = ld(survivor_second_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent VLQ/MMIO window starts, the second surviving cacheable load target reads back its seeded payload",
            warmed_survivor_second == survivor_second_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_uncache_alias = ld(cache_alias_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent VLQ uncache-late-paddr window starts, the cacheable alias warms with the original uncache backing payload",
            warmed_uncache_alias == uncache_initial_value
        );

        TEST_SETUP_EXCEPT();
        sd(uncache_vaddr, uncache_updated_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias refreshes backing memory before the uncache replay-head load is issued",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_probe_ld_x0(cache_alias_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after the uncache backing is updated but before the uncache replay-head load is issued, the potentially stale cacheable alias probe still completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        (void)ld(bad_load_vaddr);
        TEST_ASSERT("the redirect-driving invalid-leaf load still reports load page fault before the immediate redirect-adjacent cacheable/uncache load window",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == bad_load_vaddr
        );

        if (uncache_first) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_uncache = ld(uncache_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent VLQ uncache-late-paddr window, the uncache replay-head load fetches only its own refreshed backing payload",
                observed_uncache == uncache_updated_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_first = ld(survivor_first_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the redirect-adjacent uncache replay-head load, the first surviving cacheable load still returns only its own payload",
                observed_survivor_first == survivor_first_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_second = ld(survivor_second_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the redirect-adjacent uncache replay-head load, the second surviving cacheable load still returns only its own payload",
                observed_survivor_second == survivor_second_value
            );
        } else {
            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_first = ld(survivor_first_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent VLQ uncache-late-paddr window, the first surviving cacheable load still returns only its own payload before the uncache replay-head load arrives",
                observed_survivor_first == survivor_first_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_uncache = ld(uncache_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent VLQ uncache-late-paddr window, the later uncache replay-head load still fetches only its own refreshed backing payload",
                observed_uncache == uncache_updated_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_second = ld(survivor_second_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the redirect-adjacent uncache replay-head load, the second surviving cacheable load still returns only its own payload",
                observed_survivor_second == survivor_second_value
            );
        }

        fence_iorw_iorw();
        goto_priv(PRIV_M);
        TEST_ASSERT("the redirect-adjacent VLQ uncache-late-paddr window leaves both surviving cacheable targets with their expected payloads",
            read64(survivor_first_paddr) == survivor_first_value &&
            read64(survivor_second_paddr) == survivor_second_value
        );

        uncache_first = !uncache_first;
    }

    return test_status;
}

bool ai_micro_hs_virtual_lq_redirect_adjacent_enq_no_ptr_corruption_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_uncache_redirect_adjacent_uncache_load_window(false, false);
    TEST_END();
}

bool ai_micro_hs_virtual_lq_mmio_late_paddr_update_after_redirect_hits_right_slot_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_uncache_redirect_adjacent_uncache_load_window(true, true);
    TEST_END();
}

static bool ai_micro_uncache_redirect_adjacent_narrow_load_window()
{
    const enum test_page survivor_first_page = SWITCH1;
    const enum test_page survivor_second_page = VSRW_GRW;
    const uintptr_t target_offset = 0x40U;
    const uintptr_t cache_alias_line_vaddr =
        hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE) + target_offset;
    const uintptr_t bad_load_base_vaddr = hs_page_base(VSI_GI) + 0x1c0U;
    const uintptr_t survivor_first_offset = 0x120U;
    const uintptr_t survivor_second_offset = 0x1a0U;
    const uintptr_t survivor_first_vaddr =
        hs_page_base(survivor_first_page) + survivor_first_offset;
    const uintptr_t survivor_second_vaddr =
        hs_page_base(survivor_second_page) + survivor_second_offset;
    const uintptr_t survivor_first_paddr =
        phys_page_base(survivor_first_page) + survivor_first_offset;
    const uintptr_t survivor_second_paddr =
        phys_page_base(survivor_second_page) + survivor_second_offset;
    const uintptr_t uncache_target_paddr = phys_page_base(X) + target_offset;
    static const unsigned load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        2U,
        4U,
    };
    static const uint64_t survivor_first_values[] = {
        0x1021324354657687ULL,
        0x2031425364758697ULL,
        0x30415263748596a7ULL,
    };
    static const uint64_t survivor_second_values[] = {
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
        0xaabbccddeeff0011ULL,
    };
    static const uint64_t uncache_initial_values[] = {
        0x1817161514131211ULL,
        0x2726252423222120ULL,
        0x3736353433323130ULL,
    };
    static const uint64_t uncache_updated_values[] = {
        0x8070605040302010ULL,
        0x8f7e6d5c4b3a2918ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    bool test_status = true;
    bool uncache_first = true;

    for (unsigned round = 0; round < (sizeof(load_sizes) / sizeof(load_sizes[0])); ++round) {
        const unsigned load_size = load_sizes[round];
        const unsigned load_offset = load_offsets[round];
        const uintptr_t bad_load_vaddr =
            bad_load_base_vaddr + (uintptr_t)round * sizeof(uint64_t);
        const uintptr_t uncache_target_vaddr =
            ai_micro_prepare_nc_cache_pair() + target_offset;
        const uint64_t survivor_first_value = survivor_first_values[round];
        const uint64_t survivor_second_value = survivor_second_values[round];
        const uint64_t uncache_initial_value = uncache_initial_values[round];
        const uint64_t uncache_updated_value = uncache_updated_values[round];
        const uint64_t expected_narrow_value =
            ai_micro_uncache_extract_unsigned(uncache_updated_value, load_offset, load_size);

        goto_priv(PRIV_M);
        write64(survivor_first_paddr, survivor_first_value);
        write64(survivor_second_paddr, survivor_second_value);

        goto_priv(PRIV_HS);
        AI_MICRO_INVALIDATE_CACHE_ALIAS(cache_alias_line_vaddr,
            "before seeding the redirect-adjacent narrow-load window, invalidating the cacheable alias line leaves the probe setup exception-free");

        TEST_SETUP_EXCEPT();
        sd(uncache_target_vaddr, uncache_initial_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias seeds the original payload before the redirect-adjacent narrow-load window",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_survivor_first = ld(survivor_first_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent narrow-load window starts, the first surviving cacheable load target reads back its seeded payload",
            warmed_survivor_first == survivor_first_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_survivor_second = ld(survivor_second_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent narrow-load window starts, the second surviving cacheable load target reads back its seeded payload",
            warmed_survivor_second == survivor_second_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_uncache_alias = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent narrow-load window starts, the cacheable alias warms with the original uncache backing payload",
            warmed_uncache_alias == uncache_initial_value
        );

        TEST_SETUP_EXCEPT();
        sd(uncache_target_vaddr, uncache_updated_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias refreshes backing memory before the redirect-adjacent narrow uncache load is issued",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_probe_ld_x0(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after the uncache backing is updated but before the redirect-adjacent narrow uncache load is issued, the potentially stale cacheable alias probe still completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        (void)ld(bad_load_vaddr);
        TEST_ASSERT("the redirect-driving invalid-leaf load still reports load page fault before the redirect-adjacent narrow uncache load window",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == bad_load_vaddr
        );

        if (uncache_first) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_uncache_narrow =
                ai_micro_uncache_narrow_load(load_size, uncache_target_vaddr + load_offset);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent narrow-load window, the uncache replay-head load still returns the byte lane selected by its own refreshed offset",
                observed_uncache_narrow == expected_narrow_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_first = ld(survivor_first_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the redirect-adjacent narrow uncache load, the first surviving cacheable load still returns only its own payload",
                observed_survivor_first == survivor_first_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_second = ld(survivor_second_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the redirect-adjacent narrow uncache load, the second surviving cacheable load still returns only its own payload",
                observed_survivor_second == survivor_second_value
            );
        } else {
            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_first = ld(survivor_first_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent narrow-load window, the first surviving cacheable load still returns only its own payload before the narrow uncache replay-head load arrives",
                observed_survivor_first == survivor_first_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_uncache_narrow =
                ai_micro_uncache_narrow_load(load_size, uncache_target_vaddr + load_offset);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent narrow-load window, the later uncache replay-head load still returns the byte lane selected by its own refreshed offset",
                observed_uncache_narrow == expected_narrow_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_second = ld(survivor_second_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the redirect-adjacent narrow uncache load, the second surviving cacheable load still returns only its own payload",
                observed_survivor_second == survivor_second_value
            );
        }

        fence_iorw_iorw();
        goto_priv(PRIV_M);
        TEST_ASSERT("the redirect-adjacent narrow uncache load window leaves both surviving cacheable targets with their expected payloads",
            read64(survivor_first_paddr) == survivor_first_value &&
            read64(survivor_second_paddr) == survivor_second_value
        );

        uncache_first = !uncache_first;
    }

    return test_status;
}

static bool ai_micro_uncache_redirect_adjacent_signedness_switch_window()
{
    const enum test_page survivor_first_page = SWITCH1;
    const enum test_page survivor_second_page = VSRW_GRW;
    const uintptr_t target_offset = 0x40U;
    const uintptr_t cache_alias_line_vaddr =
        hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE) + target_offset;
    const uintptr_t bad_load_base_vaddr = hs_page_base(VSI_GI) + 0x240U;
    const uintptr_t survivor_first_offset = 0x160U;
    const uintptr_t survivor_second_offset = 0x1e0U;
    const uintptr_t survivor_first_vaddr =
        hs_page_base(survivor_first_page) + survivor_first_offset;
    const uintptr_t survivor_second_vaddr =
        hs_page_base(survivor_second_page) + survivor_second_offset;
    const uintptr_t survivor_first_paddr =
        phys_page_base(survivor_first_page) + survivor_first_offset;
    const uintptr_t survivor_second_paddr =
        phys_page_base(survivor_second_page) + survivor_second_offset;
    const uintptr_t uncache_target_paddr = phys_page_base(X) + target_offset;
    static const unsigned load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        7U,
        2U,
        2U,
        4U,
        4U,
    };
    static const bool signed_loads[] = {
        true,
        false,
        true,
        false,
        true,
        false,
    };
    static const uint64_t survivor_first_values[] = {
        0x1021324354657687ULL,
        0x2031425364758697ULL,
        0x30415263748596a7ULL,
        0x405162738495a6b7ULL,
        0x5061728394a5b6c7ULL,
        0x60718293a4b5c6d7ULL,
    };
    static const uint64_t survivor_second_values[] = {
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
        0xaabbccddeeff0011ULL,
        0xbbccddeeff001122ULL,
        0xccddeeff00112233ULL,
        0xddeeff0011223344ULL,
    };
    static const uint64_t uncache_initial_values[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
        0x3837363534333231ULL,
        0x4847464544434241ULL,
        0x5857565554535251ULL,
        0x6867666564636261ULL,
    };
    static const uint64_t uncache_updated_values[] = {
        0x8070605040302010ULL,
        0x8171615141312111ULL,
        0x7060508f7e302010ULL,
        0x716151907f312111ULL,
        0xf0e1d2c3b4a59687ULL,
        0xf1e2d3c4b5a69788ULL,
    };
    bool test_status = true;
    bool uncache_first = true;

    for (unsigned round = 0; round < (sizeof(load_sizes) / sizeof(load_sizes[0])); ++round) {
        const unsigned load_size = load_sizes[round];
        const unsigned load_offset = load_offsets[round];
        const bool signed_load = signed_loads[round];
        const uintptr_t bad_load_vaddr =
            bad_load_base_vaddr + (uintptr_t)round * sizeof(uint64_t);
        const uintptr_t uncache_target_vaddr =
            ai_micro_prepare_nc_cache_pair() + target_offset;
        const uint64_t survivor_first_value = survivor_first_values[round];
        const uint64_t survivor_second_value = survivor_second_values[round];
        const uint64_t uncache_initial_value = uncache_initial_values[round];
        const uint64_t uncache_updated_value = uncache_updated_values[round];
        const uint64_t expected_scalar_value =
            ai_micro_uncache_extract_scalar(uncache_updated_value, load_offset,
                load_size, signed_load);

        goto_priv(PRIV_M);
        write64(survivor_first_paddr, survivor_first_value);
        write64(survivor_second_paddr, survivor_second_value);

        goto_priv(PRIV_HS);
        AI_MICRO_INVALIDATE_CACHE_ALIAS(cache_alias_line_vaddr,
            "before seeding the redirect-adjacent signedness-switch uncache window, invalidating the cacheable alias line leaves the probe setup exception-free");

        TEST_SETUP_EXCEPT();
        sd(uncache_target_vaddr, uncache_initial_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias seeds the original payload before the redirect-adjacent signedness-switch uncache window",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_survivor_first = ld(survivor_first_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent signedness-switch uncache window starts, the first surviving cacheable load target reads back its seeded payload",
            warmed_survivor_first == survivor_first_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_survivor_second = ld(survivor_second_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent signedness-switch uncache window starts, the second surviving cacheable load target reads back its seeded payload",
            warmed_survivor_second == survivor_second_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_uncache_alias = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the redirect-adjacent signedness-switch uncache window starts, the cacheable alias warms with the original uncache backing payload",
            warmed_uncache_alias == uncache_initial_value
        );

        TEST_SETUP_EXCEPT();
        sd(uncache_target_vaddr, uncache_updated_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias refreshes backing memory before the signedness-switch replay-head load is issued",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_probe_ld_x0(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after the uncache backing is updated but before the signedness-switch replay-head load is issued, the potentially stale cacheable alias probe still completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        (void)ld(bad_load_vaddr);
        TEST_ASSERT("the redirect-driving invalid-leaf load still reports load page fault before the signedness-switch replay-head uncache window",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == bad_load_vaddr
        );

        if (uncache_first) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_uncache_scalar =
                ai_micro_uncache_scalar_load(signed_load, load_size,
                    uncache_target_vaddr + load_offset);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent signedness-switch uncache window, the replay-head scalar load still returns the value selected by its own current width and signedness",
                observed_uncache_scalar == expected_scalar_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_first = ld(survivor_first_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the signedness-switch replay-head uncache load, the first surviving cacheable load still returns only its own payload",
                observed_survivor_first == survivor_first_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_second = ld(survivor_second_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the signedness-switch replay-head uncache load, the second surviving cacheable load still returns only its own payload",
                observed_survivor_second == survivor_second_value
            );
        } else {
            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_first = ld(survivor_first_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent signedness-switch uncache window, the first surviving cacheable load still returns only its own payload before the replay-head scalar load arrives",
                observed_survivor_first == survivor_first_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_uncache_scalar =
                ai_micro_uncache_scalar_load(signed_load, load_size,
                    uncache_target_vaddr + load_offset);
            AI_ASSERT_NO_EXCEPT_AND("in the redirect-adjacent signedness-switch uncache window, the later replay-head scalar load still returns the value selected by its own current width and signedness",
                observed_uncache_scalar == expected_scalar_value
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_survivor_second = ld(survivor_second_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("after the signedness-switch replay-head uncache load, the second surviving cacheable load still returns only its own payload",
                observed_survivor_second == survivor_second_value
            );
        }

        fence_iorw_iorw();
        goto_priv(PRIV_M);
        TEST_ASSERT("the redirect-adjacent signedness-switch uncache window leaves both surviving cacheable targets with their expected payloads",
            read64(survivor_first_paddr) == survivor_first_value &&
            read64(survivor_second_paddr) == survivor_second_value
        );

        uncache_first = !uncache_first;
    }

    return test_status;
}

bool ai_micro_hs_virtual_lq_uncache_late_paddr_narrow_scalar_lane_matches_offset_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_uncache_redirect_adjacent_narrow_load_window();
    TEST_END();
}

bool ai_micro_hs_store_mmio_storequeue_head_data_visible()
{
    TEST_START();

    uintptr_t cache_vaddr = hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE);
    uintptr_t mmio_paddr = ai_micro_uncache_head_mmio_backing_paddr();
    uintptr_t mmio_vaddr = ai_micro_prepare_mmio_cache_pair();
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t updated_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(mmio_paddr, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second alias to the default 3T PMA/IO line observes the original payload before the storequeue-head MMIO-style store probe",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(mmio_vaddr, updated_value);
    AI_ASSERT_NO_EXCEPT_AND("an HS storequeue-head MMIO-style store retires without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t alias_after_store = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second alias to the default 3T PMA/IO line observes the MMIO-style store payload without requiring cache invalidation",
        alias_after_store == updated_value
    );

    AI_MICRO_EXPECT_MMIO_CBO_INVAL_SAF(cache_vaddr,
        "cbo.inval on the default 3T PMA/IO alias reports Store AF without changing data");

    TEST_SETUP_EXCEPT();
    uint64_t refreshed_cached = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the CBO access-fault follow-up, the second alias still observes the MMIO-style store payload",
        refreshed_cached == updated_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_mmio = ld(mmio_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reading back through the MMIO-style alias observes the payload written by the storequeue-head store",
        observed_mmio == updated_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the storequeue-head MMIO-style store updates the backing memory with the expected payload",
        read64(mmio_paddr) == updated_value
    );
    ai_micro_uncache_head_reset_env();

    TEST_END();
}

bool ai_micro_hs_virtual_lq_uncache_late_paddr_narrow_scalar_signedness_matches_current_uop_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_uncache_redirect_adjacent_signedness_switch_window();
    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_storequeue_head_data_visible()
{
    TEST_START();

    uintptr_t cache_vaddr = hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE);
    uintptr_t nc_paddr = phys_page_base(X);
    uintptr_t nc_vaddr = ai_micro_prepare_nc_cache_pair();
    const uint64_t initial_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t updated_value = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(nc_paddr, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable alias warms with the original payload before the storequeue-head NC store probe",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, updated_value);
    AI_ASSERT_NO_EXCEPT_AND("an HS storequeue-head NC store retires without surfacing an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t stale_cached = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable alias still holds the stale payload before invalidation after the storequeue-head NC store",
        stale_cached == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_nc = ld(nc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reading back through the NC alias observes the payload written by the storequeue-head store",
        observed_nc == updated_value
    );

    ai_micro_uncache_head_reset_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_storequeue_head_narrow_scalar_lane_matches_offset_corner()
{
    TEST_START();

    const uintptr_t target_offset = 0x40U;
    const uintptr_t cache_alias_line_vaddr =
        hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE) + target_offset;
    const uintptr_t nc_target_paddr = phys_page_base(X) + target_offset;
    static const unsigned store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned store_offsets[] = {
        7U,
        2U,
        4U,
    };
    static const uint64_t initial_values[] = {
        0x1817161514131211ULL,
        0x2726252423222120ULL,
        0x3736353433323130ULL,
    };
    static const uint64_t store_values[] = {
        0x00000000000000a5ULL,
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
    };

    for (unsigned round = 0; round < (sizeof(store_sizes) / sizeof(store_sizes[0])); ++round) {
        const unsigned store_size = store_sizes[round];
        const unsigned store_offset = store_offsets[round];
        const uintptr_t nc_target_vaddr =
            ai_micro_prepare_nc_cache_pair() + target_offset;
        const uint64_t initial_value = initial_values[round];
        const uint64_t store_value = store_values[round];
        const uint64_t expected_final_value =
            ai_micro_uncache_merge_unsigned(initial_value, store_value, store_offset, store_size);

        goto_priv(PRIV_HS);
        AI_MICRO_INVALIDATE_CACHE_ALIAS(cache_alias_line_vaddr,
            "before seeding the storequeue-head narrow NC store probe, invalidating the cacheable alias line completes without exception");

        TEST_SETUP_EXCEPT();
        sd(nc_target_vaddr, initial_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias seeds the original payload before the storequeue-head narrow NC store probe",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the cacheable alias warms with the original payload before the storequeue-head narrow NC store probe",
            warmed == initial_value
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_narrow_store(store_size, nc_target_vaddr + store_offset, store_value);
        AI_ASSERT_NO_EXCEPT_AND("a storequeue-head narrow NC store retires without surfacing an exception",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_probe_ld_x0(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before invalidation after the storequeue-head narrow NC store, the potentially stale cacheable alias probe still completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_nc = ld(nc_target_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("reading back through the NC alias observes the exact byte-lane merge written by the storequeue-head narrow store",
            observed_nc == expected_final_value
        );
    }

    ai_micro_uncache_head_reset_pbmt_only();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_uncache_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner()
{
    TEST_START();

    const uintptr_t target_offset = 0x80U;
    const uintptr_t cache_alias_line_vaddr =
        hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE) + target_offset;
    const uintptr_t nc_target_paddr = phys_page_base(X) + target_offset;
    const uintptr_t nc_adjacent_paddr = nc_target_paddr + sizeof(uint64_t);
    static const uint64_t initial_values[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
    };
    static const uint64_t adjacent_values[] = {
        0x8877665544332211ULL,
        0x99aabbccddeeff00ULL,
    };
    static const unsigned store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned store_offsets[] = {
        7U,
        2U,
        4U,
    };
    static const uint64_t store_values[] = {
        0x00000000000000a5ULL,
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
    };
    bool reverse_order = false;

    for (unsigned round = 0; round < (sizeof(initial_values) / sizeof(initial_values[0])); ++round) {
        const uintptr_t nc_target_vaddr =
            ai_micro_prepare_nc_cache_pair() + target_offset;
        const uintptr_t nc_adjacent_vaddr = nc_target_vaddr + sizeof(uint64_t);
        const uint64_t initial_value = initial_values[round];
        const uint64_t adjacent_value = adjacent_values[round];
        uint64_t expected_final_value = initial_value;

        goto_priv(PRIV_HS);
        AI_MICRO_INVALIDATE_CACHE_ALIAS(cache_alias_line_vaddr,
            "before seeding the storequeue-head narrow width-switch probe, invalidating the cacheable alias line completes without exception");

        TEST_SETUP_EXCEPT();
        sd(nc_target_vaddr, initial_value);
        sd(nc_adjacent_vaddr, adjacent_value);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the NC alias seeds the target and adjacent words before the storequeue-head narrow width-switch probe",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the cacheable alias warms with the original payload before the storequeue-head narrow width-switch probe",
            warmed == initial_value
        );

        for (unsigned step = 0; step < (sizeof(store_sizes) / sizeof(store_sizes[0])); ++step) {
            const unsigned index = reverse_order ?
                (unsigned)(sizeof(store_sizes) / sizeof(store_sizes[0]) - 1U - step) :
                step;
            expected_final_value = ai_micro_uncache_merge_unsigned(
                expected_final_value,
                store_values[index],
                store_offsets[index],
                store_sizes[index]);

            TEST_SETUP_EXCEPT();
            ai_micro_uncache_narrow_store(store_sizes[index],
                nc_target_vaddr + store_offsets[index],
                store_values[index]);
            AI_ASSERT_NO_EXCEPT_AND("each storequeue-head narrow NC store in the width-switch sequence retires without surfacing an exception",
                true
            );
        }

        TEST_SETUP_EXCEPT();
        ai_micro_uncache_probe_ld_x0(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before invalidation after the storequeue-head narrow width-switch sequence, the potentially stale cacheable alias probe still completes without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_nc = ld(nc_target_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("reading back through the NC alias observes the exact byte-age merge written by the storequeue-head narrow width-switch sequence",
            observed_nc == expected_final_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_nc_adjacent = ld(nc_adjacent_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the adjacent NC word remains unchanged across the storequeue-head narrow width-switch sequence",
            observed_nc_adjacent == adjacent_value
        );

        reverse_order = !reverse_order;
    }

    ai_micro_uncache_head_reset_pbmt_only();

    TEST_END();
}

bool ai_micro_hs_store_mmio_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner()
{
    TEST_START();

    const uintptr_t target_offset = 0x80U;
    const uintptr_t cache_alias_line_vaddr =
        hs_page_base(AI_MICRO_HEAD_CACHE_ALIAS_PAGE) + target_offset;
    const uintptr_t mmio_target_paddr =
        ai_micro_uncache_head_mmio_backing_paddr() + target_offset;
    const uintptr_t mmio_adjacent_paddr = mmio_target_paddr + sizeof(uint64_t);
    static const uint64_t initial_values[] = {
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    static const uint64_t adjacent_values[] = {
        0x8877665544332211ULL,
        0x99aabbccddeeff00ULL,
    };
    static const unsigned store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned store_offsets[] = {
        7U,
        2U,
        4U,
    };
    static const uint64_t store_values[] = {
        0x00000000000000a5ULL,
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
    };
    bool reverse_order = false;

    for (unsigned round = 0; round < (sizeof(initial_values) / sizeof(initial_values[0])); ++round) {
        const uintptr_t mmio_target_vaddr =
            ai_micro_prepare_mmio_cache_pair() + target_offset;
        const uintptr_t mmio_adjacent_vaddr = mmio_target_vaddr + sizeof(uint64_t);
        const uint64_t initial_value = initial_values[round];
        const uint64_t adjacent_value = adjacent_values[round];
        uint64_t expected_final_value = initial_value;

        goto_priv(PRIV_M);
        write64(mmio_target_paddr, initial_value);
        write64(mmio_adjacent_paddr, adjacent_value);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the second alias to the default 3T PMA/IO line observes the original payload before the storequeue-head narrow MMIO width-switch probe",
            warmed == initial_value
        );

        for (unsigned step = 0; step < (sizeof(store_sizes) / sizeof(store_sizes[0])); ++step) {
            const unsigned index = reverse_order ?
                (unsigned)(sizeof(store_sizes) / sizeof(store_sizes[0]) - 1U - step) :
                step;
            expected_final_value = ai_micro_uncache_merge_unsigned(
                expected_final_value,
                store_values[index],
                store_offsets[index],
                store_sizes[index]);

            TEST_SETUP_EXCEPT();
            ai_micro_uncache_narrow_store(store_sizes[index],
                mmio_target_vaddr + store_offsets[index],
                store_values[index]);
            AI_ASSERT_NO_EXCEPT_AND("each storequeue-head narrow MMIO-style store in the width-switch sequence retires without surfacing an exception",
                true
            );
        }

        TEST_SETUP_EXCEPT();
        uint64_t alias_after_store = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the second alias to the default 3T PMA/IO line observes the exact byte-age merge without requiring cache invalidation",
            alias_after_store == expected_final_value
        );

        AI_MICRO_EXPECT_MMIO_CBO_INVAL_SAF(cache_alias_line_vaddr,
            "cbo.inval on the default 3T PMA/IO line reports Store AF without changing data");

        TEST_SETUP_EXCEPT();
        uint64_t refreshed_cached = ld(cache_alias_line_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after the CBO access-fault follow-up, the second alias still observes the exact byte-age merge written by the MMIO sequence",
            refreshed_cached == expected_final_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_mmio = ld(mmio_target_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("reading back through the MMIO-style alias observes the exact byte-age merge written by the storequeue-head narrow width-switch sequence",
            observed_mmio == expected_final_value
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_mmio_adjacent = ld(mmio_adjacent_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the adjacent MMIO-style word remains unchanged across the storequeue-head narrow width-switch sequence",
            observed_mmio_adjacent == adjacent_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the storequeue-head narrow MMIO width-switch sequence updates only its intended byte lanes in backing memory while preserving the adjacent word",
            read64(mmio_target_paddr) == expected_final_value &&
            read64(mmio_adjacent_paddr) == adjacent_value
        );

        reverse_order = !reverse_order;
    }

    ai_micro_uncache_head_reset_env();

    TEST_END();
}
