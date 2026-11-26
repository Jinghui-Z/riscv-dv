#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>
#include <string.h>

#define AI_AP12_VL_E32 8U
#define AI_AP12_BYTES (AI_AP12_VL_E32 * sizeof(uint32_t))
#define AI_AP12_INDEX_VL 4U
#define AI_AP12_ROUNDS 3U
#define AI_AP12_VS_OFF 9U
#define AI_AP12_VS_MASK (3ULL << AI_AP12_VS_OFF)
#define AI_AP12_VS_DIRTY (3ULL << AI_AP12_VS_OFF)

extern bool ai_micro_hs_load_vpmatchfail_and_ldld_same_cycle_rollback_priority_correct_corner(void);
extern bool ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner(void);
extern bool ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner(void);
extern bool ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner(void);
extern bool ai_micro_hs_same_rob_row_dual_uncache_load_replay_no_popcount_assert_corner(void);
extern bool ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner(void);
extern bool ai_micro_hs_vsplit_masked_unitstride_load_all_inactive_no_fault_or_dest_clobber_corner(void);
extern bool ai_micro_ap122_non_aligned_old_load_rar_query_manual_smoke_corner(void);
extern bool ai_micro_ap122_mab_split_alias_reuse_no_paddr_pollution_manual_smoke_corner(void);
extern bool ai_micro_ap125_prefetch_then_same_line_demand_load_rar_ignore_manual_smoke_corner(void);
extern bool ai_micro_ap125_prefetch_repeated_faulting_alias_ignore_lifetime_reuse_manual_smoke_corner(void);

static inline pte_t *ai_ap12_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap12_map_alias(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_ap12_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_ap12_set_page_perm(enum test_page page, uint64_t perm)
{
    *ai_ap12_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | PTE_V | PTE_AD | perm;
}

static inline void ai_ap12_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, AI_AP12_VS_MASK);
    CSRS(CSR_MSTATUS, AI_AP12_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_ap12_write_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_ap12_bytes_match(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_ap12_fill_pattern(uint8_t *bytes, unsigned len,
    uint8_t base, uint8_t step)
{
    for (unsigned i = 0; i < len; ++i) {
        bytes[i] = (uint8_t)(base + (uint8_t)(i * step));
    }
}

static inline uint64_t ai_ap12_le64(const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (i * 8U);
    }
    return value;
}

static inline bool ai_ap12_words_match(uintptr_t paddr,
    const uint32_t *expected, unsigned words)
{
    for (unsigned i = 0; i < words; ++i) {
        if (read32(paddr + i * sizeof(uint32_t)) != expected[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_ap12_unitstride_load_dump(uintptr_t src_vaddr,
    uintptr_t dump_vaddr, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, ta, ma\n\t"
        "vle32.v v8, (%[src])\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_ap12_indexed_load_dump(uintptr_t base_vaddr,
    uintptr_t offsets_vaddr, uintptr_t dump_vaddr, unsigned ordered)
{
    const char *unused = ordered ? "ordered" : "unordered";
    (void)unused;

    if (ordered) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
            "vle64.v v16, (%[offsets])\n\t"
            "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
            "vloxei64.v v8, (%[base]), v16\n\t"
            "vse32.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base_vaddr),
              [offsets] "r"(offsets_vaddr),
              [dump] "r"(dump_vaddr),
              [vl] "r"((unsigned)AI_AP12_INDEX_VL)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
            "vle64.v v16, (%[offsets])\n\t"
            "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
            "vluxei64.v v8, (%[base]), v16\n\t"
            "vse32.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            :
            : [base] "r"(base_vaddr),
              [offsets] "r"(offsets_vaddr),
              [dump] "r"(dump_vaddr),
              [vl] "r"((unsigned)AI_AP12_INDEX_VL)
            : "t0", "memory");
    }
}

static inline void ai_ap12_masked_indexed_load_dump(uintptr_t base_vaddr,
    uintptr_t offsets_vaddr, uintptr_t old_vaddr, uintptr_t dump_vaddr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "vle64.v v16, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v8, (%[old])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vluxei64.v v8, (%[base]), v16, v0.t\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base_vaddr),
          [offsets] "r"(offsets_vaddr),
          [old] "r"(old_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"((unsigned)AI_AP12_INDEX_VL)
        : "t0", "memory");
}

static inline void ai_ap12_vector_scalar_sequence(uintptr_t src_vaddr,
    uintptr_t dump_vaddr, uintptr_t scalar_vaddr, uintptr_t marker_vaddr,
    uint64_t marker_value, uint64_t *scalar_out, uint64_t *marker_out)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, 0xfedcba9876543210\n\t"
        "li t2, 7\n\t"
        "divu t3, t1, t2\n\t"
        "addi t3, t3, 1\n\t"
        "divu t3, t1, t3\n\t"
        "andi t3, t3, 0\n\t"
        "add t3, t3, %[src]\n\t"
        "vsetvli t0, %[vl], e32, m2, ta, ma\n\t"
        "vle32.v v8, (t3)\n\t"
        "ld t4, 0(%[scalar])\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        "ld t5, 0(%[marker])\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "sd t4, 0(%[scalar_out])\n\t"
        "sd t5, 0(%[marker_out])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [scalar] "r"(scalar_vaddr),
          [marker] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [scalar_out] "r"(scalar_out),
          [marker_out] "r"(marker_out),
          [vl] "r"((unsigned)AI_AP12_VL_E32)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

bool ai_micro_ap12_unitstride_same_rob_multiflow_refresh_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t line_off = 0x400U;
    const uintptr_t src_vaddr = hs_page_base(alias_page) + line_off;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x400U;
    const uintptr_t base_paddr = phys_page_base(base_page) + line_off;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x400U;
    const uintptr_t guard_before = dump_paddr - 1U;
    const uintptr_t guard_after = dump_paddr + AI_AP12_BYTES;
    const uint8_t guard_before_value = 0x12U;
    const uint8_t guard_after_value = 0x21U;
    uint8_t src_bytes[AI_AP12_ROUNDS][AI_AP12_BYTES];
    uint8_t dump_seed[AI_AP12_BYTES];
    uint64_t marker_value = 0ULL;

    for (unsigned round = 0; round < AI_AP12_ROUNDS; ++round) {
        ai_ap12_fill_pattern(src_bytes[round], AI_AP12_BYTES,
            (uint8_t)(0x20U + round * 0x30U), (uint8_t)(3U + round));
    }
    ai_ap12_fill_pattern(dump_seed, AI_AP12_BYTES, 0xe0U, 1U);

    ai_ap12_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    ai_ap12_map_alias(alias_page, base_page);
    sfence_vma();

    for (unsigned round = 0; round < AI_AP12_ROUNDS; ++round) {
        goto_priv(PRIV_M);
        ai_ap12_write_bytes(base_paddr, src_bytes[round], AI_AP12_BYTES);
        ai_ap12_write_bytes(dump_paddr, dump_seed, AI_AP12_BYTES);
        write8(guard_before, guard_before_value);
        write8(guard_after, guard_after_value);
        marker_value ^= read64(base_paddr + ((round + 1U) * sizeof(uint64_t)));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_ap12_unitstride_load_dump(src_vaddr, dump_vaddr, AI_AP12_VL_E32);
        TEST_ASSERT("AP-12 unit-stride same-ROB multi-flow vector load retires without visible false RAR rollback or exception",
            excpt.triggered == false &&
            CSRR(CSR_VL) == AI_AP12_VL_E32,
            "round=%u vl=%lu", round, CSRR(CSR_VL)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-12 unit-stride same-ROB multi-flow vector load dumps only the current round payload and does not reuse stale released flow state",
            ai_ap12_bytes_match(dump_paddr, src_bytes[round],
                AI_AP12_BYTES) &&
            read8(guard_before) == guard_before_value &&
            read8(guard_after) == guard_after_value,
            "round=%u marker=0x%lx", round, marker_value
        );
    }

    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap12_indexed_alias_ordering_mask_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t base_off = 0x500U;
    const uintptr_t base_vaddr = hs_page_base(alias_page) + base_off;
    const uintptr_t offsets_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t old_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t unordered_dump_vaddr = hs_page_base(aux_page) + 0x200U;
    const uintptr_t ordered_dump_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t masked_dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t base_paddr = phys_page_base(base_page) + base_off;
    const uintptr_t offsets_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t old_paddr = phys_page_base(aux_page) + 0x180U;
    const uintptr_t unordered_dump_paddr = phys_page_base(aux_page) + 0x200U;
    const uintptr_t ordered_dump_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t masked_dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uint32_t lane_values[AI_AP12_INDEX_VL] = {
        0x12121212U, 0x34343434U, 0x56565656U, 0x78787878U
    };
    const uint32_t old_values[AI_AP12_INDEX_VL] = {
        0x9a9a9a9aU, 0xbcbcbcbcU, 0xdedededeU, 0xf0f0f0f0U
    };
    const uint64_t offsets[AI_AP12_INDEX_VL] = { 12ULL, 0ULL, 8ULL, 4ULL };
    const uint32_t expected[AI_AP12_INDEX_VL] = {
        lane_values[3], lane_values[0], lane_values[2], lane_values[1]
    };
    const uint32_t masked_expected[AI_AP12_INDEX_VL] = {
        lane_values[3], old_values[1], lane_values[2], old_values[3]
    };

    ai_ap12_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    ai_ap12_map_alias(alias_page, base_page);
    for (unsigned i = 0; i < AI_AP12_INDEX_VL; ++i) {
        write32(base_paddr + i * sizeof(uint32_t), lane_values[i]);
        write64(offsets_paddr + i * sizeof(uint64_t), offsets[i]);
        write32(old_paddr + i * sizeof(uint32_t), old_values[i]);
        write32(unordered_dump_paddr + i * sizeof(uint32_t), 0x11111111U);
        write32(ordered_dump_paddr + i * sizeof(uint32_t), 0x22222222U);
        write32(masked_dump_paddr + i * sizeof(uint32_t), 0x33333333U);
    }
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap12_indexed_load_dump(base_vaddr, offsets_vaddr,
        unordered_dump_vaddr, 0U);
    TEST_ASSERT("AP-12 unordered indexed load same-ROB alias elements retire without false RAR and return the lane image in index order",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP12_INDEX_VL
    );

    TEST_SETUP_EXCEPT();
    ai_ap12_indexed_load_dump(base_vaddr, offsets_vaddr,
        ordered_dump_vaddr, 1U);
    TEST_ASSERT("AP-12 ordered indexed load contrast also returns the same lane image without over-suppressing ordered semantics",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP12_INDEX_VL
    );

    TEST_SETUP_EXCEPT();
    ai_ap12_masked_indexed_load_dump(base_vaddr, offsets_vaddr, old_vaddr,
        masked_dump_vaddr);
    TEST_ASSERT("AP-12 masked indexed load suppresses inactive alias lanes from participating in the same-ROB RAR smoke window",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP12_INDEX_VL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-12 unordered and ordered indexed alias dumps both match expected active lane payload",
        ai_ap12_words_match(unordered_dump_paddr, expected,
            AI_AP12_INDEX_VL) &&
        ai_ap12_words_match(ordered_dump_paddr, expected,
            AI_AP12_INDEX_VL)
    );
    TEST_ASSERT("AP-12 masked indexed alias dump updates only active even lanes and leaves inactive odd lanes at the old destination image",
        ai_ap12_words_match(masked_dump_paddr, masked_expected,
            AI_AP12_INDEX_VL)
    );

    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap12_vector_scalar_true_younger_rar_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t base_off = 0x600U;
    const uintptr_t src_vaddr = hs_page_base(alias_page) + base_off;
    const uintptr_t scalar_vaddr = hs_page_base(alias_page) + base_off + 8U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t base_paddr = phys_page_base(base_page) + base_off;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x380U;
    const uint64_t marker_value = 0x12121212a012ULL;
    uint8_t payload[AI_AP12_BYTES];
    uint8_t dump_seed[AI_AP12_BYTES];
    uint64_t expected_scalar = 0ULL;
    uint64_t scalar_observed = 0ULL;
    uint64_t marker_observed = 0ULL;

    ai_ap12_fill_pattern(payload, sizeof(payload), 0x40U, 5U);
    ai_ap12_fill_pattern(dump_seed, sizeof(dump_seed), 0x90U, 1U);
    expected_scalar = ai_ap12_le64(payload + 8U);

    ai_ap12_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    ai_ap12_map_alias(alias_page, base_page);
    ai_ap12_write_bytes(base_paddr, payload, sizeof(payload));
    ai_ap12_write_bytes(dump_paddr, dump_seed, sizeof(dump_seed));
    write64(marker_paddr, 0ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap12_vector_scalar_sequence(src_vaddr, dump_vaddr, scalar_vaddr,
        marker_vaddr, marker_value, &scalar_observed, &marker_observed);
    TEST_ASSERT("AP-12 vector-then-scalar same-line sequence retires without false same-ROB suppression breaking true younger-load progress",
        excpt.triggered == false &&
        scalar_observed == expected_scalar &&
        marker_observed == marker_value &&
        CSRR(CSR_VL) == AI_AP12_VL_E32,
        "scalar=0x%lx expected=0x%lx marker=0x%lx",
        scalar_observed, expected_scalar, marker_observed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-12 vector-then-scalar same-line sequence preserves the full vector payload and the independent marker store",
        ai_ap12_bytes_match(dump_paddr, payload, sizeof(payload)) &&
        read64(marker_paddr) == marker_value
    );

    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap12_replay_alias_exception_reuse_manual_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t ok_vaddr = hs_page_base(base_page) + 0x700U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x700U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x500U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x700U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x500U;
    uint8_t payload[AI_AP12_BYTES];
    uint8_t dump_seed[AI_AP12_BYTES];
    uint64_t observed_vstart = 0ULL;

    ai_ap12_fill_pattern(payload, sizeof(payload), 0x60U, 7U);
    ai_ap12_fill_pattern(dump_seed, sizeof(dump_seed), 0xc0U, 1U);

    ai_ap12_enable_vector_state();
    goto_priv(PRIV_M);
    ai_ap12_write_bytes(base_paddr, payload, sizeof(payload));
    ai_ap12_write_bytes(dump_paddr, dump_seed, sizeof(dump_seed));

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap12_set_page_perm(fault_page, PTE_X);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_ap12_unitstride_load_dump(fault_vaddr, dump_vaddr, AI_AP12_VL_E32);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-12 exception/revoke smoke first takes a precise vector load page fault before reusing a nearby VirtualLoadQueue window",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%lx tval=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, observed_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap12_unitstride_load_dump(ok_vaddr, dump_vaddr, AI_AP12_VL_E32);
    TEST_ASSERT("AP-12 exception/revoke smoke immediately reuses the vector load path on a legal page without stale RAR response or stale exception state",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_AP12_VL_E32
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-12 exception/revoke smoke observes only the repaired legal payload after the faulting same-offset vector load was revoked",
        ai_ap12_bytes_match(dump_paddr, payload, sizeof(payload))
    );

    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap12_virtual_loadqueue_rar_direct_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-12 phase0: unit-stride same-ROB multi-flow load repeatedly refreshes the destination image without stale flow release symptoms",
        ai_micro_ap12_unitstride_same_rob_multiflow_refresh_manual_smoke_corner()
    );
    TEST_ASSERT("AP-12 phase1: unordered/ordered indexed alias loads and masked inactive lanes cover same-ROB element ordering and mask interaction",
        ai_micro_ap12_indexed_alias_ordering_mask_manual_smoke_corner()
    );
    TEST_ASSERT("AP-12 phase2: vector load plus younger scalar same-line marker keeps the cross-instruction progress path alive",
        ai_micro_ap12_vector_scalar_true_younger_rar_manual_smoke_corner()
    );
    TEST_ASSERT("AP-12 phase3: vector fault/reuse sequence checks stale RAR response and revoke/exception cleanup symptoms",
        ai_micro_ap12_replay_alias_exception_reuse_manual_smoke_corner()
    );

    TEST_END();
}

bool ai_micro_ap12_rar_existing_nearby_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-12 phase0: existing vp_match_fail plus ld-ld rollback priority approximation preserves correct flush scope",
        ai_micro_hs_load_vpmatchfail_and_ldld_same_cycle_rollback_priority_correct_corner()
    );
    TEST_ASSERT("AP-12 phase1: existing alias plus ld-ld combined event approximation does not lose markers or stale replay state",
        ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner()
    );
    TEST_ASSERT("AP-12 phase2: existing pointer-wrap replay smoke preserves no-false-release behavior near queue wrap",
        ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner()
    );
    TEST_ASSERT("AP-12 phase3: existing ld-ld flushAfter smoke preserves the immediately following store marker",
        ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner()
    );

    TEST_END();
}

bool ai_micro_ap12_same_rob_replay_and_ignore_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-12 phase0: same-ROB-row dual uncache load replay smoke covers dense-row replay ownership without popcount assertion symptoms",
        ai_micro_hs_same_rob_row_dual_uncache_load_replay_no_popcount_assert_corner()
    );
    TEST_ASSERT("AP-12 phase1: same-ROB-row dual misalign replay smoke covers physical-index selection not stealing the sibling load",
        ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner()
    );
    TEST_ASSERT("AP-12 phase2: all-inactive vector load remains ignored and does not participate in false RAR-like destination clobber",
        ai_micro_hs_vsplit_masked_unitstride_load_all_inactive_no_fault_or_dest_clobber_corner()
    );
    TEST_ASSERT("AP-12 phase3: AP-122 non-aligned older-load RAR query smoke remains clean around updatePaddrValid and MAB split aliases",
        ai_micro_ap122_non_aligned_old_load_rar_query_manual_smoke_corner()
    );
    TEST_ASSERT("AP-12 phase4: AP-122 alias reuse smoke checks no stale paddr pollution across repeated RAR/replay-like windows",
        ai_micro_ap122_mab_split_alias_reuse_no_paddr_pollution_manual_smoke_corner()
    );
    TEST_ASSERT("AP-12 phase5: AP-125 software-prefetch ignore smoke covers ignoreRARCheck entries near same-line demand loads",
        ai_micro_ap125_prefetch_then_same_line_demand_load_rar_ignore_manual_smoke_corner()
    );
    TEST_ASSERT("AP-12 phase6: AP-125 repeated prefetch lifetime smoke covers ignored-RAR entry reuse around later demand accesses",
        ai_micro_ap125_prefetch_repeated_faulting_alias_ignore_lifetime_reuse_manual_smoke_corner()
    );

    TEST_END();
}
