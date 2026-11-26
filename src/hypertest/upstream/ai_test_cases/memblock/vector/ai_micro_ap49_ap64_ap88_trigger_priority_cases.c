#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_AP49_MCONTROL6_BASE 0x6000000000000058ULL
#define AI_AP49_MCONTROL6_LOAD 0x1ULL
#define AI_AP49_MCONTROL6_STORE 0x2ULL

static inline pte_t *ai_ap49_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ap49_set_hs_leaf(enum test_page page, uint64_t flags)
{
    *ai_ap49_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
    sfence_vma();
}

static inline void ai_ap49_restore_hs_page_tables(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_ap49_mcontrol6_tdata1(uint64_t access_bits)
{
    return AI_AP49_MCONTROL6_BASE | access_bits;
}

static inline void ai_ap49_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_ap49_arm_mcontrol6_slot(size_t slot,
    uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_ap49_mcontrol6_tdata1(access_bits));
}

static inline uint64_t ai_ap49_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_ap49_restore_mstatus(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_ap49_write32s(uintptr_t paddr,
    const uint32_t *values,
    unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write32(paddr + i * sizeof(uint32_t), values[i]);
    }
}

static inline bool ai_ap49_read32s_match(uintptr_t paddr,
    const uint32_t *values,
    unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if (read32(paddr + i * sizeof(uint32_t)) != values[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_ap49_vlse32_to_v6(const uint32_t *base,
    intptr_t stride,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vlse32.v v6, (%[base]), %[stride]\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [stride] "r"(stride), [vl] "r"(vl)
        : "t0", "memory");
}

bool ai_micro_ap49_vector_load_fault_vs_trigger_priority_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t base_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x300U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x300U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x340U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x340U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t later_trigger_vaddr = hs_page_base(high_page) + 8U;
    const uint32_t low_data[1] = { 0x11223344U };
    const uint32_t high_data[4] = {
        0x55667788U, 0x99aabbccU, 0xddeeff00U, 0x13579bdfU
    };
    const uint32_t old_data[4] = {
        0x01020304U, 0x11121314U, 0x21222324U, 0x31323334U
    };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(base_paddr, low_data, 1U);
    ai_ap49_write32s(high_paddr, high_data, 4U);
    ai_ap49_write32s(old_paddr, old_data, 4U);
    ai_ap49_write32s(scratch_paddr, old_data, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-49 load-priority setup preloads old v6 data before the fault-versus-trigger probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    ai_ap49_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_ap49_clear_mcontrol6_slot(0);
    ai_ap49_arm_mcontrol6_slot(0, later_trigger_vaddr, AI_AP49_MCONTROL6_LOAD);
    saved_mstatus = ai_ap49_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49 unit-stride vector load reports the earlier element1 load page fault and suppresses the later element3 breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)fault_vaddr,
        (unsigned long long)observed_vstart
    );
    ai_ap49_restore_mstatus(saved_mstatus);
    ai_ap49_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-49 earlier load fault leaves source and scratch guard images stable while cause/tval/vstart prove the later breakpoint was suppressed",
        ai_ap49_read32s_match(base_paddr, low_data, 1U) &&
        ai_ap49_read32s_match(high_paddr, high_data, 4U) &&
        ai_ap49_read32s_match(scratch_paddr, old_data, 4U)
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap49_vector_load_same_element_trigger_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t base_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x380U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x380U;
    const uintptr_t same_trigger_vaddr = hs_page_base(high_page);
    const uint32_t low_data[1] = { 0x2468ace0U };
    const uint32_t high_data[4] = {
        0x10203040U, 0x50607080U, 0x90a0b0c0U, 0xd0e0f001U
    };
    const uint32_t old_data[4] = {
        0xa0a1a2a3U, 0xb0b1b2b3U, 0xc0c1c2c3U, 0xd0d1d2d3U
    };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(base_paddr, low_data, 1U);
    ai_ap49_write32s(high_paddr, high_data, 4U);
    ai_ap49_write32s(old_paddr, old_data, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-49 same-element setup preloads old v6 data before BP+PF priority probing",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    ai_ap49_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_ap49_clear_mcontrol6_slot(0);
    ai_ap49_arm_mcontrol6_slot(0, same_trigger_vaddr, AI_AP49_MCONTROL6_LOAD);
    saved_mstatus = ai_ap49_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49 same-element unit-stride vector load reports breakpoint with fullVA/vstart of the faulting trigger element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == same_trigger_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)same_trigger_vaddr,
        (unsigned long long)observed_vstart
    );
    ai_ap49_restore_mstatus(saved_mstatus);
    ai_ap49_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49 same-element retry exposes the original load page fault after clearing the matching breakpoint trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == same_trigger_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)same_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap64_non_us_strided_load_same_element_trigger_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const intptr_t stride = 8;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t base_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x3c0U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x3c0U;
    const uintptr_t same_trigger_vaddr = hs_page_base(high_page);
    const uint32_t low_data[2] = { 0x01010101U, 0x02020202U };
    const uint32_t high_data[4] = {
        0x03030303U, 0x04040404U, 0x05050505U, 0x06060606U
    };
    const uint32_t old_data[4] = {
        0x10101010U, 0x20202020U, 0x30303030U, 0x40404040U
    };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(base_paddr, low_data, 2U);
    ai_ap49_write32s(high_paddr, high_data, 4U);
    ai_ap49_write32s(old_paddr, old_data, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-64 non-US strided setup preloads old v6 data before BP+LPF priority probing",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    ai_ap49_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_ap49_clear_mcontrol6_slot(0);
    ai_ap49_arm_mcontrol6_slot(0, same_trigger_vaddr, AI_AP49_MCONTROL6_LOAD);
    saved_mstatus = ai_ap49_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap49_vlse32_to_v6((const uint32_t *)base_vaddr, stride, 4U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-64 non-US strided load reports the true strided element breakpoint address instead of using a unit-stride cancel/mask coordinate",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == same_trigger_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)same_trigger_vaddr,
        (unsigned long long)observed_vstart
    );
    ai_ap49_restore_mstatus(saved_mstatus);
    ai_ap49_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap49_vlse32_to_v6((const uint32_t *)base_vaddr, stride, 4U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-64 non-US strided load exposes the original load page fault at the same strided element after clearing the trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == same_trigger_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)same_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap49_vector_store_fault_vs_trigger_priority_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t base_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x480U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x480U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t later_trigger_vaddr = hs_page_base(high_page) + 8U;
    const uint32_t initial_low[1] = { 0x0bad0000U };
    const uint32_t initial_high[4] = {
        0x0bad0001U, 0x0bad0002U, 0x0bad0003U, 0x0bad0004U
    };
    const uint32_t src[4] = {
        0x60000600U, 0x60000601U, 0x60000602U, 0x60000603U
    };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(base_paddr, initial_low, 1U);
    ai_ap49_write32s(high_paddr, initial_high, 4U);
    ai_ap49_write32s(src_paddr, src, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-49 store-priority setup stages source data before the store fault-versus-trigger probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    ai_ap49_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_R);
    ai_ap49_clear_mcontrol6_slot(0);
    ai_ap49_arm_mcontrol6_slot(0, later_trigger_vaddr, AI_AP49_MCONTROL6_STORE);
    saved_mstatus = ai_ap49_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49 unit-stride vector store reports the earlier element1 store page fault and suppresses the later element3 breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)fault_vaddr,
        (unsigned long long)observed_vstart
    );
    ai_ap49_restore_mstatus(saved_mstatus);
    ai_ap49_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-49 earlier store page fault does not write the fault page or later-trigger suffix while proving later BP was not reported",
        read32(base_paddr) == src[0] &&
        ai_ap49_read32s_match(high_paddr, initial_high, 4U)
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap49_vector_store_trigger_cutoff_manual_corner()
{
    TEST_START();

    const enum test_page data_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t data_vaddr = hs_page_base(data_page) + 0x500U;
    const uintptr_t data_paddr = phys_page_base(data_page) + 0x500U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x580U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x580U;
    const uintptr_t trigger_vaddr = data_vaddr + 2U * sizeof(uint32_t);
    const uint32_t initial[4] = {
        0x10000100U, 0x10000101U, 0x10000102U, 0x10000103U
    };
    const uint32_t src[4] = {
        0x20000200U, 0x20000201U, 0x20000202U, 0x20000203U
    };
    const uint32_t expected_cutoff[4] = {
        0x20000200U, 0x20000201U, 0x10000102U, 0x10000103U
    };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(data_paddr, initial, 4U);
    ai_ap49_write32s(src_paddr, src, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-49 store-cutoff setup stages distinct source elements before the non-first store breakpoint",
        excpt.triggered == false
    );

    ai_ap49_clear_mcontrol6_slot(0);
    ai_ap49_arm_mcontrol6_slot(0, trigger_vaddr, AI_AP49_MCONTROL6_STORE);
    saved_mstatus = ai_ap49_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)data_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49 unit-stride vector store reports the non-first element2 breakpoint with matching tval/vstart",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == trigger_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)trigger_vaddr,
        (unsigned long long)observed_vstart
    );
    ai_ap49_restore_mstatus(saved_mstatus);
    ai_ap49_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-49 StoreQueue cutoff mask keeps the prefix elements before the breakpoint and suppresses the triggered/suffix elements",
        ai_ap49_read32s_match(data_paddr, expected_cutoff, 4U)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)data_vaddr, 4);
    TEST_ASSERT("AP-49 after clearing the store trigger, retrying the same vector store completes and proves no stale breakpoint metadata remains",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-49 cleared-trigger retry writes the full vector store payload after the earlier cutoff breakpoint",
        ai_ap49_read32s_match(data_paddr, src, 4U)
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap49_ap88_vector_store_same_element_trigger_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t base_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x600U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x600U;
    const uintptr_t same_trigger_vaddr = hs_page_base(high_page);
    const uint32_t initial_low[2] = { 0xaaaa0000U, 0xaaaa0001U };
    const uint32_t initial_high[2] = { 0xbbbb0000U, 0xbbbb0001U };
    const uint32_t src[4] = {
        0xcccc0000U, 0xcccc0001U, 0xcccc0002U, 0xcccc0003U
    };
    const uint32_t expected_prefix[2] = { 0xcccc0000U, 0xcccc0001U };
    uint64_t observed_vstart = 0ULL;
    uint64_t saved_mstatus = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(base_paddr, initial_low, 2U);
    ai_ap49_write32s(high_paddr, initial_high, 2U);
    ai_ap49_write32s(src_paddr, src, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-49/AP-88 store same-element setup stages source data before a breakpoint plus store page fault probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    ai_ap49_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_R);
    ai_ap49_clear_mcontrol6_slot(0);
    ai_ap49_arm_mcontrol6_slot(0, same_trigger_vaddr, AI_AP49_MCONTROL6_STORE);
    saved_mstatus = ai_ap49_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49/AP-88 same-element vector store BP+SPF reports breakpoint and keeps trap-visible address metadata on the triggered element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == same_trigger_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)same_trigger_vaddr,
        (unsigned long long)observed_vstart
    );
    ai_ap49_restore_mstatus(saved_mstatus);
    ai_ap49_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-49/AP-88 same-element store breakpoint writes only the prefix before the triggered faulting element and leaves the no-write page unchanged",
        ai_ap49_read32s_match(base_paddr, expected_prefix, 2U) &&
        ai_ap49_read32s_match(high_paddr, initial_high, 2U)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)base_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-49/AP-88 clearing the store breakpoint exposes the original store page fault at the same vector element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == same_trigger_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%llx tval=0x%llx expected=0x%llx vstart=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)same_trigger_vaddr,
        (unsigned long long)observed_vstart
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap88_pure_cold_tlb_vector_store_no_bogus_exception_manual_corner()
{
    TEST_START();

    const enum test_page data_page = VSRW_GRWX;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t data_vaddr = hs_page_base(data_page) + 0x680U;
    const uintptr_t data_paddr = phys_page_base(data_page) + 0x680U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x700U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x700U;
    const uint32_t initial[4] = {
        0x880c0000U, 0x880c0001U, 0x880c0002U, 0x880c0003U
    };
    const uint32_t src[4] = {
        0x88c00000U, 0x88c00001U, 0x88c00002U, 0x88c00003U
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap49_write32s(data_paddr, initial, 4U);
    ai_ap49_write32s(src_paddr, src, 4U);

    ai_ap49_restore_hs_page_tables();
    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-88 pure-store setup stages source data before the cold-TLB vector store probe",
        excpt.triggered == false
    );

    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)data_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-88 pure cold-TLB unit-stride vector store retires without exposing a bogus exception-buffer entry",
        excpt.triggered == false &&
        observed_vstart == 0ULL,
        "triggered=%d cause=0x%llx tval=0x%llx vstart=0x%llx",
        excpt.triggered,
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.tval,
        (unsigned long long)observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-88 pure cold-TLB vector store writes the full payload and does not leave stale breakpoint or fault metadata visible",
        ai_ap49_read32s_match(data_paddr, src, 4U) &&
        ai_ap49_read32s_match(src_paddr, src, 4U)
    );

    ai_ap49_restore_hs_page_tables();
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
