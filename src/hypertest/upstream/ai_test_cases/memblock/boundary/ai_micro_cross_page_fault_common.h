#ifndef AI_MICRO_CROSS_PAGE_FAULT_COMMON_H
#define AI_MICRO_CROSS_PAGE_FAULT_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_load_cross_page_data_merge();
bool ai_micro_hs_load_cross_page_low_hit_high_fill_data_merge();
bool ai_micro_hs_load_cross_page_low_fill_high_hit_data_merge();
bool ai_micro_hs_load_cross_page_1b7b_data_merge_corner();
bool ai_micro_hs_load_cross_page_7b1b_data_merge_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_load_stable_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_load_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_stable_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_adjacent_preserved_corner();
bool ai_micro_hs_vload_mergebuffer_delayed_wb_not_applied_to_reused_mbindex_corner();
bool ai_micro_hs_vload_mergebuffer_inactive_wb_metadata_not_read_from_reused_entry_corner();
bool ai_micro_hs_vector_load_merge_buffer_pressure_no_hang_corner();
bool ai_micro_hs_vector_load_merge_buffer_pressure_with_redirect_or_exception_corner();
bool ai_micro_hs_load_cross_page_low_half_page_fault();
bool ai_micro_hs_load_cross_page_high_half_nc_misaligned();
bool ai_micro_hs_load_cross_page_low_half_nc_misaligned();
bool ai_micro_hs_load_cross_page_high_half_page_fault();
bool ai_micro_hs_load_cross_page_high_half_page_fault_then_valid_load_recovers_corner();
bool ai_micro_hs_load_cross_page_high_half_page_fault_then_valid_load_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner();
bool ai_micro_hs_load_cross_page_high_half_page_fault_then_access_fault_refreshes_cause_corner();
bool ai_micro_hs_load_cross_page_high_half_access_fault_then_page_fault_refreshes_cause_corner();
bool ai_micro_hs_cross_page_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner();
bool ai_micro_hs_store_cross_page_high_half_page_fault_then_access_fault_refreshes_cause_corner();
bool ai_micro_hs_store_cross_page_high_half_access_fault_then_page_fault_refreshes_cause_corner();
bool ai_micro_hs_cross_page_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner();
bool ai_micro_hs_load_cross_page_two_pmp_entries_allow_no_false_access_fault_corner();
bool ai_micro_hs_store_cross_page_two_pmp_entries_allow_updates_both_entries_corner();
bool ai_micro_hs_load_cross_page_start_pmp_deny_tail_allow_tval_original_corner();
bool ai_micro_hs_cross_page_overlap_pmp_start_priority_deny_corner();
bool ai_micro_hs_load_cross_page_pmp_reconfig_allow_deny_allow_recovers_corner();
bool ai_micro_hs_store_cross_page_pmp_reconfig_allow_deny_allow_recovers_corner();
bool ai_micro_hs_store_cross_page_low_half_page_fault();
bool ai_micro_hs_store_cross_page_low_half_nc_misaligned();
bool ai_micro_hs_store_cross_page_data_split();
bool ai_micro_hs_store_cross_page_1b7b_data_split_corner();
bool ai_micro_hs_store_cross_page_7b1b_data_split_corner();
bool ai_micro_hs_store_cross_page_low_hit_high_fill_data_split();
bool ai_micro_hs_store_cross_page_low_fill_high_hit_data_split();
bool ai_micro_hs_store_cross_page_head_split_upper_half_paddr_and_sqdeq_correct_corner();
bool ai_micro_hs_store_cross_page_reused_upper_half_paddr_not_rebound_to_next_pair_corner();
bool ai_micro_hs_store_cross_page_reused_upper_half_paddr_not_rebound_to_next_pair_adjacent_preserved_corner();
bool ai_micro_hs_store_misalign_direct_wb_and_mabuf_wb_same_cycle_no_drop_corner();
bool ai_micro_hs_vector_store_misalign_direct_feedback_and_mabuf_feedback_no_collision_bug_corner();
bool ai_micro_hs_vector_store_misalign_direct_feedback_and_mabuf_feedback_adjacent_preserved_corner();
bool ai_micro_hs_store_wb_port0_scalar_then_sq_mabuf_adjacent_window_corner();
bool ai_micro_hs_store_wb_port0_sq_mabuf_then_scalar_no_lost_completion_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_store_stable_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_store_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_stable_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_high_half_io_access_fault();
bool ai_micro_hs_store_cross_page_high_half_page_fault();
bool ai_micro_hs_store_cross_page_high_half_page_fault_no_side_effect_corner();
bool ai_micro_hs_store_cross_page_high_half_page_fault_then_valid_store_recovers_corner();
bool ai_micro_hs_store_cross_page_high_half_page_fault_then_valid_store_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner();
bool ai_micro_hs_vector_store_partial_commit_then_late_page_fault_blocks_following_flows_corner();
bool ai_micro_hs_vector_store_repeated_late_page_fault_then_repaired_full_store_recovers_corner();
bool ai_micro_hs_vector_store_late_fault_vecwriteback_owner_not_rebound_to_younger_vector_store_corner();
bool ai_micro_hs_vector_store_late_fault_vecwriteback_owner_not_rebound_to_younger_vector_store_adjacent_preserved_corner();
bool ai_micro_hs_vector_load_unitstride_split_element_second_flow_page_fault_no_partial_dest_update_corner();
bool ai_micro_hs_vector_store_unitstride_split_element_second_flow_page_fault_no_partial_faulting_element_commit_corner();
bool ai_micro_hs_vector_store_unitstride_split_element_third_flow_page_fault_no_partial_faulting_element_commit_corner();
bool ai_micro_hs_vector_store_unitstride_split_fault_vstart_refreshes_from_second_to_third_flow_page_fault_corner();
bool ai_micro_hs_store_cross_page_high_half_io_access_fault();

#define AI_MICRO_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_PMP_TOR_NO_W 0x0dULL

#define AI_MICRO_CROSS_PAGE_IO_MEM_BASE  0x30000000000ULL
#define AI_MICRO_CROSS_PAGE_IO_MEM_LIMIT 0x40000000000ULL

static inline uintptr_t ai_micro_cross_page_addr(enum test_page low_page, uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline void ai_micro_configure_pmp_window(uintptr_t deny_start, uintptr_t deny_end,
    uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_PMP_TOR_RWX | (deny_cfg << 8) | (AI_MICRO_PMP_TOR_RWX << 16));
}

static inline uintptr_t ai_micro_page_end_phys(enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline pte_t *ai_micro_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uintptr_t ai_micro_cross_page_io_paddr(enum test_page page)
{
    return phys_page_base_offset(page, AI_MICRO_CROSS_PAGE_IO_MEM_BASE);
}

static inline void ai_micro_reset_cross_page_pbmt_env(void)
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
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_prepare_cross_page_pbmt_mem_3t_4t_window(void)
{
    ai_micro_reset_cross_page_pbmt_env();

    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, AI_MICRO_CROSS_PAGE_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_CROSS_PAGE_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline void ai_micro_set_hs_leaf_pbmt_paddr(enum test_page page,
    uintptr_t paddr, uint64_t pbmt_bits)
{
    *ai_micro_hs_leaf_entry(page) = (paddr >> 2) | PTE_V | PTE_AD | PTE_RWX | pbmt_bits;
}

static inline void ai_micro_set_hs_leaf_paddr(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    *ai_micro_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_set_hs_leaf(enum test_page page, uint64_t flags)
{
    ai_micro_set_hs_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_set_hs_leaf_pbmt(enum test_page page, uint64_t pbmt_bits)
{
    ai_micro_set_hs_leaf_pbmt_paddr(page, phys_page_base(page), pbmt_bits);
}

static inline uintptr_t ai_micro_cross_page_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
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

static inline void ai_micro_prepare_cross_page_pbmt(enum test_page low_page, uint64_t low_pbmt,
    enum test_page high_page, uint64_t high_pbmt)
{
    goto_priv(PRIV_M);
    ai_micro_reset_cross_page_pbmt_env();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_set_hs_leaf_pbmt(low_page, low_pbmt);
    ai_micro_set_hs_leaf_pbmt(high_page, high_pbmt);
    sfence_vma();
}

static inline void ai_micro_prepare_cross_page_high_half_io_pbmt(enum test_page low_page,
    enum test_page high_page)
{
    ai_micro_prepare_cross_page_pbmt_mem_3t_4t_window();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_set_hs_leaf_pbmt(low_page, 0);
    ai_micro_set_hs_leaf_pbmt_paddr(high_page,
        ai_micro_cross_page_io_paddr(high_page), PTE_Pbmt2);
    sfence_vma();
}

static inline void ai_micro_write_bytes(uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_bytes_match(uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ai_micro_le64(const uint8_t *bytes)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < 8U; ++i) {
        value |= ((uint64_t)bytes[i]) << (i * 8U);
    }

    return value;
}

static inline void ai_micro_cross_page_fence_i(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".4byte 0x0000100f\n\t"
        ".option pop\n\t"
        :
        :
        : "memory");
}

static inline uint64_t ai_micro_cross_page_expected_low_after_4b_store(
    uint64_t original_low, uint64_t store_value)
{
    return (original_low & 0xffffffffULL) |
        ((store_value & 0xffffffffULL) << 32);
}

static inline uint64_t ai_micro_cross_page_expected_high_after_4b_store(
    uint64_t original_high, uint64_t store_value)
{
    return (original_high & 0xffffffff00000000ULL) |
        (store_value >> 32);
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_cross_page_store_pair_once(uintptr_t first_vaddr, uint64_t first_value,
    uintptr_t second_vaddr, uint64_t second_value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "sd a3, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_cross_page_store_pair_delayed(uintptr_t first_vaddr,
    uint64_t first_value, uintptr_t second_vaddr, uint64_t second_value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a2\n"
        "sd a3, 0(t2)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "memory");
}

static bool ai_micro_cross_page_scalar_load_after_vector_store_stable(bool check_adjacent)
{
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint64_t expected_value = 0x2b2a292827262524ULL;
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t vector_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t low_phys_start = phys_page_base(VSRW_GRW) + PAGE_SIZE - 8U;
    const uintptr_t high_phys_start = phys_page_base(VSRW_GRX);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar cross-page load stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload across the same HS page pair before the scalar split-load stable probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store, the shared boundary bytes match the staged payload before the scalar split-load stable probe",
        ai_micro_bytes_match(low_phys_start, source_pattern, 8U) &&
        ai_micro_bytes_match(high_phys_start, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-page store, the adjacent sentinels stay unchanged before the scalar split-load adjacent-preservation probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector cross-page store touches the shared path, a scalar cross-page load still returns the vector-written payload without sticky split state",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-load cross-page stable sequence, the shared boundary bytes still match the vector-written payload",
        ai_micro_bytes_match(low_phys_start, source_pattern, 8U) &&
        ai_micro_bytes_match(high_phys_start, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-load cross-page adjacent-preservation sequence, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    return test_status;
}

static bool ai_micro_cross_page_vector_load_fault_then_other_vector_load_no_rebind(
    bool recheck_young_after_old,
    bool check_adjacent)
{
    enum test_page old_low_page = VSR_GX;
    enum test_page old_high_page = VSX_GRWX;
    enum test_page young_low_page = VSRWX_GRX;
    enum test_page young_high_page = VSRWX_GR;
    const uint8_t old_pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t young_pattern[16] = {
        0x90, 0x91, 0x92, 0x93,
        0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b,
        0x9c, 0x9d, 0x9e, 0x9f
    };
    const uintptr_t old_vaddr = ai_micro_cross_page_addr(old_low_page, 8U);
    const uintptr_t old_second_half_vaddr = old_vaddr + 8U;
    const uintptr_t young_vaddr = ai_micro_cross_page_addr(young_low_page, 8U);
    const uintptr_t young_scalar_vaddr = ai_micro_cross_page_addr(young_low_page, 4U);
    const uintptr_t old_low_phys = phys_page_base(old_low_page) + PAGE_SIZE - 8U;
    const uintptr_t old_high_phys = phys_page_base(old_high_page);
    const uintptr_t young_low_phys = phys_page_base(young_low_page) + PAGE_SIZE - 8U;
    const uintptr_t young_high_phys = phys_page_base(young_high_page);
    const uintptr_t scratch_old_phys = phys_page_base(VSRWX_GRWX) + 0x80U;
    const uintptr_t scratch_young_vaddr = hs_page_base(VSRWX_GRWX) + 0xc0U;
    const uintptr_t scratch_young_phys = phys_page_base(VSRWX_GRWX) + 0xc0U;
    const uintptr_t scratch_recheck_vaddr = hs_page_base(VSRWX_GRWX) + 0x100U;
    const uintptr_t scratch_recheck_phys = phys_page_base(VSRWX_GRWX) + 0x100U;
    const uint8_t old_before = 0x31U;
    const uint8_t old_after = 0x13U;
    const uint8_t young_before = 0x52U;
    const uint8_t young_after = 0x25U;
    const uint8_t scratch_old_before = 0x61U;
    const uint8_t scratch_old_after = 0x16U;
    const uint8_t scratch_young_before = 0x72U;
    const uint8_t scratch_young_after = 0x27U;
    const uint8_t scratch_recheck_before = 0x83U;
    const uint8_t scratch_recheck_after = 0x38U;
    uint64_t young_scalar_value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(old_low_phys, old_pattern, 8U);
    ai_micro_write_bytes(old_high_phys, old_pattern + 8U, 8U);
    ai_micro_write_bytes(young_low_phys, young_pattern, 8U);
    ai_micro_write_bytes(young_high_phys, young_pattern + 8U, 8U);
    write8(old_low_phys - 1U, old_before);
    write8(old_high_phys + 8U, old_after);
    write8(young_low_phys - 1U, young_before);
    write8(young_high_phys + 8U, young_after);
    write8(scratch_old_phys - 1U, scratch_old_before);
    write8(scratch_old_phys + 16U, scratch_old_after);
    write8(scratch_young_phys - 1U, scratch_young_before);
    write8(scratch_young_phys + 16U, scratch_young_after);
    write8(scratch_recheck_phys - 1U, scratch_recheck_before);
    write8(scratch_recheck_phys + 16U, scratch_recheck_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf(old_low_page, PTE_V | PTE_AD | PTE_R);
    ai_micro_set_hs_leaf(old_high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(young_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(young_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)old_vaddr, 16);
    TEST_ASSERT("the older vector split load first faults on the second half before any younger cross-page vector load reuses the visible merge-buffer state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == old_second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older faulting vector split-load attempt leaves both the old boundary payload and the younger candidate boundary payload unchanged before the younger request is issued",
        ai_micro_bytes_match(old_low_phys, old_pattern, 8U) &&
        ai_micro_bytes_match(old_high_phys, old_pattern + 8U, 8U) &&
        ai_micro_bytes_match(young_low_phys, young_pattern, 8U) &&
        ai_micro_bytes_match(young_high_phys, young_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the older faulting vector split-load attempt also leaves all boundary and scratch sentinels unchanged before the younger request is issued",
            read8(old_low_phys - 1U) == old_before &&
            read8(old_high_phys + 8U) == old_after &&
            read8(young_low_phys - 1U) == young_before &&
            read8(young_high_phys + 8U) == young_after &&
            read8(scratch_old_phys - 1U) == scratch_old_before &&
            read8(scratch_old_phys + 16U) == scratch_old_after &&
            read8(scratch_young_phys - 1U) == scratch_young_before &&
            read8(scratch_young_phys + 16U) == scratch_young_after &&
            read8(scratch_recheck_phys - 1U) == scratch_recheck_before &&
            read8(scratch_recheck_phys + 16U) == scratch_recheck_after
        );
    }

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)young_vaddr, 16);
    TEST_ASSERT("after the older faulting vector split load has already occupied the earlier path, a younger vector split load on another page pair still completes normally with its own payload",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_young_vaddr, 16);
    TEST_ASSERT("storing the younger vector split-load payload into a scratch page succeeds immediately after the older faulting request",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger vector split-load scratch copy matches only the younger boundary payload, showing that no older delayed merge-buffer state leaked into the reused live entry",
        ai_micro_bytes_match(scratch_young_phys, young_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the younger vector split load completes, all boundary and scratch sentinels still remain unchanged in the adjacent-preservation variant",
            read8(old_low_phys - 1U) == old_before &&
            read8(old_high_phys + 8U) == old_after &&
            read8(young_low_phys - 1U) == young_before &&
            read8(young_high_phys + 8U) == young_after &&
            read8(scratch_old_phys - 1U) == scratch_old_before &&
            read8(scratch_old_phys + 16U) == scratch_old_after &&
            read8(scratch_young_phys - 1U) == scratch_young_before &&
            read8(scratch_young_phys + 16U) == scratch_young_after &&
            read8(scratch_recheck_phys - 1U) == scratch_recheck_before &&
            read8(scratch_recheck_phys + 16U) == scratch_recheck_after
        );
    }

    hspt_init();
    ai_micro_set_hs_leaf(old_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(old_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(young_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(young_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    TEST_ASSERT("after the younger vector split-load succeeds and the older page pair is repaired for later use, both boundary payloads and the younger scratch copy remain bound to their original backing bytes",
        ai_micro_bytes_match(old_low_phys, old_pattern, 8U) &&
        ai_micro_bytes_match(old_high_phys, old_pattern + 8U, 8U) &&
        ai_micro_bytes_match(young_low_phys, young_pattern, 8U) &&
        ai_micro_bytes_match(young_high_phys, young_pattern + 8U, 8U) &&
        ai_micro_bytes_match(scratch_young_phys, young_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the older faulting page pair is repaired without being re-accessed, the boundary sentinels and all scratch sentinels still match their expected values",
            read8(old_low_phys - 1U) == old_before &&
            read8(old_high_phys + 8U) == old_after &&
            read8(young_low_phys - 1U) == young_before &&
            read8(young_high_phys + 8U) == young_after &&
            read8(scratch_old_phys - 1U) == scratch_old_before &&
            read8(scratch_old_phys + 16U) == scratch_old_after &&
            read8(scratch_young_phys - 1U) == scratch_young_before &&
            read8(scratch_young_phys + 16U) == scratch_young_after &&
            read8(scratch_recheck_phys - 1U) == scratch_recheck_before &&
            read8(scratch_recheck_phys + 16U) == scratch_recheck_after
        );
    }

    if (recheck_young_after_old) {
        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)young_vaddr, 16);
        TEST_ASSERT("after the older faulting page pair is repaired and the earlier allocation episode has been closed, reissuing the younger vector split load still completes normally without inheriting stale inactive-writeback metadata",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)scratch_recheck_vaddr, 16);
        TEST_ASSERT("after reissuing the younger vector split load, storing its payload into a fresh scratch page succeeds and preserves the younger request's own identity",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the reissued younger vector split-load scratch copy still matches only the younger payload, so the earlier older allocation episode does not report the younger live metadata",
            ai_micro_bytes_match(scratch_recheck_phys, young_pattern, 16U)
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    young_scalar_value = ld(young_scalar_vaddr);
    TEST_ASSERT("a scalar cross-page load over the younger boundary still observes the younger payload immediately after the vector split-load no-rebind sequence",
        excpt.triggered == false &&
        young_scalar_value == 0x9b9a999897969594ULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_page_vector_load_merge_pressure(bool inject_old_fault)
{
    enum test_page old_low_page = VSRWX_GRWX;
    enum test_page old_high_page = VSRWX_GRW;
    enum test_page pair0_low_page = VSRWX_GRX;
    enum test_page pair0_high_page = VSRWX_GR;
    enum test_page pair1_low_page = VSRW_GRWX;
    enum test_page pair1_high_page = VSRW_GRW;
    enum test_page pair2_low_page = VSRX_GRWX;
    enum test_page pair2_high_page = VSRX_GRW;
    enum test_page scratch_page = SCRATCHPAD;
    const uint8_t old_pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t pair0_pattern[16] = {
        0x90, 0x91, 0x92, 0x93,
        0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b,
        0x9c, 0x9d, 0x9e, 0x9f
    };
    const uint8_t pair1_pattern[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf
    };
    const uint8_t pair2_pattern[16] = {
        0xb0, 0xb1, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb,
        0xbc, 0xbd, 0xbe, 0xbf
    };
    const uintptr_t old_vaddr = ai_micro_cross_page_addr(old_low_page, 8U);
    const uintptr_t old_second_half_vaddr = old_vaddr + 8U;
    const uintptr_t pair0_vaddr = ai_micro_cross_page_addr(pair0_low_page, 8U);
    const uintptr_t pair1_vaddr = ai_micro_cross_page_addr(pair1_low_page, 8U);
    const uintptr_t pair2_vaddr = ai_micro_cross_page_addr(pair2_low_page, 8U);
    const uintptr_t pair0_scalar_vaddr = ai_micro_cross_page_addr(pair0_low_page, 4U);
    const uintptr_t pair1_scalar_vaddr = ai_micro_cross_page_addr(pair1_low_page, 4U);
    const uintptr_t pair2_scalar_vaddr = ai_micro_cross_page_addr(pair2_low_page, 4U);
    const uintptr_t scratch0_vaddr = hs_page_base(scratch_page) + 0x180U;
    const uintptr_t scratch1_vaddr = hs_page_base(scratch_page) + 0x1c0U;
    const uintptr_t scratch2_vaddr = hs_page_base(scratch_page) + 0x200U;
    const uintptr_t scratch_old_vaddr = hs_page_base(scratch_page) + 0x240U;
    const uintptr_t old_low_phys = phys_page_base(old_low_page) + PAGE_SIZE - 8U;
    const uintptr_t old_high_phys = phys_page_base(old_high_page);
    const uintptr_t pair0_low_phys = phys_page_base(pair0_low_page) + PAGE_SIZE - 8U;
    const uintptr_t pair0_high_phys = phys_page_base(pair0_high_page);
    const uintptr_t pair1_low_phys = phys_page_base(pair1_low_page) + PAGE_SIZE - 8U;
    const uintptr_t pair1_high_phys = phys_page_base(pair1_high_page);
    const uintptr_t pair2_low_phys = phys_page_base(pair2_low_page) + PAGE_SIZE - 8U;
    const uintptr_t pair2_high_phys = phys_page_base(pair2_high_page);
    const uintptr_t scratch0_phys = phys_page_base(scratch_page) + 0x180U;
    const uintptr_t scratch1_phys = phys_page_base(scratch_page) + 0x1c0U;
    const uintptr_t scratch2_phys = phys_page_base(scratch_page) + 0x200U;
    const uintptr_t scratch_old_phys = phys_page_base(scratch_page) + 0x240U;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(old_low_phys, old_pattern, 8U);
    ai_micro_write_bytes(old_high_phys, old_pattern + 8U, 8U);
    ai_micro_write_bytes(pair0_low_phys, pair0_pattern, 8U);
    ai_micro_write_bytes(pair0_high_phys, pair0_pattern + 8U, 8U);
    ai_micro_write_bytes(pair1_low_phys, pair1_pattern, 8U);
    ai_micro_write_bytes(pair1_high_phys, pair1_pattern + 8U, 8U);
    ai_micro_write_bytes(pair2_low_phys, pair2_pattern, 8U);
    ai_micro_write_bytes(pair2_high_phys, pair2_pattern + 8U, 8U);
    ai_micro_write_bytes(scratch0_phys, old_pattern, 16U);
    ai_micro_write_bytes(scratch1_phys, old_pattern, 16U);
    ai_micro_write_bytes(scratch2_phys, old_pattern, 16U);
    ai_micro_write_bytes(scratch_old_phys, pair2_pattern, 16U);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_set_hs_leaf(old_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(old_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair0_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair0_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair1_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair1_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair2_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(pair2_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    if (inject_old_fault) {
        ai_micro_set_hs_leaf(old_low_page, PTE_V | PTE_AD | PTE_R);
        ai_micro_set_hs_leaf(old_high_page, PTE_V | PTE_AD | PTE_X);
    }
    sfence_vma();

    goto_priv(PRIV_HS);
    if (inject_old_fault) {
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)old_vaddr, 16);
        TEST_ASSERT("an older faulting vector split load reports the second-half page fault before the younger merge-buffer-pressure probes begin",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == old_second_half_vaddr
        );
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)pair0_vaddr, 16);
    TEST_ASSERT("the first younger vector split load in the merge-buffer-pressure probe completes normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch0_vaddr, 16);
    TEST_ASSERT("writing the first younger vector split-load payload into scratch succeeds under merge-buffer pressure",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)pair1_vaddr, 16);
    TEST_ASSERT("the second younger vector split load in the merge-buffer-pressure probe also completes normally without no-nack hang behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch1_vaddr, 16);
    TEST_ASSERT("writing the second younger vector split-load payload into scratch succeeds under continued merge-buffer pressure",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)pair2_vaddr, 16);
    TEST_ASSERT("the third younger vector split load in the merge-buffer-pressure probe still completes normally so the shared path keeps making forward progress",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch2_vaddr, 16);
    TEST_ASSERT("writing the third younger vector split-load payload into scratch also succeeds under the same merge-buffer-pressure window",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector merge-buffer-pressure probe, all three younger scratch copies match only their own source payloads so no writeback is dropped or cross-bound",
        ai_micro_bytes_match(scratch0_phys, pair0_pattern, 16U) &&
        ai_micro_bytes_match(scratch1_phys, pair1_pattern, 16U) &&
        ai_micro_bytes_match(scratch2_phys, pair2_pattern, 16U)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t pair0_scalar = ld(pair0_scalar_vaddr);
    TEST_ASSERT("after the first younger vector split load, a scalar cross-page load over that boundary still observes the correct payload",
        excpt.triggered == false &&
        pair0_scalar == 0x9b9a999897969594ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t pair1_scalar = ld(pair1_scalar_vaddr);
    TEST_ASSERT("after the second younger vector split load, a scalar cross-page load over that boundary still observes the correct payload",
        excpt.triggered == false &&
        pair1_scalar == 0xabaaa9a8a7a6a5a4ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t pair2_scalar = ld(pair2_scalar_vaddr);
    TEST_ASSERT("after the third younger vector split load, a scalar cross-page load over that boundary still observes the correct payload",
        excpt.triggered == false &&
        pair2_scalar == 0xbbbab9b8b7b6b5b4ULL
    );

    if (inject_old_fault) {
        goto_priv(PRIV_M);
        ai_micro_set_hs_leaf(old_low_page, PTE_V | PTE_AD | PTE_RWX);
        ai_micro_set_hs_leaf(old_high_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)old_vaddr, 16);
        TEST_ASSERT("after the earlier older exception and three younger completions, reissuing the repaired older vector split load still completes normally without merge-buffer hang",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)scratch_old_vaddr, 16);
        TEST_ASSERT("storing the repaired older vector split-load payload into scratch succeeds after the earlier exception-pressure window",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the exception-inclusive merge-buffer-pressure probe, the repaired older scratch copy matches only the older payload and the three younger scratch copies remain intact",
            ai_micro_bytes_match(scratch_old_phys, old_pattern, 16U) &&
            ai_micro_bytes_match(scratch0_phys, pair0_pattern, 16U) &&
            ai_micro_bytes_match(scratch1_phys, pair1_pattern, 16U) &&
            ai_micro_bytes_match(scratch2_phys, pair2_pattern, 16U)
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the vector merge-buffer-pressure probe leaves all four backing page pairs unchanged because the test only performs loads while scratch pages receive the copied payloads",
        ai_micro_bytes_match(old_low_phys, old_pattern, 8U) &&
        ai_micro_bytes_match(old_high_phys, old_pattern + 8U, 8U) &&
        ai_micro_bytes_match(pair0_low_phys, pair0_pattern, 8U) &&
        ai_micro_bytes_match(pair0_high_phys, pair0_pattern + 8U, 8U) &&
        ai_micro_bytes_match(pair1_low_phys, pair1_pattern, 8U) &&
        ai_micro_bytes_match(pair1_high_phys, pair1_pattern + 8U, 8U) &&
        ai_micro_bytes_match(pair2_low_phys, pair2_pattern, 8U) &&
        ai_micro_bytes_match(pair2_high_phys, pair2_pattern + 8U, 8U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_page_reused_upper_half_paddr_window(
    bool check_adjacent)
{
    static const uint64_t pair0_prev_pattern[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1122334455667788ULL,
        0x55aa33cc77ee11ffULL,
    };
    static const uint64_t pair0_low_pattern[] = {
        0x89abcdef01234567ULL,
        0x1021324354657687ULL,
        0x66778899aabbccddULL,
    };
    static const uint64_t pair0_high_pattern[] = {
        0xa1b2c3d4e5f60718ULL,
        0x98badcfe10325476ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t pair0_next_pattern[] = {
        0xdeadbeefcafef00dULL,
        0x0badf00dfacefeedULL,
        0x7766554433221100ULL,
    };
    static const uint64_t pair1_prev_pattern[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x2468ace013579bdfULL,
    };
    static const uint64_t pair1_low_pattern[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    static const uint64_t pair1_high_pattern[] = {
        0x8877665544332211ULL,
        0x123456789abcdef0ULL,
        0x0ff1ce55aa773311ULL,
    };
    static const uint64_t pair1_next_pattern[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0xcafebabedeadbeefULL,
    };
    static const uint64_t pair0_store_value[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL,
    };
    static const uint64_t pair1_store_value[] = {
        0x8877665544332211ULL,
        0x0f1e2d3c4b5a6978ULL,
        0xa1b2c3d4e5f60718ULL,
    };
    const enum test_page pair0_low_page = VSRW_GRW;
    const enum test_page pair0_high_page = VSRW_GRX;
    const enum test_page pair1_low_page = VSRWX_GRWX;
    const enum test_page pair1_high_page = VSRWX_GRW;
    const uintptr_t pair0_vaddr = ai_micro_cross_page_addr(pair0_low_page, 4U);
    const uintptr_t pair1_vaddr = ai_micro_cross_page_addr(pair1_low_page, 4U);
    const uintptr_t pair0_prev_vaddr = hs_page_base(pair0_low_page) + PAGE_SIZE - 16U;
    const uintptr_t pair0_high_probe_vaddr = hs_page_base(pair0_high_page);
    const uintptr_t pair0_next_vaddr = hs_page_base(pair0_high_page) + 8U;
    const uintptr_t pair1_prev_vaddr = hs_page_base(pair1_low_page) + PAGE_SIZE - 16U;
    const uintptr_t pair1_high_probe_vaddr = hs_page_base(pair1_high_page);
    const uintptr_t pair1_next_vaddr = hs_page_base(pair1_high_page) + 8U;
    const uintptr_t pair0_prev_phys = phys_page_base(pair0_low_page) + PAGE_SIZE - 16U;
    const uintptr_t pair0_low_phys = ai_micro_page_end_phys(pair0_low_page);
    const uintptr_t pair0_high_phys = phys_page_base(pair0_high_page);
    const uintptr_t pair0_next_phys = phys_page_base(pair0_high_page) + 8U;
    const uintptr_t pair1_prev_phys = phys_page_base(pair1_low_page) + PAGE_SIZE - 16U;
    const uintptr_t pair1_low_phys = ai_micro_page_end_phys(pair1_low_page);
    const uintptr_t pair1_high_phys = phys_page_base(pair1_high_page);
    const uintptr_t pair1_next_phys = phys_page_base(pair1_high_page) + 8U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        uint64_t warmed_pair0_high = 0ULL;
        uint64_t warmed_pair1_high = 0ULL;

        goto_priv(PRIV_M);
        write64(pair0_prev_phys, pair0_prev_pattern[round]);
        write64(pair0_low_phys, pair0_low_pattern[round]);
        write64(pair0_high_phys, pair0_high_pattern[round]);
        write64(pair0_next_phys, pair0_next_pattern[round]);
        write64(pair1_prev_phys, pair1_prev_pattern[round]);
        write64(pair1_low_phys, pair1_low_pattern[round]);
        write64(pair1_high_phys, pair1_high_pattern[round]);
        write64(pair1_next_phys, pair1_next_pattern[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        warmed_pair0_high = ld(pair0_high_probe_vaddr);
        warmed_pair1_high = ld(pair1_high_probe_vaddr);
        TEST_ASSERT("before the reused-upper-half-paddr cross-page split-store probe, both participating upper-half pages are readable with their own seeded payloads",
            excpt.triggered == false &&
            warmed_pair0_high == pair0_high_pattern[round] &&
            warmed_pair1_high == pair1_high_pattern[round]
        );

        TEST_SETUP_EXCEPT();
        if (round == 0U) {
            ai_micro_cross_page_store_pair_once(pair0_vaddr,
                pair0_store_value[round], pair1_vaddr, pair1_store_value[round]);
        } else if (round == 1U) {
            ai_micro_cross_page_store_pair_once(pair1_vaddr,
                pair1_store_value[round], pair0_vaddr, pair0_store_value[round]);
        } else {
            ai_micro_cross_page_store_pair_delayed(pair0_vaddr,
                pair0_store_value[round], pair1_vaddr, pair1_store_value[round]);
        }
        TEST_ASSERT("two consecutive HS cross-page split stores on different page pairs still retire normally without losing one pair's upper-half routing state",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_pair0 = ld(pair0_vaddr);
        TEST_ASSERT("after the consecutive cross-page split-store window, the first page pair still reads back only its own stored payload",
            excpt.triggered == false &&
            observed_pair0 == pair0_store_value[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_pair1 = ld(pair1_vaddr);
        TEST_ASSERT("after the consecutive cross-page split-store window, the second page pair still reads back only its own stored payload",
            excpt.triggered == false &&
            observed_pair1 == pair1_store_value[round]
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_pair0_prev = ld(pair0_prev_vaddr);
            TEST_ASSERT("the outer word before the first cross-page split-store boundary remains unchanged across the reused-upper-half-paddr window",
                excpt.triggered == false &&
                observed_pair0_prev == pair0_prev_pattern[round]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_pair0_next = ld(pair0_next_vaddr);
            TEST_ASSERT("the outer word after the first cross-page split-store boundary remains unchanged across the reused-upper-half-paddr window",
                excpt.triggered == false &&
                observed_pair0_next == pair0_next_pattern[round]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_pair1_prev = ld(pair1_prev_vaddr);
            TEST_ASSERT("the outer word before the second cross-page split-store boundary remains unchanged across the reused-upper-half-paddr window",
                excpt.triggered == false &&
                observed_pair1_prev == pair1_prev_pattern[round]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_pair1_next = ld(pair1_next_vaddr);
            TEST_ASSERT("the outer word after the second cross-page split-store boundary remains unchanged across the reused-upper-half-paddr window",
                excpt.triggered == false &&
                observed_pair1_next == pair1_next_pattern[round]
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("the reused-upper-half-paddr cross-page split-store probe leaves each page pair with only its own split bytes and never cross-binds either upper-half fragment to the other pair",
            read64(pair0_prev_phys) == pair0_prev_pattern[round] &&
            read64(pair0_low_phys) == ai_micro_cross_page_expected_low_after_4b_store(
                pair0_low_pattern[round], pair0_store_value[round]) &&
            read64(pair0_high_phys) == ai_micro_cross_page_expected_high_after_4b_store(
                pair0_high_pattern[round], pair0_store_value[round]) &&
            read64(pair0_next_phys) == pair0_next_pattern[round] &&
            read64(pair1_prev_phys) == pair1_prev_pattern[round] &&
            read64(pair1_low_phys) == ai_micro_cross_page_expected_low_after_4b_store(
                pair1_low_pattern[round], pair1_store_value[round]) &&
            read64(pair1_high_phys) == ai_micro_cross_page_expected_high_after_4b_store(
                pair1_high_pattern[round], pair1_store_value[round]) &&
            read64(pair1_next_phys) == pair1_next_pattern[round]
        );

        goto_priv(PRIV_HS);
    }

    return test_status;
}

static bool ai_micro_vector_store_misalign_direct_and_mabuf_window(bool check_adjacent)
{
    const uintptr_t direct_prev_vaddr = hs_page_base(VSRWX_GRWX) + 0x100U;
    const uintptr_t direct_vaddr = hs_page_base(VSRWX_GRWX) + 0x108U;
    const uintptr_t direct_next_vaddr = hs_page_base(VSRWX_GRWX) + 0x118U;
    const uintptr_t direct_prev_phys = phys_page_base(VSRWX_GRWX) + 0x100U;
    const uintptr_t direct_low_phys = phys_page_base(VSRWX_GRWX) + 0x108U;
    const uintptr_t direct_high_phys = phys_page_base(VSRWX_GRWX) + 0x110U;
    const uintptr_t direct_next_phys = phys_page_base(VSRWX_GRWX) + 0x118U;
    const uintptr_t mabuf_prev_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 16U);
    const uintptr_t mabuf_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 8U);
    const uintptr_t mabuf_next_vaddr = hs_page_base(VSRW_GRX) + 8U;
    const uintptr_t mabuf_prev_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - 16U;
    const uintptr_t mabuf_low_phys = phys_page_base(VSRW_GRW) + PAGE_SIZE - 8U;
    const uintptr_t mabuf_high_phys = phys_page_base(VSRW_GRX);
    const uintptr_t mabuf_next_phys = phys_page_base(VSRW_GRX) + 8U;
    const uintptr_t direct_src_vaddr = hs_page_base(VSRX_GRWX) + 0x200U;
    const uintptr_t direct_src_phys = phys_page_base(VSRX_GRWX) + 0x200U;
    const uintptr_t mabuf_src_vaddr = hs_page_base(VSRX_GRW) + 0x240U;
    const uintptr_t mabuf_src_phys = phys_page_base(VSRX_GRW) + 0x240U;
    const uintptr_t younger_vaddr = hs_page_base(SWITCH1) + 0x200U;
    const uintptr_t younger_phys = phys_page_base(SWITCH1) + 0x200U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        uint8_t direct_prev_bytes[8];
        uint8_t direct_init[16];
        uint8_t direct_src[16];
        uint8_t direct_next_bytes[8];
        uint8_t mabuf_prev_bytes[8];
        uint8_t mabuf_init[16];
        uint8_t mabuf_src[16];
        uint8_t mabuf_next_bytes[8];
        uint8_t younger_init_bytes[8];
        uint8_t younger_update_bytes[8];
        uint64_t warmed_younger;
        uint64_t observed_direct_low;
        uint64_t observed_direct_high;
        uint64_t observed_mabuf_low;
        uint64_t observed_mabuf_high;
        uint64_t observed_younger;

        for (unsigned i = 0; i < 8U; ++i) {
            direct_prev_bytes[i] = (uint8_t)(0x80U + round * 0x10U + i);
            direct_next_bytes[i] = (uint8_t)(0xa0U + round * 0x10U + i);
            mabuf_prev_bytes[i] = (uint8_t)(0xc0U + round * 0x08U + i);
            mabuf_next_bytes[i] = (uint8_t)(0xe0U + round * 0x08U + i);
            younger_init_bytes[i] = (uint8_t)(0x10U + round * 0x10U + i);
            younger_update_bytes[i] = (uint8_t)(0x50U + round * 0x10U + i);
        }
        for (unsigned i = 0; i < 16U; ++i) {
            direct_init[i] = (uint8_t)(0x20U + round * 0x10U + i);
            direct_src[i] = (uint8_t)(0x40U + round * 0x10U + i);
            mabuf_init[i] = (uint8_t)(0x60U + round * 0x10U + i);
            mabuf_src[i] = (uint8_t)(0x90U + round * 0x10U + i);
        }

        goto_priv(PRIV_M);
        ai_micro_write_bytes(direct_prev_phys, direct_prev_bytes, 8U);
        ai_micro_write_bytes(direct_low_phys, direct_init, 16U);
        ai_micro_write_bytes(direct_next_phys, direct_next_bytes, 8U);
        ai_micro_write_bytes(mabuf_prev_phys, mabuf_prev_bytes, 8U);
        ai_micro_write_bytes(mabuf_low_phys, mabuf_init, 8U);
        ai_micro_write_bytes(mabuf_high_phys, mabuf_init + 8U, 8U);
        ai_micro_write_bytes(mabuf_next_phys, mabuf_next_bytes, 8U);
        ai_micro_write_bytes(direct_src_phys, direct_src, 16U);
        ai_micro_write_bytes(mabuf_src_phys, mabuf_src, 16U);
        ai_micro_write_bytes(younger_phys, younger_init_bytes, 8U);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        warmed_younger = ld(younger_vaddr);
        TEST_ASSERT("before the mixed vector misaligned-store completion probe starts, the younger aligned target still reads back its seeded value",
            excpt.triggered == false &&
            warmed_younger == ai_micro_le64(younger_init_bytes)
        );

        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)direct_src_vaddr, 16);
        TEST_ASSERT("a vector unit-stride load can stage the same-page cross-16B payload before the mixed vector misaligned-store completion probe",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)direct_vaddr, 16);
        TEST_ASSERT("the leading same-page vector cross-16B store still retires normally through the local notCross4KB completion path",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)mabuf_src_vaddr, 16);
        TEST_ASSERT("a second vector unit-stride load can stage the cross-page payload before the mixed vector misaligned-store completion probe",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)mabuf_vaddr, 16);
        TEST_ASSERT("the following vector cross-page store still retires normally through the StoreMisalignBuffer-backed split path",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        sd(younger_vaddr, ai_micro_le64(younger_update_bytes));
        TEST_ASSERT("after the mixed vector direct and StoreMisalignBuffer misaligned stores, the younger aligned HS store still retires normally",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        observed_direct_low = ld(direct_vaddr);
        observed_direct_high = ld(direct_vaddr + 8U);
        observed_mabuf_low = ld(mabuf_vaddr);
        observed_mabuf_high = ld(mabuf_vaddr + 8U);
        observed_younger = ld(younger_vaddr);
        TEST_ASSERT("after the mixed vector misaligned-store window, both vector boundary payloads and the younger aligned payload are immediately readable with their own identities intact",
            excpt.triggered == false &&
            observed_direct_low == ai_micro_le64(direct_src) &&
            observed_direct_high == ai_micro_le64(direct_src + 8U) &&
            observed_mabuf_low == ai_micro_le64(mabuf_src) &&
            observed_mabuf_high == ai_micro_le64(mabuf_src + 8U) &&
            observed_younger == ai_micro_le64(younger_update_bytes)
        );

        if (check_adjacent) {
            uint64_t observed_direct_prev;
            uint64_t observed_direct_next;
            uint64_t observed_mabuf_prev;
            uint64_t observed_mabuf_next;

            TEST_SETUP_EXCEPT();
            observed_direct_prev = ld(direct_prev_vaddr);
            observed_direct_next = ld(direct_next_vaddr);
            observed_mabuf_prev = ld(mabuf_prev_vaddr);
            observed_mabuf_next = ld(mabuf_next_vaddr);
            TEST_ASSERT("the mixed vector misaligned-store completion probe leaves every surrounding guard word unchanged",
                excpt.triggered == false &&
                observed_direct_prev == ai_micro_le64(direct_prev_bytes) &&
                observed_direct_next == ai_micro_le64(direct_next_bytes) &&
                observed_mabuf_prev == ai_micro_le64(mabuf_prev_bytes) &&
                observed_mabuf_next == ai_micro_le64(mabuf_next_bytes)
            );
        }

        fence_iorw_iorw();
        goto_priv(PRIV_M);
        TEST_ASSERT("the mixed vector direct-feedback and StoreMisalignBuffer-feedback probe commits each store exactly once, preserves both boundary payloads, and does not corrupt adjacent guards or the younger aligned target",
            ai_micro_bytes_match(direct_prev_phys, direct_prev_bytes, 8U) &&
            ai_micro_bytes_match(direct_low_phys, direct_src, 8U) &&
            ai_micro_bytes_match(direct_high_phys, direct_src + 8U, 8U) &&
            ai_micro_bytes_match(direct_next_phys, direct_next_bytes, 8U) &&
            ai_micro_bytes_match(mabuf_prev_phys, mabuf_prev_bytes, 8U) &&
            ai_micro_bytes_match(mabuf_low_phys, mabuf_src, 8U) &&
            ai_micro_bytes_match(mabuf_high_phys, mabuf_src + 8U, 8U) &&
            ai_micro_bytes_match(mabuf_next_phys, mabuf_next_bytes, 8U) &&
            ai_micro_bytes_match(younger_phys, younger_update_bytes, 8U)
        );

        goto_priv(PRIV_HS);
    }

    return test_status;
}

static bool ai_micro_cross_page_scalar_store_after_vector_load_stable(bool check_adjacent)
{
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0xef, 0xcd, 0xab, 0x89,
        0x67, 0x45, 0x23, 0x01,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint64_t store_value = 0x0123456789abcdefULL;
    const uintptr_t vector_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_page_addr(VSRW_GRW, 4U);
    const uintptr_t low_phys_start = phys_page_base(VSRW_GRW) + PAGE_SIZE - 8U;
    const uintptr_t high_phys_start = phys_page_base(VSRW_GRX);
    const uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x80U;
    const uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x80U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys_start, pattern, 8U);
    ai_micro_write_bytes(high_phys_start, pattern + 8U, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte payload across the same HS page pair before the mixed vector-load then scalar cross-page store stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-page payload into a scratch page preserves the original 16-byte byte sequence before the scalar split-store stable probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load still matches the exact boundary payload before the scalar split-store stable probe",
        ai_micro_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-page load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar split-store adjacent-preservation probe",
            ai_micro_bytes_match(low_phys_start, pattern, 8U) &&
            ai_micro_bytes_match(high_phys_start, pattern + 8U, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, store_value);
    TEST_ASSERT("after the vector cross-page load touches the shared path, an overlapping scalar cross-page store still commits normally and overwrites only its own eight-byte window",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-store cross-page stable sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_bytes_match(high_phys_start, expected_final + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-store cross-page adjacent-preservation sequence, the shared boundary sentinels and scratch sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-load then scalar-store cross-page stable sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == store_value
    );

    return test_status;
}

static bool ai_micro_vector_store_cross_page_partial_fault_window(bool repeat_fault)
{
    const enum test_page low_page = VSRW_GX;
    const enum test_page high_page = VSRX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_addr(low_page, 8U);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x200U;
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_phys = phys_page_base(src_page) + 0x200U;
    const uintptr_t sentinel_before_addr = low_phys - 1U;
    const uintptr_t sentinel_after_addr = high_phys + 8U;
    const uint8_t sentinel_before = 0x69U;
    const uint8_t sentinel_after = 0x96U;
    const uint8_t initial_low[8] = {
        0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe
    };
    const uint8_t initial_high[8] = {
        0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01
    };
    const uint8_t src_pattern0[16] = {
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7
    };
    const uint8_t src_pattern1[16] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7
    };
    const uint8_t *src_pattern = repeat_fault ? src_pattern1 : src_pattern0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, initial_low, 8U);
    ai_micro_write_bytes(high_phys, initial_high, 8U);
    ai_micro_write_bytes(src_phys, src_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)src_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the 16-byte payload before the late-fault vector cross-page store probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector cross-page store whose second half loses write permission reports store page fault after issuing the earlier writable bytes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the late-fault vector cross-page store, only the low-half bytes are committed while the protected high-half bytes remain untouched",
        ai_micro_bytes_match(low_phys, src_pattern, 8U) &&
        ai_micro_bytes_match(high_phys, initial_high, 8U)
    );
    TEST_ASSERT("the late-fault vector cross-page store also preserves the outer sentinels around the two-page window",
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    return test_status;
}

static bool ai_micro_vector_store_late_fault_then_younger_vector_store_owner_bound(
    bool check_adjacent)
{
    const enum test_page old_low_page = VSRW_GX;
    const enum test_page old_high_page = VSRX_GRWX;
    const enum test_page young_low_page = VSRWX_GRWX;
    const enum test_page young_high_page = VSRWX_GRW;
    const enum test_page old_src_page = SWITCH1;
    const enum test_page young_src_page = SCRATCHPAD;
    const uintptr_t old_boundary_vaddr = ai_micro_cross_page_addr(old_low_page, 8U);
    const uintptr_t young_boundary_vaddr = ai_micro_cross_page_addr(young_low_page, 8U);
    const uintptr_t old_scalar_vaddr = ai_micro_cross_page_addr(old_low_page, 4U);
    const uintptr_t young_scalar_vaddr = ai_micro_cross_page_addr(young_low_page, 4U);
    const uintptr_t old_src_vaddr = hs_page_base(old_src_page) + 0x240U;
    const uintptr_t young_src_vaddr = hs_page_base(young_src_page) + 0x280U;
    const uintptr_t old_low_phys = phys_page_base(old_low_page) + PAGE_SIZE - 8U;
    const uintptr_t old_high_phys = phys_page_base(old_high_page);
    const uintptr_t young_low_phys = phys_page_base(young_low_page) + PAGE_SIZE - 8U;
    const uintptr_t young_high_phys = phys_page_base(young_high_page);
    const uintptr_t old_src_phys = phys_page_base(old_src_page) + 0x240U;
    const uintptr_t young_src_phys = phys_page_base(young_src_page) + 0x280U;
    const uintptr_t old_before_addr = old_low_phys - 1U;
    const uintptr_t old_after_addr = old_high_phys + 8U;
    const uintptr_t young_before_addr = young_low_phys - 1U;
    const uintptr_t young_after_addr = young_high_phys + 8U;
    const uint8_t old_before = 0x61U;
    const uint8_t old_after = 0x16U;
    const uint8_t young_before = 0x72U;
    const uint8_t young_after = 0x27U;
    const uint8_t old_initial[16] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    const uint8_t young_initial[16] = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t old_src_pattern[16] = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f
    };
    const uint8_t young_src_pattern[16] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    uint64_t old_scalar_value = 0;
    uint64_t young_scalar_value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(old_low_phys, old_initial, 8U);
    ai_micro_write_bytes(old_high_phys, old_initial + 8U, 8U);
    ai_micro_write_bytes(young_low_phys, young_initial, 8U);
    ai_micro_write_bytes(young_high_phys, young_initial + 8U, 8U);
    ai_micro_write_bytes(old_src_phys, old_src_pattern, 16U);
    ai_micro_write_bytes(young_src_phys, young_src_pattern, 16U);
    write8(old_before_addr, old_before);
    write8(old_after_addr, old_after);
    write8(young_before_addr, young_before);
    write8(young_after_addr, young_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)old_src_vaddr, 16);
    TEST_ASSERT("the older vector-store probe can stage its own 16-byte payload before entering the late-fault cross-page store window",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)old_boundary_vaddr, 16);
    TEST_ASSERT("the older vector cross-page store reports store page fault after issuing only the writable low-half bytes of its own payload",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the older late-fault vector store, only the older low-half bytes become visible while the younger boundary remains untouched before its own store issues",
        ai_micro_bytes_match(old_low_phys, old_src_pattern, 8U) &&
        ai_micro_bytes_match(old_high_phys, old_initial + 8U, 8U) &&
        ai_micro_bytes_match(young_low_phys, young_initial, 8U) &&
        ai_micro_bytes_match(young_high_phys, young_initial + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the older late-fault vector-store episode also preserves both boundary sentinels before the younger vector store begins",
            read8(old_before_addr) == old_before &&
            read8(old_after_addr) == old_after &&
            read8(young_before_addr) == young_before &&
            read8(young_after_addr) == young_after
        );
    }

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)young_src_vaddr, 16);
    TEST_ASSERT("the younger vector-store probe can stage its own 16-byte payload after the older faulting store has already occupied the earlier late-writeback path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)young_boundary_vaddr, 16);
    TEST_ASSERT("while the older vector store is still fault-pending for repair, a younger vector cross-page store on another writable boundary still completes normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    young_scalar_value = ld(young_scalar_vaddr);
    TEST_ASSERT("immediately after the younger vector store completes, a scalar cross-page load over the younger boundary observes only the younger payload",
        excpt.triggered == false &&
        young_scalar_value == 0xcbcac9c8c7c6c5c4ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the older page pair, the younger boundary already holds only the younger payload while the older boundary still shows only its low-half-visible fault footprint",
        ai_micro_bytes_match(old_low_phys, old_src_pattern, 8U) &&
        ai_micro_bytes_match(old_high_phys, old_initial + 8U, 8U) &&
        ai_micro_bytes_match(young_low_phys, young_src_pattern, 8U) &&
        ai_micro_bytes_match(young_high_phys, young_src_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the younger vector store completes under the older late-fault window, both boundary sentinels still remain intact",
            read8(old_before_addr) == old_before &&
            read8(old_after_addr) == old_after &&
            read8(young_before_addr) == young_before &&
            read8(young_after_addr) == young_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(old_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(old_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(old_src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(young_src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)old_src_vaddr, 16);
    TEST_ASSERT("after repairing the older upper page, the older vector-store probe can stage the same payload again for recovery",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)old_boundary_vaddr, 16);
    TEST_ASSERT("after the older page pair is repaired, reissuing the older vector cross-page store completes without exception and must not borrow the younger store's commit identity",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    old_scalar_value = ld(old_scalar_vaddr);
    TEST_ASSERT("after repair, a scalar cross-page load over the older boundary observes only the older vector-store payload",
        excpt.triggered == false &&
        old_scalar_value == 0x8b8a898887868584ULL
    );

    TEST_SETUP_EXCEPT();
    young_scalar_value = ld(young_scalar_vaddr);
    TEST_ASSERT("after the repaired older vector store completes, a scalar cross-page load over the younger boundary still observes only the younger payload",
        excpt.triggered == false &&
        young_scalar_value == 0xcbcac9c8c7c6c5c4ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the full old-repair plus younger-completion sequence, the older and younger boundaries each retain only their own 16-byte payloads with no cross-owner corruption or drop",
        ai_micro_bytes_match(old_low_phys, old_src_pattern, 8U) &&
        ai_micro_bytes_match(old_high_phys, old_src_pattern + 8U, 8U) &&
        ai_micro_bytes_match(young_low_phys, young_src_pattern, 8U) &&
        ai_micro_bytes_match(young_high_phys, young_src_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the repaired older vector store and the younger store both settle, the outer sentinels around both boundaries still remain unchanged",
            read8(old_before_addr) == old_before &&
            read8(old_after_addr) == old_after &&
            read8(young_before_addr) == young_before &&
            read8(young_after_addr) == young_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

#endif /* AI_MICRO_CROSS_PAGE_FAULT_COMMON_H */
