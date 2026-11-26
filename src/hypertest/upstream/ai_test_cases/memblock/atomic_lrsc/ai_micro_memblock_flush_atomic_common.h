#ifndef AI_MICRO_MEMBLOCK_FLUSH_ATOMIC_COMMON_H
#define AI_MICRO_MEMBLOCK_FLUSH_ATOMIC_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_fence_drain_then_amo_no_flush_owner_collision_corner();
bool ai_micro_hs_fence_drain_then_lrsc_no_flush_owner_or_reservation_loss_corner();
bool ai_micro_hs_amo_then_fence_no_stale_atomic_flush_owner_or_marker_drop_corner();
bool ai_micro_hs_lrsc_then_fence_no_stale_reservation_flush_owner_or_marker_drop_corner();
bool ai_micro_hs_load_fault_return_then_amo_borrowed_tlb_no_stale_response_corner();
bool ai_micro_hs_cbo_flush_then_amo_same_block_no_cmo_atomic_flush_owner_collision_corner();
bool ai_micro_hs_dual_amo_storedata_mux_no_cross_target_addend_corner();
bool ai_micro_hs_cboinval_then_amo_no_stale_inval_owner_or_old_value_corner();
bool ai_micro_hs_amo_then_cboinval_no_stale_atomic_flush_owner_or_inval_drop_corner();
bool ai_micro_hs_cboinval_then_lrsc_no_stale_inval_owner_or_reservation_loss_corner();
bool ai_micro_hs_lrsc_then_cboinval_no_stale_reservation_or_inval_drop_corner();
bool ai_micro_hs_fence_then_cboinval_no_stale_fence_flush_owner_or_inval_drop_corner();
bool ai_micro_hs_cbozero_then_amo_no_stale_zero_flush_owner_or_old_value_corner();
bool ai_micro_hs_cbozero_then_vsegment_no_stale_zero_flush_owner_or_payload_corner();
bool ai_micro_hs_cbozero_then_normal_vector_no_stale_zero_flush_owner_or_payload_corner();
bool ai_micro_hs_cbozero_then_cross16_load_no_stale_zero_flush_owner_or_merge_payload_corner();
bool ai_micro_hs_cbom_then_cross16_load_no_stale_cmo_flush_owner_or_merge_payload_corner();
bool ai_micro_hs_cboinval_then_cross16_load_no_stale_inval_owner_or_merge_payload_corner();
bool ai_micro_hs_cbom_then_cross16_store_no_stale_cmo_flush_owner_or_partial_payload_corner();
bool ai_micro_hs_cboinval_then_cross16_store_no_stale_inval_owner_or_partial_payload_corner();
bool ai_micro_hs_cross16_store_then_cboinval_no_stale_split_store_or_inval_owner_corner();
bool ai_micro_hs_cross16_store_then_cbom_no_stale_split_store_or_cmo_owner_corner();
bool ai_micro_hs_cross16_store_then_cbozero_no_stale_split_store_or_zero_owner_corner();
bool ai_micro_hs_cross16_store_then_amo_no_stale_split_store_or_atomic_owner_corner();
bool ai_micro_hs_cross16_store_then_lrsc_no_stale_split_store_or_reservation_owner_corner();
bool ai_micro_hs_amo_then_cross16_load_no_stale_atomic_owner_or_merge_payload_corner();
bool ai_micro_hs_lrsc_then_cross16_load_no_stale_reservation_owner_or_merge_payload_corner();
bool ai_micro_hs_amo_then_cross16_store_no_stale_atomic_owner_or_partial_payload_corner();
bool ai_micro_hs_lrsc_then_cross16_store_no_stale_reservation_owner_or_partial_payload_corner();
bool ai_micro_hs_cross16_store_then_fence_no_stale_split_store_or_sbisempty_corner();
bool ai_micro_hs_cross16_load_then_fence_no_stale_merge_or_sbisempty_corner();
bool ai_micro_hs_fence_then_cross16_store_no_stale_fence_owner_or_partial_payload_corner();
bool ai_micro_hs_fence_then_cross16_load_no_stale_fence_owner_or_merge_payload_corner();
bool ai_micro_hs_cross16_load_then_cbozero_no_stale_merge_or_zero_owner_corner();
bool ai_micro_hs_cross16_load_then_cbom_no_stale_merge_or_cmo_owner_corner();
bool ai_micro_hs_cross16_load_then_cboinval_no_stale_merge_or_inval_owner_corner();
bool ai_micro_hs_cross16_load_then_amo_no_stale_merge_or_atomic_owner_corner();
bool ai_micro_hs_cross16_load_then_lrsc_no_stale_merge_or_reservation_owner_corner();
bool ai_micro_hs_cbozero_then_cross16_store_no_stale_zero_flush_owner_or_partial_payload_corner();
bool ai_micro_hs_cbozero_then_lrsc_no_stale_zero_flush_owner_or_reservation_loss_corner();
bool ai_micro_hs_amo_then_cbozero_no_stale_atomic_flush_owner_or_zero_drop_corner();
bool ai_micro_hs_lrsc_then_cbozero_no_stale_reservation_or_zero_drop_corner();
bool ai_micro_hs_cbom_then_lrsc_same_block_no_stale_cmo_flush_owner_or_sc_loss_corner();
bool ai_micro_hs_lrsc_then_cbom_same_block_no_stale_atomic_flush_owner_or_clean_drop_corner();
bool ai_micro_hs_amo_then_cbom_same_block_no_stale_atomic_flush_owner_or_clean_drop_corner();
bool ai_micro_hs_cbom_then_normal_vector_no_stale_cmo_flush_owner_or_payload_corner();
bool ai_micro_hs_normal_vector_then_cbom_no_stale_vector_store_or_cmo_owner_corner();
bool ai_micro_hs_normal_vector_store_then_fence_no_stale_vector_writeback_or_sbisempty_corner();
bool ai_micro_hs_fence_then_normal_vector_store_no_stale_fence_owner_or_vector_payload_corner();
bool ai_micro_hs_amo_then_normal_vector_store_no_stale_atomic_owner_or_vector_payload_corner();
bool ai_micro_hs_lrsc_then_normal_vector_store_no_stale_reservation_owner_or_vector_payload_corner();
bool ai_micro_hs_normal_vector_store_then_amo_no_stale_vector_writeback_or_atomic_old_value_corner();
bool ai_micro_hs_normal_vector_store_then_lrsc_no_stale_vector_writeback_or_reservation_loss_corner();
bool ai_micro_hs_cbom_then_vsegment_no_stale_cmo_flush_owner_or_segment_payload_corner();
bool ai_micro_hs_vsegment_then_cbom_no_stale_segment_store_or_cmo_owner_corner();
bool ai_micro_hs_cbom_then_fence_no_stale_cmo_flush_owner_or_marker_drop_corner();
bool ai_micro_hs_fence_then_cbom_no_stale_fence_flush_owner_or_cmo_drop_corner();
bool ai_micro_hs_fence_then_cbozero_no_stale_fence_flush_owner_or_zero_drop_corner();
bool ai_micro_hs_cbom_then_cbozero_no_stale_cmo_owner_or_zero_drop_corner();
bool ai_micro_hs_cbozero_then_cbom_no_stale_zero_owner_or_cmo_marker_drop_corner();
bool ai_micro_hs_vsegment_store_then_cbozero_no_stale_segment_flush_owner_or_zero_drop_corner();
bool ai_micro_hs_normal_vector_store_then_cbozero_no_stale_vector_writeback_or_zero_drop_corner();
bool ai_micro_hs_cbozero_then_fence_no_stale_zero_flush_owner_or_marker_drop_corner();
bool ai_micro_hs_wrong_path_amo_redirect_no_stale_atomic_state_or_spurious_fault_corner();

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_FLUSH_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS 8U
#define AI_MEMBLOCK_CMO_VSEG_VL 8U
#define AI_MEMBLOCK_CMO_VECTOR_BYTES 16U

static inline void ai_memblock_cmo_amo_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_amo_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_inval_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_inval_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_amo_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cmo_amo_block_matches_seed(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        const uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_amo_block_matches_two_updates(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word,
    uint64_t first_value,
    size_t second_word,
    uint64_t second_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == first_word) {
            expected = first_value;
        } else if (i == second_word) {
            expected = second_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_amo_block_matches_three_updates(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word,
    uint64_t first_value,
    size_t second_word,
    uint64_t second_value,
    size_t third_word,
    uint64_t third_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == first_word) {
            expected = first_value;
        } else if (i == second_word) {
            expected = second_value;
        } else if (i == third_word) {
            expected = third_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_amo_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_amo_zero_block_matches_two_updates(
    uintptr_t paddr,
    size_t first_word,
    uint64_t first_value,
    size_t second_word,
    uint64_t second_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == first_word) {
            expected = first_value;
        } else if (i == second_word) {
            expected = second_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline pte_t *ai_memblock_atomic_tlb_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_atomic_tlb_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_atomic_tlb_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_cmo_cboinval_amo_then_marker(
    uintptr_t inval_vaddr,
    uintptr_t amo_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uintptr_t neighbor_vaddr,
    uint64_t neighbor_update,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[neighbor_update], 0(%[neighbor_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old)
        : [inval_addr] "r"(inval_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_update] "r"(neighbor_update),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
}

static inline void ai_memblock_cmo_amo_cboinval_then_marker(
    uintptr_t amo_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uintptr_t inval_vaddr,
    uintptr_t neighbor_vaddr,
    uint64_t neighbor_update,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old)
{
    uint64_t first_old = 0ULL;
    uint64_t second_old = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[neighbor_update], 0(%[neighbor_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old)
        : [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [inval_addr] "r"(inval_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_update] "r"(neighbor_update),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
}

static inline void ai_memblock_cmo_cboinval_lrsc_then_marker(
    uintptr_t inval_vaddr,
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t neighbor_vaddr,
    uint64_t neighbor_update,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t old_value = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "lr.d %[old_value], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "sd %[neighbor_update], 0(%[neighbor_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status] "=&r"(status)
        : [inval_addr] "r"(inval_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_payload] "r"(sc_payload),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_update] "r"(neighbor_update),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *lr_observed = old_value;
    *sc_status = status;
}

static inline void ai_memblock_cmo_lrsc_cboinval_then_marker(
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t inval_vaddr,
    uintptr_t neighbor_vaddr,
    uint64_t neighbor_update,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t old_value = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[old_value], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_payload], 0(%[lrsc_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[neighbor_update], 0(%[neighbor_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status] "=&r"(status)
        : [lrsc_addr] "r"(lrsc_vaddr),
          [sc_payload] "r"(sc_payload),
          [inval_addr] "r"(inval_vaddr),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_update] "r"(neighbor_update),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *lr_observed = old_value;
    *sc_status = status;
}

static inline void ai_memblock_cmo_vsegment_copy_then_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src_addr])\n\t"
        "vsseg2e8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CMO_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_cmo_vsegment_copy_only(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src_addr])\n\t"
        "vsseg2e8.v v8, (%[dst_addr])\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [vl] "r"((int)AI_MEMBLOCK_CMO_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_cmo_normal_vector_copy_then_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_cmo_normal_vector_copy_only(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [vl] "r"((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r"(vtype)
	    : "t0", "memory");
}

static inline void ai_memblock_cmo_normal_vector_store_fence_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "fence rw, rw\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[reloaded], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [reloaded] "=&r" (reloaded)
        : [src_addr] "r" (src_vaddr),
          [dst_addr] "r" (dst_vaddr),
          [marker_addr] "r" (marker_vaddr),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *marker_reloaded = reloaded;
}

static inline void ai_memblock_cmo_fence_then_normal_vector_store_marker(
    uintptr_t pressure_vaddr,
    uint64_t pressure_update,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[pressure_update], 0(%[pressure_addr])\n\t"
        "fence rw, rw\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[reloaded], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [reloaded] "=&r" (reloaded)
        : [pressure_addr] "r" (pressure_vaddr),
          [pressure_update] "r" (pressure_update),
          [src_addr] "r" (src_vaddr),
          [dst_addr] "r" (dst_vaddr),
          [marker_addr] "r" (marker_vaddr),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *marker_reloaded = reloaded;
}

static inline void ai_memblock_cmo_amo_then_normal_vector_store_marker(
    uintptr_t amo_vaddr,
    uint64_t amo_addend,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *amo_old,
    uint64_t *amo_observed,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t observed = 0ULL;
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[observed], 0(%[amo_addr])\n\t"
        "ld %[reloaded], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [observed] "=&r" (observed),
          [reloaded] "=&r" (reloaded)
        : [amo_addr] "r" (amo_vaddr),
          [amo_addend] "r" (amo_addend),
          [src_addr] "r" (src_vaddr),
          [dst_addr] "r" (dst_vaddr),
          [marker_addr] "r" (marker_vaddr),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *amo_old = old_value;
    *amo_observed = observed;
    *marker_reloaded = reloaded;
}

static inline void ai_memblock_cmo_lrsc_then_normal_vector_store_marker(
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *lrsc_reloaded,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t status = 1ULL;
    uint64_t reloaded_target = 0ULL;
    uint64_t reloaded_marker = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[old_value], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[reloaded_target], 0(%[lrsc_addr])\n\t"
        "ld %[reloaded_marker], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [status] "=&r" (status),
          [reloaded_target] "=&r" (reloaded_target),
          [reloaded_marker] "=&r" (reloaded_marker)
        : [lrsc_addr] "r" (lrsc_vaddr),
          [sc_payload] "r" (sc_payload),
          [src_addr] "r" (src_vaddr),
          [dst_addr] "r" (dst_vaddr),
          [marker_addr] "r" (marker_vaddr),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *lr_observed = old_value;
    *sc_status = status;
    *lrsc_reloaded = reloaded_target;
    *marker_reloaded = reloaded_marker;
}

static inline void ai_memblock_cmo_normal_vector_store_then_amo_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t amo_vaddr,
    uint64_t amo_addend,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *amo_old,
    uint64_t *amo_observed,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t observed = 0ULL;
    uint64_t reloaded = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "amoadd.d %[old_value], %[amo_addend], (%[amo_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[observed], 0(%[amo_addr])\n\t"
        "ld %[reloaded], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [observed] "=&r" (observed),
          [reloaded] "=&r" (reloaded)
        : [src_addr] "r" (src_vaddr),
          [dst_addr] "r" (dst_vaddr),
          [amo_addr] "r" (amo_vaddr),
          [amo_addend] "r" (amo_addend),
          [marker_addr] "r" (marker_vaddr),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *amo_old = old_value;
    *amo_observed = observed;
    *marker_reloaded = reloaded;
}

static inline void ai_memblock_cmo_normal_vector_store_then_lrsc_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *lrsc_reloaded,
    uint64_t *marker_reloaded)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t old_value = 0ULL;
    uint64_t status = 1ULL;
    uint64_t reloaded_target = 0ULL;
    uint64_t reloaded_marker = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "lr.d %[old_value], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[reloaded_target], 0(%[lrsc_addr])\n\t"
        "ld %[reloaded_marker], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r" (old_value),
          [status] "=&r" (status),
          [reloaded_target] "=&r" (reloaded_target),
          [reloaded_marker] "=&r" (reloaded_marker)
        : [src_addr] "r" (src_vaddr),
          [dst_addr] "r" (dst_vaddr),
          [lrsc_addr] "r" (lrsc_vaddr),
          [sc_payload] "r" (sc_payload),
          [marker_addr] "r" (marker_vaddr),
          [marker_update] "r" (marker_update),
          [vl] "r" ((int)AI_MEMBLOCK_CMO_VECTOR_BYTES),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *lr_observed = old_value;
    *sc_status = status;
    *lrsc_reloaded = reloaded_target;
    *marker_reloaded = reloaded_marker;
}

static inline uint64_t ai_memblock_cmo_cross16_load_then_marker(
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    return observed;
}

static inline uint64_t ai_memblock_cmo_cross16_load_fence_then_marker(
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        "fence rw, rw\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    return observed;
}

static inline void ai_memblock_cmo_fence_cross16_load_then_marker(
    uintptr_t head_store_vaddr,
    uint64_t head_payload,
    uintptr_t word1_vaddr,
    uint64_t word1_payload,
    uintptr_t word2_vaddr,
    uint64_t word2_payload,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *load_observed)
{
    uint64_t observed_load = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[head_payload], 0(%[head_addr])\n\t"
        "sd %[word1_payload], 0(%[word1_addr])\n\t"
        "sd %[word2_payload], 0(%[word2_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[load_out], 0(%[load_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [load_out] "=&r"(observed_load)
        : [head_addr] "r"(head_store_vaddr),
          [head_payload] "r"(head_payload),
          [word1_addr] "r"(word1_vaddr),
          [word1_payload] "r"(word1_payload),
          [word2_addr] "r"(word2_vaddr),
          [word2_payload] "r"(word2_payload),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *load_observed = observed_load;
}

static inline uint64_t ai_memblock_cmo_cross16_load_expected(
    uint64_t low_word,
    uint64_t high_word)
{
    return (low_word >> 56) | ((high_word & 0x00ffffffffffffffULL) << 8);
}

static inline uint64_t ai_memblock_cmo_cross16_load_cbozero_then_marker(
    uintptr_t load_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [load_addr] "r"(load_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    return observed;
}

static inline uint64_t ai_memblock_cmo_cross16_load_cbom_then_marker(
    uintptr_t load_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [load_addr] "r"(load_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    return observed;
}

static inline uint64_t ai_memblock_cmo_cboinval_cross16_load_then_marker(
    uintptr_t inval_vaddr,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [inval_addr] "r"(inval_vaddr),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    return observed;
}

static inline uint64_t ai_memblock_cmo_cross16_load_cboinval_then_marker(
    uintptr_t load_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    uint64_t observed = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[observed], 0(%[load_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [load_addr] "r"(load_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    return observed;
}

static inline void ai_memblock_cmo_cross16_load_amo_then_marker(
    uintptr_t load_vaddr,
    uintptr_t amo_vaddr,
    uint64_t amo_addend,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *load_observed,
    uint64_t *amo_old_observed)
{
    uint64_t observed_load = 0ULL;
    uint64_t observed_amo_old = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[load_out], 0(%[load_addr])\n\t"
        "amoadd.d %[amo_old_out], %[amo_addend], (%[amo_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [load_out] "=&r"(observed_load),
          [amo_old_out] "=&r"(observed_amo_old)
        : [load_addr] "r"(load_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [amo_addend] "r"(amo_addend),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *load_observed = observed_load;
    *amo_old_observed = observed_amo_old;
}

static inline void ai_memblock_cmo_amo_cross16_load_then_marker(
    uintptr_t amo_vaddr,
    uint64_t amo_addend,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *amo_old_observed,
    uint64_t *load_observed)
{
    uint64_t observed_amo_old = 0ULL;
    uint64_t observed_load = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[amo_old_out], %[amo_addend], (%[amo_addr])\n\t"
        "ld %[load_out], 0(%[load_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [amo_old_out] "=&r"(observed_amo_old),
          [load_out] "=&r"(observed_load)
        : [amo_addr] "r"(amo_vaddr),
          [amo_addend] "r"(amo_addend),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *amo_old_observed = observed_amo_old;
    *load_observed = observed_load;
}

static inline void ai_memblock_cmo_lrsc_cross16_load_then_marker(
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *load_observed)
{
    uint64_t observed_lr = 0ULL;
    uint64_t status = 1ULL;
    uint64_t observed_load = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[lr_out], 0(%[lrsc_addr])\n\t"
        "sc.d %[status_out], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "ld %[load_out], 0(%[load_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [lr_out] "=&r"(observed_lr),
          [status_out] "=&r"(status),
          [load_out] "=&r"(observed_load)
        : [lrsc_addr] "r"(lrsc_vaddr),
          [sc_payload] "r"(sc_payload),
          [load_addr] "r"(load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *lr_observed = observed_lr;
    *sc_status = status;
    *load_observed = observed_load;
}

static inline void ai_memblock_cmo_cross16_load_lrsc_then_marker(
    uintptr_t load_vaddr,
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *load_observed,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t observed_load = 0ULL;
    uint64_t observed_lr = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[load_out], 0(%[load_addr])\n\t"
        "lr.d %[lr_out], 0(%[lrsc_addr])\n\t"
        "sc.d %[status_out], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [load_out] "=&r"(observed_load),
          [lr_out] "=&r"(observed_lr),
          [status_out] "=&r"(status)
        : [load_addr] "r"(load_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_payload] "r"(sc_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *load_observed = observed_load;
    *lr_observed = observed_lr;
    *sc_status = status;
}

static inline uint64_t ai_memblock_cmo_cross16_store_low_word(
    uint64_t payload)
{
    return (payload & 0xffULL) << 56;
}

static inline uint64_t ai_memblock_cmo_cross16_store_high_word(
    uint64_t payload)
{
    return payload >> 8;
}

static inline uint64_t ai_memblock_cmo_cross16_store_low_word_from_seed(
    uint64_t old_word,
    uint64_t payload)
{
    return (old_word & 0x00ffffffffffffffULL) |
        ((payload & 0xffULL) << 56);
}

static inline uint64_t ai_memblock_cmo_cross16_store_high_word_from_seed(
    uint64_t old_word,
    uint64_t payload)
{
    return (old_word & 0xff00000000000000ULL) | (payload >> 8);
}

static inline bool ai_memblock_cmo_zero_block_matches_cross16_store(
    uintptr_t paddr,
    uint64_t payload)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == 1U) {
            expected = ai_memblock_cmo_cross16_store_low_word(payload);
        } else if (i == 2U) {
            expected = ai_memblock_cmo_cross16_store_high_word(payload);
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_block_matches_head_and_cross16_store(
    uintptr_t paddr,
    uint64_t seed,
    uint64_t head_value,
    uint64_t cross16_payload)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == 0U) {
            expected = head_value;
        } else if (i == 1U) {
            expected =
                ai_memblock_cmo_cross16_store_low_word_from_seed(expected,
                    cross16_payload);
        } else if (i == 2U) {
            expected =
                ai_memblock_cmo_cross16_store_high_word_from_seed(expected,
                    cross16_payload);
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_block_matches_cross16_store(
    uintptr_t paddr,
    uint64_t seed,
    uint64_t cross16_payload)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == 1U) {
            expected =
                ai_memblock_cmo_cross16_store_low_word_from_seed(expected,
                    cross16_payload);
        } else if (i == 2U) {
            expected =
                ai_memblock_cmo_cross16_store_high_word_from_seed(expected,
                    cross16_payload);
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cmo_cross16_store_then_marker(
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");
}

static inline void ai_memblock_cmo_cboinval_cross16_store_then_marker(
    uintptr_t inval_vaddr,
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");
}

static inline void ai_memblock_cmo_cross16_store_cboinval_then_marker(
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t inval_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");
}

static inline void ai_memblock_cmo_cross16_store_fence_then_marker(
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "fence rw, rw\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");
}

static inline void ai_memblock_cmo_fence_cross16_store_then_marker(
    uintptr_t head_store_vaddr,
    uint64_t head_payload,
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[head_payload], 0(%[head_addr])\n\t"
        "fence rw, rw\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [head_addr] "r"(head_store_vaddr),
          [head_payload] "r"(head_payload),
          [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");
}

static inline void ai_memblock_cmo_cross16_store_only(
    uintptr_t store_vaddr,
    uint64_t store_payload)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        ".option pop\n\t"
        :
        : [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload)
        : "memory");
}

static inline void ai_memblock_cmo_amo_cross16_store_then_marker(
    uintptr_t amo_vaddr,
    uint64_t amo_addend,
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *amo_old_observed)
{
    uint64_t observed_amo_old = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[amo_old_out], %[amo_addend], (%[amo_addr])\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [amo_old_out] "=&r"(observed_amo_old)
        : [amo_addr] "r"(amo_vaddr),
          [amo_addend] "r"(amo_addend),
          [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *amo_old_observed = observed_amo_old;
}

static inline void ai_memblock_cmo_cross16_store_amo_then_marker(
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t amo_vaddr,
    uint64_t amo_addend,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *amo_old_observed)
{
    uint64_t observed_amo_old = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "amoadd.d %[amo_old_out], %[amo_addend], (%[amo_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [amo_old_out] "=&r"(observed_amo_old)
        : [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [amo_addr] "r"(amo_vaddr),
          [amo_addend] "r"(amo_addend),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *amo_old_observed = observed_amo_old;
}

static inline void ai_memblock_cmo_cross16_store_lrsc_then_marker(
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t observed_lr = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "lr.d %[lr_out], 0(%[lrsc_addr])\n\t"
        "sc.d %[status_out], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [lr_out] "=&r"(observed_lr),
          [status_out] "=&r"(status)
        : [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_payload] "r"(sc_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *lr_observed = observed_lr;
    *sc_status = status;
}

static inline void ai_memblock_cmo_lrsc_cross16_store_then_marker(
    uintptr_t lrsc_vaddr,
    uint64_t sc_payload,
    uintptr_t store_vaddr,
    uint64_t store_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t observed_lr = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[lr_out], 0(%[lrsc_addr])\n\t"
        "sc.d %[status_out], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "sd %[store_payload], 0(%[store_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [lr_out] "=&r"(observed_lr),
          [status_out] "=&r"(status)
        : [lrsc_addr] "r"(lrsc_vaddr),
          [sc_payload] "r"(sc_payload),
          [store_addr] "r"(store_vaddr),
          [store_payload] "r"(store_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *lr_observed = observed_lr;
    *sc_status = status;
}

static inline void ai_memblock_cmo_lrsc_then_marker(
    uintptr_t target_vaddr,
    uint64_t sc_payload,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t old_value = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[old_value], 0(%[target_addr])\n\t"
        "sc.d %[status], %[sc_payload], 0(%[target_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status] "=&r"(status)
        : [target_addr] "r"(target_vaddr),
          [sc_payload] "r"(sc_payload),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");

    *lr_observed = old_value;
    *sc_status = status;
}

static inline void ai_memblock_cmo_lrsc_only(
    uintptr_t target_vaddr,
    uint64_t sc_payload,
    uint64_t *lr_observed,
    uint64_t *sc_status)
{
    uint64_t old_value = 0ULL;
    uint64_t status = 1ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[old_value], 0(%[target_addr])\n\t"
        "sc.d %[status], %[sc_payload], 0(%[target_addr])\n\t"
        ".option pop\n\t"
        : [old_value] "=&r"(old_value),
          [status] "=&r"(status)
        : [target_addr] "r"(target_vaddr),
          [sc_payload] "r"(sc_payload)
        : "memory");

    *lr_observed = old_value;
    *sc_status = status;
}

#define AI_MEMBLOCK_ATOMIC_REDIRECT_TRAIN_ROUNDS 19U
#define AI_MEMBLOCK_ATOMIC_REDIRECT_PROBE_ROUNDS 5U

static __attribute__((noinline)) void
ai_memblock_atomic_wrong_path_branch_window(
    uintptr_t control_vaddr,
    uintptr_t target_vaddr,
    uintptr_t marker_vaddr,
    uint64_t amo_addend,
    uint64_t marker_value,
    uintptr_t rounds)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 1\n\t"
        "ld t1, 0(%[control_addr])\n\t"
        "beqz t1, 2f\n\t"
        "amoadd.d x0, %[amo_addend], (%[target_addr])\n\t"
        "2:\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [control_addr] "r"(control_vaddr),
          [target_addr] "r"(target_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [amo_addend] "r"(amo_addend),
          [marker_value] "r"(marker_value),
          [rounds] "r"(rounds)
        : "t0", "t1", "memory");
}

#endif /* AI_MICRO_MEMBLOCK_FLUSH_ATOMIC_COMMON_H */
