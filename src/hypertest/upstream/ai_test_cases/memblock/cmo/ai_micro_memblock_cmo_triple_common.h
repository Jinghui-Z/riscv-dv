#ifndef AI_MICRO_MEMBLOCK_CMO_TRIPLE_COMMON_H
#define AI_MICRO_MEMBLOCK_CMO_TRIPLE_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CMO_TRIPLE_ROUNDS 3U
#define AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS 8U

static inline void ai_memblock_cmo_triple_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}


static inline void ai_memblock_cmo_triple_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}


static inline void ai_memblock_cmo_triple_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}


static inline bool ai_memblock_cmo_triple_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}


static inline bool ai_memblock_cmo_triple_block_matches_seed(uintptr_t paddr,
    uint64_t seed);
static inline bool ai_memblock_cmo_triple_block_zero_except_word(
    uintptr_t paddr,
    size_t preserved_word,
    uint64_t preserved_value);
static inline bool ai_memblock_cmo_triple_block_zero_except_two_words(
    uintptr_t paddr,
    size_t first_preserved_word,
    uint64_t first_preserved_value,
    size_t second_preserved_word,
    uint64_t second_preserved_value);

static inline void ai_memblock_cmo_triple_inval_cbom_zero_window(
    uintptr_t inval_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_zero_cbom_inval_window(
    uintptr_t zero_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_inval_zero_cbom_window(
    uintptr_t inval_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_cbom_zero_inval_window(
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_zero_inval_cbom_window(
    uintptr_t zero_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_cbom_inval_zero_window(
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}








static inline bool ai_memblock_cmo_triple_block_matches_seed(
    uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        const uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline void ai_memblock_cmo_triple_two_block_mixed_window(
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block0_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_observed,
    uint64_t *block1_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_readback;
    uint64_t block1_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        "ld %[block0_readback], 0(%[block0_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_readback] "=&r"(block0_readback),
          [block1_readback] "=&r"(block1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block0_load] "r"(block0_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_observed = block0_readback;
    *block1_observed = block1_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_two_block_opposite_window(
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_zero_vaddr,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block0_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_observed,
    uint64_t *block1_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_readback;
    uint64_t block1_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        "ld %[block0_readback], 0(%[block0_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_readback] "=&r"(block0_readback),
          [block1_readback] "=&r"(block1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_inval] "r"(block0_inval_vaddr),
          [block1_zero] "r"(block1_zero_vaddr),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block0_load] "r"(block0_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_observed = block0_readback;
    *block1_observed = block1_readback;
    *marker_observed = marker_readback;
}


static inline void ai_memblock_cmo_triple_two_block_dual_zero_window(
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_zero_vaddr,
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block0_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_observed,
    uint64_t *block1_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_readback;
    uint64_t block1_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        "ld %[block0_readback], 0(%[block0_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_readback] "=&r"(block0_readback),
          [block1_readback] "=&r"(block1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_zero] "r"(block0_zero_vaddr),
          [block1_zero] "r"(block1_zero_vaddr),
          [block0_inval] "r"(block0_inval_vaddr),
          [block1_clean] "r"(block1_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block0_load] "r"(block0_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_observed = block0_readback;
    *block1_observed = block1_readback;
    *marker_observed = marker_readback;
}





static inline void ai_memblock_cmo_triple_two_block_dual_preserve_window(
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block0_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_observed,
    uint64_t *block1_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_readback;
    uint64_t block1_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        "ld %[block0_readback], 0(%[block0_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_readback] "=&r"(block0_readback),
          [block1_readback] "=&r"(block1_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_inval] "r"(block0_inval_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_clean] "r"(block1_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block0_load] "r"(block0_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_observed = block0_readback;
    *block1_observed = block1_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_round_robin_window(
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_zero_vaddr,
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block0_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        "ld %[block0_readback], 0(%[block0_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_readback] "=&r"(block0_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_zero] "r"(block2_zero_vaddr),
          [block0_inval] "r"(block0_inval_vaddr),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block0_load] "r"(block0_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_observed = block0_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline bool ai_memblock_cmo_triple_block_zero_except_word(
    uintptr_t paddr,
    size_t preserved_word,
    uint64_t preserved_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == preserved_word) ?
            preserved_value : 0ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline bool ai_memblock_cmo_triple_block_zero_except_two_words(
    uintptr_t paddr,
    size_t first_preserved_word,
    uint64_t first_preserved_value,
    size_t second_preserved_word,
    uint64_t second_preserved_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == first_preserved_word) {
            expected = first_preserved_value;
        } else if (i == second_preserved_word) {
            expected = second_preserved_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline bool ai_memblock_cmo_triple_block_zero_except_three_words(
    uintptr_t paddr,
    size_t first_preserved_word,
    uint64_t first_preserved_value,
    size_t second_preserved_word,
    uint64_t second_preserved_value,
    size_t third_preserved_word,
    uint64_t third_preserved_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == first_preserved_word) {
            expected = first_preserved_value;
        } else if (i == second_preserved_word) {
            expected = second_preserved_value;
        } else if (i == third_preserved_word) {
            expected = third_preserved_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline void ai_memblock_cmo_triple_three_block_store_after_zero_window(
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_zero_vaddr,
    uintptr_t block0_store_vaddr,
    uint64_t block0_store_value,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_store_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed)
{
    uint64_t block0_store_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_zero], 4\n\t"
        "sd %[block0_store_value], 0(%[block0_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        "ld %[block0_store_readback], 0(%[block0_store])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        ".option pop\n\t"
        : [block0_store_readback] "=&r"(block0_store_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback)
        : [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_zero] "r"(block2_zero_vaddr),
          [block0_store] "r"(block0_store_vaddr),
          [block0_store_value] "r"(block0_store_value),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_store_observed = block0_store_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
}



static inline bool ai_memblock_cmo_triple_block_matches_seed_except_word(
    uintptr_t paddr,
    uint64_t seed,
    size_t changed_word,
    uint64_t changed_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == changed_word) ?
            changed_value : seed + i * 0x1111111111111111ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline bool ai_memblock_cmo_triple_block_matches_seed_except_two_words(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_changed_word,
    uint64_t first_changed_value,
    size_t second_changed_word,
    uint64_t second_changed_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == first_changed_word) {
            expected = first_changed_value;
        } else if (i == second_changed_word) {
            expected = second_changed_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline bool ai_memblock_cmo_triple_block_matches_seed_except_four_words(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_changed_word,
    uint64_t first_changed_value,
    size_t second_changed_word,
    uint64_t second_changed_value,
    size_t third_changed_word,
    uint64_t third_changed_value,
    size_t fourth_changed_word,
    uint64_t fourth_changed_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == first_changed_word) {
            expected = first_changed_value;
        } else if (i == second_changed_word) {
            expected = second_changed_value;
        } else if (i == third_changed_word) {
            expected = third_changed_value;
        } else if (i == fourth_changed_word) {
            expected = fourth_changed_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}


static inline void ai_memblock_cmo_triple_three_block_store_after_inval_cbom_window(
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_zero_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_store_vaddr,
    uint64_t block0_store_value,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block0_store_load_vaddr,
    uintptr_t block0_seed_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_store_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_store_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_store_value], 0(%[block0_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        "ld %[block0_store_readback], 0(%[block0_store_load])\n\t"
        "ld %[block0_seed_readback], 0(%[block0_seed_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_store_readback] "=&r"(block0_store_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_inval] "r"(block0_inval_vaddr),
          [block1_zero] "r"(block1_zero_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_store] "r"(block0_store_vaddr),
          [block0_store_value] "r"(block0_store_value),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block0_store_load] "r"(block0_store_load_vaddr),
          [block0_seed_load] "r"(block0_seed_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_store_observed = block0_store_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_zero_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_seed_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_seed_readback], 0(%[block0_seed_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_inval] "r"(block0_inval_vaddr),
          [block1_zero] "r"(block1_zero_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_seed_load] "r"(block0_seed_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_dual_store_window(
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_store_vaddr,
    uint64_t block0_store_value,
    uintptr_t block2_store_vaddr,
    uint64_t block2_store_value,
    uintptr_t block1_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block0_store_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_store_load_vaddr,
    uintptr_t block2_seed_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_store_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_store_observed,
    uint64_t *block2_seed_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_store_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_store_readback;
    uint64_t block2_seed_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_store_value], 0(%[block0_store])\n\t"
        "sd %[block2_store_value], 0(%[block2_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        "ld %[block0_store_readback], 0(%[block0_store_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_store_readback], 0(%[block2_store_load])\n\t"
        "ld %[block2_seed_readback], 0(%[block2_seed_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_store_readback] "=&r"(block0_store_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_store_readback] "=&r"(block2_store_readback),
          [block2_seed_readback] "=&r"(block2_seed_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_store] "r"(block0_store_vaddr),
          [block0_store_value] "r"(block0_store_value),
          [block2_store] "r"(block2_store_vaddr),
          [block2_store_value] "r"(block2_store_value),
          [block1_clean] "r"(block1_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block0_store_load] "r"(block0_store_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_store_load] "r"(block2_store_load_vaddr),
          [block2_seed_load] "r"(block2_seed_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_store_observed = block0_store_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_store_observed = block2_store_readback;
    *block2_seed_observed = block2_seed_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_older_store_then_cmo_window(
    uintptr_t block0_store_vaddr,
    uint64_t block0_store_value,
    uintptr_t block1_store_vaddr,
    uint64_t block1_store_value,
    uintptr_t block2_store_vaddr,
    uint64_t block2_store_value,
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_store_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_store_load_vaddr,
    uintptr_t block1_seed_load_vaddr,
    uintptr_t block2_store_load_vaddr,
    uintptr_t block2_seed_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_store_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_store_observed,
    uint64_t *block1_seed_observed,
    uint64_t *block2_store_observed,
    uint64_t *block2_seed_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_store_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_store_readback;
    uint64_t block1_seed_readback;
    uint64_t block2_store_readback;
    uint64_t block2_seed_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_store_value], 0(%[block0_store])\n\t"
        "sd %[block1_store_value], 0(%[block1_store])\n\t"
        "sd %[block2_store_value], 0(%[block2_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_store_readback], 0(%[block0_store_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_store_readback], 0(%[block1_store_load])\n\t"
        "ld %[block1_seed_readback], 0(%[block1_seed_load])\n\t"
        "ld %[block2_store_readback], 0(%[block2_store_load])\n\t"
        "ld %[block2_seed_readback], 0(%[block2_seed_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_store_readback] "=&r"(block0_store_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_store_readback] "=&r"(block1_store_readback),
          [block1_seed_readback] "=&r"(block1_seed_readback),
          [block2_store_readback] "=&r"(block2_store_readback),
          [block2_seed_readback] "=&r"(block2_seed_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_store] "r"(block0_store_vaddr),
          [block0_store_value] "r"(block0_store_value),
          [block1_store] "r"(block1_store_vaddr),
          [block1_store_value] "r"(block1_store_value),
          [block2_store] "r"(block2_store_vaddr),
          [block2_store_value] "r"(block2_store_value),
          [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_store_load] "r"(block0_store_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_store_load] "r"(block1_store_load_vaddr),
          [block1_seed_load] "r"(block1_seed_load_vaddr),
          [block2_store_load] "r"(block2_store_load_vaddr),
          [block2_seed_load] "r"(block2_seed_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_store_observed = block0_store_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_store_observed = block1_store_readback;
    *block1_seed_observed = block1_seed_readback;
    *block2_store_observed = block2_store_readback;
    *block2_seed_observed = block2_seed_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_zero_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_zero] "r"(block0_zero_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_zero_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_inval_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_zero], 4\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_zero] "r"(block2_zero_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_inval] "r"(block0_inval_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_inval_vaddr,
    uintptr_t block1_zero_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_zero_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_inval] "r"(block0_inval_vaddr),
          [block1_zero] "r"(block1_zero_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_zero] "r"(block0_zero_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_clean_vaddr,
    uintptr_t block1_zero_vaddr,
    uintptr_t block2_inval_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_inval_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_seed_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_inval], 0\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_seed_readback], 0(%[block0_seed_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_clean] "r"(block0_clean_vaddr),
          [block1_zero] "r"(block1_zero_vaddr),
          [block2_inval] "r"(block2_inval_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_inval] "r"(block0_inval_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr),
          [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_seed_load] "r"(block0_seed_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_window(
    uintptr_t block0_older_store_vaddr,
    uint64_t block0_older_store_value,
    uintptr_t block0_zero_vaddr,
    uintptr_t block1_inval_vaddr,
    uintptr_t block2_zero_vaddr,
    uintptr_t block0_middle_store_vaddr,
    uint64_t block0_middle_store_value,
    uintptr_t block1_clean_vaddr,
    uintptr_t block2_clean_vaddr,
    uintptr_t block0_inval_vaddr,
    uintptr_t block0_younger_store_vaddr,
    uint64_t block0_younger_store_value,
    uintptr_t block0_clean_vaddr,
    uintptr_t block0_flush_vaddr,
    uintptr_t block1_flush_vaddr,
    uintptr_t block2_flush_vaddr,
    uintptr_t block0_older_load_vaddr,
    uintptr_t block0_middle_load_vaddr,
    uintptr_t block0_younger_load_vaddr,
    uintptr_t block0_zero_load_vaddr,
    uintptr_t block1_load_vaddr,
    uintptr_t block2_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_middle_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_middle_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 0(%[block0_older_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_zero], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_inval], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_zero], 4\n\t"
        "sd %[block0_middle_store_value], 0(%[block0_middle_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_inval], 0\n\t"
        "sd %[block0_younger_store_value], 0(%[block0_younger_store])\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_clean], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[block0_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block1_flush], 2\n\t"
        ".insn i 0x0f, 0x2, x0, %[block2_flush], 2\n\t"
        ".option pop\n\t"
        :
        : [block0_older_store] "r"(block0_older_store_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_zero] "r"(block0_zero_vaddr),
          [block1_inval] "r"(block1_inval_vaddr),
          [block2_zero] "r"(block2_zero_vaddr),
          [block0_middle_store] "r"(block0_middle_store_vaddr),
          [block0_middle_store_value] "r"(block0_middle_store_value),
          [block1_clean] "r"(block1_clean_vaddr),
          [block2_clean] "r"(block2_clean_vaddr),
          [block0_inval] "r"(block0_inval_vaddr),
          [block0_younger_store] "r"(block0_younger_store_vaddr),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [block0_clean] "r"(block0_clean_vaddr),
          [block0_flush] "r"(block0_flush_vaddr),
          [block1_flush] "r"(block1_flush_vaddr),
          [block2_flush] "r"(block2_flush_vaddr)
        : "memory");

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[block0_older_readback], 0(%[block0_older_load])\n\t"
        "ld %[block0_middle_readback], 0(%[block0_middle_load])\n\t"
        "ld %[block0_younger_readback], 0(%[block0_younger_load])\n\t"
        "ld %[block0_zero_readback], 0(%[block0_zero_load])\n\t"
        "ld %[block1_readback], 0(%[block1_load])\n\t"
        "ld %[block2_readback], 0(%[block2_load])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_middle_readback] "=&r"(block0_middle_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0_older_load] "r"(block0_older_load_vaddr),
          [block0_middle_load] "r"(block0_middle_load_vaddr),
          [block0_younger_load] "r"(block0_younger_load_vaddr),
          [block0_zero_load] "r"(block0_zero_load_vaddr),
          [block1_load] "r"(block1_load_vaddr),
          [block2_load] "r"(block2_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *block0_older_observed = block0_older_readback;
    *block0_middle_observed = block0_middle_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_middle_store_value,
    uint64_t block0_younger_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_middle_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_middle_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_middle_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_younger_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_middle_readback], 32(%[block0])\n\t"
        "ld %[block0_younger_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_middle_readback] "=&r"(block0_middle_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_middle_store_value] "r"(block0_middle_store_value),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_middle_observed = block0_middle_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_middle_store_value,
    uint64_t block0_younger_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_middle_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_middle_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_middle_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_younger_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_middle_readback], 32(%[block0])\n\t"
        "ld %[block0_younger_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_middle_readback] "=&r"(block0_middle_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_middle_store_value] "r"(block0_middle_store_value),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_middle_observed = block0_middle_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_middle_store_value,
    uint64_t block0_younger_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_middle_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_middle_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_middle_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_younger_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_middle_readback], 32(%[block0])\n\t"
        "ld %[block0_younger_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_middle_readback] "=&r"(block0_middle_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_middle_store_value] "r"(block0_middle_store_value),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_middle_observed = block0_middle_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_middle_store_value,
    uint64_t block0_younger_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_middle_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_middle_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_middle_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_younger_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_middle_readback], 32(%[block0])\n\t"
        "ld %[block0_younger_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_middle_readback] "=&r"(block0_middle_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_middle_store_value] "r"(block0_middle_store_value),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_middle_observed = block0_middle_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_middle_store_value,
    uint64_t block0_younger_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_middle_observed,
    uint64_t *block0_younger_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_middle_readback;
    uint64_t block0_younger_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_middle_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_younger_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_middle_readback], 32(%[block0])\n\t"
        "ld %[block0_younger_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_middle_readback] "=&r"(block0_middle_readback),
          [block0_younger_readback] "=&r"(block0_younger_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_middle_store_value] "r"(block0_middle_store_value),
          [block0_younger_store_value] "r"(block0_younger_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_middle_observed = block0_middle_readback;
    *block0_younger_observed = block0_younger_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_inval_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_inval_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_zero_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_zero_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_inval_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_cbom_store_inval_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_cbom_store_zero_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_zero_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_zero_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_zero_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_zero_readback] "=&r"(block0_zero_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_zero_observed = block0_zero_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}



static inline void ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_cbom_store_cbom_window(
    uintptr_t block0_vaddr,
    uintptr_t block1_vaddr,
    uintptr_t block2_vaddr,
    uintptr_t marker_vaddr,
    uint64_t block0_older_store_value,
    uint64_t block0_second_store_value,
    uint64_t block0_third_store_value,
    uint64_t block0_fourth_store_value,
    uint64_t marker_value,
    uint64_t *block0_older_observed,
    uint64_t *block0_second_observed,
    uint64_t *block0_third_observed,
    uint64_t *block0_fourth_observed,
    uint64_t *block0_seed_observed,
    uint64_t *block1_observed,
    uint64_t *block2_observed,
    uint64_t *marker_observed)
{
    uint64_t block0_older_readback;
    uint64_t block0_second_readback;
    uint64_t block0_third_readback;
    uint64_t block0_fourth_readback;
    uint64_t block0_seed_readback;
    uint64_t block1_readback;
    uint64_t block2_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[block0_older_store_value], 8(%[block0])\n\t"
        "addi t0, %[block0], 24\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "addi t0, %[block1], 16\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 4\n\t"
        "addi t0, %[block2], 8\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 0\n\t"
        "sd %[block0_second_store_value], 32(%[block0])\n\t"
        "addi t0, %[block1], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block2], 48\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "addi t0, %[block0], 40\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_third_store_value], 48(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        "sd %[block0_fourth_store_value], 0(%[block0])\n\t"
        "addi t0, %[block0], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 1\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block1], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "addi t0, %[block2], 56\n\t"
        ".insn i 0x0f, 0x2, x0, t0, 2\n\t"
        "ld %[block0_fourth_readback], 0(%[block0])\n\t"
        "ld %[block0_older_readback], 8(%[block0])\n\t"
        "ld %[block0_seed_readback], 16(%[block0])\n\t"
        "ld %[block0_second_readback], 32(%[block0])\n\t"
        "ld %[block0_third_readback], 48(%[block0])\n\t"
        "ld %[block1_readback], 24(%[block1])\n\t"
        "ld %[block2_readback], 40(%[block2])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [block0_older_readback] "=&r"(block0_older_readback),
          [block0_second_readback] "=&r"(block0_second_readback),
          [block0_third_readback] "=&r"(block0_third_readback),
          [block0_fourth_readback] "=&r"(block0_fourth_readback),
          [block0_seed_readback] "=&r"(block0_seed_readback),
          [block1_readback] "=&r"(block1_readback),
          [block2_readback] "=&r"(block2_readback),
          [marker_readback] "=&r"(marker_readback)
        : [block0] "r"(block0_vaddr),
          [block1] "r"(block1_vaddr),
          [block2] "r"(block2_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [block0_older_store_value] "r"(block0_older_store_value),
          [block0_second_store_value] "r"(block0_second_store_value),
          [block0_third_store_value] "r"(block0_third_store_value),
          [block0_fourth_store_value] "r"(block0_fourth_store_value),
          [marker_value] "r"(marker_value)
        : "t0", "memory");

    *block0_older_observed = block0_older_readback;
    *block0_second_observed = block0_second_readback;
    *block0_third_observed = block0_third_readback;
    *block0_fourth_observed = block0_fourth_readback;
    *block0_seed_observed = block0_seed_readback;
    *block1_observed = block1_readback;
    *block2_observed = block2_readback;
    *marker_observed = marker_readback;
}

#endif /* AI_MICRO_MEMBLOCK_CMO_TRIPLE_COMMON_H */
