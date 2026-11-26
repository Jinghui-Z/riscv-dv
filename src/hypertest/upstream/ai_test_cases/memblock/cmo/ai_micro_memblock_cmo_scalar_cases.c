#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CMO_SCALAR_ROUNDS 3U
#define AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS 8U

static inline void ai_memblock_cmo_scalar_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_scalar_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_scalar_prepare_hs_cbom_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_scalar_reset_hs_cbom_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_scalar_prepare_hs_inval_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_scalar_reset_hs_inval_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_scalar_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cmo_scalar_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_scalar_block_zero_except_word(
    uintptr_t paddr,
    size_t non_zero_word,
    uint64_t non_zero_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == non_zero_word) ? non_zero_value : 0ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_scalar_block_matches_seed_except_word(
    uintptr_t paddr,
    uint64_t seed,
    size_t changed_word,
    uint64_t changed_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == changed_word) ? changed_value :
            seed + i * 0x1111111111111111ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline uint64_t ai_memblock_cmo_scalar_byte_at(uint64_t word,
    size_t byte_index)
{
    return (word >> (byte_index * 8U)) & 0xffULL;
}

static inline uint64_t ai_memblock_cmo_scalar_halfword_at(uint64_t word,
    size_t byte_index)
{
    return (word >> (byte_index * 8U)) & 0xffffULL;
}

static inline uint64_t ai_memblock_cmo_scalar_sign_extend8(uint64_t value)
{
    value &= 0xffULL;
    return (value & 0x80ULL) ? (value | ~0xffULL) : value;
}

static inline uint64_t ai_memblock_cmo_scalar_sign_extend16(uint64_t value)
{
    value &= 0xffffULL;
    return (value & 0x8000ULL) ? (value | ~0xffffULL) : value;
}

static inline void ai_memblock_cbozero_then_scalar_load_window(
    uintptr_t zero_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t loaded_value;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "ld %[loaded_value], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [loaded_value] "=&r"(loaded_value),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = loaded_value;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cboinval_then_narrow_loads_window(
    uintptr_t inval_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *lb_observed,
    uint64_t *lbu_observed,
    uint64_t *lh_observed,
    uint64_t *lhu_observed,
    uint64_t *marker_observed)
{
    uint64_t lb_readback;
    uint64_t lbu_readback;
    uint64_t lh_readback;
    uint64_t lhu_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "lb %[lb_readback], 1(%[load_addr])\n\t"
        "lbu %[lbu_readback], 2(%[load_addr])\n\t"
        "lh %[lh_readback], 4(%[load_addr])\n\t"
        "lhu %[lhu_readback], 6(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lb_readback] "=&r"(lb_readback),
          [lbu_readback] "=&r"(lbu_readback),
          [lh_readback] "=&r"(lh_readback),
          [lhu_readback] "=&r"(lhu_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lb_observed = lb_readback;
    *lbu_observed = lbu_readback;
    *lh_observed = lh_readback;
    *lhu_observed = lhu_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cboinval_then_scalar_load_window(
    uintptr_t inval_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t loaded_value;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "ld %[loaded_value], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [loaded_value] "=&r"(loaded_value),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = loaded_value;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cboinval_then_scalar_store_window(
    uintptr_t inval_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cboinval_then_narrow_stores_window(
    uintptr_t inval_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t byte_value,
    uint64_t halfword_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sb %[byte_value], 1(%[store_addr])\n\t"
        "sh %[halfword_value], 4(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"(byte_value),
          [halfword_value] "r"(halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbozero_then_scalar_store_window(
    uintptr_t zero_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbozero_then_narrow_stores_window(
    uintptr_t zero_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t byte_value,
    uint64_t halfword_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sb %[byte_value], 1(%[store_addr])\n\t"
        "sh %[halfword_value], 4(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"(byte_value),
          [halfword_value] "r"(halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbozero_then_narrow_loads_window(
    uintptr_t zero_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *lb_observed,
    uint64_t *lbu_observed,
    uint64_t *lh_observed,
    uint64_t *lhu_observed,
    uint64_t *lw_observed,
    uint64_t *ld_observed,
    uint64_t *marker_observed)
{
    uint64_t lb_readback;
    uint64_t lbu_readback;
    uint64_t lh_readback;
    uint64_t lhu_readback;
    uint64_t lw_readback;
    uint64_t ld_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "lb %[lb_readback], 1(%[load_addr])\n\t"
        "lbu %[lbu_readback], 2(%[load_addr])\n\t"
        "lh %[lh_readback], 4(%[load_addr])\n\t"
        "lhu %[lhu_readback], 6(%[load_addr])\n\t"
        "lw %[lw_readback], 8(%[load_addr])\n\t"
        "ld %[ld_readback], 16(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lb_readback] "=&r"(lb_readback),
          [lbu_readback] "=&r"(lbu_readback),
          [lh_readback] "=&r"(lh_readback),
          [lhu_readback] "=&r"(lhu_readback),
          [lw_readback] "=&r"(lw_readback),
          [ld_readback] "=&r"(ld_readback),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lb_observed = lb_readback;
    *lbu_observed = lbu_readback;
    *lh_observed = lh_readback;
    *lhu_observed = lhu_readback;
    *lw_observed = lw_readback;
    *ld_observed = ld_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_scalar_store_then_cbozero_window(
    uintptr_t store_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *store_after_zero_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [store_addr] "r"(store_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_after_zero_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_narrow_stores_then_cbozero_window(
    uintptr_t store_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t byte_value,
    uint64_t halfword_value,
    uint64_t marker_value,
    uint64_t *store_after_zero_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sb %[byte_value], 1(%[store_addr])\n\t"
        "sh %[halfword_value], 4(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [store_addr] "r"(store_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"(byte_value),
          [halfword_value] "r"(halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_after_zero_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_narrow_loads_then_cbozero_window(
    uintptr_t load_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *lb_observed,
    uint64_t *lbu_observed,
    uint64_t *lh_observed,
    uint64_t *lhu_observed,
    uint64_t *zero_readback_observed,
    uint64_t *marker_observed)
{
    uint64_t lb_readback;
    uint64_t lbu_readback;
    uint64_t lh_readback;
    uint64_t lhu_readback;
    uint64_t zero_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lb %[lb_readback], 1(%[load_addr])\n\t"
        "lbu %[lbu_readback], 2(%[load_addr])\n\t"
        "lh %[lh_readback], 4(%[load_addr])\n\t"
        "lhu %[lhu_readback], 6(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[zero_readback], 0(%[load_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lb_readback] "=&r"(lb_readback),
          [lbu_readback] "=&r"(lbu_readback),
          [lh_readback] "=&r"(lh_readback),
          [lhu_readback] "=&r"(lhu_readback),
          [zero_readback] "=&r"(zero_readback),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lb_observed = lb_readback;
    *lbu_observed = lbu_readback;
    *lh_observed = lh_readback;
    *lhu_observed = lhu_readback;
    *zero_readback_observed = zero_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_scalar_load_then_cbozero_window(
    uintptr_t load_vaddr,
    uintptr_t zero_vaddr,
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
        "ld %[load_readback], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_scalar_load_then_cboinval_window(
    uintptr_t load_vaddr,
    uintptr_t inval_vaddr,
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
        "ld %[load_readback], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_narrow_loads_then_cboinval_window(
    uintptr_t load_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *lb_observed,
    uint64_t *lbu_observed,
    uint64_t *lh_observed,
    uint64_t *lhu_observed,
    uint64_t *marker_observed)
{
    uint64_t lb_readback;
    uint64_t lbu_readback;
    uint64_t lh_readback;
    uint64_t lhu_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lb %[lb_readback], 1(%[load_addr])\n\t"
        "lbu %[lbu_readback], 2(%[load_addr])\n\t"
        "lh %[lh_readback], 4(%[load_addr])\n\t"
        "lhu %[lhu_readback], 6(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lb_readback] "=&r"(lb_readback),
          [lbu_readback] "=&r"(lbu_readback),
          [lh_readback] "=&r"(lh_readback),
          [lhu_readback] "=&r"(lhu_readback),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lb_observed = lb_readback;
    *lbu_observed = lbu_readback;
    *lh_observed = lh_readback;
    *lhu_observed = lhu_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_scalar_store_then_cboinval_window(
    uintptr_t store_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [store_addr] "r"(store_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_narrow_stores_then_cboinval_window(
    uintptr_t store_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t marker_vaddr,
    uint64_t byte_value,
    uint64_t halfword_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sb %[byte_value], 1(%[store_addr])\n\t"
        "sh %[halfword_value], 4(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [store_addr] "r"(store_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"(byte_value),
          [halfword_value] "r"(halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cboinval_then_cbom_window(
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
        : [inval_addr] "r"(inval_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbom_then_scalar_store_window(
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbom_then_cboinval_window(
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
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cboinval_then_cbozero_window(
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
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbozero_then_cboinval_window(
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
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [zero_addr] "r"(zero_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbom_then_narrow_stores_window(
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t byte_value,
    uint64_t halfword_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sb %[byte_value], 1(%[store_addr])\n\t"
        "sh %[halfword_value], 4(%[store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [store_addr] "r"(store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"(byte_value),
          [halfword_value] "r"(halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbom_then_narrow_loads_window(
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *lb_observed,
    uint64_t *lbu_observed,
    uint64_t *lh_observed,
    uint64_t *lhu_observed,
    uint64_t *marker_observed)
{
    uint64_t lb_readback;
    uint64_t lbu_readback;
    uint64_t lh_readback;
    uint64_t lhu_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "lb %[lb_readback], 1(%[load_addr])\n\t"
        "lbu %[lbu_readback], 2(%[load_addr])\n\t"
        "lh %[lh_readback], 4(%[load_addr])\n\t"
        "lhu %[lhu_readback], 6(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lb_readback] "=&r"(lb_readback),
          [lbu_readback] "=&r"(lbu_readback),
          [lh_readback] "=&r"(lh_readback),
          [lhu_readback] "=&r"(lhu_readback),
          [marker_readback] "=&r"(marker_readback)
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lb_observed = lb_readback;
    *lbu_observed = lbu_readback;
    *lh_observed = lh_readback;
    *lhu_observed = lhu_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_cbom_then_scalar_load_window(
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
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_scalar_load_then_cbom_window(
    uintptr_t load_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
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
        "ld %[load_readback], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_scalar_store_then_cbom_window(
    uintptr_t store_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t store_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_value], 0(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [store_addr] "r"(store_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [store_value] "r"(store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_narrow_stores_then_cbom_window(
    uintptr_t store_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t byte_value,
    uint64_t halfword_value,
    uint64_t marker_value,
    uint64_t *store_observed,
    uint64_t *marker_observed)
{
    uint64_t store_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sb %[byte_value], 1(%[store_addr])\n\t"
        "sh %[halfword_value], 4(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[store_readback], 0(%[store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [store_readback] "=&r"(store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [store_addr] "r"(store_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [byte_value] "r"(byte_value),
          [halfword_value] "r"(halfword_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *store_observed = store_readback;
    *marker_observed = marker_readback;
}

static inline void ai_memblock_narrow_loads_then_cbom_window(
    uintptr_t load_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *lb_observed,
    uint64_t *lbu_observed,
    uint64_t *lh_observed,
    uint64_t *lhu_observed,
    uint64_t *marker_observed)
{
    uint64_t lb_readback;
    uint64_t lbu_readback;
    uint64_t lh_readback;
    uint64_t lhu_readback;
    uint64_t marker_readback;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lb %[lb_readback], 1(%[load_addr])\n\t"
        "lbu %[lbu_readback], 2(%[load_addr])\n\t"
        "lh %[lh_readback], 4(%[load_addr])\n\t"
        "lhu %[lhu_readback], 6(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lb_readback] "=&r"(lb_readback),
          [lbu_readback] "=&r"(lbu_readback),
          [lh_readback] "=&r"(lh_readback),
          [lhu_readback] "=&r"(lhu_readback),
          [marker_readback] "=&r"(marker_readback)
        : [load_addr] "r"(load_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lb_observed = lb_readback;
    *lbu_observed = lbu_readback;
    *lh_observed = lh_readback;
    *lhu_observed = lhu_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbozero_then_scalar_load_no_stale_zero_flush_owner_or_load_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x880U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x880U;
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t scalar_load_vaddr = source_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x880U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x880U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1021324354657687ULL,
        0x2031425364758697ULL,
        0x30415263748596a7ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0badf00d5a5aa5a5ULL,
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then scalar-load owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_scalar_load_window(zero_vaddr,
            scalar_load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by ordinary aligned scalar load keeps zero ownership ordered so load0 observes zero and marker progresses",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then scalar-load owner-window leaves the full source block zeroed and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then scalar-load owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_scalar_load_no_stale_inval_owner_or_refill_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x780U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x780U;
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t scalar_load_vaddr =
        source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x780U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x780U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1234567890abcdefULL,
        0xfedcba0987654321ULL,
        0x55aa00ff33cc77eeULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xee77cc33ff00aa55ULL,
        0x0f0e0d0c0b0a0908ULL,
        0x8070605040302010ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1020304050607080ULL,
        0x8090a0b0c0d0e0f0ULL,
        0x3141592653589793ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2718281828459045ULL,
        0x1123581321345589ULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x76543210fedcba98ULL,
        0xcafebabedeadbeefULL,
        0x0badf00d5a5aa5a5ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_inval = block_seeds[round] +
            2U * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then scalar-load refill window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sfence_vma_rs(inval_vaddr, (uint64_t)-1);
        AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,x0 retires on the same VA that the following CBO.INVAL will target",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_scalar_load_window(inval_vaddr,
            scalar_load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by ordinary aligned scalar load releases inval ownership so the refill returns current payload and marker progresses",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t same_va_observed = ld(inval_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after SFENCE.VMA followed by CBO.INVAL on the same VA, a same-VA scalar load refills the current backing payload",
            same_va_observed == expected_inval
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then scalar-load refill window preserves the source block image and all guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then scalar-load refill window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], load_word, expected_load) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_scalar_store_no_stale_inval_owner_or_store_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 4U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x680U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x680U;
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t scalar_store_vaddr =
        source_vaddr + store_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x680U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x680U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t store_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x01020304aabbccddULL,
        0x55667788ddeeff00ULL,
        0x99aabbcc00112233ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x123456789abcdef0ULL,
        0xf0e1d2c3b4a59687ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xaaaaaaaa55555555ULL,
        0xcccccccc33333333ULL,
        0xf0f0f0f00f0f0f0fULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13572468ace0bdf1ULL,
        0xf1bde0ac68245713ULL,
        0xdeadbeef11223344ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55aa55aa00ff00ffULL,
        0xff00ff0055aa55aaULL,
        0x1021324354657687ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then scalar-store owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_scalar_store_window(inval_vaddr,
            scalar_store_vaddr, marker_vaddr, store_updates[round],
            marker_updates[round], &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by ordinary aligned scalar store releases inval ownership so the store payload and marker survive",
            store_observed == store_updates[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then scalar-store owner-window preserves the source block except for the later scalar store and keeps all guards intact",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_store_vaddr) == store_updates[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then scalar-store owner-window contains the later store word, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, store_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_narrow_scalar_loads_no_stale_inval_owner_or_signed_lane_leak_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xb00U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xb00U;
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x4a5b6c7d8e9fb0c1ULL,
        0x5b6c7d8e9fb0c1d2ULL,
        0x6c7d8e9fb0c1d2e3ULL,
    };
    const uint64_t lane_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8001f00dff807f66ULL,
        0x7ffec00380ff5a55ULL,
        0xffff80017e817c7bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0xfdb97531eca86420ULL,
        0x0102040810204080ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x1234432112344321ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2211443366558877ULL,
        0x7788556633441122ULL,
        0xaabbccddeeff0011ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfaceb00cfaceb00cULL,
        0xdec0deeddec0deedULL,
        0xfeed1234feed1234ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0c0ffee00c0ffee0ULL,
        0x0ddc0ffe0ddc0ffeULL,
        0x1029384756102938ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t lb_observed = 0ULL;
        uint64_t lbu_observed = 0ULL;
        uint64_t lh_observed = 0ULL;
        uint64_t lhu_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t lane_word = lane_words[round];
        const uint64_t expected_lb = ai_memblock_cmo_scalar_sign_extend8(
            ai_memblock_cmo_scalar_byte_at(lane_word, 1U));
        const uint64_t expected_lbu =
            ai_memblock_cmo_scalar_byte_at(lane_word, 2U);
        const uint64_t expected_lh = ai_memblock_cmo_scalar_sign_extend16(
            ai_memblock_cmo_scalar_halfword_at(lane_word, 4U));
        const uint64_t expected_lhu =
            ai_memblock_cmo_scalar_halfword_at(lane_word, 6U);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, lane_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then narrow-load refill window starts, source lanes contain signedness candidates and marker is readable",
            ld(source_vaddr) == lane_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_narrow_loads_window(inval_vaddr,
            source_vaddr, marker_vaddr, marker_updates[round],
            &lb_observed, &lbu_observed, &lh_observed, &lhu_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by lb/lbu/lh/lhu must refill current narrow lanes with correct signed and unsigned extension",
            lb_observed == expected_lb &&
            lbu_observed == expected_lbu &&
            lh_observed == expected_lh &&
            lhu_observed == expected_lhu &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then narrow-load refill window preserves the source block image and all guards",
            ld(source_vaddr) == lane_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then narrow-load refill window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], 0U, lane_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_narrow_scalar_stores_no_stale_inval_owner_or_byte_mask_drop_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 0U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xb80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xb80U;
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t base_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8070605040302010ULL,
        0xf1e2d3c4b5a69788ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t byte_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa6ULL,
        0x39ULL,
        0xd4ULL,
    };
    const uint64_t halfword_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x5aa5ULL,
        0xc33cULL,
        0xe77eULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x777788889999aaaaULL,
        0xbbbbccccddddeeeeULL,
        0x2468246813571357ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1020304050607080ULL,
        0x9080706050403020ULL,
        0xabcdefabcdef1234ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55aa77110badf00dULL,
        0xccdd33441122aabbULL,
        0x0123456789abcdefULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0xa55aa55a5aa55aa5ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t base_word = base_words[round];
        const uint64_t expected_word =
            (base_word & ~((0xffULL << 8) | (0xffffULL << 32))) |
            (byte_updates[round] << 8) |
            (halfword_updates[round] << 32);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, base_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then narrow-store owner-window starts, source word0/tail and marker are readable with seeded values",
            ld(source_vaddr) == base_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_narrow_stores_window(inval_vaddr,
            source_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by byte and halfword scalar stores releases inval ownership so the narrow byte mask, halfword mask, and marker survive",
            store_observed == expected_word &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then narrow-store owner-window preserves untouched lanes, source tail, marker guard, and surrounding guards",
            ld(source_vaddr) == expected_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then narrow-store owner-window contains the byte/halfword merged word0, preserved sibling words, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, expected_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_scalar_load_then_cboinval_no_lost_load0_or_stale_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x580U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x580U;
    const uintptr_t scalar_load_vaddr =
        source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x580U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x580U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a1a2a3a4a5a6a7aULL,
        0x8b7b6b5b4b3b2b1bULL,
        0xc0ffee123456789aULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa9876543210ffeedULL,
        0x1122cc44ee668800ULL,
        0x55aa77cc99ee33ddULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0102030405060708ULL,
        0x8877665544332211ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1020304050607081ULL,
        0x9080706050403021ULL,
        0x31415926abcdef12ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xaaaaaaaa00000001ULL,
        0xbbbbbbbb00000002ULL,
        0xcccccccc00000003ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-load then CBO.INVAL owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_load_then_cboinval_window(scalar_load_vaddr,
            inval_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary aligned scalar load immediately followed by CBO.INVAL keeps load0 writeback and inval ownership ordered so the load payload and marker survive",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-load then CBO.INVAL owner-window preserves the source backing image after refill and keeps all guards intact",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-load then CBO.INVAL owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], load_word, expected_load) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_scalar_store_then_cboinval_no_stale_storequeue_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 4U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x480U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x480U;
    const uintptr_t scalar_store_vaddr =
        source_vaddr + store_word * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x480U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x480U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x405162738495a6b7ULL,
        0x5162738495a6b7c8ULL,
        0x62738495a6b7c8d9ULL,
    };
    const uint64_t store_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0ddba11c12345678ULL,
        0x87654321c11abdd0ULL,
        0x5eedf00dace0bdf1ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1111aaaa2222bbbbULL,
        0x3333cccc4444ddddULL,
        0x5555eeee6666ffffULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xffff6666eeee5555ULL,
        0xdddd4444cccc3333ULL,
        0xbbbb2222aaaa1111ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2468ace013579bdfULL,
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa5a500005a5a0000ULL,
        0x5a5a0000a5a50000ULL,
        0xc3c300003c3c0000ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-store then CBO.INVAL owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_store_then_cboinval_window(scalar_store_vaddr,
            inval_vaddr, marker_vaddr, store_updates[round],
            marker_updates[round], &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary aligned scalar store immediately followed by CBO.INVAL keeps StoreQueue and inval ownership ordered so the store payload and marker survive",
            store_observed == store_updates[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-store then CBO.INVAL owner-window preserves the prior store word and all guards after refill",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_store_vaddr) == store_updates[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-store then CBO.INVAL owner-window contains the prior store word, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, store_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_narrow_scalar_loads_then_cboinval_no_lost_lane_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xd80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xd80U;
    const uintptr_t inval_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x60718293a4b5c6d7ULL,
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
    };
    const uint64_t lane_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8001f00dff807f66ULL,
        0x7ffec00380ff5a55ULL,
        0xffff80017e817c7bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xabcdef0123456789ULL,
        0x9876543210fedcbaULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc001d00dc001d00dULL,
        0xd15ea5ed12345678ULL,
        0x0ddba11a0ddba11aULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x11100f0e0d0c0b0aULL,
        0x2468135724681357ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t lb_observed = 0ULL;
        uint64_t lbu_observed = 0ULL;
        uint64_t lh_observed = 0ULL;
        uint64_t lhu_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t lane_word = lane_words[round];
        const uint64_t expected_lb = ai_memblock_cmo_scalar_sign_extend8(
            ai_memblock_cmo_scalar_byte_at(lane_word, 1U));
        const uint64_t expected_lbu =
            ai_memblock_cmo_scalar_byte_at(lane_word, 2U);
        const uint64_t expected_lh = ai_memblock_cmo_scalar_sign_extend16(
            ai_memblock_cmo_scalar_halfword_at(lane_word, 4U));
        const uint64_t expected_lhu =
            ai_memblock_cmo_scalar_halfword_at(lane_word, 6U);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, lane_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the narrow-load then CBO.INVAL owner-window starts, source lanes contain signedness candidates and marker is readable",
            ld(source_vaddr) == lane_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_narrow_loads_then_cboinval_window(source_vaddr,
            inval_vaddr, marker_vaddr, marker_updates[round],
            &lb_observed, &lbu_observed, &lh_observed, &lhu_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("lb/lbu/lh/lhu immediately followed by CBO.INVAL must preserve the older narrow load results while inval ownership retires cleanly",
            lb_observed == expected_lb &&
            lbu_observed == expected_lbu &&
            lh_observed == expected_lh &&
            lhu_observed == expected_lhu &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the narrow-load then CBO.INVAL owner-window preserves the source block image and all guards",
            ld(source_vaddr) == lane_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the narrow-load then CBO.INVAL owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], 0U, lane_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_narrow_scalar_stores_then_cboinval_no_stale_mask_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 0U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xd00U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xd00U;
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t base_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8070605040302010ULL,
        0xf1e2d3c4b5a69788ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t byte_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x4bULL,
        0xd2ULL,
        0x9eULL,
    };
    const uint64_t halfword_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1ee1ULL,
        0x6cc6ULL,
        0xb44bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1122112233443344ULL,
        0x5566556677887788ULL,
        0x99aa99aabbccbbccULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x1234432112344321ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2211443366558877ULL,
        0x7788556633441122ULL,
        0xaabbccddeeff0011ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfaceb00cfaceb00cULL,
        0xdec0deeddec0deedULL,
        0xfeed1234feed1234ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0c0ffee00c0ffee0ULL,
        0x0ddc0ffe0ddc0ffeULL,
        0x1029384756102938ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t base_word = base_words[round];
        const uint64_t expected_word =
            (base_word & ~((0xffULL << 8) | (0xffffULL << 32))) |
            (byte_updates[round] << 8) |
            (halfword_updates[round] << 32);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, base_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the narrow-store then CBO.INVAL owner-window starts, source word0/tail and marker are readable with seeded values",
            ld(source_vaddr) == base_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_narrow_stores_then_cboinval_window(source_vaddr,
            inval_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("byte and halfword scalar stores immediately followed by CBO.INVAL must preserve the merged narrow-store payload after inval ownership retires",
            store_observed == expected_word &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the narrow-store then CBO.INVAL owner-window preserves the merged word0, source tail, marker guard, and surrounding guards",
            ld(source_vaddr) == expected_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the narrow-store then CBO.INVAL owner-window contains the byte/halfword merged word0, preserved sibling words, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, expected_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_scalar_store_then_cbozero_no_stale_storequeue_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 2U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xd80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xd80U;
    const uintptr_t scalar_store_vaddr =
        source_vaddr + store_word * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0f2e4d6c8baa9978ULL,
        0x102f4e6d8cabba89ULL,
        0x21405f7e9dbcab9aULL,
    };
    const uint64_t store_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x11223344aabbccddULL,
        0x55667788ddeeff00ULL,
        0x99aabbcc00112233ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xabcdefabcdef1234ULL,
        0x1234abcdefabcdefULL,
        0x55aa55aa00ff00ffULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xff00ff0055aa55aaULL,
        0x0f0e0d0c0b0a0908ULL,
        0x8070605040302010ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1010101020202020ULL,
        0x3030303040404040ULL,
        0x5050505060606060ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xd00df00dcafebeefULL,
        0xbeefcafed00df00dULL,
        0x0123456789abcdefULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x89abcdeffedcba98ULL,
        0x13579bdf2468ace0ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_after_zero = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-store then CBO.ZERO owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_store_then_cbozero_window(scalar_store_vaddr,
            zero_vaddr, marker_vaddr, store_updates[round],
            marker_updates[round], &store_after_zero, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary aligned scalar store immediately followed by CBO.ZERO keeps StoreQueue and zero ownership ordered so the later zero clears the store word and marker progresses",
            store_after_zero == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-store then CBO.ZERO owner-window leaves the full source block zeroed and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(scalar_store_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-store then CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_narrow_scalar_stores_then_cbozero_no_stale_mask_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xc00U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xc00U;
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x5e4d3c2b1a091827ULL,
        0x6f5e4d3c2b1a2938ULL,
        0x807f6e5d4c3b2a19ULL,
    };
    const uint64_t byte_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x81ULL,
        0x42ULL,
        0xbdULL,
    };
    const uint64_t halfword_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x7e81ULL,
        0xb24dULL,
        0x18e7ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0badc0de0badc0deULL,
        0x1234432112344321ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0102030405060708ULL,
        0x8070605040302010ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xaaaabbbbccccddddULL,
        0xddddccccbbbbaaaaULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc0ffee00c0ffee00ULL,
        0x0ddc0ffe0ddc0ffeULL,
        0xfeedfacefeedfaceULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1357246813572468ULL,
        0x2468135724681357ULL,
        0xabcdef0123456789ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_after_zero = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the narrow-store then CBO.ZERO owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_narrow_stores_then_cbozero_window(source_vaddr,
            zero_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &store_after_zero, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("byte and halfword scalar stores immediately followed by CBO.ZERO must not let stale narrow masks survive the later zero owner",
            store_after_zero == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the narrow-store then CBO.ZERO owner-window leaves the full source block zeroed and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the narrow-store then CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_narrow_scalar_loads_then_cbozero_no_lost_lane_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xc80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xc80U;
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t lane_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8001f00dff807f66ULL,
        0x7ffec00380ff5a55ULL,
        0xffff80017e817c7bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55aa00ff55aa00ffULL,
        0xff0055aaff0055aaULL,
        0x1021324354657687ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0f0f0f0ff0f0f0f0ULL,
        0xf0f0f0f00f0f0f0fULL,
        0xa5a55a5ac3c33c3cULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0xfdb97531eca86420ULL,
        0x0102040810204080ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc001c0dec001c0deULL,
        0xdec0deeddec0deedULL,
        0xfeed1234feed1234ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t lb_observed = 0ULL;
        uint64_t lbu_observed = 0ULL;
        uint64_t lh_observed = 0ULL;
        uint64_t lhu_observed = 0ULL;
        uint64_t zero_readback = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t lane_word = lane_words[round];
        const uint64_t expected_lb = ai_memblock_cmo_scalar_sign_extend8(
            ai_memblock_cmo_scalar_byte_at(lane_word, 1U));
        const uint64_t expected_lbu =
            ai_memblock_cmo_scalar_byte_at(lane_word, 2U);
        const uint64_t expected_lh = ai_memblock_cmo_scalar_sign_extend16(
            ai_memblock_cmo_scalar_halfword_at(lane_word, 4U));
        const uint64_t expected_lhu =
            ai_memblock_cmo_scalar_halfword_at(lane_word, 6U);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, lane_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the narrow-load then CBO.ZERO owner-window starts, source lanes contain signedness candidates and marker is readable",
            ld(source_vaddr) == lane_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_narrow_loads_then_cbozero_window(source_vaddr,
            zero_vaddr, marker_vaddr, marker_updates[round],
            &lb_observed, &lbu_observed, &lh_observed, &lhu_observed,
            &zero_readback, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("lb/lbu/lh/lhu immediately followed by CBO.ZERO must preserve the older narrow load results while the later zero owner clears the block",
            lb_observed == expected_lb &&
            lbu_observed == expected_lbu &&
            lh_observed == expected_lh &&
            lhu_observed == expected_lhu &&
            zero_readback == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the narrow-load then CBO.ZERO owner-window leaves the full source block zeroed and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the narrow-load then CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_scalar_load_then_cbozero_no_lost_load0_or_stale_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xe80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xe80U;
    const uintptr_t scalar_load_vaddr =
        source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1029384756abcdefULL,
        0x213a495867bcdef0ULL,
        0x324b5a6978cdef01ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0badcafe11223344ULL,
        0x55667788badcafe0ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x00112233cafef00dULL,
        0x44556677feedfaceULL,
        0x8899aabb0ddba11cULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x123456780000ffffULL,
        0xfedcba9800001111ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa55aa55a01020304ULL,
        0x5aa55aa5f0e0d0c0ULL,
        0x13579bdf02468aceULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2468ace013579bdfULL,
        0x1122eeff3344ccddULL,
        0xddccbbaa99887766ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-load then CBO.ZERO owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_load_then_cbozero_window(scalar_load_vaddr,
            zero_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary aligned scalar load immediately followed by CBO.ZERO keeps load0 writeback and zero ownership ordered so the old load payload and marker survive",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-load then CBO.ZERO owner-window leaves the full source block zeroed and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(scalar_load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-load then CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_scalar_load_then_cbom_no_lost_load0_or_stale_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xc80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xc80U;
    const uintptr_t scalar_load_vaddr =
        source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xd4c3b2a1908f7e6dULL,
        0xe5d4c3b2a1908f7eULL,
        0xf6e5d4c3b2a1908fULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0xabcdef0012345678ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xdeadbeef00112233ULL,
        0xbeefdead44556677ULL,
        0x778899aabbccddeeULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1111000022223333ULL,
        0x4444000055556666ULL,
        0x7777000088889999ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xabcddcba01233210ULL,
        0x01233210abcddcbaULL,
        0xf00dfacecafebeefULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a0b0c0d0e0f0102ULL,
        0x1020304050607080ULL,
        0x8070605040302010ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-load then CBOM owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_load_then_cbom_window(scalar_load_vaddr,
            clean_vaddr, flush_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary aligned scalar load immediately followed by CBOM clean/flush keeps load0 writeback and CMO ownership ordered so the load payload and marker survive",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-load then CBOM owner-window preserves the source block image and all guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-load then CBOM owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], load_word, expected_load) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_scalar_load_no_stale_cmo_flush_owner_or_load_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 5U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xb80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xb80U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t scalar_load_vaddr =
        source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x445566778899aabbULL,
        0xbb99aa8877665544ULL,
        0x66778899aabbccddULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x89abcdef76543210ULL,
        0x7654321089abcdefULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa5a5a5a55a5a5a5aULL,
        0x5a5a5a5aa5a5a5a5ULL,
        0x123456789abcdef0ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then scalar-load owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbom_then_scalar_load_window(clean_vaddr, flush_vaddr,
            scalar_load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by ordinary aligned scalar load releases the CMO owner so load0 observes the current block payload and marker progresses",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then scalar-load owner-window preserves the source block image and all guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then scalar-load owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], load_word, expected_load) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_scalar_store_no_stale_cmo_flush_owner_or_store_payload_drop_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 4U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xa80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xa80U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t scalar_store_vaddr =
        source_vaddr + store_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
        0x93a4b5c6d7e8f901ULL,
    };
    const uint64_t store_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1122aa55cc33ee77ULL,
        0x8877ee33cc55aa22ULL,
        0x5a6b7c8d9e0f1021ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0ddba11cafef00d1ULL,
        0x0ddba11cafef00d2ULL,
        0x0ddba11cafef00d3ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55ccaa3377ee1122ULL,
        0x66ddbb4488ff2233ULL,
        0x77eecc5599003344ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0xfeed1234beef5678ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1029384756abcdefULL,
        0xefcdab7658473920ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then scalar-store owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbom_then_scalar_store_window(clean_vaddr, flush_vaddr,
            scalar_store_vaddr, marker_vaddr, store_updates[round],
            marker_updates[round], &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by ordinary aligned scalar store releases the CMO owner so the later store payload and marker survive",
            store_observed == store_updates[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then scalar-store owner-window preserves the clean/flush block image except for the later scalar store and all guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_store_vaddr) == store_updates[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then scalar-store owner-window contains the preserved block image, later store word, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, store_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_narrow_scalar_stores_no_stale_cmo_owner_or_byte_mask_drop_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 0U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xe00U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xe00U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x91a2b3c4d5e6f708ULL,
        0xa2b3c4d5e6f70819ULL,
        0xb3c4d5e6f708192aULL,
    };
    const uint64_t base_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1020304050607080ULL,
        0x8877665544332211ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t byte_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2dULL,
        0xe4ULL,
        0x6bULL,
    };
    const uint64_t halfword_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x4dd4ULL,
        0x9aa9ULL,
        0xc33cULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x7071727374757677ULL,
        0x8786858483828180ULL,
        0x123456789abcdef0ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x11100f0e0d0c0b0aULL,
        0x2468135724681357ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x1029384756102938ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfaceb00cfaceb00cULL,
        0xdec0deeddec0deedULL,
        0xfeed1234feed1234ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t base_word = base_words[round];
        const uint64_t expected_word =
            (base_word & ~((0xffULL << 8) | (0xffffULL << 32))) |
            (byte_updates[round] << 8) |
            (halfword_updates[round] << 32);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, base_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then narrow-store owner-window starts, source word0/tail and marker are readable with seeded values",
            ld(source_vaddr) == base_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbom_then_narrow_stores_window(clean_vaddr, flush_vaddr,
            source_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by byte and halfword scalar stores releases the CMO owner so the narrow masks and marker survive",
            store_observed == expected_word &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then narrow-store owner-window preserves untouched lanes, source tail, marker guard, and surrounding guards",
            ld(source_vaddr) == expected_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then narrow-store owner-window contains the byte/halfword merged word0, preserved sibling words, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, expected_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_narrow_scalar_loads_no_stale_cmo_owner_or_signed_lane_leak_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xe80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xe80U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc1d2e3f405162738ULL,
        0xd2e3f40516273849ULL,
        0xe3f405162738495aULL,
    };
    const uint64_t lane_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x80f1a55aff7e6d5cULL,
        0x7f0280cc55aa4433ULL,
        0xffff80017e817c7bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x0eca8642fdb97531ULL,
        0x1123581321345589ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa33cc77ee1199ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1011121314151617ULL,
        0x2726252423222120ULL,
        0x3141592653589793ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfeedfacecafebeefULL,
        0xbeadc0def00d1234ULL,
        0x0badf00d55aa66ccULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1029384756102938ULL,
        0x8374655647382910ULL,
        0xa5a55a5ac3c33c3cULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t lb_observed = 0ULL;
        uint64_t lbu_observed = 0ULL;
        uint64_t lh_observed = 0ULL;
        uint64_t lhu_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t source_word = lane_words[round];
        const uint64_t expected_lb =
            ai_memblock_cmo_scalar_sign_extend8(
                ai_memblock_cmo_scalar_byte_at(source_word, 1U));
        const uint64_t expected_lbu =
            ai_memblock_cmo_scalar_byte_at(source_word, 2U);
        const uint64_t expected_lh =
            ai_memblock_cmo_scalar_sign_extend16(
                ai_memblock_cmo_scalar_halfword_at(source_word, 4U));
        const uint64_t expected_lhu =
            ai_memblock_cmo_scalar_halfword_at(source_word, 6U);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, source_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then narrow-load owner-window starts, source lanes contain signedness candidates and marker is readable",
            ld(source_vaddr) == source_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbom_then_narrow_loads_window(clean_vaddr, flush_vaddr,
            source_vaddr, marker_vaddr, marker_updates[round],
            &lb_observed, &lbu_observed, &lh_observed, &lhu_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by lb/lbu/lh/lhu releases the CMO owner so narrow signed and unsigned lanes return current payload",
            lb_observed == expected_lb &&
            lbu_observed == expected_lbu &&
            lh_observed == expected_lh &&
            lhu_observed == expected_lhu &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then narrow-load owner-window preserves the source block image, marker guard, and surrounding guards",
            ld(source_vaddr) == source_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then narrow-load owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], 0U, source_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_scalar_store_then_cbom_no_stale_storequeue_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 4U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xf80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xf80U;
    const uintptr_t scalar_store_vaddr =
        source_vaddr + store_word * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x61728394a5b6c7d8ULL,
        0x728394a5b6c7d8e9ULL,
        0x8394a5b6c7d8e9faULL,
    };
    const uint64_t store_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc001d00df00d1234ULL,
        0x1234f00dd00dc001ULL,
        0x5a5ac3c33c3ca5a5ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xaaaabbbbccccddddULL,
        0x1111222233334444ULL,
        0xdeadbeefcafed00dULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xddddccccbbbbaaaaULL,
        0x4444333322221111ULL,
        0x0ddba11cfeedfaceULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0101010123232323ULL,
        0x4545454567676767ULL,
        0x89898989ababababULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfeed0000face1111ULL,
        0xbeef2222cafe3333ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x0a0c0e1012141618ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the scalar-store then CBOM owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_scalar_store_then_cbom_window(scalar_store_vaddr,
            clean_vaddr, flush_vaddr, marker_vaddr, store_updates[round],
            marker_updates[round], &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("ordinary aligned scalar store immediately followed by CBOM clean/flush keeps StoreQueue and CMO ownership ordered so the store payload and marker survive",
            store_observed == store_updates[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the scalar-store then CBOM owner-window preserves the block image except for the prior scalar store and all guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(scalar_store_vaddr) == store_updates[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar-store then CBOM owner-window contains the prior store word, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, store_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_narrow_scalar_stores_then_cbom_no_stale_mask_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 0U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xf00U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xf00U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xd1e2f30415263748ULL,
        0xe2f3041526374859ULL,
        0xf30415263748596aULL,
    };
    const uint64_t base_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x66554433221100ffULL,
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t byte_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa6ULL,
        0x3dULL,
        0xe1ULL,
    };
    const uint64_t halfword_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55aaULL,
        0xc33cULL,
        0x7e81ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0908070605040302ULL,
        0x1122334455667788ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8877665544332211ULL,
        0x01030507090b0d0fULL,
        0xf0e0d0c0b0a09080ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1234567812345678ULL,
        0x8765432187654321ULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa0b1c2d3e4f50617ULL,
        0x1706f5e4d3c2b1a0ULL,
        0x5a5a1234c3c3abcdULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xdead0000beef1111ULL,
        0xbeef2222dead3333ULL,
        0x0123456789abcdefULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t base_word = base_words[round];
        const uint64_t expected_word =
            (base_word & ~((0xffULL << 8) | (0xffffULL << 32))) |
            (byte_updates[round] << 8) |
            (halfword_updates[round] << 32);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, base_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the narrow-store then CBOM owner-window starts, source word0/tail and marker are readable with seeded values",
            ld(source_vaddr) == base_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_narrow_stores_then_cbom_window(source_vaddr, clean_vaddr,
            flush_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("byte and halfword scalar stores immediately followed by CBOM clean/flush keep narrow masks ordered before the CMO owner and marker",
            store_observed == expected_word &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the narrow-store then CBOM owner-window preserves merged word0, source tail, marker guard, and surrounding guards",
            ld(source_vaddr) == expected_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the narrow-store then CBOM owner-window contains the byte/halfword merged word0, preserved sibling words, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], store_word, expected_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_narrow_scalar_loads_then_cbom_no_lost_lane_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xf80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xf80U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa3b4c5d6e7f8091aULL,
        0xb4c5d6e7f8091a2bULL,
        0xc5d6e7f8091a2b3cULL,
    };
    const uint64_t lane_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8001f00dff807f66ULL,
        0x7ffec00380ff5a55ULL,
        0xffff80017e817c7bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x18293a4b5c6d7e8fULL,
        0xf1e2d3c4b5a69788ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x76540123fedc89abULL,
        0x89abfedc01237654ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc001d00dfadebeefULL,
        0xbeeffade0ddba11aULL,
        0x1020304050607080ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0123012301230123ULL,
        0x4567456745674567ULL,
        0x89ab89ab89ab89abULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t lb_observed = 0ULL;
        uint64_t lbu_observed = 0ULL;
        uint64_t lh_observed = 0ULL;
        uint64_t lhu_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t source_word = lane_words[round];
        const uint64_t expected_lb =
            ai_memblock_cmo_scalar_sign_extend8(
                ai_memblock_cmo_scalar_byte_at(source_word, 1U));
        const uint64_t expected_lbu =
            ai_memblock_cmo_scalar_byte_at(source_word, 2U);
        const uint64_t expected_lh =
            ai_memblock_cmo_scalar_sign_extend16(
                ai_memblock_cmo_scalar_halfword_at(source_word, 4U));
        const uint64_t expected_lhu =
            ai_memblock_cmo_scalar_halfword_at(source_word, 6U);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, source_word);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the narrow-load then CBOM owner-window starts, source lanes contain signedness candidates and marker is readable",
            ld(source_vaddr) == source_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_narrow_loads_then_cbom_window(source_vaddr, clean_vaddr,
            flush_vaddr, marker_vaddr, marker_updates[round],
            &lb_observed, &lbu_observed, &lh_observed, &lhu_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("lb/lbu/lh/lhu immediately followed by CBOM clean/flush keep narrow lane results ordered before the CMO owner and marker",
            lb_observed == expected_lb &&
            lbu_observed == expected_lbu &&
            lh_observed == expected_lh &&
            lhu_observed == expected_lhu &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the narrow-load then CBOM owner-window preserves the source block image, marker guard, and surrounding guards",
            ld(source_vaddr) == source_word &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the narrow-load then CBOM owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], 0U, source_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_cbom_no_stale_inval_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 2U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1000U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1000U;
    const uintptr_t inval_vaddr = source_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1000U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1000U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2143658709badcfeULL,
        0x325476981acbed0fULL,
        0x436587a92bdcfe10ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xabcdef0123456789ULL,
        0x9876543210fedcbaULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa5a5a5a55a5a5a5aULL,
        0x5a5a5a5aa5a5a5a5ULL,
        0x123456789abcdef0ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();
    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then CBOM owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_cbom_window(inval_vaddr, clean_vaddr,
            flush_vaddr, load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by CBOM clean/flush releases inval ownership before CMO completion and preserves load and marker progress",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then CBOM owner-window preserves source block image, marker guard, and surrounding guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then CBOM owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], load_word, expected_load) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();
    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_cboinval_no_stale_cmo_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1080U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1080U;
    const uintptr_t clean_vaddr = source_vaddr + sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1080U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1080U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
        0x93a4b5c6d7e8f901ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0ddba11cafef00d1ULL,
        0x0ddba11cafef00d2ULL,
        0x0ddba11cafef00d3ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55ccaa3377ee1122ULL,
        0x66ddbb4488ff2233ULL,
        0x77eecc5599003344ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0xfeed1234beef5678ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1029384756abcdefULL,
        0xefcdab7658473920ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();
    ai_memblock_cmo_scalar_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then CBO.INVAL owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbom_then_cboinval_window(clean_vaddr, flush_vaddr,
            inval_vaddr, load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by CBO.INVAL releases CMO ownership before inval completion and preserves load and marker progress",
            load_observed == expected_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then CBO.INVAL owner-window preserves source block image, marker guard, and surrounding guards",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then CBO.INVAL owner-window contains the preserved source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_matches_seed_except_word(source_paddr,
                block_seeds[round], load_word, expected_load) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_cbom_env();
    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_scalar_store_no_stale_zero_flush_owner_or_store_payload_drop_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 2U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x980U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x980U;
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t scalar_store_vaddr =
        source_vaddr + store_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x980U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x980U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x405162738495a6b7ULL,
        0x5162738495a6b7c8ULL,
        0x62738495a6b7c8d9ULL,
    };
    const uint64_t store_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55aa771133cc99eeULL,
        0xa55a600dd00f7331ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0xcafed00d0badbeefULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1020304050607080ULL,
        0x8070605040302010ULL,
        0x1234432112344321ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x9999888877776666ULL,
        0x6666777788889999ULL,
        0x1357246813572468ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x2468ace02468ace0ULL,
        0xeca86420eca86420ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0ace0ace0ace0aceULL,
        0x0bdf0bdf0bdf0bdfULL,
        0x0c010c010c010c01ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then scalar-store owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_scalar_store_window(zero_vaddr,
            scalar_store_vaddr, marker_vaddr, store_updates[round],
            marker_updates[round], &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by ordinary aligned scalar store keeps zero ownership ordered so the later store payload and marker survive",
            store_observed == store_updates[round] &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then scalar-store owner-window leaves only the later store word nonzero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(scalar_store_vaddr) == store_updates[round] &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then scalar-store owner-window contains zeroed siblings, the later store word, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_zero_except_word(source_paddr,
                store_word, store_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_narrow_scalar_stores_no_stale_zero_owner_or_byte_mask_drop_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t store_word = 0U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xa00U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xa00U;
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x6d5c4b3a291807f6ULL,
        0x7e6d5c4b3a291807ULL,
        0x8f7e6d5c4b3a2918ULL,
    };
    const uint64_t byte_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x5aULL,
        0xc3ULL,
        0x7eULL,
    };
    const uint64_t halfword_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa55aULL,
        0x3cc3ULL,
        0x7ee7ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0102030405060708ULL,
        0x8070605040302010ULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xfeedfacecafebeefULL,
        0xdecafbad12345678ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0badf00d55aa7711ULL,
        0x1122aabb3344ccddULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x123456789abcdef0ULL,
        0x0fedcba987654321ULL,
        0xa5a55a5ac3c33c3cULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_word =
            (byte_updates[round] << 8) |
            (halfword_updates[round] << 32);
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then narrow-store owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_narrow_stores_window(zero_vaddr,
            source_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by byte and halfword scalar stores keeps zero ownership ordered so the narrow byte mask, halfword mask, and marker survive",
            store_observed == expected_word &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then narrow-store owner-window leaves only byte1 and bytes4-5 nonzero while zeroed siblings and guards remain intact",
            ld(source_vaddr) == expected_word &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then narrow-store owner-window contains zeroed siblings, byte/halfword store masks in word0, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_zero_except_word(source_paddr,
                store_word, expected_word) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_narrow_scalar_loads_no_stale_zero_owner_or_signed_lane_leak_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0xa80U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0xa80U;
    const uintptr_t zero_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x90ef807f66554433ULL,
        0xa1fe918877665544ULL,
        0xb2cd82f088776655ULL,
    };
    const uint64_t lane_old_words[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x8001f00dff807f66ULL,
        0x7ffec00380ff5a55ULL,
        0xffff80017e817c7bULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xabcdef0123456789ULL,
        0x9876543210fedcbaULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc001d00dc001d00dULL,
        0xd15ea5ed12345678ULL,
        0x0ddba11a0ddba11aULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x11100f0e0d0c0b0aULL,
        0x2468135724681357ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t lb_observed = 0ULL;
        uint64_t lbu_observed = 0ULL;
        uint64_t lh_observed = 0ULL;
        uint64_t lhu_observed = 0ULL;
        uint64_t lw_observed = 0ULL;
        uint64_t ld_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_tail = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(source_paddr, lane_old_words[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then narrow-load owner-window starts, source lanes contain stale-sign candidates and marker is readable",
            ld(source_vaddr) == lane_old_words[round] &&
            ld(source_tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_narrow_loads_window(zero_vaddr,
            source_vaddr, marker_vaddr, marker_updates[round],
            &lb_observed, &lbu_observed, &lh_observed, &lhu_observed,
            &lw_observed, &ld_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by lb/lbu/lh/lhu/lw/ld must return zero for every loaded lane rather than stale signed or unsigned payload",
            lb_observed == 0ULL &&
            lbu_observed == 0ULL &&
            lh_observed == 0ULL &&
            lhu_observed == 0ULL &&
            lw_observed == 0ULL &&
            ld_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then narrow-load owner-window leaves the full source block zero and preserves all guards",
            ld(source_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then narrow-load owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_cbozero_no_stale_inval_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 2U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1100U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1100U;
    const uintptr_t inval_vaddr = source_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1100U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1100U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x13579bdf02468aceULL,
        0x2468ace013579bdfULL,
        0x3579bdf12468ace0ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0ace13579bdf2468ULL,
        0x1bdf2468ace03579ULL,
        0x2ce03579bdf1468aULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xf13579bdf02468acULL,
        0xe2468ace013579bdULL,
        0xd3579bdf12468aceULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x55aa00ff33cc6699ULL,
        0x669911ee44dd7700ULL,
        0x77aa22ff55ee8811ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_inval_env();
    ai_memblock_cmo_scalar_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then CBO.ZERO owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_cbozero_window(inval_vaddr, zero_vaddr,
            load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by CBO.ZERO releases inval ownership before zero completion and keeps marker progress visible",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then CBO.ZERO owner-window leaves the whole source block zero and preserves all guards",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_env();
    ai_memblock_cmo_scalar_reset_hs_inval_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_cboinval_no_stale_zero_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1180U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1180U;
    const uintptr_t zero_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1180U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1180U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x89abcdef76543210ULL,
        0x9abcdef087654321ULL,
        0xabcdef0198765432ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x010f1e2d3c4b5a69ULL,
        0x102e3d4c5b6a7988ULL,
        0x203f4e5d6c7b8a99ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xff00ee11dd22cc33ULL,
        0xee11dd22cc33bb44ULL,
        0xdd22cc33bb44aa55ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x9081726354453627ULL,
        0x8172635445362718ULL,
        0x7263544536271809ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_SCALAR_ROUNDS] = {
        0xc001c0dec001c0deULL,
        0xd112d1efd112d1efULL,
        0xe223e200e223e200ULL,
    };

    ai_memblock_cmo_scalar_prepare_hs_env();
    ai_memblock_cmo_scalar_prepare_hs_inval_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_SCALAR_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_SCALAR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_scalar_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then CBO.INVAL owner-window starts, source block and marker are readable with seeded values",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_cboinval_window(zero_vaddr, inval_vaddr,
            load_vaddr, marker_vaddr, marker_updates[round],
            &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by CBO.INVAL keeps zero ownership committed before inval completion and marker progress visible",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then CBO.INVAL owner-window leaves the whole source block zero and preserves all guards",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then CBO.INVAL owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_scalar_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_scalar_reset_hs_inval_env();
    ai_memblock_cmo_scalar_reset_hs_env();

    TEST_END();
}
