#include "ai_micro_trap_entry_store_fault_common.h"

static volatile uintptr_t ai_micro_ap8_skip_younger_resume_epc = 0;

extern void ai_micro_ap8_skip_younger_vector_store_handler(void);

asm(
".balign 0x4\n"
".global ai_micro_ap8_skip_younger_vector_store_handler\n"
"ai_micro_ap8_skip_younger_vector_store_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_ap8_skip_younger_resume_epc\n"
"    ld t1, 0(t0)\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

static inline void ai_micro_ap8_vse8_from_v8_m2_unit(uint8_t *dst, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 1);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vse8.v v8, (%[dst])\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [vl] "r"(vl),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline __attribute__((always_inline)) void
ai_micro_ap8_scalar_fault_then_skipped_vector_store(
    uintptr_t fault_vaddr, uint8_t *dst, unsigned vl, uintptr_t resume_epc)
{
    (void)resume_epc;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t0, 0(%[fault_addr])\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vse8.v v8, (%[dst])\n\t"
        ".option pop\n\t"
        :
        : [fault_addr] "r"(fault_vaddr),
          [dst] "r"(dst),
          [vl] "r"(vl),
          [vtype] "r"(VTYPE(VSEW_FROM_BITS(8), 1))
        : "t0", "memory");
}

static inline void ai_micro_ap8_masked_vsse8_even_from_v8_m2(
    uint8_t *dst, intptr_t stride, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 1);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vid.v v0\n\t"
        "vand.vi v0, v0, 1\n\t"
        "vmseq.vi v0, v0, 0\n\t"
        "vsse8.v v8, (%[dst]), %[stride], v0.t\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst),
          [stride] "r"(stride),
          [vl] "r"(vl),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline bool ai_micro_ap8_even_prefix_masked_fault_image_matches(
    const uint8_t *old_image, const uint8_t *new_image, unsigned active_vl,
    unsigned fault_idx)
{
    for (unsigned i = 0; i < active_vl; ++i) {
        uint8_t expected = ((i < fault_idx) && ((i & 1U) == 0U)) ?
            new_image[i] : old_image[i];

        if (read8(ai_micro_trap_entry_strided_target_phys(i)) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_micro_ap9_seed_strided_source_and_targets(
    uintptr_t source_phys, const uint8_t *old_image, const uint8_t *new_image,
    unsigned requested_vl)
{
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
    }
}

static inline bool ai_micro_ap9_run_late31_lastflow_fault(
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

static inline bool ai_micro_ap9_cross16_sd_window_matches(
    uintptr_t base_phys, const uint8_t *old_image, unsigned window_bytes,
    unsigned store_offset, uint64_t store_value)
{
    for (unsigned i = 0; i < window_bytes; ++i) {
        uint8_t expected = old_image[i];

        if (i >= store_offset && i < store_offset + sizeof(uint64_t)) {
            expected = (uint8_t)(store_value >> (8U * (i - store_offset)));
        }
        if (read8(base_phys + i) != expected) {
            return false;
        }
    }

    return true;
}

bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_TRAP_ENTRY_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t pressure_first_seed = 0x13579bdf2468ace0ULL;
    const uint64_t pressure_second_seed = 0x0eca8642fdb97531ULL;
    const uint64_t pressure_third_seed = 0x55aa33cc77ee1199ULL;
    const uint64_t pressure_fourth_seed = 0x99ee77cc33aa5511ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_probe[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uintptr_t early_probe_addrs[2];
    uint8_t expected_early_probe[2];
    uint8_t older_scalar_value = 0;
    uint64_t observed_source_vl = 0;
    bool target_prewarm_visible = false;
    bool fill_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    early_probe_addrs[0] = fault_vaddr;
    early_probe_addrs[1] = ai_micro_trap_entry_strided_target_vaddr(idx_plus_one);
    expected_early_probe[0] = old_image[fault_idx];
    expected_early_probe[1] = old_image[idx_plus_one];
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_probe[i] = AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_probe[i] = expected_after_fault[i];
        }
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
    }
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(ai_micro_trap_entry_strided_target_page(i),
            leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(fill_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_prepare_strided_probe(target_base_vaddr, PAGE_SIZE,
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);
    ai_micro_trap_entry_prepare_early_byte_probes(early_probe_addrs,
        sizeof(early_probe_addrs) / sizeof(early_probe_addrs[0]),
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_parameter_strided_vector_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late31 requested-33 e8,m2 strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late31 requested-33 e8,m2 strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_trap_entry_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31-of-33 PMP deny-write strided store still sees the original first-byte image before trap entry observes the same parameter stream",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    TEST_SETUP_EXCEPT();
    target_prewarm_visible = ai_micro_trap_entry_strided_vaddr_window_matches(
        target_base_vaddr, PAGE_SIZE, old_image, active_vl);
    TEST_ASSERT("a full active-window scalar prewarm across all 32 target pages still sees the old image before the late31 PMP deny-write strided store is issued, so the later fault runs under hot translated-address pressure",
        excpt.triggered == false &&
        target_prewarm_visible
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the requested-33 e8,m2 source vector load resolves to active vl=32 before the late31 PMP deny-write strided store fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    ai_micro_trap_entry_dense_store_tail_refresh(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT -
            AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT,
        AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("the late31-of-33 requested-33 e8,m2 strided store reports a precise store access fault on the denied page before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SAF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late31-of-33 requested-33 e8,m2 strided store reports vstart=31 before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load fires immediately at handler head and itself also resolves to active vl=32",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_strided_probe_vl == active_vl,
        "probe_fired=%lu observed_probe_vl=%lu expected_active_vl=%u",
        ai_micro_trap_entry_probe_fired, ai_micro_trap_entry_strided_probe_vl,
        active_vl
    );
    TEST_ASSERT("after the late31 trap-entry same-parameter strided-load snapshot, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the handler-head scalar byte probes still see the old late31 fault byte and the old inactive idx32 byte before the full same-parameter strided load snapshot is taken",
        ai_micro_trap_entry_early_probe_bytes_match(expected_early_probe,
            sizeof(expected_early_probe) / sizeof(expected_early_probe[0])),
        "early31=0x%x exp31=0x%x early32=0x%x exp32=0x%x",
        ai_micro_trap_entry_early_probe_bytes[0], expected_early_probe[0],
        ai_micro_trap_entry_early_probe_bytes[1], expected_early_probe[1]
    );
    TEST_ASSERT("the late31-of-33 PMP deny-write episode commits prefix elements0-30, keeps fault element31 old in memory, and leaves inactive idx32 old",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        read8(ai_micro_trap_entry_strided_target_phys(idx_plus_one)) ==
            old_image[idx_plus_one]
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load sees committed-prefix bytes on active elements0-30 while still seeing old fault element31 and leaving inactive idx32 at the sentinel",
        ai_micro_trap_entry_strided_probe_bytes_match(expected_probe, requested_vl)
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31 PMP deny-write strided-store trap and the trap-entry same-parameter strided-load probe",
        ai_micro_trap_entry_bytes_match(source_phys, new_image, requested_vl)
    );
    TEST_ASSERT("the two dense translated-store pressure bursts remain architecturally visible across the later late31 strided-store fault before the trap-entry same-parameter strided load returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_TRAP_ENTRY_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t pressure_first_seed = 0x02468ace13579bdfULL;
    const uint64_t pressure_second_seed = 0xfdb97531eca86420ULL;
    const uint64_t pressure_third_seed = 0x66778899aabbccddULL;
    const uint64_t pressure_fourth_seed = 0xddccbbaa99887766ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t fault_leaf_flags = PTE_V | PTE_AD | PTE_R;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_probe[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uintptr_t early_probe_addrs[2];
    uint8_t expected_early_probe[2];
    uint8_t older_scalar_value = 0;
    uint64_t observed_source_vl = 0;
    bool target_prewarm_visible = false;
    bool fill_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    early_probe_addrs[0] = fault_vaddr;
    early_probe_addrs[1] = ai_micro_trap_entry_strided_target_vaddr(idx_plus_one);
    expected_early_probe[0] = old_image[fault_idx];
    expected_early_probe[1] = old_image[idx_plus_one];
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_probe[i] = AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_probe[i] = expected_after_fault[i];
        }
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
    }
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(ai_micro_trap_entry_strided_target_page(i),
            leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(ai_micro_trap_entry_strided_target_page(fault_idx),
        fault_leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(fill_page, leaf_flags);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_prepare_strided_probe(target_base_vaddr, PAGE_SIZE,
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);
    ai_micro_trap_entry_prepare_early_byte_probes(early_probe_addrs,
        sizeof(early_probe_addrs) / sizeof(early_probe_addrs[0]),
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_parameter_strided_vector_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late31 requested-33 e8,m2 HS-leaf-no-write strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late31 requested-33 e8,m2 HS-leaf-no-write strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_trap_entry_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31-of-33 HS-leaf-no-write strided store still sees the original first-byte image before trap entry observes the same parameter stream",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    TEST_SETUP_EXCEPT();
    target_prewarm_visible = ai_micro_trap_entry_strided_vaddr_window_matches(
        target_base_vaddr, PAGE_SIZE, old_image, active_vl);
    TEST_ASSERT("a full active-window scalar prewarm across all 32 target pages still sees the old image before the late31 HS-leaf-no-write strided store is issued, so the later fault runs under hot translated-address pressure",
        excpt.triggered == false &&
        target_prewarm_visible
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the requested-33 e8,m2 source vector load resolves to active vl=32 before the late31 HS-leaf-no-write strided store fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    ai_micro_trap_entry_dense_store_tail_refresh(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT -
            AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT,
        AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    TEST_ASSERT("the late31-of-33 requested-33 e8,m2 strided store reports a precise store page fault on the HS leaf with write cleared before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late31-of-33 requested-33 e8,m2 HS-leaf-no-write strided store reports vstart=31 before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load fires immediately at handler head and itself also resolves to active vl=32 in the HS-leaf-no-write case",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_strided_probe_vl == active_vl,
        "probe_fired=%lu observed_probe_vl=%lu expected_active_vl=%u",
        ai_micro_trap_entry_probe_fired, ai_micro_trap_entry_strided_probe_vl,
        active_vl
    );
    TEST_ASSERT("after the late31 HS-leaf-no-write trap-entry same-parameter strided-load snapshot, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the handler-head scalar byte probes still see the old late31 fault byte and the old inactive idx32 byte before the full same-parameter strided load snapshot is taken in the HS-leaf-no-write case",
        ai_micro_trap_entry_early_probe_bytes_match(expected_early_probe,
            sizeof(expected_early_probe) / sizeof(expected_early_probe[0])),
        "early31=0x%x exp31=0x%x early32=0x%x exp32=0x%x",
        ai_micro_trap_entry_early_probe_bytes[0], expected_early_probe[0],
        ai_micro_trap_entry_early_probe_bytes[1], expected_early_probe[1]
    );
    TEST_ASSERT("the late31-of-33 HS-leaf-no-write episode commits prefix elements0-30, keeps fault element31 old in memory, and leaves inactive idx32 old",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        read8(ai_micro_trap_entry_strided_target_phys(idx_plus_one)) ==
            old_image[idx_plus_one]
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load sees committed-prefix bytes on active elements0-30 while still seeing old fault element31 and leaving inactive idx32 at the sentinel in the HS-leaf-no-write case",
        ai_micro_trap_entry_strided_probe_bytes_match(expected_probe, requested_vl)
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31 HS-leaf-no-write strided-store trap and the trap-entry same-parameter strided-load probe",
        ai_micro_trap_entry_bytes_match(source_phys, new_image, requested_vl)
    );
    TEST_ASSERT("the two dense translated-store pressure bursts remain architecturally visible across the later late31 HS-leaf-no-write strided-store fault before the trap-entry same-parameter strided load returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_active_suffix_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned idx_plus_one = fault_idx + 1U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t pressure_first_seed = 0x1122334455667788ULL;
    const uint64_t pressure_second_seed = 0x8877665544332211ULL;
    const uint64_t pressure_third_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t pressure_fourth_seed = 0x81706f5e4d3c2b1aULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_probe[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uintptr_t early_probe_addrs[4];
    uint8_t expected_early_probe[4];
    uint8_t older_scalar_value = 0;
    uint64_t observed_source_vl = 0;
    uint8_t fault_mem = 0;
    uint8_t next_mem = 0;
    uint8_t last_active_mem = 0;
    uint8_t inactive_mem = 0;
    uint8_t fault_probe = 0;
    uint8_t next_probe = 0;
    uint8_t last_active_probe = 0;
    uint8_t inactive_probe = 0;
    bool target_prewarm_visible = false;
    bool fill_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    early_probe_addrs[0] = fault_vaddr;
    early_probe_addrs[1] = ai_micro_trap_entry_strided_target_vaddr(idx_plus_one);
    early_probe_addrs[2] = ai_micro_trap_entry_strided_target_vaddr(last_active_idx);
    early_probe_addrs[3] = ai_micro_trap_entry_strided_target_vaddr(inactive_idx);
    expected_early_probe[0] = old_image[fault_idx];
    expected_early_probe[1] = old_image[idx_plus_one];
    expected_early_probe[2] = old_image[last_active_idx];
    expected_early_probe[3] = old_image[inactive_idx];
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_probe[i] = AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_probe[i] = expected_after_fault[i];
        }
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
    }
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(ai_micro_trap_entry_strided_target_page(i),
            leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(fill_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_prepare_strided_probe(target_base_vaddr, PAGE_SIZE,
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);
    ai_micro_trap_entry_prepare_early_byte_probes(early_probe_addrs,
        sizeof(early_probe_addrs) / sizeof(early_probe_addrs[0]),
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_parameter_strided_vector_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late29 requested-33 e8,m2 strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late29 requested-33 e8,m2 strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_trap_entry_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 PMP deny-write strided store still sees the original first-byte image before trap entry observes the same parameter stream",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    TEST_SETUP_EXCEPT();
    target_prewarm_visible = ai_micro_trap_entry_strided_vaddr_window_matches(
        target_base_vaddr, PAGE_SIZE, old_image, active_vl);
    TEST_ASSERT("a full active-window scalar prewarm across all 32 target pages still sees the old image before the late29 PMP deny-write strided store is issued, so the later fault runs under hot translated-address pressure with active suffix elements behind it",
        excpt.triggered == false &&
        target_prewarm_visible
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the requested-33 e8,m2 source vector load resolves to active vl=32 before the late29 PMP deny-write strided store fires, leaving active suffix elements30-31 behind the fault element",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u fault_idx=%u",
        excpt.triggered, observed_source_vl, active_vl, fault_idx
    );

    ai_micro_trap_entry_dense_store_tail_refresh(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT -
            AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT,
        AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);
    fault_mem = read8(ai_micro_trap_entry_strided_target_phys(fault_idx));
    next_mem = read8(ai_micro_trap_entry_strided_target_phys(idx_plus_one));
    last_active_mem = read8(ai_micro_trap_entry_strided_target_phys(last_active_idx));
    inactive_mem = read8(ai_micro_trap_entry_strided_target_phys(inactive_idx));
    fault_probe = ai_micro_trap_entry_strided_probe_bytes[fault_idx];
    next_probe = ai_micro_trap_entry_strided_probe_bytes[idx_plus_one];
    last_active_probe = ai_micro_trap_entry_strided_probe_bytes[last_active_idx];
    inactive_probe = ai_micro_trap_entry_strided_probe_bytes[inactive_idx];

    TEST_ASSERT("the late29-of-33 requested-33 e8,m2 strided store reports a precise store access fault on the denied page before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SAF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 requested-33 e8,m2 strided store reports vstart=29 before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load fires immediately at handler head and itself also resolves to active vl=32 in the late29 PMP deny-write case",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_strided_probe_vl == active_vl,
        "probe_fired=%lu observed_probe_vl=%lu expected_active_vl=%u",
        ai_micro_trap_entry_probe_fired, ai_micro_trap_entry_strided_probe_vl,
        active_vl
    );
    TEST_ASSERT("after the late29 trap-entry same-parameter strided-load snapshot, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the handler-head scalar byte probes still see old fault element29, old active suffix elements30-31, and old inactive idx32 before the full same-parameter strided load snapshot is taken",
        ai_micro_trap_entry_early_probe_bytes_match(expected_early_probe,
            sizeof(expected_early_probe) / sizeof(expected_early_probe[0])),
        "early29=0x%x exp29=0x%x early30=0x%x exp30=0x%x early31=0x%x exp31=0x%x early32=0x%x exp32=0x%x",
        ai_micro_trap_entry_early_probe_bytes[0], expected_early_probe[0],
        ai_micro_trap_entry_early_probe_bytes[1], expected_early_probe[1],
        ai_micro_trap_entry_early_probe_bytes[2], expected_early_probe[2],
        ai_micro_trap_entry_early_probe_bytes[3], expected_early_probe[3]
    );
    TEST_ASSERT("the late29-of-33 PMP deny-write episode commits active prefix elements0-28 while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        inactive_mem == old_image[inactive_idx],
        "mem29=0x%x exp29=0x%x mem30=0x%x exp30=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        fault_mem, old_image[fault_idx],
        next_mem, old_image[idx_plus_one],
        last_active_mem, old_image[last_active_idx],
        inactive_mem, old_image[inactive_idx]
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load sees committed-prefix bytes on active elements0-28 while still seeing old active fault-plus-suffix elements29-31 and leaving inactive idx32 at the sentinel",
        ai_micro_trap_entry_strided_probe_bytes_match(expected_probe, requested_vl),
        "probe29=0x%x exp29=0x%x probe30=0x%x exp30=0x%x probe31=0x%x exp31=0x%x probe32=0x%x exp32=0x%x",
        fault_probe, old_image[fault_idx],
        next_probe, old_image[idx_plus_one],
        last_active_probe, old_image[last_active_idx],
        inactive_probe, AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29 PMP deny-write strided-store trap and the trap-entry same-parameter strided-load probe",
        ai_micro_trap_entry_bytes_match(source_phys, new_image, requested_vl)
    );
    TEST_ASSERT("the two dense translated-store pressure bursts remain architecturally visible across the later late29 strided-store fault before the trap-entry same-parameter strided load returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_active_suffix_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned idx_plus_one = fault_idx + 1U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t pressure_first_seed = 0x2233445566778899ULL;
    const uint64_t pressure_second_seed = 0x9988776655443322ULL;
    const uint64_t pressure_third_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t pressure_fourth_seed = 0x78695a4b3c2d1e0fULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t fault_leaf_flags = PTE_V | PTE_AD | PTE_R;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_probe[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uintptr_t early_probe_addrs[4];
    uint8_t expected_early_probe[4];
    uint8_t older_scalar_value = 0;
    uint64_t observed_source_vl = 0;
    uint8_t fault_mem = 0;
    uint8_t next_mem = 0;
    uint8_t last_active_mem = 0;
    uint8_t inactive_mem = 0;
    uint8_t fault_probe = 0;
    uint8_t next_probe = 0;
    uint8_t last_active_probe = 0;
    uint8_t inactive_probe = 0;
    bool target_prewarm_visible = false;
    bool fill_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    early_probe_addrs[0] = fault_vaddr;
    early_probe_addrs[1] = ai_micro_trap_entry_strided_target_vaddr(idx_plus_one);
    early_probe_addrs[2] = ai_micro_trap_entry_strided_target_vaddr(last_active_idx);
    early_probe_addrs[3] = ai_micro_trap_entry_strided_target_vaddr(inactive_idx);
    expected_early_probe[0] = old_image[fault_idx];
    expected_early_probe[1] = old_image[idx_plus_one];
    expected_early_probe[2] = old_image[last_active_idx];
    expected_early_probe[3] = old_image[inactive_idx];
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_probe[i] = AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_probe[i] = expected_after_fault[i];
        }
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
    }
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE, 0ULL);
    }

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    for (unsigned i = 0; i < requested_vl; ++i) {
        ai_micro_trap_entry_set_hs_leaf(ai_micro_trap_entry_strided_target_page(i),
            leaf_flags);
    }
    ai_micro_trap_entry_set_hs_leaf(ai_micro_trap_entry_strided_target_page(fault_idx),
        fault_leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(fill_page, leaf_flags);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);
    ai_micro_trap_entry_prepare_strided_probe(target_base_vaddr, PAGE_SIZE,
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);
    ai_micro_trap_entry_prepare_early_byte_probes(early_probe_addrs,
        sizeof(early_probe_addrs) / sizeof(early_probe_addrs[0]),
        AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_trap_entry_same_parameter_strided_vector_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    TEST_ASSERT("the first full-page dense translated-store pressure burst completes without exception before the late29 requested-33 e8,m2 HS-leaf-no-write strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    TEST_ASSERT("the second full-page dense translated-store pressure burst also completes without exception before the late29 requested-33 e8,m2 HS-leaf-no-write strided-store fault plus trap-entry same-parameter strided-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_trap_entry_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 HS-leaf-no-write strided store still sees the original first-byte image before trap entry observes the same parameter stream",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    TEST_SETUP_EXCEPT();
    target_prewarm_visible = ai_micro_trap_entry_strided_vaddr_window_matches(
        target_base_vaddr, PAGE_SIZE, old_image, active_vl);
    TEST_ASSERT("a full active-window scalar prewarm across all 32 target pages still sees the old image before the late29 HS-leaf-no-write strided store is issued, so the later fault runs under hot translated-address pressure with active suffix elements behind it",
        excpt.triggered == false &&
        target_prewarm_visible
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the requested-33 e8,m2 source vector load resolves to active vl=32 before the late29 HS-leaf-no-write strided store fires, leaving active suffix elements30-31 behind the fault element",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u fault_idx=%u",
        excpt.triggered, observed_source_vl, active_vl, fault_idx
    );

    ai_micro_trap_entry_dense_store_tail_refresh(fill_vaddr_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT -
            AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT,
        AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    fill_visible = ai_micro_trap_entry_dense_store_window_match(fill_phys_base,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_fourth_seed);
    fault_mem = read8(ai_micro_trap_entry_strided_target_phys(fault_idx));
    next_mem = read8(ai_micro_trap_entry_strided_target_phys(idx_plus_one));
    last_active_mem = read8(ai_micro_trap_entry_strided_target_phys(last_active_idx));
    inactive_mem = read8(ai_micro_trap_entry_strided_target_phys(inactive_idx));
    fault_probe = ai_micro_trap_entry_strided_probe_bytes[fault_idx];
    next_probe = ai_micro_trap_entry_strided_probe_bytes[idx_plus_one];
    last_active_probe = ai_micro_trap_entry_strided_probe_bytes[last_active_idx];
    inactive_probe = ai_micro_trap_entry_strided_probe_bytes[inactive_idx];

    TEST_ASSERT("the late29-of-33 requested-33 e8,m2 strided store reports a precise store page fault on the HS leaf with write cleared before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_cause == CAUSE_SPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 requested-33 e8,m2 HS-leaf-no-write strided store reports vstart=29 before the trap-entry same-parameter strided load runs",
        ai_micro_trap_entry_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load fires immediately at handler head and itself also resolves to active vl=32 in the late29 HS-leaf-no-write case",
        ai_micro_trap_entry_probe_fired == 1ULL &&
        ai_micro_trap_entry_strided_probe_vl == active_vl,
        "probe_fired=%lu observed_probe_vl=%lu expected_active_vl=%u",
        ai_micro_trap_entry_probe_fired, ai_micro_trap_entry_strided_probe_vl,
        active_vl
    );
    TEST_ASSERT("after the late29 HS-leaf-no-write trap-entry same-parameter strided-load snapshot, the handler also replays the prepared translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_trap_entry_handler_pressure_fired == 1ULL
    );
    TEST_ASSERT("the handler-head scalar byte probes still see old fault element29, old active suffix elements30-31, and old inactive idx32 before the full same-parameter strided load snapshot is taken in the HS-leaf-no-write case",
        ai_micro_trap_entry_early_probe_bytes_match(expected_early_probe,
            sizeof(expected_early_probe) / sizeof(expected_early_probe[0])),
        "early29=0x%x exp29=0x%x early30=0x%x exp30=0x%x early31=0x%x exp31=0x%x early32=0x%x exp32=0x%x",
        ai_micro_trap_entry_early_probe_bytes[0], expected_early_probe[0],
        ai_micro_trap_entry_early_probe_bytes[1], expected_early_probe[1],
        ai_micro_trap_entry_early_probe_bytes[2], expected_early_probe[2],
        ai_micro_trap_entry_early_probe_bytes[3], expected_early_probe[3]
    );
    TEST_ASSERT("the late29-of-33 HS-leaf-no-write episode commits active prefix elements0-28 while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        inactive_mem == old_image[inactive_idx],
        "mem29=0x%x exp29=0x%x mem30=0x%x exp30=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        fault_mem, old_image[fault_idx],
        next_mem, old_image[idx_plus_one],
        last_active_mem, old_image[last_active_idx],
        inactive_mem, old_image[inactive_idx]
    );
    TEST_ASSERT("the trap-entry same-parameter requested-33 e8,m2 strided load sees committed-prefix bytes on active elements0-28 while still seeing old active fault-plus-suffix elements29-31 and leaving inactive idx32 at the sentinel in the HS-leaf-no-write case",
        ai_micro_trap_entry_strided_probe_bytes_match(expected_probe, requested_vl),
        "probe29=0x%x exp29=0x%x probe30=0x%x exp30=0x%x probe31=0x%x exp31=0x%x probe32=0x%x exp32=0x%x",
        fault_probe, old_image[fault_idx],
        next_probe, old_image[idx_plus_one],
        last_active_probe, old_image[last_active_idx],
        inactive_probe, AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29 HS-leaf-no-write strided-store trap and the trap-entry same-parameter strided-load probe",
        ai_micro_trap_entry_bytes_match(source_phys, new_image, requested_vl)
    );
    TEST_ASSERT("the two dense translated-store pressure bursts remain architecturally visible across the later late29 HS-leaf-no-write strided-store fault before the trap-entry same-parameter strided load returns",
        fill_visible
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap8_older_scalar_load_fault_skips_younger_vector_store_no_wrongpath_leak_manual_smoke_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page target_page = SWITCH2;
    const uintptr_t fault_vaddr = hs_page_base(VSI_GI);
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x240U;
    const uintptr_t source_phys = phys_page_base(source_page) + 0x240U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x2c0U;
    const uintptr_t target_phys = phys_page_base(target_page) + 0x2c0U;
    const unsigned vector_bytes = AI_MICRO_TRAP_ENTRY_VECTOR_BYTES;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t marker_value = 0x8c0ffee0badcafeULL;
    const uintptr_t marker_vaddr = target_vaddr + 0x80U;
    const uintptr_t marker_phys = target_phys + 0x80U;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];

    for (unsigned i = 0; i < vector_bytes; i++) {
        old_image[i] = (uint8_t)(0x40U + i);
        new_image[i] = (uint8_t)(0xc0U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, new_image, vector_bytes);
    ai_micro_trap_entry_write_bytes(target_phys, old_image, vector_bytes);
    write64(marker_phys, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_LPF));
    ai_micro_trap_entry_set_hs_leaf(source_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(target_page, leaf_flags);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_ap8_skip_younger_vector_store_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        vector_bytes);
    TEST_ASSERT("AP-8 older-redirect setup can stage the younger vector-store payload before forcing the scalar fault",
        excpt.triggered == false &&
        CSRR(CSR_VL) == vector_bytes,
        "triggered=%d vl=%lu expected=%u",
        excpt.triggered, CSRR(CSR_VL), vector_bytes
    );

    ai_micro_ap8_skip_younger_resume_epc =
        (uintptr_t)&&ap8_after_skipped_vector_store;
    TEST_SETUP_EXCEPT();
    ai_micro_ap8_scalar_fault_then_skipped_vector_store(fault_vaddr,
        (uint8_t *)target_vaddr, vector_bytes,
        ai_micro_ap8_skip_younger_resume_epc);

ap8_after_skipped_vector_store:
    excpt.testing = false;
    sd(marker_vaddr, marker_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    TEST_ASSERT("AP-8 older scalar load page fault is captured before the younger vector store window",
        ai_micro_trap_entry_trap_cause == CAUSE_LPF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr,
        "cause=0x%lx tval=0x%lx expected_tval=0x%lx",
        ai_micro_trap_entry_trap_cause, ai_micro_trap_entry_trap_tval,
        fault_vaddr
    );
    TEST_ASSERT("AP-8 older-redirect manual smoke proves the skipped younger vector store left no wrong-path payload in memory",
        ai_micro_trap_entry_bytes_match(target_phys, old_image, vector_bytes) &&
        read64(marker_phys) == marker_value,
        "target0=0x%x old0=0x%x target15=0x%x old15=0x%x marker=0x%lx",
        read8(target_phys), old_image[0],
        read8(target_phys + vector_bytes - 1U), old_image[vector_bytes - 1U],
        read64(marker_phys)
    );
    TEST_ASSERT("AP-8 older-redirect manual smoke keeps the staged vector source payload intact after the fault skip",
        ai_micro_trap_entry_bytes_match(source_phys, new_image, vector_bytes)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap8_masked_even_late28_fault_preserves_inactive_and_suffix_old_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 28U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    bool prewarm_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
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
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prewarm_visible = ai_micro_trap_entry_strided_vaddr_window_matches(
        target_base_vaddr, PAGE_SIZE, old_image, active_vl);
    TEST_ASSERT("AP-8 masked setup prewarms all active translated target pages before the even-lane faulting vector store",
        excpt.triggered == false &&
        prewarm_visible
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    TEST_ASSERT("AP-8 masked setup stages the e8,m2 source payload with active vl=32",
        excpt.triggered == false &&
        CSRR(CSR_VL) == active_vl,
        "triggered=%d vl=%lu expected=%u",
        excpt.triggered, CSRR(CSR_VL), active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_ap8_masked_vsse8_even_from_v8_m2((uint8_t *)target_base_vaddr,
        PAGE_SIZE, requested_vl);
    TEST_ASSERT("AP-8 masked even-lane vector store faults precisely on active even element28",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, fault_vaddr,
        CSRR(CSR_VSTART)
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-8 masked even-lane fault commits only active even prefix stores and keeps masked odd lanes, fault, active suffix, and inactive idx32 old",
        ai_micro_ap8_even_prefix_masked_fault_image_matches(old_image,
            new_image, active_vl, fault_idx) &&
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)) ==
            old_image[inactive_idx],
        "even26=0x%x exp=0x%x odd27=0x%x old27=0x%x fault28=0x%x old28=0x%x odd29=0x%x old29=0x%x inactive32=0x%x old32=0x%x",
        read8(ai_micro_trap_entry_strided_target_phys(26U)), new_image[26],
        read8(ai_micro_trap_entry_strided_target_phys(27U)), old_image[27],
        read8(ai_micro_trap_entry_strided_target_phys(28U)), old_image[28],
        read8(ai_micro_trap_entry_strided_target_phys(29U)), old_image[29],
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)),
        old_image[inactive_idx]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap8_adjacent_vector_store_after_late29_fault_is_skipped_no_wrongpath_payload_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const enum test_page source_page = SWITCH1;
    const enum test_page younger_source_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t younger_source_vaddr = hs_page_base(younger_source_page) + 0x300U;
    const uintptr_t younger_source_phys = phys_page_base(younger_source_page) + 0x300U;
    const uintptr_t younger_target_vaddr = hs_page_base(younger_source_page) + 0x380U;
    const uintptr_t younger_target_phys = phys_page_base(younger_source_page) + 0x380U;
    const uintptr_t marker_vaddr = younger_target_vaddr + 0x80U;
    const uintptr_t marker_phys = younger_target_phys + 0x80U;
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t marker_value = 0x8a8b8c8d8e8f9091ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t younger_old[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t younger_new[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    for (unsigned i = 0; i < AI_MICRO_TRAP_ENTRY_VECTOR_BYTES; ++i) {
        younger_old[i] = (uint8_t)(0x20U + i);
        younger_new[i] = (uint8_t)(0xe0U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    ai_micro_trap_entry_write_bytes(younger_source_phys, younger_new,
        sizeof(younger_new));
    ai_micro_trap_entry_write_bytes(younger_target_phys, younger_old,
        sizeof(younger_old));
    write64(marker_phys, 0ULL);
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
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
    ai_micro_trap_entry_set_hs_leaf(younger_source_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    ai_micro_trap_entry_reset_local_state(fault_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_trap_entry_trap_scratch[
        sizeof(ai_micro_trap_entry_trap_scratch) /
        sizeof(ai_micro_trap_entry_trap_scratch[0])]);
    CSRW(stvec, ai_micro_ap8_skip_younger_vector_store_handler);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    TEST_ASSERT("AP-8 adjacent-vector setup stages the first vector-store source payload",
        excpt.triggered == false &&
        CSRR(CSR_VL) == active_vl
    );

    ai_micro_ap8_skip_younger_resume_epc =
        (uintptr_t)&&ap8_after_adjacent_younger_vector_store;
    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vsse8.v v8, (%[fault_dst]), %[stride]\n\t"
        "vsetvl t0, %[yvlen], %[vtype]\n\t"
        "vle8.v v8, (%[young_src])\n\t"
        "vse8.v v8, (%[young_dst])\n\t"
        ".option pop\n\t"
        :
        : [fault_dst] "r"(target_base_vaddr),
          [stride] "r"((intptr_t)PAGE_SIZE),
          [vl] "r"(requested_vl),
          [yvlen] "r"(AI_MICRO_TRAP_ENTRY_VECTOR_BYTES),
          [vtype] "r"(VTYPE(VSEW_FROM_BITS(8), 1)),
          [young_src] "r"(younger_source_vaddr),
          [young_dst] "r"(younger_target_vaddr)
        : "t0", "memory");

ap8_after_adjacent_younger_vector_store:
    excpt.testing = false;
    sd(marker_vaddr, marker_value);

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-8 adjacent-vector first store reports the late29 store access fault before the second vector store can become architectural",
        ai_micro_trap_entry_trap_cause == CAUSE_SAF &&
        ai_micro_trap_entry_trap_tval == fault_vaddr &&
        ai_micro_trap_entry_trap_vstart == fault_idx,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        ai_micro_trap_entry_trap_cause, ai_micro_trap_entry_trap_tval,
        fault_vaddr, ai_micro_trap_entry_trap_vstart
    );
    TEST_ASSERT("AP-8 adjacent-vector first store keeps its prefix/fault/suffix image precise while the skipped second vector target remains old",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        ai_micro_trap_entry_bytes_match(younger_target_phys, younger_old,
            sizeof(younger_old)) &&
        read64(marker_phys) == marker_value,
        "first29=0x%x old29=0x%x young0=0x%x old0=0x%x marker=0x%lx",
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx)),
        old_image[fault_idx], read8(younger_target_phys), younger_old[0],
        read64(marker_phys)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap8_prior_full_vector_store_wrap_pressure_late31_fault_prefix_survives_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const enum test_page warm_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t warm_vaddr = hs_page_base(warm_page) + 0x440U;
    const uintptr_t warm_phys = phys_page_base(warm_page) + 0x440U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page);
    const uintptr_t pressure_phys = phys_page_base(pressure_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t pressure_seed_a = 0x3197538642a0b1c2ULL;
    const uint64_t pressure_seed_b = 0x2c1b0a2468357913ULL;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t warm_old[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    uint8_t warm_new[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    bool pressure_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    for (unsigned i = 0; i < AI_MICRO_TRAP_ENTRY_VECTOR_BYTES; ++i) {
        warm_old[i] = (uint8_t)(0x50U + i);
        warm_new[i] = (uint8_t)(0xb0U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    ai_micro_trap_entry_write_bytes(warm_phys, warm_old, sizeof(warm_old));
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
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
    ai_micro_trap_entry_set_hs_leaf(warm_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(pressure_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_a,
        pressure_seed_b);
    for (unsigned round = 0; round < 4U; ++round) {
        for (unsigned i = 0; i < AI_MICRO_TRAP_ENTRY_VECTOR_BYTES; ++i) {
            warm_new[i] = (uint8_t)(0xb0U + round * 0x10U + i);
        }
        goto_priv(PRIV_M);
        ai_micro_trap_entry_write_bytes(source_phys, warm_new,
            sizeof(warm_new));
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
        TEST_ASSERT("AP-8 wrap-pressure warm vector load prepares a full vector store source without exception",
            excpt.triggered == false
        );
        TEST_SETUP_EXCEPT();
        ai_micro_ap8_vse8_from_v8_m2_unit((uint8_t *)warm_vaddr,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
        TEST_ASSERT("AP-8 wrap-pressure warm full vector store retires before the later 32-entry faulting vector store",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    TEST_ASSERT("AP-8 wrap-pressure setup restages the late31 source payload after prior vector store pressure",
        excpt.triggered == false &&
        CSRR(CSR_VL) == active_vl
    );
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr,
        PAGE_SIZE, requested_vl);
    const bool late31_triggered = excpt.triggered;
    const uint64_t late31_cause = excpt.cause;
    const uint64_t late31_tval = excpt.tval;
    const uint64_t late31_vstart = CSRR(CSR_VSTART);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    pressure_visible = ai_micro_trap_entry_dense_store_window_match(
        pressure_phys, AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT,
        pressure_seed_b);
    TEST_ASSERT("AP-8 wrap-pressure late31 vector store still reports the precise denied tail fault",
        late31_triggered == true &&
        late31_cause == CAUSE_SAF &&
        late31_tval == fault_vaddr,
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        late31_triggered, late31_cause, late31_tval, fault_vaddr,
        late31_vstart
    );
    TEST_ASSERT("AP-8 wrap-pressure late31 fault preserves active prefix0-30, keeps fault31 and inactive32 old, and keeps prior pressure/warm stores visible",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)) ==
            old_image[inactive_idx] &&
        ai_micro_trap_entry_bytes_match(warm_phys, warm_new,
            sizeof(warm_new)) &&
        pressure_visible,
        "prefix30=0x%x exp=0x%x fault31=0x%x old31=0x%x inactive32=0x%x old32=0x%x warm0=0x%x exp=0x%x",
        read8(ai_micro_trap_entry_strided_target_phys(30U)), new_image[30],
        read8(ai_micro_trap_entry_strided_target_phys(31U)), old_image[31],
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)),
        old_image[inactive_idx], read8(warm_phys), warm_new[0]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap8_late31_fault_then_cbo_zero_and_marker_no_stale_exception_state_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const enum test_page cbo_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t cbo_vaddr = hs_page_base(cbo_page) + 0x400U;
    const uintptr_t cbo_phys = phys_page_base(cbo_page) + 0x400U;
    const uintptr_t marker_vaddr = cbo_vaddr + AI_MICRO_TRAP_ENTRY_BLOCK_BYTES;
    const uintptr_t marker_phys = cbo_phys + AI_MICRO_TRAP_ENTRY_BLOCK_BYTES;
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t cbo_seed = 0x1029384756abcdefULL;
    const uint64_t marker_value = 0x0a0b0c0d0e0f1011ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_menvcfg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    ai_micro_trap_entry_write_bytes(source_phys, new_image, requested_vl);
    ai_micro_trap_entry_seed_block(cbo_phys, cbo_seed);
    write64(marker_phys, 0ULL);
    for (unsigned i = 0; i < requested_vl; ++i) {
        write8(ai_micro_trap_entry_strided_target_phys(i), old_image[i]);
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
    ai_micro_trap_entry_set_hs_leaf(cbo_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vle8_to_v8_m2((const uint8_t *)source_vaddr,
        requested_vl);
    TEST_ASSERT("AP-8 CBO-followup setup stages the late31 source payload with active vl=32",
        excpt.triggered == false &&
        CSRR(CSR_VL) == active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_vsse8_from_v8_m2((uint8_t *)target_base_vaddr,
        PAGE_SIZE, requested_vl);
    TEST_ASSERT("AP-8 CBO-followup first observes the precise late31 store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    ai_micro_trap_entry_restore_pmp_full_access();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_vaddr);
    TEST_ASSERT("AP-8 CBO-followup cbo.zero after the vector-store fault retires without inheriting stale exception state",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(marker_vaddr, marker_value);
    TEST_ASSERT("AP-8 CBO-followup scalar marker after cbo.zero also retires without stale vector exception state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-8 CBO-followup keeps vector fault image precise and applies only the intended cbo.zero plus marker effects",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)) ==
            old_image[inactive_idx] &&
        ai_micro_trap_entry_block_is_zero(cbo_phys) &&
        read64(marker_phys) == marker_value,
        "fault31=0x%x old31=0x%x inactive32=0x%x old32=0x%x cbo0=0x%lx marker=0x%lx",
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx)),
        old_image[fault_idx],
        read8(ai_micro_trap_entry_strided_target_phys(inactive_idx)),
        old_image[inactive_idx], read64(cbo_phys), read64(marker_phys)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap9_late31_lastflow_then_first_scalar_two_store_same_and_different_line_visible_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t same_line_vaddr = fault_vaddr + 8U;
    const uintptr_t same_line_phys =
        ai_micro_trap_entry_strided_target_phys(fault_idx) + 8U;
    const uintptr_t different_line_vaddr = hs_page_base(marker_page) + 0x480U;
    const uintptr_t different_line_phys = phys_page_base(marker_page) + 0x480U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page);
    const uintptr_t pressure_phys = phys_page_base(pressure_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t same_line_old = 0x1122334455667788ULL;
    const uint64_t same_line_new = 0x8877665544332211ULL;
    const uint64_t different_line_old = 0x0badbeef10203040ULL;
    const uint64_t different_line_new = 0x405060708090a0b0ULL;
    const uint64_t pressure_seed_a = 0x759351dbaeca8642ULL;
    const uint64_t pressure_seed_b = 0x2468ace0bdf13579ULL;
    const uint64_t pressure_seed_c = 0x3141592653589793ULL;
    const uint64_t pressure_seed_d = 0x2718281828459045ULL;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    bool late31_fault_ok = false;
    bool pressure_after_visible = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_ap9_seed_strided_source_and_targets(source_phys, old_image,
        new_image, requested_vl);
    write64(same_line_phys, same_line_old);
    write64(different_line_phys, different_line_old);
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
    TEST_ASSERT("AP-9 scalar-store setup creates dense StoreQueue pressure before the late31 lastFlow store fault",
        excpt.triggered == false
    );

    late31_fault_ok = ai_micro_ap9_run_late31_lastflow_fault(source_vaddr,
        target_base_vaddr, fault_vaddr, requested_vl, active_vl, fault_idx);
    TEST_ASSERT("AP-9 scalar-store setup reaches the precise late31 lastFlow store access fault before the first post-redirect store",
        late31_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(same_line_vaddr, same_line_new);
    TEST_ASSERT("AP-9 first post-redirect scalar store to the same cache line as the old fault element retires without stale lastFlow exception inheritance",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(different_line_vaddr, different_line_new);
    TEST_ASSERT("AP-9 second post-redirect scalar store to a different cache block also retires without stale exception inheritance",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_c,
        pressure_seed_d);
    TEST_ASSERT("AP-9 scalar-store follow-up keeps StoreQueue pressure active after the recovery stores",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    pressure_after_visible = ai_micro_trap_entry_dense_store_window_match(
        pressure_phys, AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT,
        pressure_seed_d);
    TEST_ASSERT("AP-9 scalar-store recovery keeps the late31 vector fault image precise while both same-line and different-block stores become visible",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        read64(same_line_phys) == same_line_new &&
        read64(different_line_phys) == different_line_new &&
        pressure_after_visible,
        "fault31=0x%x old31=0x%x same=0x%lx diff=0x%lx pressure=%d",
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx)),
        old_image[fault_idx], read64(same_line_phys),
        read64(different_line_phys), pressure_after_visible
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap9_late31_lastflow_then_first_cbo_zero_marker_and_cbom_owner_visible_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page cbo_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t cbo_zero_vaddr = hs_page_base(cbo_page) + 0x400U;
    const uintptr_t cbo_zero_phys = phys_page_base(cbo_page) + 0x400U;
    const uintptr_t marker_vaddr = hs_page_base(cbo_page) + 0x448U;
    const uintptr_t marker_phys = phys_page_base(cbo_page) + 0x448U;
    const uintptr_t owner_vaddr = hs_page_base(cbo_page) + 0x500U;
    const uintptr_t owner_phys = phys_page_base(cbo_page) + 0x500U;
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t zero_seed = 0x6a5b4c3d2e1f9081ULL;
    const uint64_t marker_value = 0x13572468ace0bdf1ULL;
    const uint64_t owner_old = 0x0102030405060708ULL;
    const uint64_t owner_new = 0xf8e7d6c5b4a39281ULL;
    uintptr_t old_medeleg = 0;
    uintptr_t old_menvcfg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    bool late31_fault_ok = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    old_menvcfg = CSRR(CSR_MENVCFG);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE01);
    ai_micro_ap9_seed_strided_source_and_targets(source_phys, old_image,
        new_image, requested_vl);
    ai_micro_trap_entry_seed_block(cbo_zero_phys, zero_seed);
    write64(marker_phys, 0ULL);
    write64(owner_phys, owner_old);

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
    ai_micro_trap_entry_set_hs_leaf(cbo_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    late31_fault_ok = ai_micro_ap9_run_late31_lastflow_fault(source_vaddr,
        target_base_vaddr, fault_vaddr, requested_vl, active_vl, fault_idx);
    TEST_ASSERT("AP-9 CBO setup reaches the precise late31 lastFlow store access fault before the first post-redirect cbo.zero",
        late31_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_zero_vaddr);
    TEST_ASSERT("AP-9 first post-redirect cbo.zero retires without stale lastFlow exception inheritance",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(marker_vaddr, marker_value);
    TEST_ASSERT("AP-9 scalar marker after the first cbo.zero also retires, distinguishing a CBO-only drop from longer stale exception inheritance",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(owner_vaddr, owner_new);
    cbo_clean(owner_vaddr);
    cbo_flush(owner_vaddr);
    cbo_inval(owner_vaddr);
    fence_iorw_iorw();
    TEST_ASSERT("AP-9 post-redirect CBO clean/flush/inval owner handoff does not hide the latest ordinary store",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    CSRW(CSR_MENVCFG, old_menvcfg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-9 CBO recovery keeps vector fault image precise, zeros the target block, preserves the marker, and keeps the CBO-owner store image",
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl) &&
        ai_micro_trap_entry_block_is_zero(cbo_zero_phys) &&
        read64(marker_phys) == marker_value &&
        read64(owner_phys) == owner_new,
        "zero0=0x%lx marker=0x%lx owner=0x%lx fault31=0x%x old31=0x%x",
        read64(cbo_zero_phys), read64(marker_phys), read64(owner_phys),
        read8(ai_micro_trap_entry_strided_target_phys(fault_idx)),
        old_image[fault_idx]
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap9_late31_lastflow_then_narrow_and_cross16_store_halves_visible_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page side_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t narrow_vaddr = hs_page_base(side_page) + 0x300U;
    const uintptr_t narrow_phys = phys_page_base(side_page) + 0x300U;
    const uintptr_t cross_base_vaddr = hs_page_base(side_page) + 0x500U;
    const uintptr_t cross_base_phys = phys_page_base(side_page) + 0x500U;
    const unsigned cross_store_offset = 12U;
    const uintptr_t cross_store_vaddr = cross_base_vaddr + cross_store_offset;
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t narrow_old = 0x0123456789abcdefULL;
    const uint8_t narrow_byte = 0x5aU;
    const uint16_t narrow_half = 0xc33cU;
    const uint64_t cross_value = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t cross_old[24];
    bool narrow_fault_ok = false;
    bool cross_fault_ok = false;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    for (unsigned i = 0; i < sizeof(cross_old); ++i) {
        cross_old[i] = (uint8_t)(0x70U + i);
    }

    goto_priv(PRIV_M);
    ai_micro_trap_entry_enable_vector_state();
    ai_micro_trap_entry_restore_pmp_full_access();
    ai_micro_ap9_seed_strided_source_and_targets(source_phys, old_image,
        new_image, requested_vl);
    write64(narrow_phys, narrow_old);
    ai_micro_trap_entry_write_bytes(cross_base_phys, cross_old,
        sizeof(cross_old));

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
    ai_micro_trap_entry_set_hs_leaf(side_page, leaf_flags);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();

    narrow_fault_ok = ai_micro_ap9_run_late31_lastflow_fault(source_vaddr,
        target_base_vaddr, fault_vaddr, requested_vl, active_vl, fault_idx);
    TEST_ASSERT("AP-9 narrow-store round reaches a precise late31 lastFlow store access fault before the first byte store",
        narrow_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sb(narrow_vaddr, narrow_byte);
    TEST_ASSERT("AP-9 first post-redirect byte store retires without being swallowed by stale lastFlow exception state",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sh(narrow_vaddr + 2U, narrow_half);
    TEST_ASSERT("AP-9 follow-up halfword store retires and checks lane merging separately from a whole-store drop",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_micro_ap9_seed_strided_source_and_targets(source_phys, old_image,
        new_image, requested_vl);
    ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
    sfence_vma();
    goto_priv(PRIV_HS);

    cross_fault_ok = ai_micro_ap9_run_late31_lastflow_fault(source_vaddr,
        target_base_vaddr, fault_vaddr, requested_vl, active_vl, fault_idx);
    TEST_ASSERT("AP-9 cross16-store round reaches a second precise late31 lastFlow fault before the first misaligned store",
        cross_fault_ok
    );

    goto_priv(PRIV_M);
    ai_micro_trap_entry_restore_pmp_full_access();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(cross_store_vaddr, cross_value);
    TEST_ASSERT("AP-9 first post-redirect misaligned cross16 doubleword store retires without losing either split half",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-9 narrow and cross16 recovery writes only the intended byte/halfword lanes and all bytes of the split doubleword",
        read8(narrow_phys) == narrow_byte &&
        read8(narrow_phys + 1U) == ((narrow_old >> 8U) & 0xffU) &&
        read8(narrow_phys + 2U) == (uint8_t)narrow_half &&
        read8(narrow_phys + 3U) == (uint8_t)(narrow_half >> 8U) &&
        ai_micro_ap9_cross16_sd_window_matches(cross_base_phys, cross_old,
            sizeof(cross_old), cross_store_offset, cross_value) &&
        ai_micro_trap_entry_strided_target_window_matches(expected_after_fault,
            active_vl),
        "narrow0=0x%x narrow1=0x%x narrow2=0x%x narrow3=0x%x cross12=0x%x cross19=0x%x",
        read8(narrow_phys), read8(narrow_phys + 1U),
        read8(narrow_phys + 2U), read8(narrow_phys + 3U),
        read8(cross_base_phys + cross_store_offset),
        read8(cross_base_phys + cross_store_offset + 7U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap9_late31_lastflow_then_first_vector_store_visible_under_repeated_recovery_manual_smoke_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL;
    const unsigned active_vl = AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page vector_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_trap_entry_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_trap_entry_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET;
    const uintptr_t vector_vaddr = hs_page_base(vector_page) + 0x600U;
    const uintptr_t vector_phys = phys_page_base(vector_page) + 0x600U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page);
    const uintptr_t pressure_phys = phys_page_base(pressure_page);
    const uintptr_t deny_start = phys_page_base(
        ai_micro_trap_entry_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t pressure_seed_a = 0x1020304050607080ULL;
    const uint64_t pressure_seed_b = 0x8877665544332211ULL;
    uintptr_t old_medeleg = 0;
    uint8_t old_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t new_image[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t expected_after_fault[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
    uint8_t vector_old[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES];
    bool all_rounds_ok = true;

    ai_micro_trap_entry_strided_fill_patterns(old_image, new_image,
        expected_after_fault, requested_vl, fault_idx);
    for (unsigned i = 0; i < sizeof(vector_old); ++i) {
        vector_old[i] = (uint8_t)(0x10U + i);
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
    ai_micro_trap_entry_set_hs_leaf(vector_page, leaf_flags);
    ai_micro_trap_entry_set_hs_leaf(pressure_page, leaf_flags);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_trap_entry_dense_store_pressure_window(pressure_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_a,
        pressure_seed_b);
    TEST_ASSERT("AP-9 repeated vector-store recovery starts with dense StoreQueue pressure before the repeated late31 faults",
        excpt.triggered == false
    );

    for (unsigned round = 0; round < 4U; ++round) {
        uint8_t round_new[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
        uint8_t round_expected[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL];
        uintptr_t round_vector_vaddr = vector_vaddr +
            round * AI_MICRO_TRAP_ENTRY_BLOCK_BYTES;
        uintptr_t round_vector_phys = vector_phys +
            round * AI_MICRO_TRAP_ENTRY_BLOCK_BYTES;
        bool late31_fault_ok = false;

        for (unsigned i = 0; i < requested_vl; ++i) {
            round_new[i] = (uint8_t)(new_image[i] + round * 0x11U);
            round_expected[i] = (i < fault_idx) ? round_new[i] : old_image[i];
        }

        goto_priv(PRIV_M);
        ai_micro_ap9_seed_strided_source_and_targets(source_phys, old_image,
            round_new, requested_vl);
        ai_micro_trap_entry_write_bytes(round_vector_phys, vector_old,
            sizeof(vector_old));
        ai_micro_trap_entry_configure_pmp_window(deny_start, deny_end,
            AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W);
        sfence_vma();

        late31_fault_ok = ai_micro_ap9_run_late31_lastflow_fault(source_vaddr,
            target_base_vaddr, fault_vaddr, requested_vl, active_vl,
            fault_idx);
        TEST_ASSERT("AP-9 repeated vector-store recovery round reaches a precise late31 lastFlow fault before the first vector store",
            late31_fault_ok,
            "round=%u", round
        );

        goto_priv(PRIV_M);
        ai_micro_trap_entry_restore_pmp_full_access();
        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_ap8_vse8_from_v8_m2_unit((uint8_t *)round_vector_vaddr,
            AI_MICRO_TRAP_ENTRY_VECTOR_BYTES);
        TEST_ASSERT("AP-9 first post-redirect vector store retires in each recovery round without inheriting the old lastFlow exception",
            excpt.triggered == false,
            "round=%u", round
        );

        goto_priv(PRIV_M);
        all_rounds_ok = all_rounds_ok &&
            ai_micro_trap_entry_bytes_match(round_vector_phys, round_new,
                AI_MICRO_TRAP_ENTRY_VECTOR_BYTES) &&
            ai_micro_trap_entry_strided_target_window_matches(round_expected,
                active_vl);
        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    ai_micro_trap_entry_restore_pmp_full_access();
    TEST_ASSERT("AP-9 repeated recovery keeps every first vector store visible and keeps the final late31 fault image precise under pressure",
        all_rounds_ok &&
        ai_micro_trap_entry_dense_store_window_match(pressure_phys,
            AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT, pressure_seed_b),
        "all_rounds_ok=%d vec0=0x%x expected0=0x%x pressure0=0x%lx",
        all_rounds_ok, read8(vector_phys), new_image[0],
        read64(pressure_phys)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
