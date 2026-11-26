#ifndef AI_MICRO_MMODE_MEMBLOCK_COMMON_H
#define AI_MICRO_MMODE_MEMBLOCK_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>


/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_m_load_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner();
bool ai_micro_m_load_cross_page_page_fault_tval_refreshes_back_to_low_half_after_high_half_corner();
bool ai_micro_m_store_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner();
bool ai_micro_m_cross_page_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_cross_page_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_load_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner();
bool ai_micro_m_store_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner();
bool ai_micro_m_cross_page_access_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_cross_page_access_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_load_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_load_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_store_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_store_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_load_cross_page_high_half_page_fault_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_load_cross_page_high_half_access_fault_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_store_cross_page_high_half_page_fault_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_store_cross_page_high_half_access_fault_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_load_cross_page_high_half_page_fault_repeated_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_load_cross_page_high_half_access_fault_repeated_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_store_cross_page_high_half_page_fault_repeated_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_store_cross_page_high_half_access_fault_repeated_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_single_step_exception_multi_source_same_cycle_oldest_must_win_diagnostic_corner();
bool ai_micro_m_samepage_amo_fault_switch_amo_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_amo_repeated_access_fault_then_same_address_valid_atomic_recovers_corner();
bool ai_micro_m_samepage_lr_repeated_access_fault_then_same_address_valid_lr_recovers_corner();
bool ai_micro_m_samepage_amo_repair_then_different_address_fault_refreshes_new_tval_corner();
bool ai_micro_m_samepage_lr_repair_then_different_address_fault_refreshes_new_tval_corner();
bool ai_micro_m_samepage_amo_repair_then_different_address_load_fault_refreshes_new_cause_tval_corner();
bool ai_micro_m_samepage_lr_repair_then_different_address_atomic_fault_refreshes_new_cause_tval_corner();
bool ai_micro_m_samepage_amo_fault_switch_load_then_amo_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_amo_fault_switch_amo_then_store_keeps_latest_tval_corner();
bool ai_micro_m_samepage_amo_fault_switch_store_then_amo_keeps_latest_tval_corner();
bool ai_micro_m_samepage_lr_fault_switch_lr_then_load_keeps_latest_tval_corner();
bool ai_micro_m_samepage_lr_fault_switch_load_then_lr_keeps_latest_tval_corner();
bool ai_micro_m_samepage_lr_fault_switch_lr_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_lr_fault_switch_store_then_lr_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_lr_fault_switch_lr_then_amo_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_amo_fault_switch_amo_then_lr_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_lr_fault_switch_lr_then_lr_keeps_latest_tval_corner();
bool ai_micro_m_samepage_amo_fault_switch_amo_then_amo_keeps_latest_tval_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_load_fault_refreshes_new_tval_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_store_fault_refreshes_new_tval_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_store_fault_refreshes_new_cause_tval_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_load_fault_refreshes_new_cause_tval_corner();
bool ai_micro_m_samepage_cross_16b_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_cross_16b_same_address_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_store_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_store_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_width_signedness_switch_recovers_corner();
bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_width_signedness_switch_recovers_corner();
bool ai_micro_m_load_cross_16b_same_width_template_switch_no_stale_merge_corner();
bool ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_no_stale_merge_corner();
bool ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_adjacent_preserved_corner();
bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_same_boundary_new_fault_refreshes_current_template_tval_corner();
bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_original_boundary_new_fault_refreshes_back_template_tval_corner();
bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_returned_cross_producer_store_fault_refreshes_back_cause_tval_corner();
bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_cross_producer_store_fault_refreshes_current_cause_tval_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_signedness_load_recovers_corner();
bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_load_recovers_corner();
bool ai_micro_m_samepage_cross_16b_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_cross_16b_same_address_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_load_keeps_latest_tval_corner();
bool ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_store_keeps_latest_tval_corner();
bool ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_no_stale_template_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_no_stale_template_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_state_not_rebound_to_next_boundary_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_no_fake_crosspage_rebind_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_refreshes_current_template_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_new_fault_refreshes_tval_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_refreshes_back_tval_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_adjacent_preserved_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_original_boundary_new_fault_refreshes_back_template_tval_corner();
bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_new_fault_refreshes_current_template_tval_corner();
bool ai_micro_m_pbmt_nc_replay_head_narrow_scalar_signedness_matches_current_uop_corner();
bool ai_micro_m_store_pbmt_nc_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner();
bool ai_micro_m_store_pbmt_io_bus_error_then_same_alias_good_pbmt_io_store_clears_stale_exception_corner();
bool ai_micro_m_load_pbmt_narrow_bus_error_then_same_width_signedness_switch_recovery_corner();
bool ai_micro_m_load_pbmt_narrow_bad_good_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_load_pbmt_narrow_bad_good_next_bad_width_switch_refreshes_new_alias_corner();
bool ai_micro_m_load_pbmt_bad_good_store_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_store_pbmt_bad_good_load_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_store_pbmt_nc_bad_good_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_next_bad_width_switch_refreshes_new_alias_corner();
bool ai_micro_m_load_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_load_pbmt_io_bad_good_next_bad_store_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_next_bad_load_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_pbmt_io_load_store_oldest_wins_diagnostic_corner();
bool ai_micro_m_pbmt_mixed_nc_load_io_store_oldest_wins_diagnostic_corner();
bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_fault_refreshes_to_new_alias_corner();
bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_width_switch_refreshes_new_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_width_switch_refreshes_new_alias_corner();
bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_load_width_switch_refreshes_new_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_store_width_switch_refreshes_new_alias_corner();
bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_store_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_load_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_load_pbmt_narrow_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_store_pbmt_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner();
bool ai_micro_m_pbmt_mixed_nc_narrow_load_io_width_switch_store_oldest_wins_diagnostic_corner();
// These MemBlock probes deliberately avoid H extension dependencies.
// They drive the shared scalar exception path with M-mode + MPRV=1 + MPP=S
// translation and use PMP to build the access-fault half of the matrix.

#define AI_MICRO_MEMBLOCK_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MEMBLOCK_PMP_TOR_NO_R 0x0cULL
#define AI_MICRO_MEMBLOCK_PMP_TOR_NO_W 0x09ULL
#define AI_MICRO_MEMBLOCK_PMP_TOR_NO_RW 0x08ULL
#define AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE VSRWXPbmt1_GURWXPbmt1
#define AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE VSRWXPbmt2_GURWXPbmt1
#define AI_MICRO_MEMBLOCK_PBMT_IO_MEM_BASE 0x30000000000ULL
#define AI_MICRO_MEMBLOCK_PBMT_IO_MEM_LIMIT 0x40000000000ULL
#define AI_MICRO_MEMBLOCK_RESP_MMIO64_BASE 0x40070000ULL
#define AI_MICRO_MEMBLOCK_RESP_MMIO64_LIMIT 0x40080000ULL
#define AI_MICRO_MEMBLOCK_RESP_MMIO32_STAT 0x40600008ULL
#define AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR \
    (phys_page_base_offset(VSRWXPbmt3_GURWXPbmt1, \
        AI_MICRO_MEMBLOCK_PBMT_IO_MEM_BASE) + 0x7e0ULL)
#define AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT \
    (AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR - 0x10ULL)

static inline void ai_micro_m_memblock_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg);

static inline uintptr_t ai_micro_m_memblock_cross_page_addr(
    enum test_page low_page, uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline uintptr_t ai_micro_m_memblock_page_end_phys(enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline uintptr_t ai_micro_m_memblock_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline pte_t *ai_micro_m_memblock_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_m_memblock_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_m_memblock_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_m_memblock_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_m_memblock_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline bool ai_micro_m_memblock_is_pbmt_io_alias(enum test_page page)
{
    return page == AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
}

static inline bool ai_micro_m_memblock_is_pbmt_io_mem_paddr(uintptr_t paddr)
{
    return paddr >= AI_MICRO_MEMBLOCK_PBMT_IO_MEM_BASE &&
        paddr < AI_MICRO_MEMBLOCK_PBMT_IO_MEM_LIMIT;
}

static inline void ai_micro_m_memblock_prepare_pbmt_io_mem_window(void)
{
    goto_priv(PRIV_M);
    /*
     * Keep address-strong PMA/IO probes in the LinkNan 3T window instead of
     * silently converting them to DRAM.  Good/recovery accesses that must
     * complete should prefer an explicit MMIO responder such as IntrGen/UART.
     */
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0x0ULL);
    CSRW(CSR_PMAADDR0, AI_MICRO_MEMBLOCK_PBMT_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_MEMBLOCK_PBMT_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    fence_iorw_iorw();
}

static inline uintptr_t ai_micro_m_memblock_good_paddr_for_alias(
    enum test_page page, uintptr_t offset)
{
    if (ai_micro_m_memblock_is_pbmt_io_alias(page)) {
        return phys_page_base_offset(SWITCH2,
            AI_MICRO_MEMBLOCK_PBMT_IO_MEM_BASE) + offset;
    }

    return phys_page_base(SWITCH2) + offset;
}

static inline uintptr_t ai_micro_m_memblock_good_mmio64_paddr(uintptr_t offset)
{
    /*
     * LinkNan SimMMIO IntrGen is a real 8-byte MMIO responder.  Use it for
     * PBMT=IO 64-bit good/recovery probes so dedicated PMA/PBMT=IO tests do
     * not fall back to DRAM.
     */
    return AI_MICRO_MEMBLOCK_RESP_MMIO64_BASE +
        (offset & (AI_MICRO_MEMBLOCK_RESP_MMIO64_LIMIT -
            AI_MICRO_MEMBLOCK_RESP_MMIO64_BASE - sizeof(uint64_t)));
}

static inline uintptr_t ai_micro_m_memblock_good_mmio32_paddr_for_offset(
    uintptr_t access_offset)
{
    /*
     * LinkNan UART stat register is a real 32-bit MMIO responder.  Bias the
     * aliased base so the actual access address stays on the responder while
     * the test still uses its intended virtual access offset.
     */
    return AI_MICRO_MEMBLOCK_RESP_MMIO32_STAT - access_offset;
}

static inline void ai_micro_m_memblock_prepare_direct_pbmt_io_paddr(
    uintptr_t paddr)
{
    if (ai_micro_m_memblock_is_pbmt_io_mem_paddr(paddr)) {
        ai_micro_m_memblock_prepare_pbmt_io_mem_window();
    }
}

static inline uint64_t ai_micro_m_memblock_read64_direct(uintptr_t paddr)
{
    ai_micro_m_memblock_prepare_direct_pbmt_io_paddr(paddr);
    return read64(paddr);
}

static inline void ai_micro_m_memblock_write64_direct(uintptr_t paddr,
    uint64_t value)
{
    ai_micro_m_memblock_prepare_direct_pbmt_io_paddr(paddr);
    write64(paddr, value);
    fence_iorw_iorw();
}

#define read64(addr) ai_micro_m_memblock_read64_direct((uintptr_t)(addr))
#define write64(addr, value) \
    ai_micro_m_memblock_write64_direct((uintptr_t)(addr), (uint64_t)(value))

static inline bool ai_micro_m_memblock_is_pbmt_bad_page(uintptr_t paddr)
{
    return (paddr & PAGE_ADDR_MSK) ==
        (AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR & PAGE_ADDR_MSK);
}

static inline void ai_micro_m_memblock_clear_translated_data_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
}

static inline void ai_micro_m_memblock_prepare_supervisor_translation(void)
{
    ai_micro_m_memblock_clear_translated_data_mode();
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_m_memblock_prepare_pbmt_translation(void)
{
    ai_micro_m_memblock_clear_translated_data_mode();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_m_memblock_reset_pbmt_env(void)
{
    ai_micro_m_memblock_clear_translated_data_mode();
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_m_memblock_configure_pbmt_alias(
    enum test_page page, uintptr_t paddr)
{
    const bool deny_bad_page = ai_micro_m_memblock_is_pbmt_bad_page(paddr);
    const uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    ai_micro_m_memblock_clear_translated_data_mode();
    if (ai_micro_m_memblock_is_pbmt_io_mem_paddr(paddr)) {
        ai_micro_m_memblock_prepare_pbmt_io_mem_window();
    }
    if (deny_bad_page ||
        page_start == (AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR & PAGE_ADDR_MSK)) {
        ai_micro_m_memblock_configure_pmp_window(
            page_start, page_start + PAGE_SIZE,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_RW);
    }
    pbmt_hspt_to_x(page);
    hspt_leaf_change_base_paddr(page,
        ai_micro_m_memblock_alias_anchor(page, page_start));
    sfence_vma();

    return hs_page_base(page) + (paddr & (PAGE_SIZE - 1U));
}

static inline void ai_micro_m_memblock_enable_mprv_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);
}

static inline void ai_micro_m_memblock_disable_mprv_supervisor_translation(void)
{
    ai_micro_m_memblock_clear_translated_data_mode();
}

static inline void ai_micro_m_memblock_clear_machine_access_mode(void)
{
    ai_micro_m_memblock_clear_translated_data_mode();
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_m_memblock_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MEMBLOCK_PMP_TOR_RWX);
}

static inline void ai_micro_m_memblock_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MEMBLOCK_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_MEMBLOCK_PMP_TOR_RWX << 16));
}

static inline void ai_micro_m_memblock_configure_pmp_page_window(
    uintptr_t deny_paddr, uint64_t deny_cfg)
{
    const uintptr_t page_start = deny_paddr & PAGE_ADDR_MSK;

    ai_micro_m_memblock_configure_pmp_window(
        page_start, page_start + PAGE_SIZE, deny_cfg);
}

static inline void ai_micro_m_memblock_reset_env(void)
{
    ai_micro_m_memblock_restore_pmp_full_access();
    ai_micro_m_memblock_clear_machine_access_mode();
}

static inline uint64_t ai_micro_m_memblock_expected_low_after_4b_store(
    uint64_t original_low, uint64_t store_value)
{
    return (original_low & 0xffffffffULL) |
        ((store_value & 0xffffffffULL) << 32);
}

static inline uint64_t ai_micro_m_memblock_expected_high_after_4b_store(
    uint64_t original_high, uint64_t store_value)
{
    return (original_high & 0xffffffff00000000ULL) |
        ((store_value >> 32) & 0xffffffffULL);
}

static inline uint64_t ai_micro_m_memblock_expected_cross_page_4b4b_load(
    uint64_t low_pattern, uint64_t high_pattern)
{
    return (low_pattern >> 32) |
        ((high_pattern & 0xffffffffULL) << 32);
}

static inline uint64_t ai_micro_m_memblock_extract_unsigned(uint64_t value,
    unsigned offset_bytes, unsigned size_bytes)
{
    const uint64_t mask =
        (size_bytes == sizeof(uint8_t)) ? 0xffULL :
        (size_bytes == sizeof(uint16_t)) ? 0xffffULL :
        (size_bytes == sizeof(uint32_t)) ? 0xffffffffULL :
        0xffffffffffffffffULL;

    return (value >> (offset_bytes * 8U)) & mask;
}

static inline uint64_t ai_micro_m_memblock_sign_extend(uint64_t value,
    unsigned size_bytes)
{
    if (size_bytes == sizeof(uint8_t)) {
        return (uint64_t)(int64_t)(int8_t)value;
    }
    if (size_bytes == sizeof(uint16_t)) {
        return (uint64_t)(int64_t)(int16_t)value;
    }
    if (size_bytes == sizeof(uint32_t)) {
        return (uint64_t)(int64_t)(int32_t)value;
    }
    return value;
}

static inline uint64_t ai_micro_m_memblock_extract_scalar(uint64_t value,
    unsigned offset_bytes, unsigned size_bytes, bool signed_load)
{
    const uint64_t extracted = ai_micro_m_memblock_extract_unsigned(
        value, offset_bytes, size_bytes);

    if (signed_load) {
        return ai_micro_m_memblock_sign_extend(extracted, size_bytes);
    }
    return extracted;
}

static inline uint64_t ai_micro_m_memblock_extract_scalar_from_bytes(
    const uint8_t *bytes, unsigned start_byte, unsigned size_bytes,
    bool signed_load)
{
    uint64_t raw = 0ULL;

    for (unsigned i = 0; i < size_bytes; ++i) {
        raw |= ((uint64_t)bytes[start_byte + i]) << (8U * i);
    }

    if (signed_load) {
        return ai_micro_m_memblock_sign_extend(raw, size_bytes);
    }
    return raw;
}

static inline uint64_t ai_micro_m_memblock_merge_unsigned(uint64_t base,
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






















#define AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET 12U
#define AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET 13U
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LH_OFFSET 15U
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET 15U
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET 12U
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET 45U
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET 44U
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET 47U

#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LOW_PAGE SWITCH1
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_HIGH_PAGE SWITCH2
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LOW_PAGE VSRWXPbmt0_GURWXPbmt0
#define AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_HIGH_PAGE VSRWXPbmt0_GURWXPbmt1

static inline enum test_page ai_micro_m_memblock_cross_16b_low_page_for_index(
    unsigned boundary_index)
{
    return boundary_index == 0U ?
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LOW_PAGE :
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LOW_PAGE;
}

static inline enum test_page ai_micro_m_memblock_cross_16b_high_page_for_index(
    unsigned boundary_index)
{
    return boundary_index == 0U ?
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_HIGH_PAGE :
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_HIGH_PAGE;
}

static inline unsigned ai_micro_m_memblock_cross_16b_boundary_index_for_offset(
    uintptr_t offset)
{
    switch (offset) {
    case AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET:
    case AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET:
    case AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET:
        return 1U;
    default:
        return 0U;
    }
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_boundary_low_phys_for_index(
    unsigned boundary_index)
{
    return phys_page_base(
        ai_micro_m_memblock_cross_16b_low_page_for_index(boundary_index)) +
        PAGE_SIZE - sizeof(uint64_t);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_boundary_high_phys_for_index(
    unsigned boundary_index)
{
    return phys_page_base(
        ai_micro_m_memblock_cross_16b_high_page_for_index(boundary_index));
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_boundary_phys(
    unsigned boundary_index)
{
    return ai_micro_m_memblock_cross_16b_boundary_low_phys_for_index(
        boundary_index);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_guard_before_phys(void)
{
    return ai_micro_m_memblock_cross_16b_boundary_phys(0U) -
        sizeof(uint64_t);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_guard_middle_phys(void)
{
    return ai_micro_m_memblock_cross_16b_boundary_high_phys_for_index(0U) +
        16U;
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_guard_after_phys(void)
{
    return ai_micro_m_memblock_cross_16b_boundary_high_phys_for_index(1U) +
        sizeof(uint64_t);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_vaddr_for_offset(
    uintptr_t offset)
{
    const unsigned boundary_index =
        ai_micro_m_memblock_cross_16b_boundary_index_for_offset(offset);
    const uintptr_t low_base = hs_page_base(
        ai_micro_m_memblock_cross_16b_low_page_for_index(boundary_index));

    if (offset == AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET ||
        offset == AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET ||
        offset == AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET) {
        return low_base + PAGE_SIZE - 4U;
    }
    if (offset == AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET ||
        offset == AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET) {
        return low_base + PAGE_SIZE - 3U;
    }
    if (offset == AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LH_OFFSET ||
        offset == AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET ||
        offset == AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET) {
        return low_base + PAGE_SIZE - 1U;
    }

    return low_base + offset;
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_second_half_vaddr(
    uintptr_t vaddr)
{
    return (vaddr & PAGE_ADDR_MSK) + PAGE_SIZE;
}

static inline void ai_micro_m_memblock_set_cross_16b_leaves(uint64_t flags)
{
    ai_micro_m_memblock_set_leaf(
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LOW_PAGE, flags);
    ai_micro_m_memblock_set_leaf(
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_HIGH_PAGE, flags);
    ai_micro_m_memblock_set_leaf(
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LOW_PAGE, flags);
    ai_micro_m_memblock_set_leaf(
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_HIGH_PAGE, flags);
}

static inline void ai_micro_m_memblock_configure_cross_16b_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    const uintptr_t pmp_deny_start = deny_start & PAGE_ADDR_MSK;
    const uintptr_t pmp_deny_end = pmp_deny_start + PAGE_SIZE;

    (void)deny_end;
    ai_micro_m_memblock_configure_pmp_window(
        pmp_deny_start, pmp_deny_end, deny_cfg);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_addr_with_offset(
    enum test_page page, uintptr_t offset)
{
    (void)page;
    return ai_micro_m_memblock_cross_16b_vaddr_for_offset(offset);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_addr(enum test_page page)
{
    return ai_micro_m_memblock_cross_16b_addr_with_offset(
        page, AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_low_phys_with_offset(
    enum test_page page, uintptr_t offset)
{
    (void)page;
    return ai_micro_m_memblock_cross_16b_boundary_low_phys_for_index(
        ai_micro_m_memblock_cross_16b_boundary_index_for_offset(offset));
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_high_phys_with_offset(
    enum test_page page, uintptr_t offset)
{
    (void)page;
    return ai_micro_m_memblock_cross_16b_boundary_high_phys_for_index(
        ai_micro_m_memblock_cross_16b_boundary_index_for_offset(offset));
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_low_phys(enum test_page page)
{
    return ai_micro_m_memblock_cross_16b_low_phys_with_offset(
        page, AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET);
}

static inline uintptr_t ai_micro_m_memblock_cross_16b_high_phys(enum test_page page)
{
    return ai_micro_m_memblock_cross_16b_high_phys_with_offset(
        page, AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET);
}

static inline void ai_micro_m_memblock_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline void ai_micro_m_memblock_overlay_little_endian_bytes(
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

static inline bool ai_micro_m_memblock_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_m_memblock_store_by_size(
    uintptr_t addr, unsigned size, uint64_t value)
{
    switch (size) {
    case 8U:
        sd(addr, value);
        break;
    case 4U:
        sw(addr, (uint32_t)value);
        break;
    case 2U:
        sh(addr, (uint16_t)value);
        break;
    default:
        sb(addr, (uint8_t)value);
        break;
    }
}

#define AI_MICRO_MEMBLOCK_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
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

AI_MICRO_MEMBLOCK_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_m_memblock_translated_sb, "sb", uint8_t)
AI_MICRO_MEMBLOCK_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_m_memblock_translated_sh, "sh", uint16_t)
AI_MICRO_MEMBLOCK_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_m_memblock_translated_sw, "sw", uint32_t)
AI_MICRO_MEMBLOCK_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_m_memblock_translated_sd, "sd", uint64_t)

static inline bool ai_micro_m_memblock_seed_pbmt_alias_doubleword(
    enum test_page page, uintptr_t paddr, uint64_t value)
{
    const uintptr_t seed_vaddr =
        ai_micro_m_memblock_configure_pbmt_alias(page, paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(seed_vaddr, value);
    fence_iorw_iorw();
    return excpt.triggered == false;
}

static inline void ai_micro_m_memblock_translated_store_by_size(
    uintptr_t addr, unsigned size, uint64_t value)
{
    switch (size) {
    case 8U:
        ai_micro_m_memblock_translated_sd(addr, value);
        break;
    case 4U:
        ai_micro_m_memblock_translated_sw(addr, (uint32_t)value);
        break;
    case 2U:
        ai_micro_m_memblock_translated_sh(addr, (uint16_t)value);
        break;
    default:
        ai_micro_m_memblock_translated_sb(addr, (uint8_t)value);
        break;
    }
}

#define AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
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

AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_lb, "lb")
AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_lbu, "lbu")
AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_lh, "lh")
AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_lhu, "lhu")
AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_lw, "lw")
AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_lwu, "lwu")
AI_MICRO_MEMBLOCK_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_m_memblock_translated_ld, "ld")

/* Keep MPRV setup, the AMO itself, and MPRV teardown inside one asm block so
 * a compiler-inserted stack access cannot preempt the intended translated AMO.
 */
#define AI_MICRO_MEMBLOCK_TRANSLATED_AMO_INSTRUCTION(name, inst, type) \
static inline type name(uintptr_t addr, type value) \
{ \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "csrs mstatus, %3\n\t" \
        inst " %0, %0, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %2\n\t" \
        "csrc mstatus, %1\n\t" \
        : "+&r"(value) \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(addr) \
        : "memory"); \
    return value; \
}

AI_MICRO_MEMBLOCK_TRANSLATED_AMO_INSTRUCTION(
    ai_micro_m_memblock_translated_amoor_d, "amoor.d", uint64_t)

#define AI_MICRO_MEMBLOCK_TRANSLATED_LR_INSTRUCTION(name, inst) \
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

AI_MICRO_MEMBLOCK_TRANSLATED_LR_INSTRUCTION(
    ai_micro_m_memblock_translated_lr_d, "lr.d")

static inline uint64_t ai_micro_m_memblock_translated_scalar_load(
    bool signed_load, unsigned size_bytes, uintptr_t addr)
{
    if (size_bytes == sizeof(uint8_t)) {
        return signed_load ?
            ai_micro_m_memblock_translated_lb(addr) :
            ai_micro_m_memblock_translated_lbu(addr);
    }
    if (size_bytes == sizeof(uint16_t)) {
        return signed_load ?
            ai_micro_m_memblock_translated_lh(addr) :
            ai_micro_m_memblock_translated_lhu(addr);
    }
    if (size_bytes == sizeof(uint32_t)) {
        return signed_load ?
            ai_micro_m_memblock_translated_lw(addr) :
            ai_micro_m_memblock_translated_lwu(addr);
    }
    return ai_micro_m_memblock_translated_ld(addr);
}

static inline __attribute__((always_inline)) uint64_t
ai_micro_m_memblock_scalar_load_current_decode(
    bool signed_load, unsigned size_bytes, uintptr_t addr)
{
    uint64_t value = 0ULL;

    if (size_bytes == sizeof(uint8_t)) {
        if (signed_load) {
            asm volatile("lb %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        } else {
            asm volatile("lbu %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        }
        return value;
    }
    if (size_bytes == sizeof(uint16_t)) {
        if (signed_load) {
            asm volatile("lh %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        } else {
            asm volatile("lhu %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        }
        return value;
    }
    if (size_bytes == sizeof(uint32_t)) {
        if (signed_load) {
            asm volatile("lw %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        } else {
            asm volatile("lwu %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        }
        return value;
    }

    asm volatile("ld %0, 0(%1)\n\t"
        : "=r"(value) : "r"(addr) : "memory");
    return value;
}

struct ai_micro_m_memblock_exception_snapshot {
    bool triggered;
    uint64_t cause;
    uint64_t tval;
};

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_m_memblock_translated_load_amo_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_paddr,
    uint64_t second_value,
    struct ai_micro_m_memblock_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "ld t0, 0(a0)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "amoadd.d t0, a3, (a1)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [clear_mpp] "i" (3ULL << 11),
          [set_mprv] "i" (MSTATUS_MPRV),
          [set_mpp_s] "i" (MSTATUS_MPP),
          [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, tval))
        : "t0", "t1", "t2", "t3", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_m_memblock_translated_amo_load_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_paddr,
    uint64_t first_value,
    struct ai_micro_m_memblock_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "amoadd.d t0, a3, (a0)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "ld t2, 0(a1)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [clear_mpp] "i" (3ULL << 11),
          [set_mprv] "i" (MSTATUS_MPRV),
          [set_mpp_s] "i" (MSTATUS_MPP),
          [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, tval))
        : "t0", "t1", "t2", "t3", "memory");
}






































static bool ai_micro_m_memblock_samepage_cross_16b_load_fault_same_width_template_switch_window(
    bool check_adjacent)
{
    static const unsigned fault_start_bytes[2] = {
        4U,
        7U,
    };
    static const unsigned recovery_start_bytes[2] = {
        7U,
        4U,
    };
    static const uintptr_t fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t recovery_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned target = round;
        const unsigned other = target ^ 1U;
        const uintptr_t fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                fault_offsets[round]);
        const uintptr_t recovery_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                recovery_offsets[round]);
        const uintptr_t deny_start = boundary_phys[target] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;
        const uint64_t expected_recovery_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[target][round], recovery_start_bytes[round],
                sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), fault_vaddr);
        TEST_ASSERT("the first no-H same-page same-width cross-16B different-template load fault reports load access fault with the original misaligned VA before the repaired template-switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), fault_vaddr);
        TEST_ASSERT("repeating the no-H same-page same-width cross-16B different-template load fault keeps the same precise metadata before the repaired template-switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the same-page same-width cross-16B different-template load recovery probe, both boundary windows and the middle gap remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[target],
                init_patterns[target][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[other],
                init_patterns[other][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("before the same-page same-width cross-16B different-template load recovery probe, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), recovery_vaddr);
        TEST_ASSERT("after repairing the no-H same-page same-width cross-16B different-template load fault, the next load must return the value selected by the current template instead of inheriting stale merge parameters from the earlier template",
            excpt.triggered == false &&
            observed_value == expected_recovery_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the no-H same-page same-width cross-16B different-template fault-recovery sequence, both boundary windows and all guard regions remain unchanged because the recovery access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_next_boundary_template_fault_window(
    bool check_adjacent)
{
    static const uintptr_t first_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uintptr_t success_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t younger_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const unsigned success_start_bytes[2] = {
        7U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                first_fault_offsets[first]);
        const uintptr_t success_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                success_offsets[second]);
        const uintptr_t younger_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                younger_fault_offsets[second]);
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;
        const uintptr_t second_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_deny_end = second_deny_start + PAGE_SIZE;
        const uint64_t expected_success_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[second][round], success_start_bytes[second],
                sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary load fault reports load access fault on the original template-specific boundary address before the next-boundary template-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary load fault keeps the same precise metadata before the next-boundary template-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), success_vaddr);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary load fault, the first valid next-boundary load returns the value selected by the current template before the younger next-boundary template-fault probe",
            excpt.triggered == false &&
            observed_value == expected_success_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a valid next-boundary load, both boundaries and all guard regions remain unchanged because the middle access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            second_deny_start, second_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), younger_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a successful next-boundary load, a younger different-template load fault on that next boundary must refresh tval to the current template-specific address instead of staying on the middle success template",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(younger_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then next-boundary template-fault refresh load probe leaves both boundaries and all guard regions unchanged because every access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}


static bool ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_return_fault_same_width_template_window(
    bool check_adjacent)
{
    static const uintptr_t first_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uintptr_t success_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t return_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const unsigned success_start_bytes[2] = {
        7U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                first_fault_offsets[first]);
        const uintptr_t success_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                success_offsets[second]);
        const uintptr_t return_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                return_fault_offsets[first]);
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;
        const uint64_t expected_success_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[second][round], success_start_bytes[second],
                sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary load fault reports load access fault on the original template-specific boundary address before the returned template-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary load fault keeps the same precise metadata before the returned template-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), success_vaddr);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary load fault, the first valid next-boundary load returns the value selected by the current template before the returned template-fault probe",
            excpt.triggered == false &&
            observed_value == expected_success_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a valid next-boundary load, both boundaries and all guard regions remain unchanged because the middle access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), return_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a successful next-boundary load, a returned younger original-boundary different-template load fault must refresh tval back to the current template-specific address instead of staying on the middle success template",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(return_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then returned template-fault refresh load probe leaves both boundaries and all guard regions unchanged because every access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}


static bool ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_return_cross_producer_fault_same_width_template_window(
    bool check_adjacent)
{
    static const uintptr_t first_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uintptr_t success_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t return_store_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const unsigned success_start_bytes[2] = {
        7U,
        7U,
    };
    static const uint64_t return_store_fault_values[2] = {
        0x21436587a9cbed0fULL,
        0x55aa33cc77ee11ffULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                first_fault_offsets[first]);
        const uintptr_t success_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                success_offsets[second]);
        const uintptr_t return_store_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                return_store_fault_offsets[first]);
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;
        const uint64_t expected_success_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[second][round], success_start_bytes[second],
                sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary load fault reports load access fault on the original template-specific boundary address before the returned cross-producer template-fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary load fault keeps the same precise metadata before the returned cross-producer template-fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), success_vaddr);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary load fault, the first valid next-boundary load returns the value selected by the current template before the returned cross-producer store fault probe",
            excpt.triggered == false &&
            observed_value == expected_success_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a valid next-boundary load, both boundaries and all guard regions remain unchanged because the middle access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(return_store_fault_vaddr,
            return_store_fault_values[first]);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a successful next-boundary load, a returned cross-producer store fault on the original boundary must refresh cause and tval back to the current producer and current template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(return_store_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then returned cross-producer template-fault probe leaves both boundaries and all guard regions unchanged because the middle access is read-only and the returned store is denied",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}


static bool ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_next_boundary_cross_producer_fault_window(
    bool check_adjacent)
{
    static const uintptr_t first_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uintptr_t success_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t younger_store_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const unsigned success_start_bytes[2] = {
        7U,
        7U,
    };
    static const uint64_t younger_store_fault_values[2] = {
        0x21436587a9cbed0fULL,
        0x55aa33cc77ee11ffULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                first_fault_offsets[first]);
        const uintptr_t success_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                success_offsets[second]);
        const uintptr_t younger_store_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                younger_store_fault_offsets[second]);
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;
        const uintptr_t second_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_deny_end = second_deny_start + PAGE_SIZE;
        const uint64_t expected_success_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[second][round], success_start_bytes[second],
                sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary load fault reports load access fault on the original template-specific boundary address before the cross-producer next-boundary fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_fault_vaddr);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary load fault keeps the same precise metadata before the cross-producer next-boundary fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary load fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), success_vaddr);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary load fault, the first valid next-boundary load returns the value selected by the current template before the cross-producer store fault probe",
            excpt.triggered == false &&
            observed_value == expected_success_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a valid next-boundary load, both boundaries and all guard regions remain unchanged because the middle access is read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            second_deny_start, second_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(younger_store_fault_vaddr,
            younger_store_fault_values[second]);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary load fault has already been followed by a successful next-boundary load, a cross-producer store fault on that next boundary must refresh cause and tval to the current template-specific address instead of staying on the middle success template",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(younger_store_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then cross-producer next-boundary fault refresh probe leaves both boundaries and all guard regions unchanged because the middle access is read-only and the younger store is denied",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}













static bool ai_micro_m_memblock_samepage_cross_16b_store_fake_crosspage_window(
    bool check_adjacent)
{
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0x8877665544332211ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, 7U, store_values[0][round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, 7U, store_values[1][round], 8U);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            store_values[first][round]);
        TEST_ASSERT("the first no-H same-page scalar cross-16B store commits cleanly before the next-boundary fake-crosspage reuse probe",
            excpt.triggered == false
        );

        TEST_ASSERT("after the first no-H same-page scalar cross-16B store, only its own boundary reflects the expected eight-byte overlay while the next boundary and the in-between gap keep their seeded images",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                expected_boundary[first], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                init_patterns[second][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the first no-H same-page scalar cross-16B store, the outer guard regions remain unchanged in the fake-crosspage scalar probe",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[second],
            store_values[second][round]);
        TEST_ASSERT("the second no-H same-page scalar cross-16B store also commits cleanly after the earlier fake-crosspage-tagged scalar store",
            excpt.triggered == false
        );

        TEST_ASSERT("after two no-H same-page scalar cross-16B stores on different boundaries, each boundary keeps only its own expected eight-byte overlay and the middle gap remains untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                expected_boundary[0], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                expected_boundary[1], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H same-page scalar boundary-reuse sequence, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_width_switch_window(
    bool check_adjacent)
{
    static const unsigned store_sizes[2] = {
        4U,
        8U,
    };
    static const unsigned start_bytes[2] = {
        5U,
        7U,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0000000089abcdefULL, 0x000000000badf00dULL },
        { 0x13579bdf2468ace0ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xd0, 0xc1, 0xb2, 0xa3, 0x94, 0x85, 0x76, 0x67,
              0x58, 0x49, 0x3a, 0x2b, 0x1c, 0x0d, 0xfe, 0xef },
            { 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98,
              0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xf0, 0x11, 0x22 },
        },
        {
            { 0x5e, 0x4d, 0x3c, 0x2b, 0x1a, 0x09, 0xf8, 0xe7,
              0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81, 0x70, 0x6f },
            { 0xb1, 0xa2, 0x93, 0x84, 0x75, 0x66, 0x57, 0x48,
              0x39, 0x2a, 0x1b, 0x0c, 0xfd, 0xee, 0xdf, 0xc0 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x13, 0x25, 0x37, 0x49, 0x5b, 0x6d, 0x7f, 0x81 },
        { 0x8f, 0x7d, 0x6b, 0x59, 0x47, 0x35, 0x23, 0x11 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x90, 0xa2, 0xb4, 0xc6, 0xd8, 0xea, 0xfc, 0x0e,
          0x1f, 0x30, 0x41, 0x52, 0x63, 0x74, 0x85, 0x96 },
        { 0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
          0x97, 0xa9, 0xbb, 0xcd, 0xdf, 0xe1, 0xf3, 0x05 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x24, 0x46, 0x68, 0x8a, 0xac, 0xce, 0xe0, 0xf2 },
        { 0x4a, 0x5c, 0x6e, 0x70, 0x82, 0x94, 0xa6, 0xb8 },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, start_bytes[0],
            store_values[0][round], store_sizes[0]);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, start_bytes[1],
            store_values[1][round], store_sizes[1]);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[first],
            store_sizes[first], store_values[first][round]);
        TEST_ASSERT("the first no-H same-page different-width scalar cross-16B store commits without exception before the fake-crosspage width-switch follow-up",
            excpt.triggered == false
        );

        TEST_ASSERT("after the first no-H same-page different-width scalar cross-16B store, only its own boundary reflects the expected width-specific byte overlay while the other boundary and middle gap keep their seeded images",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                expected_boundary[first], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                init_patterns[second][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the first no-H same-page different-width scalar cross-16B store, the outer guard regions remain unchanged in the fake-crosspage width-switch probe",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[second],
            store_sizes[second], store_values[second][round]);
        TEST_ASSERT("the second no-H same-page different-width scalar cross-16B store also commits without exception after the earlier fake-crosspage-tagged boundary",
            excpt.triggered == false
        );

        TEST_ASSERT("after the no-H same-page different-width scalar cross-16B store sequence, each boundary keeps only its own width-specific split bytes and the middle gap remains untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                expected_boundary[0], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                expected_boundary[1], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H same-page different-width scalar cross-16B store sequence, the outer guards also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_same_width_template_switch_window(
    bool check_adjacent)
{
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x89abcdef01234567ULL },
    };
    static const unsigned start_bytes[2] = {
        4U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, start_bytes[0],
            store_values[0][round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, start_bytes[1],
            store_values[1][round], 8U);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            store_values[first][round]);
        TEST_ASSERT("the first no-H same-page same-width scalar cross-16B store commits cleanly before the different-template follow-up",
            excpt.triggered == false
        );

        TEST_ASSERT("after the first no-H same-page same-width scalar cross-16B store, only its own boundary reflects the expected template-specific 8-byte overlay while the other boundary and middle gap keep their seeded images",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                expected_boundary[first], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                init_patterns[second][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the first no-H same-page same-width scalar cross-16B store, the outer guard regions remain unchanged in the different-template probe",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[second],
            store_values[second][round]);
        TEST_ASSERT("the second no-H same-page same-width scalar cross-16B store also commits cleanly after the earlier different-template store",
            excpt.triggered == false
        );

        TEST_ASSERT("after the no-H same-page same-width scalar cross-16B different-template sequence, each boundary keeps only its own template-specific split bytes and the middle gap remains untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                expected_boundary[0], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                expected_boundary[1], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H same-page same-width scalar cross-16B different-template sequence, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_same_width_template_switch_window(
    bool check_adjacent)
{
    static const uint64_t fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t recovery_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const unsigned fault_start_bytes[2] = {
        4U,
        7U,
    };
    static const unsigned recovery_start_bytes[2] = {
        7U,
        4U,
    };
    static const uintptr_t fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t recovery_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned target = round;
        const unsigned other = target ^ 1U;
        const uintptr_t fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                fault_offsets[round]);
        const uintptr_t recovery_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                recovery_offsets[round]);
        const uintptr_t deny_start = boundary_phys[target] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary,
            init_patterns[target][round], 16U, recovery_start_bytes[round],
            recovery_store_values[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(fault_vaddr,
            fault_store_values[round]);
        TEST_ASSERT("the first no-H same-page same-width scalar cross-16B different-template store fault reports store access fault with the original misaligned VA before the repaired template-switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(fault_vaddr,
            fault_store_values[round]);
        TEST_ASSERT("repeating the no-H same-page same-width scalar cross-16B different-template store fault keeps the same precise metadata before the repaired template-switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the same-page same-width scalar cross-16B different-template store recovery probe, both boundary windows and the middle gap remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[target],
                init_patterns[target][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[other],
                init_patterns[other][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("before the same-page same-width scalar cross-16B different-template store recovery probe, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(recovery_vaddr,
            recovery_store_values[round]);
        TEST_ASSERT("after repairing the no-H same-page same-width scalar cross-16B different-template store fault, the next store must commit with the current template instead of inheriting stale split parameters from the earlier template",
            excpt.triggered == false
        );

        TEST_ASSERT("after the no-H same-page same-width scalar cross-16B different-template fault-recovery sequence, only the target boundary keeps the current template-specific split bytes while the other boundary and middle gap remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[target],
                expected_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[other],
                init_patterns[other][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H same-page same-width scalar cross-16B different-template fault-recovery sequence, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_reuse_window(
    bool check_adjacent)
{
    static const uint64_t fault_store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0x8877665544332211ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const uintptr_t deny_start = boundary_phys[first] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, 7U, fault_store_values[0][round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, 7U, fault_store_values[1][round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            fault_store_values[first][round]);
        TEST_ASSERT("the first no-H same-page scalar cross-16B boundary store fault reports store access fault on the original misaligned boundary address before the next-boundary fake-crosspage-exception probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            fault_store_values[first][round]);
        TEST_ASSERT("repeating the no-H same-page scalar cross-16B boundary store fault keeps the same precise fault metadata before the repaired next-boundary probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page scalar cross-16B boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page scalar cross-16B boundary store fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[second],
            fault_store_values[second][round]);
        TEST_ASSERT("after repairing the no-H same-page scalar cross-16B boundary store fault, the first valid store on the next boundary must commit without inheriting stale fake-crosspage state from the earlier faulting boundary",
            excpt.triggered == false
        );

        TEST_ASSERT("after the no-H fault-then-next-boundary scalar cross-16B store sequence, only the repaired target boundary keeps its expected eight-byte overlay while the faulting boundary and middle gap remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_boundary[second], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H fault-then-next-boundary scalar cross-16B store sequence, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_width_switch_window(
    bool check_adjacent)
{
    static const unsigned store_sizes[2] = {
        4U,
        8U,
    };
    static const unsigned start_bytes[2] = {
        5U,
        7U,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0000000089abcdefULL, 0x000000000badf00dULL },
        { 0x13579bdf2468ace0ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xd0, 0xc1, 0xb2, 0xa3, 0x94, 0x85, 0x76, 0x67,
              0x58, 0x49, 0x3a, 0x2b, 0x1c, 0x0d, 0xfe, 0xef },
            { 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98,
              0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xf0, 0x11, 0x22 },
        },
        {
            { 0x5e, 0x4d, 0x3c, 0x2b, 0x1a, 0x09, 0xf8, 0xe7,
              0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81, 0x70, 0x6f },
            { 0xb1, 0xa2, 0x93, 0x84, 0x75, 0x66, 0x57, 0x48,
              0x39, 0x2a, 0x1b, 0x0c, 0xfd, 0xee, 0xdf, 0xc0 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x13, 0x25, 0x37, 0x49, 0x5b, 0x6d, 0x7f, 0x81 },
        { 0x8f, 0x7d, 0x6b, 0x59, 0x47, 0x35, 0x23, 0x11 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x90, 0xa2, 0xb4, 0xc6, 0xd8, 0xea, 0xfc, 0x0e,
          0x1f, 0x30, 0x41, 0x52, 0x63, 0x74, 0x85, 0x96 },
        { 0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
          0x97, 0xa9, 0xbb, 0xcd, 0xdf, 0xe1, 0xf3, 0x05 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x24, 0x46, 0x68, 0x8a, 0xac, 0xce, 0xe0, 0xf2 },
        { 0x4a, 0x5c, 0x6e, 0x70, 0x82, 0x94, 0xa6, 0xb8 },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const uintptr_t deny_start = boundary_phys[first] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, start_bytes[0],
            store_values[0][round], store_sizes[0]);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, start_bytes[1],
            store_values[1][round], store_sizes[1]);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[first],
            store_sizes[first], store_values[first][round]);
        TEST_ASSERT("the first no-H same-page different-width scalar cross-16B boundary store fault reports store access fault on the original misaligned boundary address before the repaired next-boundary width-switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[first],
            store_sizes[first], store_values[first][round]);
        TEST_ASSERT("repeating the no-H same-page different-width scalar cross-16B boundary store fault keeps the same precise fault metadata before the repaired next-boundary width-switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page different-width scalar cross-16B boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page different-width scalar cross-16B boundary store fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[second],
            store_sizes[second], store_values[second][round]);
        TEST_ASSERT("after repairing the no-H same-page different-width scalar cross-16B boundary store fault, the first valid next-boundary store must commit with its own width template instead of inheriting stale split state from the earlier faulting boundary",
            excpt.triggered == false
        );

        TEST_ASSERT("after the no-H fault-then-next-boundary different-width scalar cross-16B store sequence, only the repaired target boundary keeps its own width-specific overlay while the faulting boundary and middle gap remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_boundary[second], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H fault-then-next-boundary different-width scalar cross-16B store sequence, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_same_address_width_switch_window(
    bool check_adjacent)
{
    static const unsigned fault_sizes[2] = {
        4U,
        8U,
    };
    static const unsigned recovery_sizes[2] = {
        8U,
        4U,
    };
    static const uint64_t fault_store_values[2] = {
        0x0000000089abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t recovery_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x000000000badf00dULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xc0, 0xb1, 0xa2, 0x93, 0x84, 0x75, 0x66, 0x57,
              0x48, 0x39, 0x2a, 0x1b, 0x0c, 0xfd, 0xee, 0xdf },
            { 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9a,
              0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08, 0xf7, 0xe6,
              0xd5, 0xc4, 0xb3, 0xa2, 0x91, 0x80, 0x7f, 0x6e },
            { 0xb2, 0xa3, 0x94, 0x85, 0x76, 0x67, 0x58, 0x49,
              0x3a, 0x2b, 0x1c, 0x0d, 0xfe, 0xed, 0xdc, 0xcb },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80 },
        { 0x81, 0x6f, 0x5d, 0x4b, 0x39, 0x27, 0x15, 0x03 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x91, 0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f,
          0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
          0x98, 0xaa, 0xbc, 0xce, 0xd0, 0xe2, 0xf4, 0x06 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x25, 0x47, 0x69, 0x8b, 0xad, 0xcf, 0xe1, 0xf3 },
        { 0x4b, 0x5d, 0x6f, 0x71, 0x83, 0x95, 0xa7, 0xb9 },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned target = round;
        const unsigned other = target ^ 1U;
        const uintptr_t deny_start = boundary_phys[target] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary,
            init_patterns[target][round], 16U, 5U,
            recovery_store_values[round], recovery_sizes[round]);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[target],
            fault_sizes[round], fault_store_values[round]);
        TEST_ASSERT("the first no-H same-address different-width scalar cross-16B store fault reports store access fault with the original misaligned VA when the second split is PMP-denied",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[target])
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[target],
            fault_sizes[round], fault_store_values[round]);
        TEST_ASSERT("repeating the no-H same-address different-width scalar cross-16B store fault keeps the same precise metadata before the same-address width-switch recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[target])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the same-address different-width scalar cross-16B store recovery probe, the repeated fault episode leaves both boundary windows and the middle gap untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[target],
                init_patterns[target][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[other],
                init_patterns[other][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("before the same-address different-width scalar cross-16B store recovery probe, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(store_vaddr[target],
            recovery_sizes[round], recovery_store_values[round]);
        TEST_ASSERT("after repairing the repeated no-H same-address different-width scalar cross-16B store fault, the first same-address width-switched store must commit with the current width template instead of inheriting stale split state from the earlier faulting template",
            excpt.triggered == false
        );

        TEST_ASSERT("after the no-H same-address different-width scalar cross-16B store recovery sequence, only the target boundary keeps the current width-specific overlay while the other boundary and middle gap remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[target],
                expected_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[other],
                init_patterns[other][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the no-H same-address different-width scalar cross-16B store recovery sequence, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_new_fault_refresh_window(
    bool check_adjacent)
{
    static const uint64_t first_fault_store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0x8877665544332211ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint64_t success_store_values[2][2] = {
        { 0x21436587a9cbed0fULL, 0x1234567890abcdefULL },
        { 0x1020304050607081ULL, 0x89abcdef01234567ULL },
    };
    static const uint64_t younger_fault_store_values[2][2] = {
        { 0x55aa33cc77ee11ffULL, 0x0badf00d13579bdfULL },
        { 0xa55aa55aa55aa55aULL, 0x2468ace0fdb97531ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_success_boundary[16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = first ^ 1U;
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;
        const uintptr_t second_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_deny_end = second_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second][round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            first_fault_store_values[first][round]);
        TEST_ASSERT("the first no-H same-page scalar cross-16B boundary store fault reports store access fault on the original misaligned boundary address before the repair-clear and younger-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            first_fault_store_values[first][round]);
        TEST_ASSERT("repeating the no-H same-page scalar cross-16B boundary store fault keeps the same precise metadata before the repair-clear and younger-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page scalar cross-16B boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page scalar cross-16B boundary store fault, the outer guard regions also remain unchanged before repair-clear",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[second],
            success_store_values[second][round]);
        TEST_ASSERT("after repairing the earlier no-H same-page scalar cross-16B boundary store fault, the first valid next-boundary store commits cleanly and establishes the newer boundary state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page scalar cross-16B fault has already been followed by a valid next-boundary store, only the next boundary keeps its expected eight-byte overlay while the faulting boundary and middle gap remain unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repaired no-H same-page scalar cross-16B fault has already been followed by a valid next-boundary store, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            second_deny_start, second_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[second],
            younger_fault_store_values[second][round]);
        TEST_ASSERT("after the repaired no-H same-page scalar cross-16B fault has already been cleared by a successful next-boundary store, a younger denied store on that next boundary must refresh tval to the current boundary instead of reviving stale fake-crosspage state from the old faulting boundary",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[second])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page scalar cross-16B fault-path repair-clear then younger-fault refresh probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and the middle gap unchanged under the younger denied store",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("the no-H same-page scalar cross-16B fault-path repair-clear then younger-fault refresh probe also keeps the outer guard regions unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}


static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_return_fault_refresh_window(
    bool check_adjacent)
{
    static const uint64_t first_fault_store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0x8877665544332211ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint64_t success_store_values[2][2] = {
        { 0x21436587a9cbed0fULL, 0x1234567890abcdefULL },
        { 0x1020304050607081ULL, 0x89abcdef01234567ULL },
    };
    static const uint64_t return_fault_store_values[2][2] = {
        { 0x0fedcba987654321ULL, 0x55aa33cc77ee11ffULL },
        { 0x5aa55aa55aa55aa5ULL, 0x2468ace0fdb97531ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t store_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_success_boundary[16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = first ^ 1U;
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second][round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            first_fault_store_values[first][round]);
        TEST_ASSERT("the first no-H same-page scalar cross-16B boundary store fault reports store access fault on the original misaligned boundary address before the repair-clear return-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            first_fault_store_values[first][round]);
        TEST_ASSERT("repeating the no-H same-page scalar cross-16B boundary store fault keeps the same precise metadata before the repair-clear return-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page scalar cross-16B boundary store fault, both boundaries and the middle gap still keep their seeded images before repair-return-fault probing",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page scalar cross-16B boundary store fault, the outer guard regions also remain unchanged before repair-return-fault probing",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[second],
            success_store_values[second][round]);
        TEST_ASSERT("after repairing the earlier no-H same-page scalar cross-16B boundary store fault, the first valid next-boundary store commits cleanly before the original-boundary younger-fault probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page scalar cross-16B boundary fault has already been followed by a valid next-boundary store, only the next boundary keeps its expected eight-byte overlay while the original faulting boundary and middle gap remain unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repaired no-H same-page scalar cross-16B boundary fault has already been followed by a valid next-boundary store, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr[first],
            return_fault_store_values[first][round]);
        TEST_ASSERT("after the repaired no-H same-page scalar cross-16B fault has already been followed by a successful next-boundary store, a younger denied store back on the original boundary must refresh tval back to the current faulting boundary instead of staying on the middle success boundary",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr[first])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page scalar cross-16B fault-path repair-clear then return-fault refresh probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and middle gap unchanged under the returned younger denied store",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("the no-H same-page scalar cross-16B fault-path repair-clear then return-fault refresh probe also keeps the outer guard regions unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}



static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_return_fault_same_width_template_window(
    bool check_adjacent)
{
    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t return_fault_store_values[2] = {
        0x21436587a9cbed0fULL,
        0x55aa33cc77ee11ffULL,
    };
    static const uintptr_t first_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uintptr_t success_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t return_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const unsigned success_start_bytes[2] = {
        7U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_success_boundary[16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(
                page, first_fault_offsets[first]);
        const uintptr_t success_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(
                page, success_offsets[second]);
        const uintptr_t return_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(
                page, return_fault_offsets[first]);
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, success_start_bytes[second],
            success_store_values[second], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the repair-clear returned-fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the repair-clear returned-fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned original-boundary fault probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and middle gap remain unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary fault has already been followed by a valid next-boundary store, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(return_fault_vaddr,
            return_fault_store_values[first]);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary fault has already been followed by a successful next-boundary store, a returned younger original-boundary store fault must refresh tval back to the current template-specific address instead of staying on the middle success template",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(return_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then returned-fault probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and middle gap unchanged under the returned younger denied store",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("the no-H same-page same-width different-template repair-clear then returned-fault probe also keeps the outer guard regions unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}


static bool ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_next_boundary_template_fault_window(
    bool check_adjacent)
{
    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t younger_fault_store_values[2] = {
        0x21436587a9cbed0fULL,
        0x55aa33cc77ee11ffULL,
    };
    static const uintptr_t first_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const uintptr_t success_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t younger_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET,
    };
    static const unsigned success_start_bytes[2] = {
        7U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_success_boundary[16];
    bool test_status = true;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(
                page, first_fault_offsets[first]);
        const uintptr_t success_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(
                page, success_offsets[second]);
        const uintptr_t younger_fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(
                page, younger_fault_offsets[second]);
        const uintptr_t first_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_deny_end = first_deny_start + PAGE_SIZE;
        const uintptr_t second_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_deny_end = second_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, success_start_bytes[second],
            success_store_values[second], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            first_deny_start, first_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the next-boundary template-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the next-boundary template-fault refresh probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, the outer guard regions also remain unchanged before repair",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the younger next-boundary template-fault probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and middle gap remain unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary fault has already been followed by a valid next-boundary store, the outer guard regions also remain unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            second_deny_start, second_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(younger_fault_vaddr,
            younger_fault_store_values[second]);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary fault has already been followed by a successful next-boundary store, a younger different-template store fault on that next boundary must refresh tval to the current template-specific address instead of staying on the middle success template",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(younger_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then next-boundary template-fault refresh probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and middle gap unchanged under the younger denied store",
            ai_micro_m_memblock_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("the no-H same-page same-width different-template repair-clear then next-boundary template-fault refresh probe also keeps the outer guard regions unchanged",
                ai_micro_m_memblock_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_m_memblock_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    ai_micro_m_memblock_reset_env();

    return test_status;
}
















__attribute__((naked, noinline, aligned(4))) static void
ai_micro_m_memblock_translated_load_store_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_paddr,
    uint64_t second_value,
    struct ai_micro_m_memblock_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "ld t0, 0(a0)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "sd a3, 0(a1)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [clear_mpp] "i" (3ULL << 11),
          [set_mprv] "i" (MSTATUS_MPRV),
          [set_mpp_s] "i" (MSTATUS_MPP),
          [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, tval))
        : "t0", "t1", "t2", "t3", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_m_memblock_translated_store_load_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_paddr,
    uint64_t first_value,
    struct ai_micro_m_memblock_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "sd a3, 0(a0)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t3, %[set_mprv]\n"
        "csrs mstatus, t3\n"
        "li t3, %[set_mpp_s]\n"
        "csrs mstatus, t3\n"
        "ld t2, 0(a1)\n"
        "li t3, %[set_mprv]\n"
        "csrc mstatus, t3\n"
        "li t3, %[clear_mpp]\n"
        "csrc mstatus, t3\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [clear_mpp] "i" (3ULL << 11),
          [set_mprv] "i" (MSTATUS_MPRV),
          [set_mpp_s] "i" (MSTATUS_MPP),
          [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_m_memblock_exception_snapshot, tval))
        : "t0", "t1", "t2", "t3", "memory");
}

#endif /* AI_MICRO_MMODE_MEMBLOCK_COMMON_H */
