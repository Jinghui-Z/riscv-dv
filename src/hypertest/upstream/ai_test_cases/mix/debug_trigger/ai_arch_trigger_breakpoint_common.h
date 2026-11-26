#ifndef AI_ARCH_TRIGGER_BREAKPOINT_COMMON_H
#define AI_ARCH_TRIGGER_BREAKPOINT_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_arch_load_execute_trigger_breakpoint();
bool ai_arch_load_execute_trigger_repeated_breakpoint_corner();
bool ai_arch_store_execute_trigger_breakpoint();
bool ai_arch_store_execute_trigger_repeated_breakpoint_no_side_effect_corner();
bool ai_arch_amo_execute_trigger_breakpoint();
bool ai_arch_amo_execute_trigger_repeated_breakpoint_no_side_effect_corner();
bool ai_arch_load_address_trigger_breakpoint();
bool ai_arch_load_address_trigger_repeated_breakpoint_corner();
bool ai_arch_load_execute_and_address_trigger_both_breakpoint_corner();
bool ai_arch_load_execute_and_address_trigger_repeated_breakpoint_corner();
bool ai_arch_load_execute_and_address_trigger_access_fault_priority_corner();
bool ai_arch_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_page_fault_priority_corner();
bool ai_arch_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_store_address_trigger_breakpoint();
bool ai_arch_store_address_trigger_repeated_breakpoint_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_both_breakpoint_corner();
bool ai_arch_store_execute_and_address_trigger_repeated_breakpoint_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_access_fault_priority_corner();
bool ai_arch_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_store_execute_and_address_trigger_page_fault_priority_corner();
bool ai_arch_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_amo_address_trigger_breakpoint();
bool ai_arch_amo_address_trigger_repeated_breakpoint_no_side_effect_corner();
bool ai_arch_amo_execute_and_address_trigger_both_breakpoint_corner();
bool ai_arch_amo_execute_and_address_trigger_page_fault_spike_corner();
bool ai_arch_amo_execute_and_address_trigger_misaligned_access_fault_spike_corner();
bool ai_arch_amo_execute_and_address_trigger_access_fault_priority_corner();
bool ai_arch_amo_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_amo_trigger_access_fault_priority();
bool ai_arch_amo_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_amo_execute_trigger_access_fault_priority();
bool ai_arch_amo_trigger_misaligned_access_fault_spike_corner();
bool ai_arch_amo_trigger_misaligned_repeated_breakpoints_then_aligned_recovers_spike_corner();
bool ai_arch_amo_execute_trigger_misaligned_access_fault_priority();
bool ai_arch_load_trigger_access_fault_priority();
bool ai_arch_load_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_widths_trigger_access_fault_priority_manual_corner();
bool ai_arch_load_execute_trigger_access_fault_priority();
bool ai_arch_load_execute_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_store_trigger_access_fault_priority();
bool ai_arch_store_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_store_execute_trigger_access_fault_priority();
bool ai_arch_store_execute_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_amo_trigger_page_fault_spike_corner();
bool ai_arch_amo_trigger_page_fault_repeated_breakpoints_then_valid_access_recovers_spike_corner();
bool ai_arch_amo_execute_trigger_page_fault_priority();
bool ai_arch_load_trigger_page_fault_priority();
bool ai_arch_load_execute_trigger_page_fault_priority();
bool ai_arch_load_execute_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner();
bool ai_arch_load_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner();
bool ai_arch_load_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner();
bool ai_arch_load_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner();
bool ai_arch_store_trigger_page_fault_priority();
bool ai_arch_store_execute_trigger_page_fault_priority();
bool ai_arch_store_execute_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_store_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner();
bool ai_arch_store_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner();
bool ai_arch_store_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner();
bool ai_arch_store_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner();
bool ai_arch_load_trigger_misaligned_priority();
bool ai_arch_load_trigger_misaligned_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_trigger_misaligned_priority();
bool ai_arch_load_execute_trigger_misaligned_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_priority_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_load_recovers_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_load_adjacent_preserved_corner();
bool ai_arch_store_trigger_misaligned_priority();
bool ai_arch_store_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_trigger_misaligned_priority();
bool ai_arch_store_execute_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_misaligned_priority_corner();
bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_store_recovers_corner();
bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_store_adjacent_preserved_corner();
bool ai_arch_load_trigger_misaligned_page_fault_priority_corner();
bool ai_arch_load_execute_trigger_misaligned_page_fault_priority_corner();
bool ai_arch_load_execute_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_priority_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fixed_cross_page_load_recovers_corner();
bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fixed_cross_page_load_adjacent_preserved_corner();
bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_load_recovers_corner();
bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_load_adjacent_preserved_corner();
bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner();
bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner();
bool ai_arch_store_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner();
bool ai_arch_store_execute_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner();
bool ai_arch_store_execute_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_repaired_cross_page_store_recovers_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_repaired_cross_page_store_adjacent_preserved_corner();
bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_store_recovers_corner();
bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_store_adjacent_preserved_corner();
bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner();
bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner();
bool ai_arch_load_trigger_pbmt_nc_over_io_misaligned_priority_corner();
bool ai_arch_load_execute_trigger_pbmt_nc_over_io_misaligned_priority_corner();
bool ai_arch_load_execute_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_priority_corner();
bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_corner();
bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_load_recovers_corner();
bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_load_adjacent_preserved_corner();
bool ai_arch_store_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner();
bool ai_arch_store_execute_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner();
bool ai_arch_store_execute_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_store_recovers_corner();
bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_store_adjacent_preserved_corner();
bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_corner();
bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_corner();
bool ai_arch_prefetch_r_execute_trigger_then_no_r_load_page_fault_corner();
bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_no_r_load_page_fault_corner();
bool ai_arch_prefetch_w_execute_trigger_then_no_w_store_page_fault_corner();
bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_no_w_store_page_fault_corner();
bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_grant_r_load_recovers_corner();
bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_grant_r_load_adjacent_preserved_corner();
bool ai_arch_prefetch_r_address_trigger_silent_then_grant_r_load_recovers_corner();
bool ai_arch_prefetch_r_address_trigger_silent_then_grant_r_load_adjacent_preserved_corner();
bool ai_arch_prefetch_r_execute_then_address_trigger_silent_grant_r_load_recovers_corner();
bool ai_arch_prefetch_r_execute_then_address_trigger_silent_grant_r_load_adjacent_preserved_corner();
bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_grant_w_store_recovers_corner();
bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_grant_w_store_adjacent_preserved_corner();
bool ai_arch_prefetch_w_address_trigger_silent_then_grant_w_store_recovers_corner();
bool ai_arch_prefetch_w_address_trigger_silent_then_grant_w_store_adjacent_preserved_corner();
bool ai_arch_prefetch_w_execute_then_address_trigger_silent_grant_w_store_recovers_corner();
bool ai_arch_prefetch_w_execute_then_address_trigger_silent_grant_w_store_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_no_x_fetch_page_fault_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_no_x_fetch_page_fault_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_noncanonical_valid_fetch_recovers_adjacent_preserved_corner();
bool ai_arch_prefetch_i_redirect_kill_no_late_ifetch_prefetch_emit_corner();
bool ai_arch_killed_prefetch_i_reused_load_slot_no_stale_frontend_prefetch_vaddr_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_all_ones_valid_fetch_recovers_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_arch_load_execute_trigger_page_fault_rd_suppressed_corner();
bool ai_arch_amo_address_trigger_rd_suppressed_no_rmw_then_clear_swaps_corner();
bool ai_arch_cbozero_execute_trigger_breakpoint_blocks_zero_then_clear_trigger_zeroes_block_corner();

static uint64_t ai_arch_trigger_mem[2] __attribute__((aligned(16)));

#define AI_ARCH_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_ARCH_MCONTROL6_LOAD 0x1ULL
#define AI_ARCH_MCONTROL6_STORE 0x2ULL
#define AI_ARCH_MCONTROL6_EXECUTE 0x4ULL
#define AI_ARCH_MCONTROL6_AMO (AI_ARCH_MCONTROL6_LOAD | AI_ARCH_MCONTROL6_STORE)
#define AI_ARCH_TRIGGER_BAD_PADDR 0x1fffffffffff8ULL

typedef struct {
    bool triggered;
    uint64_t cause;
    uint64_t tval;
} ai_arch_trigger_excpt_snapshot_t;

static inline ai_arch_trigger_excpt_snapshot_t ai_arch_snapshot_excpt(void)
{
    ai_arch_trigger_excpt_snapshot_t snapshot = {
        .triggered = excpt.triggered,
        .cause = excpt.cause,
        .tval = excpt.tval,
    };
    return snapshot;
}

static inline uintptr_t ai_arch_trigger_addr(size_t index)
{
    return (uintptr_t)&ai_arch_trigger_mem[index];
}

static inline uint64_t ai_arch_trigger_amoswap_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline uint64_t ai_arch_trigger_amoswap_w_probe_rd(uintptr_t addr,
    uint32_t value,
    uint64_t sentinel)
{
    uint64_t rd = sentinel;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(operand)
        : "memory");

    return rd;
}

static inline void ai_arch_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_arch_clear_mcontrol6_0(void)
{
    ai_arch_clear_mcontrol6_slot(0);
}

static inline uint64_t ai_arch_mcontrol6_0_tdata1(uint64_t access_bits)
{
    return AI_ARCH_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_arch_arm_mcontrol6_slot(size_t slot, uintptr_t addr, uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_arch_mcontrol6_0_tdata1(access_bits));
}

static inline void ai_arch_arm_mcontrol6_0(uintptr_t addr, uint64_t access_bits)
{
    ai_arch_arm_mcontrol6_slot(0, addr, access_bits);
}

static inline uint64_t ai_arch_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_arch_reenable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
}

static inline void ai_arch_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_arch_sync_trigger_state(void)
{
    asm volatile("fence.i" ::: "memory");
    sfence_vma();
    ai_arch_reenable_breakpoint_delivery();
}

static inline void ai_arch_log_unexpected_trigger_result(const char *message)
{
    INFO("%s", message);
    INFO_PRINT_XCEPT();
}

static inline pte_t *ai_arch_trigger_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uintptr_t ai_arch_trigger_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
{
#ifdef sv48
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit47 = (canonical_vaddr >> 47) & 1ULL;

    if (bit47) {
        bad_vaddr &= ~(1ULL << 48);
    } else {
        bad_vaddr |= (1ULL << 48);
    }
#else
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit38 = (canonical_vaddr >> 38) & 1ULL;

    if (bit38) {
        bad_vaddr &= ~(1ULL << 39);
    } else {
        bad_vaddr |= (1ULL << 39);
    }
#endif

    return bad_vaddr;
}

static inline uintptr_t ai_arch_trigger_cross_page_addr(enum test_page low_page,
    uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline uintptr_t ai_arch_trigger_page_end_phys(enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline void ai_arch_trigger_set_hs_leaf(enum test_page page,
    uintptr_t paddr,
    uint64_t flags)
{
    *ai_arch_trigger_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_arch_prepare_hs_fault_page(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_arch_trigger_prepare_pbmt_nc_alias(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt1_GURWXPbmt0);
    sfence_vma();

    return hs_page_base(VSRWXPbmt1_GURWXPbmt0);
}

static inline void ai_arch_trigger_reset_pbmt_only_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_trigger_reset_svpbmt_pma_env(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
}

static inline uintptr_t ai_arch_trigger_prepare_nc_page_over_io_region(void)
{
    goto_priv(PRIV_M);

    ai_arch_trigger_reset_svpbmt_pma_env();

    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);

    hspt_leaf_change_base_paddr(VSRWXPbmt1_GURWXPbmt0, 0x30000000000ULL);
    sfence_vma();

    return hs_page_base(VSRWXPbmt1_GURWXPbmt0);
}

static inline uintptr_t ai_arch_trigger_nc_page_over_io_backing_paddr(void)
{
    return phys_page_base_offset(VSRWXPbmt1_GURWXPbmt0, 0x30000000000ULL);
}

static inline uintptr_t ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias(void)
{
    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_trigger_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_X);
    sfence_vma();

    return ai_arch_trigger_cross_page_addr(low_page, 4U);
}

static inline uintptr_t ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias(void)
{
    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_arch_trigger_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    return ai_arch_trigger_cross_page_addr(low_page, 4U);
}

static inline uintptr_t ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return ai_arch_trigger_cross_page_addr(VSR_GX, 4U);
}

static inline uintptr_t ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return ai_arch_trigger_cross_page_addr(VSRW_GX, 4U);
}

__attribute__((naked, noinline, aligned(4))) static uint64_t ai_arch_execute_trigger_load_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld a0, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

static inline uint64_t ai_arch_trigger_scalar_load_width_once(uintptr_t addr,
    unsigned width_code,
    bool unsigned_load)
{
    uint64_t value = 0ULL;

    if (width_code == 1U) {
        if (unsigned_load) {
            asm volatile("lbu %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        } else {
            asm volatile("lb %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        }
        return value;
    }
    if (width_code == 2U) {
        if (unsigned_load) {
            asm volatile("lhu %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        } else {
            asm volatile("lh %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        }
        return value;
    }
    if (width_code == 4U) {
        if (unsigned_load) {
            asm volatile("lwu %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        } else {
            asm volatile("lw %0, 0(%1)\n\t"
                : "=r"(value) : "r"(addr) : "memory");
        }
        return value;
    }

    asm volatile("ld %0, 0(%1)\n\t"
        : "=r"(value) : "r"(addr) : "memory");
    return value;
}

__attribute__((naked, noinline, aligned(4))) static uint64_t ai_arch_execute_trigger_ld_probe_rd_once(uintptr_t addr, uint64_t sentinel)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, a0\n"
        "mv a0, a1\n"
        "ld a0, 0(t0)\n"
        "ret\n"
        ".option pop\n");
}

static inline uintptr_t ai_arch_execute_trigger_ld_probe_rd_load_pc(void)
{
    return (uintptr_t)ai_arch_execute_trigger_ld_probe_rd_once + 8U;
}

__attribute__((naked, noinline, aligned(4))) static void ai_arch_execute_trigger_store_once(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

static inline uint64_t ai_arch_trigger_expected_low_half_visible_store_fragment(
    uint64_t original_low,
    uint64_t store_value)
{
    return (original_low & 0xffffffffULL) | ((store_value & 0xffffffffULL) << 32);
}

__attribute__((naked, noinline, aligned(4))) static uint32_t ai_arch_execute_trigger_amoswap_w_once(uintptr_t addr, uint32_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "amoswap.w a0, a1, (a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_arch_execute_trigger_prefetch_i_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        ".insn i 0x13, 0x6, x0, a0, 0\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_arch_execute_trigger_prefetch_r_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        ".insn i 0x13, 0x6, x0, a0, 1\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_arch_execute_trigger_prefetch_w_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        ".insn i 0x13, 0x6, x0, a0, 3\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_arch_execute_trigger_cbozero_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        ".insn i 0x0f, 0x2, x0, a0, 4\n"
        "ret\n"
        ".option pop\n");
}

#endif /* AI_ARCH_TRIGGER_BREAKPOINT_COMMON_H */
