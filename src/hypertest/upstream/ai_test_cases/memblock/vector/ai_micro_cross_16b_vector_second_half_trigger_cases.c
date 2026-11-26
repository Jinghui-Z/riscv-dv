#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_CROSS16_VEC_OFFSET 15U
#define AI_MICRO_CROSS16_VEC_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD 0x1ULL
#define AI_MICRO_CROSS16_VEC_MCONTROL6_STORE 0x2ULL

static inline uintptr_t ai_micro_cross16_vec_addr(enum test_page page)
{
    return hs_page_base(page) + AI_MICRO_CROSS16_VEC_OFFSET;
}

static inline void ai_micro_cross16_vec_write_bytes(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_cross16_vec_bytes_match(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ai_micro_cross16_vec_mcontrol6_tdata1(uint64_t access_bits)
{
    return AI_MICRO_CROSS16_VEC_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_micro_cross16_vec_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_micro_cross16_vec_arm_mcontrol6_slot(size_t slot,
    uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_micro_cross16_vec_mcontrol6_tdata1(access_bits));
}

static inline uint64_t ai_micro_cross16_vec_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_micro_cross16_vec_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline pte_t *ai_micro_cross16_vec_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_cross16_vec_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_micro_cross16_vec_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
    sfence_vma();
}

static inline void ai_micro_cross16_vec_vle_to_v6(unsigned sew_bits,
    uintptr_t vaddr,
    int vl)
{
    switch (sew_bits) {
    case 8:
        vle8_to_v6((const uint8_t *)vaddr, vl);
        break;
    case 16:
        vle16_to_v6((const uint16_t *)vaddr, vl);
        break;
    case 32:
        vle32_to_v6((const uint32_t *)vaddr, vl);
        break;
    default:
        vle64_to_v6((const uint64_t *)vaddr, vl);
        break;
    }
}

static inline void ai_micro_cross16_vec_vse_from_v6(unsigned sew_bits,
    uintptr_t vaddr,
    int vl)
{
    switch (sew_bits) {
    case 8:
        vse8_from_v6((uint8_t *)vaddr, vl);
        break;
    case 16:
        vse16_from_v6((uint16_t *)vaddr, vl);
        break;
    case 32:
        vse32_from_v6((uint32_t *)vaddr, vl);
        break;
    default:
        vse64_from_v6((uint64_t *)vaddr, vl);
        break;
    }
}

static inline void ai_micro_cross16_vec_fill_pattern(uint8_t *bytes,
    unsigned len,
    uint8_t seed)
{
    for (unsigned i = 0; i < len; ++i) {
        bytes[i] = (uint8_t)(seed + i * 3U);
    }
}

static inline void ai_micro_cross16_vec_masked_vle8_to_v6(uintptr_t vaddr,
    uintptr_t mask_vaddr,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vle8.v v6, (%[src]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(vaddr), [mask] "r"(mask_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_cross16_vec_masked_vse8_from_v6(uintptr_t vaddr,
    uintptr_t mask_vaddr,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vse8.v v6, (%[dst]), v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(vaddr), [mask] "r"(mask_vaddr), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_cross16_vec_vle8_vstart_to_v6(uintptr_t vaddr,
    unsigned vl,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8.v v6, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(vaddr), [vl] "r"(vl), [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_micro_cross16_vec_vse8_vstart_from_v6(uintptr_t vaddr,
    unsigned vl,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vse8.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(vaddr), [vl] "r"(vl), [start] "r"(start)
        : "t0", "memory");
}

static inline void ai_micro_cross16_vec_vlse8_to_v6(uintptr_t vaddr,
    intptr_t stride,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vlse8.v v6, (%[src]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(vaddr), [stride] "r"(stride), [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_cross16_vec_vsse8_from_v6(uintptr_t vaddr,
    intptr_t stride,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vsse8.v v6, (%[dst]), %[stride]\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(vaddr), [stride] "r"(stride), [vl] "r"(vl)
        : "t0", "memory");
}

bool ai_micro_ap17_vector_load_split_element_second_flow_address_trigger_ignored_manual_smoke_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = SWITCH1;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t data_vaddr = ai_micro_cross16_vec_addr(data_page);
    const uintptr_t second_flow_vaddr = data_vaddr + 1U;
    const uintptr_t data_phys = phys_page_base(data_page) + AI_MICRO_CROSS16_VEC_OFFSET;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x240U;
    const uintptr_t old_phys = phys_page_base(old_page) + 0x240U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x280U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x280U;
    const uintptr_t data_before_addr = data_phys - 1U;
    const uintptr_t data_after_addr = data_phys + 4U;
    const uintptr_t scratch_before_addr = scratch_phys - 1U;
    const uintptr_t scratch_after_addr = scratch_phys + 4U;
    const uint8_t data_bytes[4] = { 0x91U, 0x92U, 0x93U, 0x94U };
    const uint8_t old_vd_bytes[4] = { 0x11U, 0x12U, 0x13U, 0x14U };
    const uint8_t data_before = 0x47U;
    const uint8_t data_after = 0x74U;
    const uint8_t scratch_before = 0x58U;
    const uint8_t scratch_after = 0x85U;
    uint64_t observed_vstart = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(data_phys, data_bytes, sizeof(data_bytes));
    ai_micro_cross16_vec_write_bytes(old_phys, old_vd_bytes, sizeof(old_vd_bytes));
    ai_micro_cross16_vec_write_bytes(scratch_phys, old_vd_bytes, sizeof(old_vd_bytes));
    write8(data_before_addr, data_before);
    write8(data_after_addr, data_after);
    write8(scratch_before_addr, scratch_before);
    write8(scratch_after_addr, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 1);
    TEST_ASSERT("the preload vector load seeds v6 before the AP-17 split-element second-flow load-trigger probe",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, second_flow_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)data_vaddr, 1);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a single e32 unit-stride vector load element starting at base+15 crosses the 16B boundary but must ignore an address trigger armed only at its internal second-flow start",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 1);
    TEST_ASSERT("after the AP-17 split-element load-trigger probe, dumping v6 completes so the loaded element image can be checked",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-17 split-element load-trigger probe loads the full 4-byte element and preserves source/scratch guards",
        ai_micro_cross16_vec_bytes_match(scratch_phys, data_bytes,
            sizeof(data_bytes)) &&
        ai_micro_cross16_vec_bytes_match(data_phys, data_bytes,
            sizeof(data_bytes)) &&
        read8(data_before_addr) == data_before &&
        read8(data_after_addr) == data_after &&
        read8(scratch_before_addr) == scratch_before &&
        read8(scratch_after_addr) == scratch_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap17_vector_store_split_element_second_flow_address_trigger_ignored_manual_smoke_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t data_vaddr = ai_micro_cross16_vec_addr(data_page);
    const uintptr_t second_flow_vaddr = data_vaddr + 1U;
    const uintptr_t data_phys = phys_page_base(data_page) + AI_MICRO_CROSS16_VEC_OFFSET;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x2c0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x2c0U;
    const uintptr_t data_before_addr = data_phys - 1U;
    const uintptr_t data_after_addr = data_phys + 4U;
    const uint8_t initial_data[4] = { 0x21U, 0x22U, 0x23U, 0x24U };
    const uint8_t src_bytes[4] = { 0xc1U, 0xc2U, 0xc3U, 0xc4U };
    const uint8_t data_before = 0x6dU;
    const uint8_t data_after = 0xd6U;
    uint64_t observed_vstart = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(data_phys, initial_data, sizeof(initial_data));
    ai_micro_cross16_vec_write_bytes(src_phys, src_bytes, sizeof(src_bytes));
    write8(data_before_addr, data_before);
    write8(data_after_addr, data_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 1);
    TEST_ASSERT("the source vector load stages one e32 element before the AP-17 split-element second-flow store-trigger probe",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, second_flow_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)data_vaddr, 1);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a single e32 unit-stride vector store element starting at base+15 crosses the 16B boundary but must ignore an address trigger armed only at its internal second-flow start",
        excpt.triggered == false &&
        observed_vstart == 0ULL
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("the AP-17 split-element store-trigger probe commits the full 4-byte element, preserves the staged source, and leaves adjacent guards intact",
        ai_micro_cross16_vec_bytes_match(data_phys, src_bytes,
            sizeof(src_bytes)) &&
        ai_micro_cross16_vec_bytes_match(src_phys, src_bytes,
            sizeof(src_bytes)) &&
        read8(data_before_addr) == data_before &&
        read8(data_after_addr) == data_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_load_cross_16b_samepage_second_element_address_trigger_vstart1_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page old_page = SWITCH1;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t data_vaddr = ai_micro_cross16_vec_addr(data_page);
    const uintptr_t trigger_vaddr = data_vaddr + 1U;
    const uintptr_t data_phys = phys_page_base(data_page) + AI_MICRO_CROSS16_VEC_OFFSET;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x180U;
    const uintptr_t old_phys = phys_page_base(old_page) + 0x180U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x1c0U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x1c0U;
    const uintptr_t data_before_addr = data_phys - 1U;
    const uintptr_t data_after_addr = data_phys + 2U;
    const uintptr_t scratch_before_addr = scratch_phys - 1U;
    const uintptr_t scratch_after_addr = scratch_phys + 2U;
    const uint8_t data_bytes[2] = { 0xa5U, 0x5aU };
    const uint8_t old_vd_bytes[2] = { 0x11U, 0x22U };
    const uint8_t expected_vd_bytes[2] = { 0xa5U, 0x22U };
    const uint8_t data_before = 0x3dU;
    const uint8_t data_after = 0xd3U;
    const uint8_t scratch_before = 0x4eU;
    const uint8_t scratch_after = 0xe4U;
    uint64_t observed_vstart = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(data_phys, data_bytes, sizeof(data_bytes));
    ai_micro_cross16_vec_write_bytes(old_phys, old_vd_bytes, sizeof(old_vd_bytes));
    write8(data_before_addr, data_before);
    write8(data_after_addr, data_after);
    write8(scratch_before_addr, scratch_before);
    write8(scratch_after_addr, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)old_vaddr, 2);
    TEST_ASSERT("the preload vector load seeds v6 before the same-page cross-16B second-element trigger probe",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)data_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a same-page unit-stride vle8.v starting at base+15 crosses the 16B boundary and reports breakpoint on element1 when only base+16 is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == trigger_vaddr &&
        observed_vstart == 1ULL
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 2);
    TEST_ASSERT("after the vector load breakpoint, dumping v6 from vstart=0 completes so the precise destination footprint is visible",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-page vector load trigger updates only element0 before the base+16 element1 breakpoint and preserves source/scratch sentinels",
        ai_micro_cross16_vec_bytes_match(scratch_phys, expected_vd_bytes,
            sizeof(expected_vd_bytes)) &&
        ai_micro_cross16_vec_bytes_match(data_phys, data_bytes, sizeof(data_bytes)) &&
        read8(data_before_addr) == data_before &&
        read8(data_after_addr) == data_after &&
        read8(scratch_before_addr) == scratch_before &&
        read8(scratch_after_addr) == scratch_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_cross_16b_samepage_second_element_address_trigger_first_element_only_corner()
{
    TEST_START();

    const enum test_page data_page = VSRWX_GRWX;
    const enum test_page src_page = SWITCH1;
    const uintptr_t data_vaddr = ai_micro_cross16_vec_addr(data_page);
    const uintptr_t trigger_vaddr = data_vaddr + 1U;
    const uintptr_t data_phys = phys_page_base(data_page) + AI_MICRO_CROSS16_VEC_OFFSET;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x220U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x220U;
    const uintptr_t data_before_addr = data_phys - 1U;
    const uintptr_t data_after_addr = data_phys + 2U;
    const uint8_t initial_data[2] = { 0x31U, 0x32U };
    const uint8_t src_bytes[2] = { 0xc1U, 0xc2U };
    const uint8_t expected_after_trap[2] = { 0xc1U, 0x32U };
    const uint8_t data_before = 0x6bU;
    const uint8_t data_after = 0xb6U;
    uint64_t observed_vstart = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(data_phys, initial_data, sizeof(initial_data));
    ai_micro_cross16_vec_write_bytes(src_phys, src_bytes, sizeof(src_bytes));
    write8(data_before_addr, data_before);
    write8(data_after_addr, data_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)src_vaddr, 2);
    TEST_ASSERT("the source vector load stages two bytes before the same-page cross-16B second-element store-trigger probe",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)data_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a same-page unit-stride vse8.v starting at base+15 crosses the 16B boundary and reports breakpoint on element1 when only base+16 is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == trigger_vaddr &&
        observed_vstart == 1ULL
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-page vector store trigger commits only element0 before the base+16 element1 breakpoint and leaves adjacent bytes intact",
        ai_micro_cross16_vec_bytes_match(data_phys, expected_after_trap,
            sizeof(expected_after_trap)) &&
        ai_micro_cross16_vec_bytes_match(src_phys, src_bytes, sizeof(src_bytes)) &&
        read8(data_before_addr) == data_before &&
        read8(data_after_addr) == data_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_us_vector_trigger_fullva_matrix_manual_corner()
{
    TEST_START();

    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const unsigned sew_bits[] = { 16U, 32U, 64U };
    const uintptr_t load_data_base = 0x300U;
    const uintptr_t load_old_base = 0x600U;
    const uintptr_t load_scratch_base = 0x900U;
    const uintptr_t store_data_base = 0xc00U;
    const uintptr_t store_src_base = 0xf00U;
    uint64_t saved_mstatus = 0;

    for (unsigned i = 0; i < sizeof(sew_bits) / sizeof(sew_bits[0]); ++i) {
        const unsigned sew = sew_bits[i];
        const unsigned elem_bytes = sew / 8U;
        const unsigned total_bytes = elem_bytes * 2U;
        const uintptr_t load_data_off = load_data_base + i * 0x80U + 15U;
        const uintptr_t load_old_off = load_old_base + i * 0x80U;
        const uintptr_t load_scratch_off = load_scratch_base + i * 0x80U;
        const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_data_off;
        const uintptr_t load_old_vaddr =
            hs_page_base(load_old_page) + load_old_off;
        const uintptr_t load_scratch_vaddr =
            hs_page_base(load_scratch_page) + load_scratch_off;
        const uintptr_t load_phys = phys_page_base(load_data_page) + load_data_off;
        const uintptr_t load_old_phys =
            phys_page_base(load_old_page) + load_old_off;
        const uintptr_t load_scratch_phys =
            phys_page_base(load_scratch_page) + load_scratch_off;
        const uintptr_t trigger_vaddr = load_vaddr + elem_bytes;
        uint8_t data_bytes[16] = { 0 };
        uint8_t old_bytes[16] = { 0 };
        uint8_t expected_load[16] = { 0 };
        uint8_t scratch_guard_lo = (uint8_t)(0x30U + i);
        uint8_t scratch_guard_hi = (uint8_t)(0x40U + i);
        uint64_t observed_vstart = 0;

        ai_micro_cross16_vec_fill_pattern(data_bytes, total_bytes,
            (uint8_t)(0x51U + i * 0x11U));
        ai_micro_cross16_vec_fill_pattern(old_bytes, total_bytes,
            (uint8_t)(0x91U + i * 0x11U));
        for (unsigned j = 0; j < elem_bytes; ++j) {
            expected_load[j] = data_bytes[j];
            expected_load[elem_bytes + j] = old_bytes[elem_bytes + j];
        }

        goto_priv(PRIV_M);
        ai_micro_cross16_vec_write_bytes(load_phys, data_bytes, total_bytes);
        ai_micro_cross16_vec_write_bytes(load_old_phys, old_bytes, total_bytes);
        ai_micro_cross16_vec_write_bytes(load_scratch_phys, old_bytes,
            total_bytes);
        write8(load_scratch_phys - 1U, scratch_guard_lo);
        write8(load_scratch_phys + total_bytes, scratch_guard_hi);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_cross16_vec_vle_to_v6(sew, load_old_vaddr, 2);
        TEST_ASSERT("AP-41/AP-81 preload seeds v6 before an e16/e32/e64 unit-stride load trigger-metadata probe",
            excpt.triggered == false,
            "sew=%u", sew
        );

        ai_micro_cross16_vec_clear_mcontrol6_slot(0);
        ai_micro_cross16_vec_arm_mcontrol6_slot(0, trigger_vaddr,
            AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
        saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_cross16_vec_vle_to_v6(sew, load_vaddr, 2);
        observed_vstart = CSRR(CSR_VSTART);
        TEST_ASSERT("AP-41/AP-81 e16/e32/e64 unit-stride load trigger reports the non-first element fullVa/mtval and architectural vstart",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP &&
            excpt.tval == trigger_vaddr &&
            observed_vstart == 1ULL,
            "sew=%u cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
            sew,
            (unsigned long long)excpt.cause,
            (unsigned long long)excpt.tval,
            (unsigned long long)trigger_vaddr,
            (unsigned long long)observed_vstart
        );

        ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
        ai_micro_cross16_vec_clear_mcontrol6_slot(0);

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_cross16_vec_vse_from_v6(sew, load_scratch_vaddr, 2);
        TEST_ASSERT("AP-41/AP-81 e16/e32/e64 load trigger dump completes after clearing the trigger so prefix/suffix data can be checked",
            excpt.triggered == false,
            "sew=%u", sew
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-41/AP-81 e16/e32/e64 load trigger keeps only the prefix element updated before the triggered non-first element",
            ai_micro_cross16_vec_bytes_match(load_scratch_phys, expected_load,
                total_bytes) &&
            ai_micro_cross16_vec_bytes_match(load_phys, data_bytes,
                total_bytes) &&
            read8(load_scratch_phys - 1U) == scratch_guard_lo &&
            read8(load_scratch_phys + total_bytes) == scratch_guard_hi,
            "sew=%u", sew
        );

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        hspt_init();
        sfence_vma();
    }

    for (unsigned i = 0; i < sizeof(sew_bits) / sizeof(sew_bits[0]); ++i) {
        const unsigned sew = sew_bits[i];
        const unsigned elem_bytes = sew / 8U;
        const unsigned total_bytes = elem_bytes * 2U;
        const uintptr_t store_data_off = store_data_base + i * 0x80U + 15U;
        const uintptr_t store_src_off = store_src_base + i * 0x80U;
        const uintptr_t store_vaddr =
            hs_page_base(store_data_page) + store_data_off;
        const uintptr_t store_src_vaddr =
            hs_page_base(store_src_page) + store_src_off;
        const uintptr_t store_phys =
            phys_page_base(store_data_page) + store_data_off;
        const uintptr_t store_src_phys =
            phys_page_base(store_src_page) + store_src_off;
        const uintptr_t trigger_vaddr = store_vaddr + elem_bytes;
        uint8_t initial_bytes[16] = { 0 };
        uint8_t src_bytes[16] = { 0 };
        uint8_t expected_store[16] = { 0 };
        uint8_t data_guard_lo = (uint8_t)(0x60U + i);
        uint8_t data_guard_hi = (uint8_t)(0x70U + i);
        uint64_t observed_vstart = 0;

        ai_micro_cross16_vec_fill_pattern(initial_bytes, total_bytes,
            (uint8_t)(0x21U + i * 0x11U));
        ai_micro_cross16_vec_fill_pattern(src_bytes, total_bytes,
            (uint8_t)(0xc1U + i * 0x11U));
        for (unsigned j = 0; j < elem_bytes; ++j) {
            expected_store[j] = src_bytes[j];
            expected_store[elem_bytes + j] = initial_bytes[elem_bytes + j];
        }

        goto_priv(PRIV_M);
        ai_micro_cross16_vec_write_bytes(store_phys, initial_bytes,
            total_bytes);
        ai_micro_cross16_vec_write_bytes(store_src_phys, src_bytes,
            total_bytes);
        write8(store_phys - 1U, data_guard_lo);
        write8(store_phys + total_bytes, data_guard_hi);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_cross16_vec_vle_to_v6(sew, store_src_vaddr, 2);
        TEST_ASSERT("AP-41/AP-81 source vector preload completes before the e16/e32/e64 unit-stride store trigger-metadata probe",
            excpt.triggered == false,
            "sew=%u", sew
        );

        ai_micro_cross16_vec_clear_mcontrol6_slot(0);
        ai_micro_cross16_vec_arm_mcontrol6_slot(0, trigger_vaddr,
            AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
        saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_cross16_vec_vse_from_v6(sew, store_vaddr, 2);
        observed_vstart = CSRR(CSR_VSTART);
        TEST_ASSERT("AP-41/AP-81 e16/e32/e64 unit-stride store trigger reports the non-first element fullVa/mtval and architectural vstart",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP &&
            excpt.tval == trigger_vaddr &&
            observed_vstart == 1ULL,
            "sew=%u cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
            sew,
            (unsigned long long)excpt.cause,
            (unsigned long long)excpt.tval,
            (unsigned long long)trigger_vaddr,
            (unsigned long long)observed_vstart
        );

        ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
        ai_micro_cross16_vec_clear_mcontrol6_slot(0);

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-41/AP-81 e16/e32/e64 store trigger commits only prefix bytes before the non-first breakpoint cutoff and preserves guards",
            ai_micro_cross16_vec_bytes_match(store_phys, expected_store,
                total_bytes) &&
            ai_micro_cross16_vec_bytes_match(store_src_phys, src_bytes,
                total_bytes) &&
            read8(store_phys - 1U) == data_guard_lo &&
            read8(store_phys + total_bytes) == data_guard_hi,
            "sew=%u", sew
        );

        goto_priv(PRIV_HS);
        CSRW(CSR_VSTART, 0ULL);
        hspt_init();
        sfence_vma();
    }

    TEST_END();
}

bool ai_micro_ap41_ap81_masked_active_trigger_fullva_manual_corner()
{
    TEST_START();

    const unsigned vl = 12U;
    const unsigned trigger_index = 8U;
    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const enum test_page mask_page = SWITCH2;
    const uintptr_t load_off = 0x128U + 9U;
    const uintptr_t load_old_off = 0x138U;
    const uintptr_t load_scratch_off = 0x148U;
    const uintptr_t store_off = 0x158U + 9U;
    const uintptr_t store_src_off = 0x168U;
    const uintptr_t mask_off = 0x178U;
    const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_off;
    const uintptr_t load_old_vaddr = hs_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_vaddr =
        hs_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_vaddr = hs_page_base(store_data_page) + store_off;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + store_src_off;
    const uintptr_t mask_vaddr = hs_page_base(mask_page) + mask_off;
    const uintptr_t load_phys = phys_page_base(load_data_page) + load_off;
    const uintptr_t load_old_phys = phys_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_phys =
        phys_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_phys = phys_page_base(store_data_page) + store_off;
    const uintptr_t store_src_phys = phys_page_base(store_src_page) + store_src_off;
    const uintptr_t mask_phys = phys_page_base(mask_page) + mask_off;
    const uintptr_t load_trigger_vaddr = load_vaddr + trigger_index;
    const uintptr_t store_trigger_vaddr = store_vaddr + trigger_index;
    const uint8_t mask_bytes[2] = { 0x02U, 0x01U };
    uint8_t load_data[16] = { 0 };
    uint8_t load_old[16] = { 0 };
    uint8_t load_expected[16] = { 0 };
    uint8_t store_initial[16] = { 0 };
    uint8_t store_src[16] = { 0 };
    uint8_t store_expected[16] = { 0 };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_micro_cross16_vec_fill_pattern(load_data, vl, 0x21U);
    ai_micro_cross16_vec_fill_pattern(load_old, vl, 0x81U);
    ai_micro_cross16_vec_fill_pattern(store_initial, vl, 0x41U);
    ai_micro_cross16_vec_fill_pattern(store_src, vl, 0xc1U);
    for (unsigned i = 0; i < vl; ++i) {
        load_expected[i] = load_old[i];
        store_expected[i] = store_initial[i];
    }
    load_expected[1] = load_data[1];
    store_expected[1] = store_src[1];

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(load_phys, load_data, vl);
    ai_micro_cross16_vec_write_bytes(load_old_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(load_scratch_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(store_phys, store_initial, vl);
    ai_micro_cross16_vec_write_bytes(store_src_phys, store_src, vl);
    ai_micro_cross16_vec_write_bytes(mask_phys, mask_bytes,
        sizeof(mask_bytes));

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_old_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 masked-active load setup preloads old destination bytes before separating first-active and trigger-active elements",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, load_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_masked_vle8_to_v6(load_vaddr, mask_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 masked unit-stride load reports the active line2 trigger element address instead of the first active element address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 masked-active load dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 masked-active load updates only the older active prefix lane before the later breakpoint lane",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys, load_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 masked-active store setup stages source bytes before separating first-active and trigger-active elements",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, store_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_masked_vse8_from_v6(store_vaddr, mask_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 masked unit-stride store reports the active line2 trigger element address and cutoff index",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)store_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 masked-active store commits only the older active prefix lane and preserves the trigger/suffix lanes",
        ai_micro_cross16_vec_bytes_match(store_phys, store_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, store_src, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_masked_off_trigger_ignored_manual_corner()
{
    TEST_START();

    const unsigned vl = 12U;
    const unsigned masked_off_index = 4U;
    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const enum test_page mask_page = SWITCH2;
    const uintptr_t load_off = 0x248U + 9U;
    const uintptr_t load_old_off = 0x258U;
    const uintptr_t load_scratch_off = 0x268U;
    const uintptr_t store_off = 0x278U + 9U;
    const uintptr_t store_src_off = 0x288U;
    const uintptr_t mask_off = 0x298U;
    const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_off;
    const uintptr_t load_old_vaddr = hs_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_vaddr =
        hs_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_vaddr = hs_page_base(store_data_page) + store_off;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + store_src_off;
    const uintptr_t mask_vaddr = hs_page_base(mask_page) + mask_off;
    const uintptr_t load_phys = phys_page_base(load_data_page) + load_off;
    const uintptr_t load_old_phys = phys_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_phys =
        phys_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_phys = phys_page_base(store_data_page) + store_off;
    const uintptr_t store_src_phys = phys_page_base(store_src_page) + store_src_off;
    const uintptr_t mask_phys = phys_page_base(mask_page) + mask_off;
    const uintptr_t load_trigger_vaddr = load_vaddr + masked_off_index;
    const uintptr_t store_trigger_vaddr = store_vaddr + masked_off_index;
    const uint8_t mask_bytes[2] = { 0x02U, 0x01U };
    uint8_t load_data[16] = { 0 };
    uint8_t load_old[16] = { 0 };
    uint8_t load_expected[16] = { 0 };
    uint8_t store_initial[16] = { 0 };
    uint8_t store_src[16] = { 0 };
    uint8_t store_expected[16] = { 0 };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_micro_cross16_vec_fill_pattern(load_data, vl, 0x31U);
    ai_micro_cross16_vec_fill_pattern(load_old, vl, 0x91U);
    ai_micro_cross16_vec_fill_pattern(store_initial, vl, 0x51U);
    ai_micro_cross16_vec_fill_pattern(store_src, vl, 0xd1U);
    for (unsigned i = 0; i < vl; ++i) {
        load_expected[i] = load_old[i];
        store_expected[i] = store_initial[i];
    }
    load_expected[1] = load_data[1];
    load_expected[8] = load_data[8];
    store_expected[1] = store_src[1];
    store_expected[8] = store_src[8];

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(load_phys, load_data, vl);
    ai_micro_cross16_vec_write_bytes(load_old_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(load_scratch_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(store_phys, store_initial, vl);
    ai_micro_cross16_vec_write_bytes(store_src_phys, store_src, vl);
    ai_micro_cross16_vec_write_bytes(mask_phys, mask_bytes,
        sizeof(mask_bytes));

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_old_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 masked-off load setup preloads old destination bytes before arming a trigger on an inactive lane",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, load_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_masked_vle8_to_v6(load_vaddr, mask_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 masked-off unit-stride load suppresses an address trigger whose element is inactive",
        excpt.triggered == false &&
        observed_vstart == 0ULL,
        "cause=0x%llx tval=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 masked-off load dump completes after the inactive trigger probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 masked-off load updates both active lanes and leaves the inactive trigger lane undisturbed",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys, load_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 masked-off store setup stages source bytes before arming a trigger on an inactive lane",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, store_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_masked_vse8_from_v6(store_vaddr, mask_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 masked-off unit-stride store suppresses an address trigger whose element is inactive",
        excpt.triggered == false &&
        observed_vstart == 0ULL,
        "cause=0x%llx tval=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 masked-off store commits only active lanes and does not treat the inactive matching address as the cutoff",
        ai_micro_cross16_vec_bytes_match(store_phys, store_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, store_src, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_vstart_restart_trigger_fullva_manual_corner()
{
    TEST_START();

    const unsigned vl = 8U;
    const unsigned start_index = 3U;
    const unsigned trigger_index = 5U;
    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const uintptr_t load_off = 0x348U + 11U;
    const uintptr_t load_old_off = 0x358U;
    const uintptr_t load_scratch_off = 0x368U;
    const uintptr_t store_off = 0x378U + 11U;
    const uintptr_t store_src_off = 0x388U;
    const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_off;
    const uintptr_t load_old_vaddr = hs_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_vaddr =
        hs_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_vaddr = hs_page_base(store_data_page) + store_off;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + store_src_off;
    const uintptr_t load_phys = phys_page_base(load_data_page) + load_off;
    const uintptr_t load_old_phys = phys_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_phys =
        phys_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_phys = phys_page_base(store_data_page) + store_off;
    const uintptr_t store_src_phys = phys_page_base(store_src_page) + store_src_off;
    const uintptr_t load_trigger_vaddr = load_vaddr + trigger_index;
    const uintptr_t store_trigger_vaddr = store_vaddr + trigger_index;
    uint8_t load_data[16] = { 0 };
    uint8_t load_old[16] = { 0 };
    uint8_t load_expected[16] = { 0 };
    uint8_t store_initial[16] = { 0 };
    uint8_t store_src[16] = { 0 };
    uint8_t store_expected[16] = { 0 };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    for (unsigned i = 0; i < vl; ++i) {
        load_data[i] = 0x02U;
        load_old[i] = 0x01U;
    }
    ai_micro_cross16_vec_fill_pattern(store_initial, vl, 0x71U);
    ai_micro_cross16_vec_fill_pattern(store_src, vl, 0xe1U);
    for (unsigned i = 0; i < vl; ++i) {
        load_expected[i] = load_old[i];
        store_expected[i] = store_initial[i];
    }
    load_expected[3] = load_data[3];
    load_expected[4] = load_data[4];
    store_expected[3] = store_src[3];
    store_expected[4] = store_src[4];

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(load_phys, load_data, vl);
    ai_micro_cross16_vec_write_bytes(load_old_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(load_scratch_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(store_phys, store_initial, vl);
    ai_micro_cross16_vec_write_bytes(store_src_phys, store_src, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_old_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 vstart-restart load setup preloads old destination bytes before resuming at element3",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, load_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_vle8_vstart_to_v6(load_vaddr, vl, start_index);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 unit-stride load resumed with nonzero vstart reports the absolute trigger element index and fullVa",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 vstart-restart load dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 vstart-restart load updates only resumed prefix elements before the later trigger element",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys, load_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 vstart-restart store setup stages source bytes before resuming at element3",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, store_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_vse8_vstart_from_v6(store_vaddr, vl, start_index);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 unit-stride store resumed with nonzero vstart reports the absolute trigger element index and fullVa",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)store_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 vstart-restart store commits only resumed prefix elements before the later trigger cutoff",
        ai_micro_cross16_vec_bytes_match(store_phys, store_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, store_src, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_non_us_strided_trigger_gate_manual_corner()
{
    TEST_START();

    const unsigned vl = 4U;
    const intptr_t stride = 2;
    const unsigned trigger_index = 1U;
    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const uintptr_t load_off = 0x448U + 5U;
    const uintptr_t load_old_off = 0x458U;
    const uintptr_t load_scratch_off = 0x468U;
    const uintptr_t store_off = 0x478U + 5U;
    const uintptr_t store_src_off = 0x488U;
    const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_off;
    const uintptr_t load_old_vaddr = hs_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_vaddr =
        hs_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_vaddr = hs_page_base(store_data_page) + store_off;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + store_src_off;
    const uintptr_t load_phys = phys_page_base(load_data_page) + load_off;
    const uintptr_t load_old_phys = phys_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_phys =
        phys_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_phys = phys_page_base(store_data_page) + store_off;
    const uintptr_t store_src_phys = phys_page_base(store_src_page) + store_src_off;
    const uintptr_t load_trigger_vaddr = load_vaddr + (uintptr_t)stride * trigger_index;
    const uintptr_t store_trigger_vaddr = store_vaddr + (uintptr_t)stride * trigger_index;
    const uint8_t load_data[8] = {
        0x10U, 0xa1U, 0x20U, 0xa2U, 0x30U, 0xa3U, 0x40U, 0xa4U
    };
    const uint8_t store_initial[8] = {
        0x50U, 0xb1U, 0x60U, 0xb2U, 0x70U, 0xb3U, 0x80U, 0xb4U
    };
    const uint8_t load_old[4] = { 0x91U, 0x92U, 0x93U, 0x94U };
    const uint8_t store_src[4] = { 0xd1U, 0xd2U, 0xd3U, 0xd4U };
    const uint8_t load_expected[4] = { 0x10U, 0x92U, 0x93U, 0x94U };
    const uint8_t store_expected[8] = {
        0xd1U, 0xb1U, 0x60U, 0xb2U, 0x70U, 0xb3U, 0x80U, 0xb4U
    };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(load_phys, load_data,
        sizeof(load_data));
    ai_micro_cross16_vec_write_bytes(load_old_phys, load_old,
        sizeof(load_old));
    ai_micro_cross16_vec_write_bytes(load_scratch_phys, load_old,
        sizeof(load_old));
    ai_micro_cross16_vec_write_bytes(store_phys, store_initial,
        sizeof(store_initial));
    ai_micro_cross16_vec_write_bytes(store_src_phys, store_src,
        sizeof(store_src));

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_old_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 non-US strided load setup preloads old destination bytes before probing the unit-stride triggerMask gate",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, load_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_vlse8_to_v6(load_vaddr, stride, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 strided load address trigger reports the true strided element VA instead of a unit-stride contiguous-mask VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 non-US strided load dump completes after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 non-US strided load updates only the older strided element before the breakpoint element",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys, load_expected,
            sizeof(load_expected)) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data,
            sizeof(load_data))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 non-US strided store setup stages source bytes before probing the unit-stride triggerMask gate",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, store_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_cross16_vec_vsse8_from_v6(store_vaddr, stride, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 strided store address trigger reports the true strided element VA instead of a unit-stride contiguous-mask VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)store_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 non-US strided store commits only the older strided element and leaves gap/trigger/suffix bytes intact",
        ai_micro_cross16_vec_bytes_match(store_phys, store_expected,
            sizeof(store_expected)) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, store_src,
            sizeof(store_src))
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_dual_trigger_priority_fullva_manual_corner()
{
    TEST_START();

    const unsigned vl = 8U;
    const unsigned first_trigger_index = 2U;
    const unsigned second_trigger_index = 5U;
    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const uintptr_t load_off = 0x548U + 9U;
    const uintptr_t load_old_off = 0x558U;
    const uintptr_t load_scratch_off = 0x568U;
    const uintptr_t store_off = 0x578U + 9U;
    const uintptr_t store_src_off = 0x588U;
    const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_off;
    const uintptr_t load_old_vaddr = hs_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_vaddr =
        hs_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_vaddr = hs_page_base(store_data_page) + store_off;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + store_src_off;
    const uintptr_t load_phys = phys_page_base(load_data_page) + load_off;
    const uintptr_t load_old_phys = phys_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_phys =
        phys_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t store_phys = phys_page_base(store_data_page) + store_off;
    const uintptr_t store_src_phys = phys_page_base(store_src_page) + store_src_off;
    const uintptr_t load_trigger0_vaddr = load_vaddr + first_trigger_index;
    const uintptr_t load_trigger1_vaddr = load_vaddr + second_trigger_index;
    const uintptr_t store_trigger0_vaddr = store_vaddr + first_trigger_index;
    const uintptr_t store_trigger1_vaddr = store_vaddr + second_trigger_index;
    uint8_t load_data[16] = { 0 };
    uint8_t load_old[16] = { 0 };
    uint8_t load_expected[16] = { 0 };
    uint8_t store_initial[16] = { 0 };
    uint8_t store_src[16] = { 0 };
    uint8_t store_expected[16] = { 0 };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_micro_cross16_vec_fill_pattern(load_data, vl, 0x24U);
    ai_micro_cross16_vec_fill_pattern(load_old, vl, 0x84U);
    ai_micro_cross16_vec_fill_pattern(store_initial, vl, 0x44U);
    ai_micro_cross16_vec_fill_pattern(store_src, vl, 0xc4U);
    for (unsigned i = 0; i < vl; ++i) {
        load_expected[i] = load_old[i];
        store_expected[i] = store_initial[i];
    }
    for (unsigned i = 0; i < first_trigger_index; ++i) {
        load_expected[i] = load_data[i];
        store_expected[i] = store_src[i];
    }

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(load_phys, load_data, vl);
    ai_micro_cross16_vec_write_bytes(load_old_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(load_scratch_phys, load_old, vl);
    ai_micro_cross16_vec_write_bytes(store_phys, store_initial, vl);
    ai_micro_cross16_vec_write_bytes(store_src_phys, store_src, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_old_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 dual-trigger load setup preloads old destination bytes before arming two matching address-trigger slots",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_clear_mcontrol6_slot(1);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, load_trigger0_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    ai_micro_cross16_vec_arm_mcontrol6_slot(1, load_trigger1_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 dual address-trigger load reports the selected earlier trigger slot fullVa/vstart instead of mixing in the later trigger address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger0_vaddr &&
        observed_vstart == first_trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx later=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_trigger0_vaddr,
        (unsigned long long)load_trigger1_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 dual-trigger load dump completes after clearing both trigger slots",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 dual-trigger load updates only bytes before the selected earlier trigger element",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys, load_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 dual-trigger store setup stages source bytes before arming two matching address-trigger slots",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_clear_mcontrol6_slot(1);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, store_trigger0_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    ai_micro_cross16_vec_arm_mcontrol6_slot(1, store_trigger1_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)store_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 dual address-trigger store reports the selected earlier trigger slot fullVa/vstart and uses it as the side-effect cutoff",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger0_vaddr &&
        observed_vstart == first_trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx later=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)store_trigger0_vaddr,
        (unsigned long long)store_trigger1_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_clear_mcontrol6_slot(1);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 dual-trigger store commits only bytes before the selected earlier trigger element",
        ai_micro_cross16_vec_bytes_match(store_phys, store_expected, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, store_src, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_trigger_replay_rewalk_clear_stale_mask_manual_corner()
{
    TEST_START();

    const unsigned vl = 8U;
    const unsigned trigger_index = 3U;
    const enum test_page load_data_page = VSRWX_GRWX;
    const enum test_page load_old_page = SWITCH1;
    const enum test_page load_scratch_page = SCRATCHPAD;
    const uintptr_t load_off = 0x6a0U + 11U;
    const uintptr_t load_old_off = 0x6c0U;
    const uintptr_t load_scratch_off = 0x6e0U;
    const uintptr_t load_vaddr = hs_page_base(load_data_page) + load_off;
    const uintptr_t load_old_vaddr =
        hs_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_vaddr =
        hs_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t load_phys = phys_page_base(load_data_page) + load_off;
    const uintptr_t load_old_phys =
        phys_page_base(load_old_page) + load_old_off;
    const uintptr_t load_scratch_phys =
        phys_page_base(load_scratch_page) + load_scratch_off;
    const uintptr_t load_trigger_vaddr = load_vaddr + trigger_index;
    uint8_t load_data[16] = { 0 };
    uint8_t old_data[16] = { 0 };
    uint8_t expected_after_bp[16] = { 0 };
    uint8_t expected_after_repair[16] = { 0 };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_micro_cross16_vec_fill_pattern(load_data, vl, 0x31U);
    ai_micro_cross16_vec_fill_pattern(old_data, vl, 0xb1U);
    for (unsigned i = 0; i < vl; ++i) {
        expected_after_bp[i] =
            (i < trigger_index) ? load_data[i] : old_data[i];
        expected_after_repair[i] = load_data[i];
    }

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(load_phys, load_data, vl);
    ai_micro_cross16_vec_write_bytes(load_old_phys, old_data, vl);
    ai_micro_cross16_vec_write_bytes(load_scratch_phys, old_data, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_old_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 stale-mask load setup preloads old destination bytes before the non-first-element breakpoint",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, load_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 load replay-proxy first pass reports the live non-first-element trigger fullVa/vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == load_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 stale-mask load prefix dump completes after clearing the first trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 stale-mask load first pass updates only bytes before the breakpoint element",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys,
            expected_after_bp, vl) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data, vl)
    );

    goto_priv(PRIV_HS);
    ai_micro_cross16_vec_set_hs_leaf(load_data_page, PTE_V | PTE_AD | PTE_X);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 after clearing BP and forcing an HS rewalk, load retry reports the true page fault instead of stale vecTriggerMask metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx old_trigger=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)load_vaddr,
        (unsigned long long)observed_vstart,
        (unsigned long long)load_trigger_vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)load_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 repairing the leaf after the trigger-clear rewalk lets the same unit-stride load complete",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)load_scratch_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 stale-mask load final dump completes with no residual breakpoint or page-fault metadata",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 repaired load retry copies the full payload, proving the old trigger mask did not poison the next execution",
        ai_micro_cross16_vec_bytes_match(load_scratch_phys,
            expected_after_repair, vl) &&
        ai_micro_cross16_vec_bytes_match(load_phys, load_data, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap41_ap81_store_trigger_replay_rewalk_clear_stale_mask_manual_corner()
{
    TEST_START();

    const unsigned vl = 8U;
    const unsigned trigger_index = 3U;
    const enum test_page store_data_page = VSRW_GRWX;
    const enum test_page store_src_page = VSRWX_GRW;
    const uintptr_t store_off = 0x720U + 11U;
    const uintptr_t store_src_off = 0x740U;
    const uintptr_t store_vaddr = hs_page_base(store_data_page) + store_off;
    const uintptr_t store_src_vaddr =
        hs_page_base(store_src_page) + store_src_off;
    const uintptr_t store_phys = phys_page_base(store_data_page) + store_off;
    const uintptr_t store_src_phys =
        phys_page_base(store_src_page) + store_src_off;
    const uintptr_t store_trigger_vaddr = store_vaddr + trigger_index;
    uint8_t initial_data[16] = { 0 };
    uint8_t src_data[16] = { 0 };
    uint8_t expected_after_bp[16] = { 0 };
    uint8_t expected_after_repair[16] = { 0 };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    ai_micro_cross16_vec_fill_pattern(initial_data, vl, 0x41U);
    ai_micro_cross16_vec_fill_pattern(src_data, vl, 0xd1U);
    for (unsigned i = 0; i < vl; ++i) {
        expected_after_bp[i] =
            (i < trigger_index) ? src_data[i] : initial_data[i];
        expected_after_repair[i] = src_data[i];
    }

    goto_priv(PRIV_M);
    ai_micro_cross16_vec_write_bytes(store_phys, initial_data, vl);
    ai_micro_cross16_vec_write_bytes(store_src_phys, src_data, vl);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 stale-mask store setup stages source bytes before the non-first-element breakpoint",
        excpt.triggered == false
    );

    ai_micro_cross16_vec_clear_mcontrol6_slot(0);
    ai_micro_cross16_vec_arm_mcontrol6_slot(0, store_trigger_vaddr,
        AI_MICRO_CROSS16_VEC_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross16_vec_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)store_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 store replay-proxy first pass reports the live non-first-element trigger fullVa/vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == store_trigger_vaddr &&
        observed_vstart == trigger_index,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)store_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_micro_cross16_vec_restore_machine_status(saved_mstatus);
    ai_micro_cross16_vec_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 stale-mask store first pass commits only bytes before the breakpoint element",
        ai_micro_cross16_vec_bytes_match(store_phys, expected_after_bp, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, src_data, vl)
    );

    goto_priv(PRIV_HS);
    ai_micro_cross16_vec_set_hs_leaf(store_data_page,
        PTE_V | PTE_AD | PTE_R | PTE_X);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)store_vaddr, vl);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-41/AP-81 after clearing BP and forcing an HS rewalk, store retry reports the true page fault instead of stale vecTriggerMask metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr &&
        observed_vstart == 0ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx old_trigger=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)store_vaddr,
        (unsigned long long)observed_vstart,
        (unsigned long long)store_trigger_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 trigger-cleared store retry with a read-only leaf leaves the breakpoint-prefix image unchanged",
        ai_micro_cross16_vec_bytes_match(store_phys, expected_after_bp, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, src_data, vl)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)store_src_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 store source is reloaded after the page-fault probe so the repaired retry cannot inherit partial vector state",
        excpt.triggered == false
    );
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)store_vaddr, vl);
    TEST_ASSERT("AP-41/AP-81 repairing the leaf after the trigger-clear rewalk lets the same unit-stride store complete",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-41/AP-81 repaired store retry commits the full payload, proving the old trigger mask did not poison the next execution",
        ai_micro_cross16_vec_bytes_match(store_phys,
            expected_after_repair, vl) &&
        ai_micro_cross16_vec_bytes_match(store_src_phys, src_data, vl)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
