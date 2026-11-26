#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <instructions.h>
#include <string.h>

extern bool ai_micro_hs_vector_load_unitstride_split_element_second_flow_page_fault_no_partial_dest_update_corner(void);
extern bool ai_micro_hs_vector_load_unitstride_split_element_second_flow_access_fault_no_partial_dest_update_corner(void);
extern bool ai_micro_hs_vector_store_unitstride_split_element_second_flow_page_fault_no_partial_faulting_element_commit_corner(void);
extern bool ai_micro_hs_vector_store_unitstride_split_element_second_flow_access_fault_no_partial_faulting_element_commit_corner(void);
extern bool ai_micro_hs_vector_store_unitstride_split_element_third_flow_page_fault_no_partial_faulting_element_commit_corner(void);
extern bool ai_micro_hs_vector_store_unitstride_split_fault_vstart_refreshes_from_second_to_third_flow_page_fault_corner(void);
extern bool ai_micro_hs_vector_store_repeated_late_page_fault_then_repaired_full_store_recovers_corner(void);
extern bool ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_stable_corner(void);
extern bool ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_stable_corner(void);
extern bool ai_micro_hs_vsplit_masked_unitstride_load_all_inactive_no_fault_or_dest_clobber_corner(void);
extern bool ai_micro_hs_vsplit_masked_unitstride_store_all_inactive_no_fault_or_side_effect_corner(void);
extern bool ai_micro_hs_vfof_e32_suffix_payload_not_valid_after_first_fault_corner(void);

static inline void ai_ap11_vle32ff_to_v6_full_dump(uintptr_t seed_vaddr,
    uintptr_t src_vaddr, uintptr_t dump_vaddr, unsigned req_vl,
    unsigned *observed_vl, unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle32ff.v v6, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(src_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

bool ai_micro_hs_vector_load_unitstride_split_element_first_flow_page_fault_no_partial_dest_update_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const enum test_page old_page = SWITCH1;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_cross_page_addr(low_page, 3U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 3U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x340U;
    const uintptr_t old_phys = phys_page_base(old_page) + 0x340U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x380U;
    const uintptr_t scratch_phys = phys_page_base(scratch_page) + 0x380U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 13U;
    const uintptr_t scratch_before_addr = scratch_phys - 1U;
    const uintptr_t scratch_after_addr = scratch_phys + 16U;
    const uint8_t boundary_before = 0x51U;
    const uint8_t boundary_after = 0x15U;
    const uint8_t scratch_before = 0x62U;
    const uint8_t scratch_after = 0x26U;
    const uint8_t old_vd_bytes[16] = {
        0xe0, 0xe1, 0xe2, 0xe3,
        0xe4, 0xe5, 0xe6, 0xe7,
        0xe8, 0xe9, 0xea, 0xeb,
        0xec, 0xed, 0xee, 0xef
    };
    const uint8_t fault_src_bytes[16] = {
        0x90, 0x91, 0x92, 0x93,
        0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b,
        0x9c, 0x9d, 0x9e, 0x9f
    };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, fault_src_bytes, 3U);
    ai_micro_write_bytes(high_phys, fault_src_bytes + 3U, 13U);
    ai_micro_write_bytes(old_phys, old_vd_bytes, 16U);
    ai_micro_write_bytes(scratch_phys, old_vd_bytes, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);
    write8(scratch_before_addr, scratch_before);
    write8(scratch_after_addr, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(old_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(scratch_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("the preload vector load seeds v6 before the AP-11 split-element first-flow load page-fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector load starting at page_end-3 faults on the first flow of a split element and reports architectural element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("after the first-flow split load fault, dumping v6 completes so the no-partial-destination footprint can be checked",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first-flow split load page fault leaves the whole destination register image at the old seed with no low-half or high-half partial update",
        ai_micro_bytes_match(scratch_phys, old_vd_bytes, 16U)
    );
    TEST_ASSERT("the first-flow split load page-fault probe preserves backing source bytes and adjacent sentinels",
        ai_micro_bytes_match(low_phys, fault_src_bytes, 3U) &&
        ai_micro_bytes_match(high_phys, fault_src_bytes + 3U, 13U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after &&
        read8(scratch_before_addr) == scratch_before &&
        read8(scratch_after_addr) == scratch_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_unitstride_split_element_first_flow_page_fault_no_partial_commit_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const uintptr_t fault_vaddr = ai_micro_cross_page_addr(low_page, 3U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 3U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x3c0U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x3c0U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 13U;
    const uint8_t boundary_before = 0x73U;
    const uint8_t boundary_after = 0x37U;
    const uint8_t dest_initial[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t src_pattern[16] = {
        0xb0, 0xb1, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb,
        0xbc, 0xbd, 0xbe, 0xbf
    };
    uint64_t observed_vstart = 0;

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, dest_initial, 3U);
    ai_micro_write_bytes(high_phys, dest_initial + 3U, 13U);
    ai_micro_write_bytes(src_phys, src_pattern, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("the source vector load stages the full 4x32-bit payload before the AP-11 first-flow split store page-fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("a 4x32-bit unit-stride vector store starting at page_end-3 faults on the first flow of a split element and reports architectural element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first-flow split store page fault must not expose any partial bytes from the faulting element on either side of the page boundary",
        ai_micro_bytes_match(low_phys, dest_initial, 3U) &&
        ai_micro_bytes_match(high_phys, dest_initial + 3U, 13U)
    );
    TEST_ASSERT("the first-flow split store page-fault probe preserves source payload and adjacent sentinels",
        ai_micro_bytes_match(src_phys, src_pattern, 16U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vfof_unitstride_split_element_second_flow_fault_shortens_vl_no_partial_dest_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t src_vaddr = ai_micro_cross_page_addr(low_page, 7U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x480U;
    const uintptr_t seed_phys = phys_page_base(aux_page) + 0x480U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x4c0U;
    const uintptr_t dump_phys = phys_page_base(aux_page) + 0x4c0U;
    const uintptr_t boundary_before_addr = low_phys - 1U;
    const uintptr_t boundary_after_addr = high_phys + 9U;
    const uintptr_t dump_before_addr = dump_phys - 1U;
    const uintptr_t dump_after_addr = dump_phys + 16U;
    const uint8_t boundary_before = 0x84U;
    const uint8_t boundary_after = 0x48U;
    const uint8_t dump_before = 0x95U;
    const uint8_t dump_after = 0x59U;
    const uint8_t seed_bytes[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    const uint8_t src_bytes[16] = {
        0x60, 0x61, 0x62, 0x63,
        0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b,
        0x6c, 0x6d, 0x6e, 0x6f
    };
    uint8_t expected_dump[16];
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    memcpy(expected_dump, seed_bytes, sizeof(expected_dump));
    memcpy(expected_dump, src_bytes, 4U);

    goto_priv(PRIV_M);
    ai_micro_write_bytes(low_phys, src_bytes, 7U);
    ai_micro_write_bytes(high_phys, src_bytes + 7U, 9U);
    ai_micro_write_bytes(seed_phys, seed_bytes, 16U);
    ai_micro_write_bytes(dump_phys, seed_bytes, 16U);
    write8(boundary_before_addr, boundary_before);
    write8(boundary_after_addr, boundary_after);
    write8(dump_before_addr, dump_before);
    write8(dump_after_addr, dump_after);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_ap11_vle32ff_to_v6_full_dump(seed_vaddr, src_vaddr, dump_vaddr, 4U,
        &observed_vl, &observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("a 4x32-bit vle32ff split at page_end-7 shortens vl to architectural element1 instead of trapping on the second split flow",
        excpt.triggered == false &&
        observed_vl == 1U &&
        observed_vstart == 0U
    );
    TEST_ASSERT("the AP-11 FOF split-flow probe updates only element0 and keeps the faulting split element plus suffix at the old destination seed",
        ai_micro_bytes_match(dump_phys, expected_dump, 16U)
    );
    TEST_ASSERT("the AP-11 FOF split-flow probe preserves backing source bytes and adjacent sentinels around source and dump windows",
        ai_micro_bytes_match(low_phys, src_bytes, 7U) &&
        ai_micro_bytes_match(high_phys, src_bytes + 7U, 9U) &&
        read8(boundary_before_addr) == boundary_before &&
        read8(boundary_after_addr) == boundary_after &&
        read8(dump_before_addr) == dump_before &&
        read8(dump_after_addr) == dump_after
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap11_split_second_flow_fault_metadata_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-11 phase0: second-flow page-fault vector load keeps vstart/tval on element1 and leaves the faulting destination element old",
        ai_micro_hs_vector_load_unitstride_split_element_second_flow_page_fault_no_partial_dest_update_corner()
    );
    TEST_ASSERT("AP-11 phase1: second-flow access-fault vector load mirrors the page-fault metadata and no-partial-destination behavior",
        ai_micro_hs_vector_load_unitstride_split_element_second_flow_access_fault_no_partial_dest_update_corner()
    );
    TEST_ASSERT("AP-11 phase2: second-flow page-fault vector store commits only older element0 and blocks faulting element1 partial bytes",
        ai_micro_hs_vector_store_unitstride_split_element_second_flow_page_fault_no_partial_faulting_element_commit_corner()
    );
    TEST_ASSERT("AP-11 phase3: second-flow access-fault vector store mirrors page-fault no-partial-commit behavior",
        ai_micro_hs_vector_store_unitstride_split_element_second_flow_access_fault_no_partial_faulting_element_commit_corner()
    );

    TEST_END();
}

bool ai_micro_ap11_later_flow_replay_refresh_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-11 phase0: later split-flow store fault reports architectural element2 and blocks the faulting element's partial commit",
        ai_micro_hs_vector_store_unitstride_split_element_third_flow_page_fault_no_partial_faulting_element_commit_corner()
    );
    TEST_ASSERT("AP-11 phase1: back-to-back second-flow and third-flow faults refresh vstart/tval instead of reusing stale split-flow metadata",
        ai_micro_hs_vector_store_unitstride_split_fault_vstart_refreshes_from_second_to_third_flow_page_fault_corner()
    );
    TEST_ASSERT("AP-11 phase2: repairing the blocked page after repeated late split-flow faults allows the full vector store to complete with no stale truncation",
        ai_micro_hs_vector_store_repeated_late_page_fault_then_repaired_full_store_recovers_corner()
    );

    TEST_END();
}

bool ai_micro_ap11_samepage_mask_fof_split_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-11 phase0: same-page cross-16B vector load merges a legal split payload without treating flowIdx as a new architectural element",
        ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_stable_corner()
    );
    TEST_ASSERT("AP-11 phase1: same-page cross-16B vector store commits a legal split payload without rebinding split-flow owner state",
        ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_stable_corner()
    );
    TEST_ASSERT("AP-11 phase2: all-inactive masked unit-stride load through an unreadable page must not enqueue a false split-flow exception or clobber dest",
        ai_micro_hs_vsplit_masked_unitstride_load_all_inactive_no_fault_or_dest_clobber_corner()
    );
    TEST_ASSERT("AP-11 phase3: all-inactive masked unit-stride store through an unwritable page must not enqueue a false split-flow exception or side effect",
        ai_micro_hs_vsplit_masked_unitstride_store_all_inactive_no_fault_or_side_effect_corner()
    );
    TEST_ASSERT("AP-11 phase4: direct vle32ff split second-flow fault shortens VL at element1 and leaves the split faulting element old",
        ai_micro_hs_vfof_unitstride_split_element_second_flow_fault_shortens_vl_no_partial_dest_corner()
    );
    TEST_ASSERT("AP-11 phase5: existing e32 VFOF suffix oracle also proves later-page faults do not make suffix backing payload architecturally valid",
        ai_micro_hs_vfof_e32_suffix_payload_not_valid_after_first_fault_corner()
    );

    TEST_END();
}

bool ai_micro_ap11_first_flow_and_oldest_exception_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-11 phase0: split-element first-flow load page fault reports the first architectural element and leaves the destination old",
        ai_micro_hs_vector_load_unitstride_split_element_first_flow_page_fault_no_partial_dest_update_corner()
    );
    TEST_ASSERT("AP-11 phase1: split-element first-flow store page fault reports the first architectural element and blocks all partial bytes",
        ai_micro_hs_vector_store_unitstride_split_element_first_flow_page_fault_no_partial_commit_corner()
    );
    TEST_ASSERT("AP-11 phase2: repeated second/third split-flow faults keep the earliest current fault metadata rather than stale same-elemIdx state",
        ai_micro_hs_vector_store_unitstride_split_fault_vstart_refreshes_from_second_to_third_flow_page_fault_corner()
    );

    TEST_END();
}
