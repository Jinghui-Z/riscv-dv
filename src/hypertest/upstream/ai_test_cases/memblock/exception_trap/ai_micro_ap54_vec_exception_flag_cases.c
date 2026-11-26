#include "ai_micro_trap_entry_store_fault_common.h"

static inline bool ai_micro_ap54_run_late29_nonlastflow_fault(
    uintptr_t source_vaddr, uintptr_t target_base_vaddr, uintptr_t fault_vaddr,
    unsigned requested_vl, unsigned active_vl, unsigned fault_idx)
{
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    if (excpt.triggered || CSRR(CSR_VL) != active_vl) {
        return false;
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr,
        PAGE_SIZE, requested_vl);

    return excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr;
}

static inline void ai_micro_ap54_vle8_m1_to_v16(const uint8_t *src,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v16, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_ap54_vse8_m1_from_v16(uint8_t *dst,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vse8.v v16, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_ap54_vse8_m2_unit_from_v8(uint8_t *dst,
    unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 1);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vse8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_ap54_seed_strided_source_and_targets(
    uintptr_t source_phys, const uint8_t *old_image, const uint8_t *new_image,
    unsigned requested_vl)
{
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
    }
}

static inline bool ai_micro_ap54_run_late31_lastflow_fault(
    uintptr_t source_vaddr, uintptr_t target_base_vaddr, uintptr_t fault_vaddr,
    unsigned requested_vl, unsigned active_vl, unsigned fault_idx)
{
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    if (excpt.triggered || CSRR(CSR_VL) != active_vl) {
        return false;
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr,
        PAGE_SIZE, requested_vl);

    return excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr;
}

static inline bool ai_micro_ap54_unitstride_fault_and_marker_sequence(
    uintptr_t source_vaddr, uintptr_t target_vaddr, uintptr_t fault_vaddr,
    uintptr_t marker_source_vaddr, uintptr_t marker_target_vaddr,
    unsigned requested_vl, unsigned active_vl, unsigned fault_idx)
{
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    if (excpt.triggered || CSRR(CSR_VL) != active_vl) {
        return false;
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vse8_m2_unit_from_v8((uint8_t *)target_vaddr,
        requested_vl);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr)) {
        return false;
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vle8_m1_to_v16((const uint8_t *)marker_source_vaddr,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    if (excpt.triggered || CSRR(CSR_VL) != AI_MICRO_TRAP_ENTRY_VECTOR_BYTES) {
        return false;
    }

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vse8_m1_from_v16((uint8_t *)marker_target_vaddr,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    return excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES;
}

bool ai_micro_ap54_late29_nonlastflow_fault_then_post_redirect_vector_marker_visible_under_pressure_manual_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned next_idx = fault_idx + 1U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t target_base_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x300U;
    const uintptr_t marker_phys = phys_page_base(marker_page) + 0x300U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page);
    const uintptr_t pressure_phys = phys_page_base(pressure_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t pressure_seed_a = 0x13579bdf2468ace0ULL;
    const uint64_t pressure_seed_b = 0xfedcba9876543210ULL;
    const uint64_t pressure_seed_c = 0x0101020305080d15ULL;
    const uint64_t pressure_seed_d = 0x2155aa55cc33f00dULL;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t marker_old[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t marker_new[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    bool late29_fault_ok = false;
    bool pressure_after_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    for (unsigned i = 0; i < sizeof(marker_old); ++i) {
        marker_old[i] = (uint8_t)(0x40U + i);
        marker_new[i] = (uint8_t)(0xb0U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(marker_phys, marker_old,
        sizeof(marker_old));
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(pressure_phys + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(
            ai_micro_trap_entry_strided_target_page(i), leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(marker_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(pressure_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_a,
        pressure_seed_b);
    TEST_ASSERT("AP-54 late29 setup creates dense StoreQueue/SBuffer pressure before the non-lastFlow vector-store fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_ap54_seed_strided_source_and_targets(source_phys, old_image,
        new_image, requested_vl);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();
    late29_fault_ok = ai_micro_ap54_run_late29_nonlastflow_fault(
        source_vaddr, target_base_vaddr, fault_vaddr, requested_vl,
        active_vl, fault_idx);
    TEST_ASSERT("AP-54 first phase reaches a precise late29 non-lastFlow store access fault so vecExceptionFlag should protect active suffix elements",
        late29_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, marker_new,
        sizeof(marker_new));
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_c,
        pressure_seed_d);
    TEST_ASSERT("AP-54 keeps translated dense store pressure active after the redirect before the marker vector store",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vle8_m1_to_v16((const uint8_t *)source_vaddr,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    TEST_ASSERT("AP-54 marker source vector load stages the post-redirect payload without inheriting the old exception context",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vse8_m1_from_v16((uint8_t *)marker_vaddr,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    TEST_ASSERT("AP-54 post-redirect marker vector store retires; stale vecExceptionFlag must not clear its vecValid",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    pressure_after_visible = ai_micro_trap_entry_dense_store_window_match(
        pressure_phys, AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT,
        pressure_seed_d);
    TEST_ASSERT("AP-54 late29 stress keeps prefix/fault/suffix precise while the post-redirect vector marker is fully visible",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        read8(ai_micro_trap_entry_strided_target_phys(next_idx)) ==
            old_image[next_idx] &&
        read8(ai_micro_trap_entry_strided_target_phys(last_active_idx)) ==
            old_image[last_active_idx] &&
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)) ==
            old_image[inactive_idx] &&
        ai_micro_trap_entry_bytes_match(marker_phys, marker_new,
            sizeof(marker_new)) &&
        pressure_after_visible,
        "mem29=0x%x old29=0x%x mem30=0x%x old30=0x%x mem31=0x%x old31=0x%x marker0=0x%x exp0=0x%x pressure=%d",
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx)),
        old_image[fault_idx],
        read8(ai_micro_trap_entry_strided_target_phys(next_idx)),
        old_image[next_idx],
        read8(ai_micro_trap_entry_strided_target_phys(last_active_idx)),
        old_image[last_active_idx],
        read8(marker_phys), marker_new[0], pressure_after_visible
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap54_repeated_late29_fault_repair_vector_marker_loop_no_stale_flag_manual_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t target_base_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t marker_vaddr_base = hs_page_base(marker_page) + 0x400U;
    const uintptr_t marker_phys_base = phys_page_base(marker_page) + 0x400U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page);
    const uintptr_t pressure_phys = phys_page_base(pressure_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t pressure_seed_a = 0x123456789abcdef0ULL;
    const uint64_t pressure_seed_b = 0x0fedcba987654321ULL;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t marker_old[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    bool all_rounds_ok = true;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    for (unsigned i = 0; i < sizeof(marker_old); ++i) {
        marker_old[i] = (uint8_t)(0x20U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(pressure_phys + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(
            ai_micro_trap_entry_strided_target_page(i), leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(marker_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(pressure_page, leaf_flags);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_a,
        pressure_seed_b);
    TEST_ASSERT("AP-54 repeated loop starts with dense stores so the vector exception flag lifetime is stressed before ROB/SQ wrap pressure",
        excpt.triggered == false
    );

    for (unsigned round = 0; round < 5U; ++round) {
        uint8_t round_new[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
        uint8_t round_expected[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
        uint8_t marker_new[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
        uintptr_t round_marker_vaddr = marker_vaddr_base +
            round * AI_MICRO_TRAP_ENTRY_BLOCK_BYTES;
        uintptr_t round_marker_phys = marker_phys_base +
            round * AI_MICRO_TRAP_ENTRY_BLOCK_BYTES;
        uint64_t pressure_round_a = pressure_seed_a + round * 0x1111111111111111ULL;
        uint64_t pressure_round_b = pressure_seed_b ^ (round * 0x0102040810204080ULL);
        bool late29_fault_ok = false;

        for (unsigned i = 0; i < requested_vl; ++i) {
            round_new[i] = (uint8_t)(new_image[i] + round * 0x13U);
            round_expected[i] = (i < fault_idx) ? round_new[i] : old_image[i];
        }
        for (unsigned i = 0; i < sizeof(marker_new); ++i) {
            marker_new[i] = (uint8_t)(0x90U + round * 0x10U + i);
        }

        goto_priv(PRIV_M);
        ai_micro_ap54_seed_strided_source_and_targets(source_phys, old_image,
            round_new, requested_vl);
        ai_micro_trap_entry_write_bytes(round_marker_phys, marker_old,
            sizeof(marker_old));
        ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
            AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
            AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_round_a,
            pressure_round_b);
        TEST_ASSERT("AP-54 repeated loop refreshes dense StoreQueue pressure before each late29 fault",
            excpt.triggered == false,
            "round=%u", round
        );

        late29_fault_ok = ai_micro_ap54_run_late29_nonlastflow_fault(
            source_vaddr, target_base_vaddr, fault_vaddr, requested_vl,
            active_vl, fault_idx);
        TEST_ASSERT("AP-54 repeated loop reaches a precise late29 non-lastFlow fault in each round",
            late29_fault_ok,
            "round=%u", round
        );

        goto_priv(PRIV_M);
        ai_micro_trap_entry_restore_pmp_full_access();
        ai_micro_trap_entry_write_bytes(source_phys, marker_new,
            sizeof(marker_new));
        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_ap54_vle8_m1_to_v16((const uint8_t *)source_vaddr,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
        TEST_ASSERT("AP-54 repeated loop restages a fresh marker vector payload after each fault",
            excpt.triggered == false &&
            CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES,
            "round=%u", round
        );

        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_ap54_vse8_m1_from_v16((uint8_t *)round_marker_vaddr,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
        TEST_ASSERT("AP-54 repeated loop marker vector store must remain visible after each redirect/recovery episode",
            excpt.triggered == false &&
            CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES,
            "round=%u", round
        );

        goto_priv(PRIV_M);
        all_rounds_ok = all_rounds_ok &&
            ai_micro_trap_entry_strided_target_window_matches(round_expected,
                active_vl) &&
            ai_micro_trap_entry_bytes_match(round_marker_phys, marker_new,
                sizeof(marker_new)) &&
            ai_micro_trap_entry_dense_store_window_match(pressure_phys,
                AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_round_b);
        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-54 repeated late29 fault/repair/vector-marker loop keeps every marker visible and every fault-plus-suffix byte old",
        all_rounds_ok,
        "all_rounds_ok=%d marker0=0x%x target29=0x%x target30=0x%x target31=0x%x",
        all_rounds_ok, read8(marker_phys_base),
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx)),
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx + 1U)),
        read8(ai_micro_trap_entry_strided_target_phys(active_vl - 1U))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap54_late31_lastflow_then_new_late29_exception_keeps_suffix_old_and_next_marker_visible_manual_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned old_fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const unsigned new_fault_idx = 29U;
    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t target_base_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t old_fault_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(old_fault_idx);
    const uintptr_t new_fault_vaddr =
        ai_micro_trap_entry_strided_target_vaddr(new_fault_idx);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x680U;
    const uintptr_t marker_phys = phys_page_base(marker_page) + 0x680U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page);
    const uintptr_t pressure_phys = phys_page_base(pressure_page);
    const uintptr_t deny_old_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(old_fault_idx));
    const uintptr_t deny_old_end = deny_old_start + PAGE_SIZE;
    const uintptr_t deny_new_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(new_fault_idx));
    const uintptr_t deny_new_end = deny_new_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t first_new[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t second_new[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t first_expected[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t second_expected[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t marker_new[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    bool old_lastflow_fault_ok = false;
    bool new_late29_fault_ok = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, first_new,
        first_expected, requested_vl, old_fault_idx);
    ai_micro_trap_entry_strided_fill_patterns(old_image, second_new,
        second_expected, requested_vl, new_fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        first_new[i] = (uint8_t)(0xa0U + i);
        first_expected[i] = (i < old_fault_idx) ? first_new[i] : old_image[i];
        second_new[i] = (uint8_t)(0xc0U + i);
        second_expected[i] = (i < new_fault_idx) ? second_new[i] : first_expected[i];
    }
    for (unsigned i = 0; i < sizeof(marker_new); ++i) {
        marker_new[i] = (uint8_t)(0x70U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(pressure_phys + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(
            ai_micro_trap_entry_strided_target_page(i), leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(marker_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(pressure_page, leaf_flags);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_ap54_seed_strided_source_and_targets(source_phys, old_image,
        first_new, requested_vl);
    ai_micro_trap_entry_configure_pmp_window(deny_old_start, deny_old_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();
    old_lastflow_fault_ok = ai_micro_ap54_run_late31_lastflow_fault(
        source_vaddr, target_base_vaddr, old_fault_vaddr, requested_vl,
        active_vl, old_fault_idx);
    TEST_ASSERT("AP-54 overlap setup first reaches an old late31 lastFlow exception that should cancel cleanly at its final flow",
        old_lastflow_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_ap54_seed_strided_source_and_targets(source_phys, first_expected,
        second_new, requested_vl);
    ai_micro_trap_entry_configure_pmp_window(deny_new_start, deny_new_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, 0x55aa33cc77dd9911ULL,
        0x1199dd77cc33aa55ULL);
    TEST_ASSERT("AP-54 overlap setup adds pressure before the new non-lastFlow exception that should own the next flag",
        excpt.triggered == false
    );

    new_late29_fault_ok = ai_micro_ap54_run_late29_nonlastflow_fault(
        source_vaddr, target_base_vaddr, new_fault_vaddr, requested_vl,
        active_vl, new_fault_idx);
    TEST_ASSERT("AP-54 overlap setup reaches a new late29 non-lastFlow exception after the old lastFlow boundary",
        new_late29_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, marker_new,
        sizeof(marker_new));
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vle8_m1_to_v16((const uint8_t *)source_vaddr,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    TEST_ASSERT("AP-54 overlap marker source load succeeds after the new exception has drained",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap54_vse8_m1_from_v16((uint8_t *)marker_vaddr,
        AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
    TEST_ASSERT("AP-54 overlap marker store succeeds, proving no stale old/new flag leaks into the next vector store",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_TRAP_ENTRY_VECTOR_BYTES
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-54 overlap case keeps the new exception suffix old while leaving the next marker vector store visible",
        ai_micro_trap_entry_strided_target_window_matches(second_expected,
            active_vl) &&
        read8(ai_micro_trap_entry_strided_target_phys(new_fault_idx)) ==
            first_expected[new_fault_idx] &&
        read8(ai_micro_trap_entry_strided_target_phys(new_fault_idx + 1U)) ==
            first_expected[new_fault_idx + 1U] &&
        ai_micro_trap_entry_bytes_match(marker_phys, marker_new,
            sizeof(marker_new)),
        "mem29=0x%x exp29=0x%x mem30=0x%x exp30=0x%x marker0=0x%x exp0=0x%x",
        read8(ai_micro_trap_entry_strided_target_phys(new_fault_idx)),
        first_expected[new_fault_idx],
        read8(ai_micro_trap_entry_strided_target_phys(new_fault_idx + 1U)),
        first_expected[new_fault_idx + 1U],
        read8(marker_phys), marker_new[0]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap54_unitstride_first_fault_then_marker_vector_store_no_stale_inherit_manual_corner()
{
    TEST_START();

    const unsigned requested_vl = 16U;
    const unsigned active_vl = 16U;
    const unsigned fault_idx = 0U;
    const enum test_page source_page = SWITCH1;
    const enum test_page target_page = SWITCH2;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x240U;
    const uintptr_t source_phys = phys_page_base(source_page) + 0x240U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x300U;
    const uintptr_t target_phys = phys_page_base(target_page) + 0x300U;
    const uintptr_t fault_vaddr = target_vaddr;
    const uintptr_t marker_source_vaddr = hs_page_base(source_page) + 0x340U;
    const uintptr_t marker_source_phys = phys_page_base(source_page) + 0x340U;
    const uintptr_t marker_target_vaddr = hs_page_base(marker_page) + 0x500U;
    const uintptr_t marker_target_phys = phys_page_base(marker_page) + 0x500U;
    const uintptr_t deny_start = phys_page_base(target_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t source_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t marker_old[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t marker_new[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    bool unitstride_fault_marker_ok = false;

    for (unsigned i = 0; i < AI_MICRO_TRAP_ENTRY_VECTOR_BYTES; ++i) {
        old_image[i] = (uint8_t)(0x30U + i);
        source_image[i] = (uint8_t)(0xa0U + i);
        marker_old[i] = (uint8_t)(0x50U + i);
        marker_new[i] = (uint8_t)(0xd0U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, source_image,
        sizeof(source_image));
    ai_micro_trap_entry_write_bytes(target_phys, old_image, sizeof(old_image));
    ai_micro_trap_entry_write_bytes(marker_source_phys, marker_new,
        sizeof(marker_new));
    ai_micro_trap_entry_write_bytes(marker_target_phys, marker_old,
        sizeof(marker_old));

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SAF));
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(target_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(marker_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    unitstride_fault_marker_ok =
        ai_micro_ap54_unitstride_fault_and_marker_sequence(source_vaddr,
            target_vaddr, fault_vaddr, marker_source_vaddr,
            marker_target_vaddr, requested_vl, active_vl, fault_idx);
    TEST_ASSERT("AP-54 UnitStride first-element fault is followed by a clean marker vector store rather than reopening stale exception inheritance",
        unitstride_fault_marker_ok
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-54 UnitStride first-fault case leaves the denied vector store old and makes the later marker vector store fully visible",
        ai_micro_trap_entry_bytes_match(target_phys, old_image,
            sizeof(old_image)) &&
        ai_micro_trap_entry_bytes_match(marker_target_phys, marker_new,
            sizeof(marker_new)),
        "target0=0x%x old0=0x%x marker0=0x%x exp0=0x%x",
        read8(target_phys), old_image[0], read8(marker_target_phys),
        marker_new[0]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
