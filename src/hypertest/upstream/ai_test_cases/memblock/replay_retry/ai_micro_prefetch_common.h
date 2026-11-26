#ifndef AI_MICRO_PREFETCH_COMMON_H
#define AI_MICRO_PREFETCH_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_prefetch_tlb_miss_silent();
bool ai_micro_hs_prefetch_repeated_tlb_miss_silent_corner();
bool ai_micro_hs_prefetch_repeated_same_address_silent_corner();
bool ai_micro_hs_prefetch_tlb_exception_silent();
bool ai_micro_hs_prefetch_page_fault_then_valid_access_recovers_corner();
bool ai_micro_hs_prefetch_page_fault_then_valid_access_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_repeated_page_fault_silent_corner();
bool ai_micro_hs_prefetch_access_fault_then_valid_access_recovers_corner();
bool ai_micro_hs_prefetch_access_fault_then_valid_access_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_repeated_access_fault_silent_corner();
bool ai_micro_hs_prefetch_dcache_miss_silent();
bool ai_micro_hs_prefetch_repeated_dcache_miss_silent_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_preserves_latest_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_repeated_silent_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_then_pmp_deny_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_then_pmp_resp_ld_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_then_pmp_resp_st_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_silent();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_deny_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_resp_ld_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_resp_st_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_repeated_silent_corner();
bool ai_micro_hs_prefetch_mshr_full_then_store_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_then_bus_error_silent_corner();
bool ai_micro_hs_prefetch_bus_error_silent();
bool ai_micro_hs_prefetch_bus_error_repeated_silent_corner();
bool ai_micro_hs_prefetch_bus_error_then_valid_access_recovers_corner();
bool ai_micro_hs_prefetch_bus_error_then_valid_access_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_pmp_deny_silent();
bool ai_micro_hs_prefetch_pmp_deny_repeated_silent_corner();
bool ai_micro_hs_prefetch_pmp_deny_then_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_then_pmp_deny_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mmio_silent();
bool ai_micro_hs_prefetch_mmio_repeated_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_pbmt_nc_repeated_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_pmp_resp_ld_silent();
bool ai_micro_hs_prefetch_pmp_resp_ld_repeated_silent_corner();
bool ai_micro_hs_prefetch_pmp_resp_ld_then_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_pmp_resp_st_silent();
bool ai_micro_hs_prefetch_pmp_resp_st_repeated_silent_corner();
bool ai_micro_hs_prefetch_pmp_resp_st_then_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_then_pmp_resp_st_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_pmp_resp_st_repeated_then_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_then_pmp_resp_ld_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner();
bool ai_micro_m_prefetch_pmp_allow_silent();
bool ai_micro_m_prefetch_unlocked_pmp_u_bypass_silent_corner();
bool ai_micro_m_prefetch_locked_pmp_u_silent();
bool ai_micro_m_prefetch_locked_pmp_u_repeated_silent_corner();
bool ai_micro_m_prefetch_locked_pmp_m_silent();
bool ai_micro_m_prefetch_locked_pmp_m_repeated_silent_corner();
bool ai_micro_m_prefetch_mprv_u_pmp_deny_silent();
bool ai_micro_m_prefetch_mprv_u_pmp_deny_repeated_silent_corner();
bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_restore_recovers_corner();
bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_restore_adjacent_preserved_corner();
bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_sfence_restore_recovers_corner();
bool ai_micro_m_prefetch_mprv_u_same_page_deny_then_sfence_restore_adjacent_preserved_corner();
bool ai_micro_m_prefetch_mprv_u_same_page_repeated_deny_then_sfence_restore_recovers_corner();
bool ai_micro_m_prefetch_mprv_u_same_page_repeated_deny_then_sfence_restore_adjacent_preserved_corner();
bool ai_micro_m_prefetch_mprv_u_deny_then_allowed_translated_access_succeeds_corner();
bool ai_micro_m_prefetch_mprv_u_repeated_deny_then_allowed_translated_access_succeeds_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_silent();
bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_then_valid_prefetch_recovers_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_then_valid_prefetch_adjacent_preserved_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_silent_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_then_valid_prefetch_recovers_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_then_valid_prefetch_adjacent_preserved_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_silent();
bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_silent_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_then_valid_prefetch_recovers_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_then_valid_prefetch_adjacent_preserved_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_then_valid_prefetch_recovers_corner();
bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_then_valid_prefetch_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_deny_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_resp_st_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_deny_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner();
bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_resp_st_restore_adjacent_preserved_corner();

#define AI_MICRO_PREFETCH_PAGE VSRWX_GRWX
#define AI_MICRO_PREFETCH_IO_ALIAS_PAGE VSRWXPbmt2_GURWXPbmt1
#define AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE VSRWXPbmt1_GURWXPbmt1
#define AI_MICRO_PREFETCH_IO_BACKING_PAGE X
#define AI_MICRO_PREFETCH_BAD_PADDR 0x1fffffffffff8ULL
#define AI_MICRO_PREFETCH_IO_MEM_BASE  0x30000000000ULL
#define AI_MICRO_PREFETCH_IO_MEM_LIMIT 0x40000000000ULL

#define AI_MICRO_PREFETCH_PMP_TOR_RWX  0x0fULL
#define AI_MICRO_PREFETCH_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_PREFETCH_PMP_TOR_NO_W 0x0dULL
#define AI_MICRO_PREFETCH_PMP_TOR_NO_R_LOCKED 0x8eULL

static const enum test_page ai_micro_prefetch_mshr_pages[] = {
    VSRWX_GRWX,
    VSRW_GRWX,
    VSRX_GRWX,
    VSR_GRWX,
    VSRWX_GURWX,
    VSRW_GURWX,
    VSRX_GURWX,
    VSR_GURWX,
};

static inline pte_t *ai_micro_prefetch_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_prefetch_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0x0fULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_prefetch_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_PREFETCH_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_PREFETCH_PMP_TOR_RWX << 16));
}

static inline void ai_micro_prefetch_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_prefetch_enable_mprv_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);
}

static inline void ai_micro_prefetch_enable_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline void ai_micro_prefetch_reset_pbmt_env(void)
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
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_prefetch_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_prefetch_io_backing_paddr(void)
{
    return phys_page_base_offset(AI_MICRO_PREFETCH_IO_BACKING_PAGE,
        AI_MICRO_PREFETCH_IO_MEM_BASE);
}

static inline void ai_micro_prefetch_prepare_pbmt_mem_3t_4t_window(void)
{
    ai_micro_prefetch_reset_pbmt_env();

    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, AI_MICRO_PREFETCH_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_PREFETCH_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline uintptr_t ai_micro_prefetch_prepare_io_alias(void)
{
    ai_micro_prefetch_prepare_pbmt_mem_3t_4t_window();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(AI_MICRO_PREFETCH_IO_ALIAS_PAGE,
        AI_MICRO_PREFETCH_IO_MEM_BASE);
    sfence_vma();
    return hs_page_base(AI_MICRO_PREFETCH_IO_ALIAS_PAGE);
}

static inline uintptr_t ai_micro_prefetch_prepare_bad_paddr_alias(enum test_page page,
    uintptr_t bad_paddr)
{
    uintptr_t bad_page_start = bad_paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(page,
        ai_micro_prefetch_alias_anchor(page, bad_page_start));
    sfence_vma();

    return hs_page_base(page) + (bad_paddr & (PAGE_SIZE - 1));
}

static inline uintptr_t ai_micro_prefetch_prepare_same_paddr_alias(
    enum test_page alias_page, enum test_page base_page)
{
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_prefetch_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline uintptr_t ai_micro_prefetch_prepare_overflow_alias(
    enum test_page alias_page, enum test_page base_page)
{
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_prefetch_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    *ai_micro_prefetch_hs_leaf_entry(alias_page) |= (1ULL << 46);
    sfence_vma();

    return hs_page_base(alias_page);
}

#endif /* AI_MICRO_PREFETCH_COMMON_H */
