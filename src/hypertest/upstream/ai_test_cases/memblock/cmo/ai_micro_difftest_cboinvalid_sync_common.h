#ifndef AI_MICRO_DIFFTEST_CBOINVALID_SYNC_COMMON_H
#define AI_MICRO_DIFFTEST_CBOINVALID_SYNC_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_DIFFSYNC_BLOCK_BYTES 64U
#define AI_MICRO_DIFFSYNC_BLOCK_WORDS 8U
#define AI_MICRO_DIFFSYNC_PRESSURE_OFFSET 0x0U

static const enum test_page ai_micro_diffsync_pressure_pages[] = {
    VSRWX_GRWX,
    VSRW_GRWX,
    VSRX_GRWX,
    VSR_GRWX,
    VSRWX_GURWX,
    VSRW_GURWX,
    VSRX_GURWX,
    VSR_GURWX,
};

static inline pte_t *ai_micro_diffsync_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}


static inline void ai_micro_diffsync_set_hs_leaf(enum test_page page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_micro_diffsync_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}


static inline void ai_micro_diffsync_prepare_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}


static inline uintptr_t ai_micro_diffsync_prepare_same_paddr_alias_with_flags(
    enum test_page alias_page, enum test_page base_page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_micro_diffsync_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | flags;
    sfence_vma();

    return hs_page_base(alias_page);
}


static inline uintptr_t ai_micro_diffsync_prepare_same_paddr_alias(
    enum test_page alias_page, enum test_page base_page)
{
    return ai_micro_diffsync_prepare_same_paddr_alias_with_flags(
        alias_page, base_page, PTE_V | PTE_AD | PTE_RWX);
}


static inline void ai_micro_diffsync_reset_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}


static inline uint64_t ai_micro_diffsync_seed_word(uint64_t seed, uintptr_t word_idx)
{
    return seed + (word_idx * 0x1111111111111111ULL);
}


static inline uint64_t ai_micro_diffsync_pressure_word(uint64_t seed,
    uintptr_t pressure_idx)
{
    return seed + (pressure_idx * 0x101ULL);
}


static inline bool ai_micro_diffsync_page_skipped(enum test_page page,
    enum test_page skip0, enum test_page skip1, enum test_page skip2)
{
    return page == skip0 || page == skip1 || page == skip2;
}


static inline void ai_micro_diffsync_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            ai_micro_diffsync_seed_word(seed, i));
    }
}


static inline bool ai_micro_diffsync_block_all_zero(uintptr_t paddr)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) != 0ULL) {
            return false;
        }
    }

    return true;
}


static inline bool ai_micro_diffsync_block_all_zero_except_word(
    uintptr_t paddr,
    uintptr_t changed_word_idx,
    uint64_t changed_value)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == changed_word_idx) ? changed_value : 0ULL;

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}


static inline bool ai_micro_diffsync_block_matches_seed(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            ai_micro_diffsync_seed_word(seed, i)) {
            return false;
        }
    }

    return true;
}


static inline uint64_t ai_micro_diffsync_merge_byte_halfword_into_word(
    uint64_t base,
    uint8_t byte_value,
    uint16_t halfword_value);

static inline void ai_micro_diffsync_seed_pressure_pages(uint64_t seed,
    enum test_page skip0, enum test_page skip1, enum test_page skip2)
{
    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];

        if (ai_micro_diffsync_page_skipped(page, skip0, skip1, skip2)) {
            continue;
        }

        write64(phys_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET,
            ai_micro_diffsync_pressure_word(seed, i));
    }
}


static inline bool ai_micro_diffsync_pressure_pages_match_seed(uint64_t seed,
    enum test_page skip0, enum test_page skip1, enum test_page skip2)
{
    for (uintptr_t i = 0; i < sizeof(ai_micro_diffsync_pressure_pages) /
        sizeof(ai_micro_diffsync_pressure_pages[0]); ++i) {
        enum test_page page = ai_micro_diffsync_pressure_pages[i];

        if (ai_micro_diffsync_page_skipped(page, skip0, skip1, skip2)) {
            continue;
        }

        if (read64(phys_page_base(page) + AI_MICRO_DIFFSYNC_PRESSURE_OFFSET) !=
            ai_micro_diffsync_pressure_word(seed, i)) {
            return false;
        }
    }

    return true;
}


static inline bool ai_micro_diffsync_block_matches_seed_except_two_words(
    uintptr_t paddr,
    uint64_t seed,
    uintptr_t first_word_idx,
    uint64_t first_expected_word,
    uintptr_t second_word_idx,
    uint64_t second_expected_word)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        uint64_t expected = ai_micro_diffsync_seed_word(seed, i);

        if (i == first_word_idx) {
            expected = first_expected_word;
        } else if (i == second_word_idx) {
            expected = second_expected_word;
        }

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}


static inline void ai_micro_diffsync_write_bytes(uintptr_t addr,
    const uint8_t *bytes, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}


static inline bool ai_micro_diffsync_bytes_match(uintptr_t addr,
    const uint8_t *bytes, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}


static inline void ai_micro_diffsync_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
}














static inline void ai_micro_diffsync_cbo_inval_prefetch_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}


static inline void ai_micro_diffsync_cbo_inval_prefetch_lrsc_zero_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t marker_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_amo_zero_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_partial_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t marker_vaddr,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t marker_value,
    uint64_t *word_observed,
    uint64_t *byte_observed,
    uint64_t *half_observed,
    uint64_t *marker_observed)
{
    uint64_t word_readback = 0ULL;
    uint64_t byte_readback = 0ULL;
    uint64_t half_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "ld %[word_readback], 0(%[word_addr])\n\t"
        "lbu %[byte_readback], 0(%[byte_addr])\n\t"
        "lhu %[half_readback], 0(%[half_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [word_readback] "=&r"(word_readback),
          [byte_readback] "=&r"(byte_readback),
          [half_readback] "=&r"(half_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *word_observed = word_readback;
    *byte_observed = byte_readback;
    *half_observed = half_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_partial_store_zero_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t marker_value,
    uint64_t *merged_before_zero,
    uint64_t *marker_observed)
{
    uint64_t merged_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "ld %[merged_readback], 0(%[word_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [merged_readback] "=&r"(merged_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *merged_before_zero = merged_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}



static inline uint64_t ai_micro_diffsync_merge_byte_halfword_into_word(
    uint64_t base,
    uint8_t byte_value,
    uint16_t halfword_value)
{
    const uint64_t byte_mask = 0xffULL << 8;
    const uint64_t halfword_mask = 0xffffULL << 32;

    return (base & ~byte_mask & ~halfword_mask) |
        ((uint64_t)byte_value << 8) |
        ((uint64_t)halfword_value << 32);
}


static inline void ai_micro_diffsync_cbo_inval_prefetch_partial_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *word_observed,
    uint64_t *byte_observed,
    uint64_t *half_observed,
    uint64_t *marker_observed)
{
    uint64_t word_readback = 0ULL;
    uint64_t byte_readback = 0ULL;
    uint64_t half_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "ld %[word_readback], 0(%[word_addr])\n\t"
        "lbu %[byte_readback], 0(%[byte_addr])\n\t"
        "lhu %[half_readback], 0(%[half_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [word_readback] "=&r"(word_readback),
          [byte_readback] "=&r"(byte_readback),
          [half_readback] "=&r"(half_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *word_observed = word_readback;
    *byte_observed = byte_readback;
    *half_observed = half_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t target_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t target_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "fence rw, rw\n\t"
        "sd %[target_value], 0(%[target_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[target_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [target_addr] "r"(target_vaddr),
          [target_value] "r"(target_value),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_store_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t target_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t target_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[target_value], 0(%[target_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[target_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [target_addr] "r"(target_vaddr),
          [target_value] "r"(target_value),
          [clean_addr] "r"(clean_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}








static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t target_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t target_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[target_value], 0(%[target_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[target_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [target_addr] "r"(target_vaddr),
          [target_value] "r"(target_value),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_store_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t store_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t sc_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [store_addr] "r"(store_vaddr),
          [store_value] "r"(store_value),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_store_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t store_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [store_addr] "r"(store_vaddr),
          [store_value] "r"(store_value),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_value,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *word_observed,
    uint64_t *byte_observed,
    uint64_t *half_observed,
    uint64_t *marker_observed)
{
    uint64_t word_readback = 0ULL;
    uint64_t byte_readback = 0ULL;
    uint64_t half_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_value], 0(%[word_addr])\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "ld %[word_readback], 0(%[word_addr])\n\t"
        "lbu %[byte_readback], 0(%[byte_addr])\n\t"
        "lhu %[half_readback], 0(%[half_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [word_readback] "=&r"(word_readback),
          [byte_readback] "=&r"(byte_readback),
          [half_readback] "=&r"(half_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_value] "r"(prior_value),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *word_observed = word_readback;
    *byte_observed = byte_readback;
    *half_observed = half_readback;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_value,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_value], 0(%[word_addr])\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_value] "r"(prior_value),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_value,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t sc_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_value], 0(%[word_addr])\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_value] "r"(prior_value),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [sc_value] "r"(sc_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}





static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_value,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_after_fence,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_value], 0(%[word_addr])\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[word_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_value] "r"(prior_value),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_after_fence = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_value,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_value], 0(%[word_addr])\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[word_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_value] "r"(prior_value),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_post_partial_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t word_vaddr,
    uintptr_t byte_store_vaddr,
    uintptr_t half_store_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint8_t byte_value,
    uint16_t halfword_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sb %[byte_value], 0(%[byte_addr])\n\t"
        "sh %[halfword_value], 0(%[half_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[word_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [word_addr] "r"(word_vaddr),
          [byte_addr] "r"(byte_store_vaddr),
          [half_addr] "r"(half_store_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"((uint64_t)byte_value),
          [halfword_value] "r"((uint64_t)halfword_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}




static inline uint64_t ai_micro_diffsync_cross16_low_word_from_seed(
    uint64_t old_word,
    uint64_t payload)
{
    return (old_word & 0x00ffffffffffffffULL) |
        ((payload & 0xffULL) << 56);
}


static inline uint64_t ai_micro_diffsync_cross16_high_word_from_seed(
    uint64_t old_word,
    uint64_t payload)
{
    return (old_word & 0xff00000000000000ULL) | (payload >> 8);
}


static inline uint64_t ai_micro_diffsync_cross16_load_from_seed(
    uint64_t low_word,
    uint64_t high_word)
{
    return (low_word >> 56) | ((high_word & 0x00ffffffffffffffULL) << 8);
}


static inline bool ai_micro_diffsync_block_matches_seed_except_three_words(
    uintptr_t paddr,
    uint64_t seed,
    uintptr_t first_word_idx,
    uint64_t first_expected_word,
    uintptr_t second_word_idx,
    uint64_t second_expected_word,
    uintptr_t third_word_idx,
    uint64_t third_expected_word)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        uint64_t expected = ai_micro_diffsync_seed_word(seed, i);

        if (i == first_word_idx) {
            expected = first_expected_word;
        } else if (i == second_word_idx) {
            expected = second_expected_word;
        } else if (i == third_word_idx) {
            expected = third_expected_word;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline bool ai_micro_diffsync_block_all_zero_except_three_words(
    uintptr_t paddr,
    uintptr_t first_word_idx,
    uint64_t first_expected_word,
    uintptr_t second_word_idx,
    uint64_t second_expected_word,
    uintptr_t third_word_idx,
    uint64_t third_expected_word)
{
    for (uintptr_t i = 0; i < AI_MICRO_DIFFSYNC_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == first_word_idx) {
            expected = first_expected_word;
        } else if (i == second_word_idx) {
            expected = second_expected_word;
        } else if (i == third_word_idx) {
            expected = third_expected_word;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_cross16_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *low_word_observed,
    uint64_t *high_word_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_word_observed = low_readback;
    *high_word_observed = high_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cross16_store_zero_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t marker_value,
    uint64_t *low_before_zero,
    uint64_t *high_before_zero,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_before_zero = low_readback;
    *high_before_zero = high_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_cross16_load_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cross16_load_zero_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_post_cross16_load_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word1_vaddr,
    uintptr_t word2_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t word1_value,
    uint64_t word2_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *word1_observed,
    uint64_t *word2_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0ULL;
    uint64_t word1_readback = 0ULL;
    uint64_t word2_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[word1_value], 0(%[word1_addr])\n\t"
        "sd %[word2_value], 0(%[word2_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[word1_readback], 0(%[word1_addr])\n\t"
        "ld %[word2_readback], 0(%[word2_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [word1_readback] "=&r"(word1_readback),
          [word2_readback] "=&r"(word2_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word1_addr] "r"(word1_vaddr),
          [word2_addr] "r"(word2_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [word1_value] "r"(word1_value),
          [word2_value] "r"(word2_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *word1_observed = word1_readback;
    *word2_observed = word2_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cross16_load_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t word1_vaddr,
    uintptr_t word2_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t word1_value,
    uint64_t word2_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *word1_observed,
    uint64_t *word2_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0ULL;
    uint64_t word1_readback = 0ULL;
    uint64_t word2_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[word1_value], 0(%[word1_addr])\n\t"
        "sd %[word2_value], 0(%[word2_addr])\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[word1_readback], 0(%[word1_addr])\n\t"
        "ld %[word2_readback], 0(%[word2_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [word1_readback] "=&r"(word1_readback),
          [word2_readback] "=&r"(word2_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [word1_addr] "r"(word1_vaddr),
          [word2_addr] "r"(word2_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [word1_value] "r"(word1_value),
          [word2_value] "r"(word2_value),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *word1_observed = word1_readback;
    *word2_observed = word2_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t pre_lrsc_value,
    uint64_t sc_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[pre_lrsc_value], 0(%[lrsc_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [pre_lrsc_value] "r"(pre_lrsc_value),
          [sc_value] "r"(sc_value),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_lrsc_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[lrsc_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t target_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t pre_amo_value,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[pre_amo_value], 0(%[target_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[target_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[target_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[target_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [target_addr] "r"(target_vaddr),
          [pre_amo_value] "r"(pre_amo_value),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_amo_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t target_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *target_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[target_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[target_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[target_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [target_addr] "r"(target_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *target_observed = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_post_cross16_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *low_word_observed,
    uint64_t *high_word_observed,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [neighbor_value] "r"(neighbor_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_word_observed = low_readback;
    *high_word_observed = high_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t target_vaddr,
    uintptr_t neighbor_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_value,
    uint64_t neighbor_value,
    uint64_t marker_value,
    uint64_t *target_after_fence,
    uint64_t *neighbor_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t neighbor_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_value], 0(%[target_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[target_addr])\n\t"
        "sd %[neighbor_value], 0(%[neighbor_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[neighbor_readback], 0(%[neighbor_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [neighbor_readback] "=&r"(neighbor_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [target_addr] "r"(target_vaddr),
          [prior_value] "r"(prior_value),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_value] "r"(neighbor_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_after_fence = target_readback;
    *neighbor_observed = neighbor_readback;
    *marker_observed = marker_readback;
}




static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t dirty_target_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t dirty_target_value,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *target_observed,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t target_readback = 0ULL;
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[dirty_target_value], 0(%[dirty_target_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "fence rw, rw\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[target_readback], 0(%[dirty_target_addr])\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [target_readback] "=&r"(target_readback),
          [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [dirty_target_addr] "r"(dirty_target_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [dirty_target_value] "r"(dirty_target_value),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *target_observed = target_readback;
    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_store_fence_cbom_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t prior_store_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_store_value,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *prior_store_observed,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t prior_readback = 0ULL;
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_store_value], 0(%[prior_store_addr])\n\t"
        "fence rw, rw\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[prior_readback], 0(%[prior_store_addr])\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [prior_readback] "=&r"(prior_readback),
          [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [prior_store_addr] "r"(prior_store_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_store_value] "r"(prior_store_value),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *prior_store_observed = prior_readback;
    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}




















static inline void ai_micro_diffsync_cbo_inval_prefetch_cross16_store_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t marker_value,
    uint64_t *low_word_observed,
    uint64_t *high_word_observed,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_word_observed = low_readback;
    *high_word_observed = high_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "fence rw, rw\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_prior_store_fence_zero_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t prior_store_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t prior_store_value,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *prior_store_observed,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t prior_readback = 0ULL;
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        "sd %[prior_store_value], 0(%[prior_store_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[prior_readback], 0(%[prior_store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [prior_readback] "=&r"(prior_readback),
          [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [prior_store_addr] "r"(prior_store_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [prior_store_value] "r"(prior_store_value),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *prior_store_observed = prior_readback;
    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_cbom_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_cbom_zero_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_inval_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t second_inval_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_inval_addr], 0\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [second_inval_addr] "r"(second_inval_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_inval_zero_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t second_inval_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_inval_addr], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [second_inval_addr] "r"(second_inval_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}



static inline void ai_micro_diffsync_cbo_inval_prefetch_zero_zero_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}

#endif /* AI_MICRO_DIFFTEST_CBOINVALID_SYNC_COMMON_H */
