#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stddef.h>

#define AI_AP16_BLOCK_WORDS      8U
#define AI_AP16_BLOCK_BYTES      64U
#define AI_AP16_WORD_PATTERN     0x1111111111111111ULL
#define AI_AP16_TARGET_OFFSET    0x300U
#define AI_AP16_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static const enum test_page ai_ap16_pressure_pages[] = {
    VSRW_GRWX,
    VSRWX_GRW,
    VSRW_GRW,
    VSRWX_GURWX,
    VSRW_GURWX,
    VSRWX_GURW,
};

static inline pte_t *ai_ap16_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap16_set_hs_leaf(enum test_page page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_ap16_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline void ai_ap16_prepare_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline void ai_ap16_reset_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_ap16_seed_word(uint64_t seed, unsigned word)
{
    return seed + ((uint64_t)word * AI_AP16_WORD_PATTERN);
}

static inline void ai_ap16_fill_expected(uint64_t expected[AI_AP16_BLOCK_WORDS],
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_AP16_BLOCK_WORDS; ++i) {
        expected[i] = ai_ap16_seed_word(seed, i);
    }
}

static inline void ai_ap16_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (unsigned i = 0; i < AI_AP16_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)), ai_ap16_seed_word(seed, i));
    }
}

static inline bool ai_ap16_block_matches_expected(uintptr_t paddr,
    const uint64_t expected[AI_AP16_BLOCK_WORDS])
{
    for (unsigned i = 0; i < AI_AP16_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) != expected[i]) {
            return false;
        }
    }

    return true;
}

static inline uint32_t ai_ap16_word_from_dword(uint64_t value,
    unsigned byte_offset)
{
    const unsigned shift = (byte_offset & 0x4U) * 8U;
    return (uint32_t)(value >> shift);
}

static inline uint64_t ai_ap16_replace_word_in_dword(uint64_t value,
    unsigned byte_offset, uint32_t word)
{
    const unsigned shift = (byte_offset & 0x4U) * 8U;
    const uint64_t mask = 0xffffffffULL << shift;
    return (value & ~mask) | ((uint64_t)word << shift);
}

static inline uint64_t ai_ap16_apply_amoadd_w_expected(uint64_t value,
    unsigned byte_offset, uint32_t addend, uint32_t *old_word,
    uint32_t *new_word)
{
    *old_word = ai_ap16_word_from_dword(value, byte_offset);
    *new_word = *old_word + addend;
    return ai_ap16_replace_word_in_dword(value, byte_offset, *new_word);
}

static inline uint64_t ai_ap16_touch_pressure_lines(uint64_t seed)
{
    uint64_t signature = 0ULL;

    for (unsigned i = 0; i < sizeof(ai_ap16_pressure_pages) /
        sizeof(ai_ap16_pressure_pages[0]); ++i) {
        const uintptr_t base = hs_page_base(ai_ap16_pressure_pages[i]) +
            AI_AP16_TARGET_OFFSET;
        const uint64_t payload = seed ^ (0x0101010101010101ULL * (i + 1U));

        signature ^= ld(base);
        sd(base + sizeof(uint64_t), payload);
        signature ^= ld(base + sizeof(uint64_t));
        prefetch_r(base + 2U * sizeof(uint64_t));
    }

    return signature;
}

static inline void ai_ap16_seed_pressure_lines(uint64_t seed)
{
    for (unsigned i = 0; i < sizeof(ai_ap16_pressure_pages) /
        sizeof(ai_ap16_pressure_pages[0]); ++i) {
        const uintptr_t paddr = phys_page_base(ai_ap16_pressure_pages[i]) +
            AI_AP16_TARGET_OFFSET;

        ai_ap16_seed_block(paddr, seed + (0x0100010001000100ULL * i));
    }
}

static inline uint64_t ai_ap16_lr_d(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lr.d %0, 0(%1)\n\t"
        : "=r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline uint32_t ai_ap16_sc_w(uintptr_t addr, uint32_t value)
{
    uint64_t status;
    uint64_t operand = value;

    asm volatile(
        "sc.w %0, %2, 0(%1)\n\t"
        : "=r"(status)
        : "r"(addr), "r"(operand)
        : "memory");
    return (uint32_t)status;
}

static inline uint32_t ai_ap16_sc_d(uintptr_t addr, uint64_t value)
{
    uint64_t status;

    asm volatile(
        "sc.d %0, %2, 0(%1)\n\t"
        : "=r"(status)
        : "r"(addr), "r"(value)
        : "memory");
    return (uint32_t)status;
}

bool ai_micro_ap16_dirty_evict_amo_w_offset_sweep_manual_smoke_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t base_paddr = phys_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t leading_guard_paddr = base_paddr - sizeof(uint64_t);
    const uintptr_t trailing_guard_paddr = base_paddr + AI_AP16_BLOCK_BYTES;
    const unsigned offsets[] = { 4U, 12U, 28U, 56U };
    const uint32_t first_addends[] = { 0x11U, 0x102U, 0x1003U, 0x10004U };
    const uint32_t second_addends[] = { 0x7U, 0x21U, 0x43U, 0x65U };
    const uint64_t seeds[] = {
        0x1021324354657687ULL,
        0x5566778899aabbccULL,
        0x89abcdef01234567ULL,
        0x0fedcba987654321ULL,
    };
    const uint64_t pressure_seed = 0x33445566778899aaULL;

    ai_ap16_prepare_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < sizeof(offsets) / sizeof(offsets[0]);
         ++round) {
        uint64_t expected[AI_AP16_BLOCK_WORDS];
        const unsigned byte_offset = offsets[round];
        const unsigned word_index = byte_offset / sizeof(uint64_t);
        const uintptr_t target_vaddr = base_vaddr + byte_offset;
        uint32_t expected_old0;
        uint32_t expected_after0;
        uint32_t expected_old1;
        uint32_t expected_after1;
        uint64_t pressure_signature;
        uint64_t observed_target_word;

        ai_ap16_fill_expected(expected, seeds[round]);
        expected[0] ^= 0x00ff00ff00ff00ffULL;
        expected[5] ^= 0x0f0f0f0f0f0f0f0fULL;

        goto_priv(PRIV_M);
        ai_ap16_seed_block(base_paddr, seeds[round]);
        ai_ap16_seed_pressure_lines(pressure_seed + round);
        write64(leading_guard_paddr, 0xa100000000000000ULL | round);
        write64(trailing_guard_paddr, 0xb200000000000000ULL | round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_AP16_BLOCK_WORDS; ++i) {
            (void)ld(base_vaddr + (i * sizeof(uint64_t)));
        }
        sd(base_vaddr, expected[0]);
        sd(base_vaddr + (5U * sizeof(uint64_t)), expected[5]);
        pressure_signature = ai_ap16_touch_pressure_lines(pressure_seed + round);
        AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 AMO.W sweep warms and dirties the target line, then builds independent dirty-line pressure before the same-line offset atomic",
            pressure_signature != 0ULL);

        expected[word_index] = ai_ap16_apply_amoadd_w_expected(
            expected[word_index], byte_offset, first_addends[round],
            &expected_old0, &expected_after0);
        expected[word_index] = ai_ap16_apply_amoadd_w_expected(
            expected[word_index], byte_offset, second_addends[round],
            &expected_old1, &expected_after1);

        TEST_SETUP_EXCEPT();
        const uint32_t old0 = amoadd_w(target_vaddr, first_addends[round]);
        const uint32_t old1 = amoadd_w(target_vaddr, second_addends[round]);
        observed_target_word = ld(base_vaddr + (word_index * sizeof(uint64_t)));
        AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 same-cacheline AMO.W at a nonzero block offset returns the current word once, then the follow-up AMO observes exactly the first result",
            old0 == expected_old0 &&
            old1 == expected_after0 &&
            observed_target_word == expected[word_index]);

        TEST_SETUP_EXCEPT();
        cbo_flush(base_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("AP-16 AMO.W sweep drains the probed dirty line without surfacing a CMO/fence exception",
            excpt.triggered == false);

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-16 AMO.W offset sweep leaves only the selected word updated and preserves all same-line untouched words plus adjacent guards after drain",
            ai_ap16_block_matches_expected(base_paddr, expected) &&
            read64(leading_guard_paddr) == (0xa100000000000000ULL | round) &&
            read64(trailing_guard_paddr) == (0xb200000000000000ULL | round),
            "round=%u offset=%u old0=0x%x old1=0x%x expected_old0=0x%x expected_old1=0x%x expected_after1=0x%x",
            round, byte_offset, old0, old1, expected_old0, expected_old1,
            expected_after1);
    }

    ai_ap16_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_ap16_dirty_evict_amo_d_miss_pressure_manual_smoke_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t base_paddr = phys_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + AI_AP16_TARGET_OFFSET;
    const unsigned offsets[] = { 8U, 16U, 40U, 56U };
    const uint64_t seeds[] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
        0x0000000000000404ULL,
    };
    const uint64_t second_addends[] = {
        0x0000000000000011ULL,
        0x0000000000000022ULL,
        0x0000000000000033ULL,
        0x0000000000000044ULL,
    };
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_addend = 0x0101010101010101ULL;

    ai_ap16_prepare_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < sizeof(offsets) / sizeof(offsets[0]);
         ++round) {
        uint64_t expected[AI_AP16_BLOCK_WORDS];
        const unsigned word_index = offsets[round] / sizeof(uint64_t);
        const uintptr_t target_vaddr = base_vaddr + offsets[round];
        const uint64_t expected_old0 = ai_ap16_seed_word(seeds[round],
            word_index);
        const uint64_t expected_after0 = expected_old0 + first_addends[round];
        const uint64_t expected_after1 = expected_after0 + second_addends[round];
        uint64_t pressure_signature;

        ai_ap16_fill_expected(expected, seeds[round]);
        expected[word_index] = expected_after1;

        goto_priv(PRIV_M);
        ai_ap16_seed_block(base_paddr, seeds[round]);
        ai_ap16_seed_block(marker_paddr, marker_seed + round);
        ai_ap16_seed_pressure_lines(0x445566778899aabbULL + round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_AP16_BLOCK_WORDS; ++i) {
            (void)ld(base_vaddr + (i * sizeof(uint64_t)));
        }
        sd(base_vaddr + 3U * sizeof(uint64_t),
            ai_ap16_seed_word(seeds[round], 3U) ^ 0x55aa55aa55aa55aaULL);
        expected[3] = ai_ap16_seed_word(seeds[round], 3U) ^
            0x55aa55aa55aa55aaULL;
        if (word_index == 3U) {
            expected[word_index] = expected_after1;
        }
        pressure_signature = ai_ap16_touch_pressure_lines(
            0x445566778899aabbULL + round);
        AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 AMO.D pressure case dirties the target line and creates miss/MSHR pressure before the nonzero-offset AMO.D",
            pressure_signature != 0ULL);

        TEST_SETUP_EXCEPT();
        const uint64_t old0 = amoadd_d(target_vaddr, first_addends[round]);
        const uint64_t old1 = amoadd_d(target_vaddr, second_addends[round]);
        const uint64_t marker_old = amoadd_d(marker_vaddr + sizeof(uint64_t),
            marker_addend + round);
        const uint64_t observed = ld(target_vaddr);
        AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 nonzero-offset AMO.D under dirty-line pressure completes exactly once, and a different-line AMO with the same low offset still makes progress",
            old0 == expected_old0 &&
            old1 == expected_after0 &&
            observed == expected_after1 &&
            marker_old == ai_ap16_seed_word(marker_seed + round, 1U));

        TEST_SETUP_EXCEPT();
        cbo_flush(base_vaddr);
        cbo_flush(marker_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("AP-16 AMO.D pressure case drains both target and different-line marker blocks without exception",
            excpt.triggered == false);

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-16 AMO.D pressure case preserves the whole target block image and proves different-line atomic traffic was not over-blocked by the WBQ conflict fix",
            ai_ap16_block_matches_expected(base_paddr, expected) &&
            read64(marker_paddr + sizeof(uint64_t)) ==
                ai_ap16_seed_word(marker_seed + round, 1U) +
                marker_addend + round,
            "round=%u offset=%u old0=0x%lx old1=0x%lx marker_old=0x%lx",
            round, offsets[round], old0, old1, marker_old);
    }

    ai_ap16_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_ap16_dirty_evict_lrsc_w_d_offset_manual_smoke_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t base_paddr = phys_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t lr_w_vaddr = base_vaddr + 4U;
    const uintptr_t lr_d_vaddr = base_vaddr + 16U;
    const uint64_t seed = 0x5a5b5c5d6a6b6c6dULL;
    const uint32_t sc_w_value = 0xcafebabeU;
    const uint64_t sc_d_value = 0x123456789abcdef0ULL;
    uint64_t expected[AI_AP16_BLOCK_WORDS];

    ai_ap16_prepare_env(MENVCFG_CBZFE);
    ai_ap16_fill_expected(expected, seed);
    expected[0] = ai_ap16_replace_word_in_dword(expected[0], 4U,
        sc_w_value);
    expected[2] = sc_d_value;

    goto_priv(PRIV_M);
    ai_ap16_seed_block(base_paddr, seed);
    ai_ap16_seed_pressure_lines(0x66778899aabbccddULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ld(base_vaddr);
    (void)ld(base_vaddr + 16U);
    sd(base_vaddr + 5U * sizeof(uint64_t),
        ai_ap16_seed_word(seed, 5U) ^ 0x0101010101010101ULL);
    expected[5] = ai_ap16_seed_word(seed, 5U) ^ 0x0101010101010101ULL;
    const uint64_t pressure_signature =
        ai_ap16_touch_pressure_lines(0x66778899aabbccddULL);
    AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 LR/SC case warms and dirties the line, then creates pressure before LR.W/LR.D at nonzero offsets",
        pressure_signature != 0ULL);

    TEST_SETUP_EXCEPT();
    const uint32_t lr_w_old = lr_w(lr_w_vaddr);
    const uint32_t sc_w_status = ai_ap16_sc_w(lr_w_vaddr, sc_w_value);
    const uint64_t lr_d_old = ai_ap16_lr_d(lr_d_vaddr);
    const uint32_t sc_d_status = ai_ap16_sc_d(lr_d_vaddr, sc_d_value);
    const uint64_t observed_w_dword = ld(base_vaddr);
    const uint64_t observed_d_dword = ld(lr_d_vaddr);
    AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 LR.W/LR.D same-line nonzero-offset sequence reads fresh values and the immediate SC.W/SC.D updates exactly the reserved words",
        lr_w_old == ai_ap16_word_from_dword(ai_ap16_seed_word(seed, 0U), 4U) &&
        sc_w_status == 0U &&
        lr_d_old == ai_ap16_seed_word(seed, 2U) &&
        sc_d_status == 0U &&
        observed_w_dword == expected[0] &&
        observed_d_dword == expected[2]);

    TEST_SETUP_EXCEPT();
    cbo_flush(base_vaddr);
    fence_iorw_iorw();
    TEST_ASSERT("AP-16 LR/SC dirty-line case drains the line without CMO/fence exception",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-16 LR/SC same-line offset case preserves the whole cacheline image except for the two successful reservation updates",
        ai_ap16_block_matches_expected(base_paddr, expected),
        "lr_w_old=0x%x sc_w_status=%u lr_d_old=0x%lx sc_d_status=%u",
        lr_w_old, sc_w_status, lr_d_old, sc_d_status);

    ai_ap16_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_ap16_redirect_recovery_amo_after_dirty_pressure_manual_smoke_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t base_paddr = phys_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t target_vaddr = base_vaddr + 24U;
    const uint64_t seed = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t fault_addend = 0x0101010101010101ULL;
    const uint64_t retry_addend = 0x0202020202020202ULL;
    const uint64_t followup_addend = 0x0303030303030303ULL;
    uint64_t expected[AI_AP16_BLOCK_WORDS];

    ai_ap16_prepare_env(MENVCFG_CBZFE);
    ai_ap16_fill_expected(expected, seed);
    expected[3] = ai_ap16_seed_word(seed, 3U) + retry_addend +
        followup_addend;

    goto_priv(PRIV_M);
    ai_ap16_seed_block(base_paddr, seed);
    ai_ap16_seed_pressure_lines(0x778899aabbccddeeULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ld(target_vaddr);
    sd(base_vaddr + sizeof(uint64_t),
        ai_ap16_seed_word(seed, 1U) ^ 0x0badf00d0badf00dULL);
    expected[1] = ai_ap16_seed_word(seed, 1U) ^ 0x0badf00d0badf00dULL;
    AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 redirect/recovery case warms and dirties the target line before forcing the first AMO into a page-fault redirect",
        true);

    ai_ap16_set_hs_leaf(target_page, PTE_V | PTE_A | PTE_D | PTE_R);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, fault_addend);
    TEST_ASSERT("AP-16 leading AMO.D at a nonzero same-line offset reports a precise store page fault when write permission is temporarily removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == target_vaddr);

    ai_ap16_set_hs_leaf(target_page, PTE_V | PTE_A | PTE_D | PTE_RWX);
    TEST_SETUP_EXCEPT();
    const uint64_t pressure_signature =
        ai_ap16_touch_pressure_lines(0x778899aabbccddeeULL);
    const uint64_t retry_old = amoadd_d(target_vaddr, retry_addend);
    const uint64_t followup_old = amoadd_d(target_vaddr, followup_addend);
    const uint64_t observed = ld(target_vaddr);
    AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 after redirect repair, dirty-line pressure plus same-line AMO.D retry completes once and the follow-up AMO observes exactly the retry result",
        pressure_signature != 0ULL &&
        retry_old == ai_ap16_seed_word(seed, 3U) &&
        followup_old == ai_ap16_seed_word(seed, 3U) + retry_addend &&
        observed == expected[3]);

    TEST_SETUP_EXCEPT();
    cbo_flush(base_vaddr);
    fence_iorw_iorw();
    TEST_ASSERT("AP-16 redirect/recovery case drains the repaired AMO line without exception",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-16 redirect/recovery case leaves only the repaired retry and follow-up AMO updates visible in the target cacheline",
        ai_ap16_block_matches_expected(base_paddr, expected),
        "retry_old=0x%lx followup_old=0x%lx observed=0x%lx",
        retry_old, followup_old, observed);

    ai_ap16_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_ap16_cbo_flush_same_line_amo_owner_manual_smoke_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t base_paddr = phys_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + AI_AP16_TARGET_OFFSET;
    const uint64_t seed = 0x1020304050607080ULL;
    const uint64_t marker_seed = 0xa0b0c0d0e0f00110ULL;
    const uint64_t dirty_word0 = 0x8877665544332211ULL;
    const uint64_t dirty_word4 = 0x99aabbccddeeff00ULL;
    const uint64_t addend_d = 0x0000000000001111ULL;
    const uint32_t addend_w = 0x2222U;
    uint64_t expected[AI_AP16_BLOCK_WORDS];
    uint32_t expected_w_old;
    uint32_t expected_w_new;

    ai_ap16_prepare_env(MENVCFG_CBZFE);
    ai_ap16_fill_expected(expected, seed);
    expected[0] = dirty_word0;
    expected[1] = ai_ap16_seed_word(seed, 1U) + addend_d;
    expected[4] = dirty_word4;
    expected[0] = ai_ap16_apply_amoadd_w_expected(expected[0], 4U,
        addend_w, &expected_w_old, &expected_w_new);

    goto_priv(PRIV_M);
    ai_ap16_seed_block(base_paddr, seed);
    ai_ap16_seed_block(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ld(base_vaddr);
    (void)ld(base_vaddr + 56U);
    sd(base_vaddr, dirty_word0);
    sd(base_vaddr + 4U * sizeof(uint64_t), dirty_word4);
    cbo_clean(base_vaddr);
    cbo_flush(base_vaddr + 56U);
    const uint64_t old_d = amoadd_d(base_vaddr + sizeof(uint64_t), addend_d);
    const uint32_t old_w = amoadd_w(base_vaddr + 4U, addend_w);
    sd(marker_vaddr, marker_seed ^ 0x00ff00ff00ff00ffULL);
    fence_iorw_iorw();
    AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 CBO clean/flush induced writeback window keeps same-line AMO.D and AMO.W ownership separate from CMO ownership",
        old_d == ai_ap16_seed_word(seed, 1U) &&
        old_w == expected_w_old &&
        ld(base_vaddr + sizeof(uint64_t)) == expected[1] &&
        ld(base_vaddr) == expected[0]);

    TEST_SETUP_EXCEPT();
    cbo_flush(base_vaddr);
    cbo_flush(marker_vaddr);
    fence_iorw_iorw();
    TEST_ASSERT("AP-16 CBO-induced same-line AMO case drains target and marker lines without exception",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-16 CBO-induced same-line AMO case preserves the final cacheline image and proves marker progress was not captured by the CMO/AMO owner window",
        ai_ap16_block_matches_expected(base_paddr, expected) &&
        read64(marker_paddr) == (marker_seed ^ 0x00ff00ff00ff00ffULL),
        "old_d=0x%lx old_w=0x%x expected_w_new=0x%x",
        old_d, old_w, expected_w_new);

    ai_ap16_reset_env(MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_ap16_same_line_load_store_atomic_conflict_manual_smoke_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page other_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t base_paddr = phys_page_base(target_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t other_vaddr = hs_page_base(other_page) + AI_AP16_TARGET_OFFSET;
    const uintptr_t other_paddr = phys_page_base(other_page) + AI_AP16_TARGET_OFFSET;
    const uint64_t seed = 0x2468ace013579bdfULL;
    const uint64_t other_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t older_store = 0xfeedfacecafebeefULL;
    const uint64_t younger_store = 0x1122334455667788ULL;
    const uint32_t lower_addend = 0x33U;
    const uint32_t upper_addend = 0x4400U;
    const uint64_t other_addend = 0x0000000000001234ULL;
    uint64_t expected[AI_AP16_BLOCK_WORDS];
    uint32_t lower_old;
    uint32_t lower_new;
    uint32_t upper_old;
    uint32_t upper_new;

    ai_ap16_prepare_env(MENVCFG_CBZFE);
    ai_ap16_fill_expected(expected, seed);
    expected[1] = older_store;
    expected[6] = younger_store;
    expected[0] = ai_ap16_apply_amoadd_w_expected(expected[0], 0U,
        lower_addend, &lower_old, &lower_new);
    expected[0] = ai_ap16_apply_amoadd_w_expected(expected[0], 4U,
        upper_addend, &upper_old, &upper_new);

    goto_priv(PRIV_M);
    ai_ap16_seed_block(base_paddr, seed);
    ai_ap16_seed_block(other_paddr, other_seed);
    ai_ap16_seed_pressure_lines(0x8899aabbccddeeffULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ld(base_vaddr);
    (void)ld(base_vaddr + 56U);
    sd(base_vaddr + sizeof(uint64_t), older_store);
    const uint64_t same_line_load = ld(base_vaddr + sizeof(uint64_t));
    const uint32_t observed_lower_old = amoadd_w(base_vaddr, lower_addend);
    sd(base_vaddr + 6U * sizeof(uint64_t), younger_store);
    const uint32_t observed_upper_old = amoadd_w(base_vaddr + 4U,
        upper_addend);
    const uint64_t other_old = amoadd_d(other_vaddr + 32U, other_addend);
    const uint64_t observed_word0 = ld(base_vaddr);
    AI_AP16_ASSERT_NO_EXCEPT_AND("AP-16 same-line load/store/AMO mix keeps lower and upper word atomics bound to their own lanes while a different-line AMO still progresses",
        same_line_load == older_store &&
        observed_lower_old == lower_old &&
        observed_upper_old == upper_old &&
        other_old == ai_ap16_seed_word(other_seed, 4U) &&
        observed_word0 == expected[0]);

    TEST_SETUP_EXCEPT();
    cbo_flush(base_vaddr);
    cbo_flush(other_vaddr);
    fence_iorw_iorw();
    TEST_ASSERT("AP-16 mixed load/store/atomic conflict case drains target and different-line blocks without exception",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-16 mixed load/store/atomic conflict case preserves same-line store payloads, lower/upper AMO lanes, and different-line AMO payload",
        ai_ap16_block_matches_expected(base_paddr, expected) &&
        read64(other_paddr + 32U) == ai_ap16_seed_word(other_seed, 4U) +
            other_addend,
        "lower_old=0x%x upper_old=0x%x lower_new=0x%x upper_new=0x%x other_old=0x%lx",
        observed_lower_old, observed_upper_old, lower_new, upper_new,
        other_old);

    ai_ap16_reset_env(MENVCFG_CBZFE);

    TEST_END();
}
