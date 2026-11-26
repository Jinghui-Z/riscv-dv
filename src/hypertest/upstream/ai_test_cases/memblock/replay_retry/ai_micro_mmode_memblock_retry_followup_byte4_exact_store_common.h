#ifndef AI_MICRO_MMODE_MEMBLOCK_RETRY_FOLLOWUP_BYTE4_EXACT_STORE_COMMON_H
#define AI_MICRO_MMODE_MEMBLOCK_RETRY_FOLLOWUP_BYTE4_EXACT_STORE_COMMON_H

#include <page_tables.h>
#include <rvh_test.h>
#include <stdint.h>


/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_byte_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_halfword_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_halfword_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_bridge_halfword_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_byte_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_second_upper_byte_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_center_semantic_byte_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_tail_bridge_word_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_adjacent_bridge_word_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_high_half_word_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_word_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_halfword_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_same_template_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_word_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_byte_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_halfword_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_halfword_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_tail_last_halfword_low_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_tail_last_halfword_high_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_low_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_high_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_tail_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_tail_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_tail_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_tail_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_tail_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_half_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_first_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_second_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_first_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_second_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_last_halfword_high_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_last_halfword_low_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_last_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_bridge_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_adjacent_bridge_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_bridge_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_adjacent_bridge_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_head_middle_bridge_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_word_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_second_upper_byte_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_head_middle_bridge_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_bridge_halfword_load_corner();
bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_first_byte_load_corner();
/* Keep the BYTE4 exact-high-half follow-up store separate from the widened
 * BYTE5/BYTE6/BYTE7 cases so the exact 4B stale-owner suspicion can evolve
 * independently.
 */

#define AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET 12U

static inline pte_t *ai_micro_mab_retry_byte4_exact_store_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_byte4_exact_store_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_byte4_exact_store_hs_leaf_entry(page) =
        (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_byte4_exact_store_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_byte4_exact_store_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void
ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void
ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_RWX);
}

static inline void
ai_micro_mab_retry_byte4_exact_store_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_RWX |
        (deny_cfg << 8) |
        (AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_byte4_exact_store_reset_env(void)
{
    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();
    ai_micro_mab_retry_byte4_exact_store_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_byte4_exact_store_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_byte4_exact_store_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void
ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
    uint8_t *dst, const uint8_t *seed, unsigned len, unsigned start_byte,
    uint64_t value, unsigned size)
{
    for (unsigned i = 0; i < len; ++i) {
        dst[i] = seed[i];
    }
    for (unsigned i = 0; i < size; ++i) {
        dst[start_byte + i] = (uint8_t)((value >> (8U * i)) & 0xffU);
    }
}

static inline uint64_t ai_micro_mab_retry_byte4_exact_store_read_little_endian(
    const uint8_t *bytes, unsigned size)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < size; ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_STORE_INSTRUCTION( \
    name, inst, type) \
static inline void name(uintptr_t addr, type value) \
{ \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %0\n\t" \
        "csrs mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        inst " %3, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        "csrc mstatus, %0\n\t" \
        : \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(value), "r"(addr) \
        : "memory"); \
}

AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_sb, "sb", uint8_t)
AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_sh, "sh", uint16_t)
AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_sw, "sw", uint32_t)
AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_sd, "sd", uint64_t)

#define AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_LOAD_INSTRUCTION( \
    name, inst) \
static inline uint64_t name(uintptr_t addr) \
{ \
    uint64_t value = 0ULL; \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "csrs mstatus, %3\n\t" \
        inst " %0, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %2\n\t" \
        "csrc mstatus, %1\n\t" \
        : "=&r"(value) \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(addr) \
        : "memory"); \
    return value; \
}

AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_lbu, "lbu")
AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_lhu, "lhu")
AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_lwu, "lwu")
AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_mab_retry_byte4_exact_store_translated_ld, "ld")

#endif /* AI_MICRO_MMODE_MEMBLOCK_RETRY_FOLLOWUP_BYTE4_EXACT_STORE_COMMON_H */
