#include "../boundary/ai_micro_cross_page_fault_common.h"

#define AI_MICRO_US_REPLAY_BYTES 32U
#define AI_MICRO_US_REPLAY_VL 8U

static inline void ai_micro_us_replay_vse32_vstart_from_v6(
    uintptr_t dst, unsigned vl, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vse32.v v6, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dst] "r"(dst), [vl] "r"(vl), [start] "r"(start)
        : "t0", "memory");
}

bool ai_micro_hs_vector_store_misalign_prefix_then_late_page_fault_replay_keeps_completed_flow_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const uintptr_t store_vaddr = ai_micro_cross_page_addr(low_page, 19U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 19U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x680U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x680U;
    const uintptr_t before_guard_addr = low_phys - 1U;
    const uintptr_t after_guard_addr = high_phys + 13U;
    const uint8_t before_guard = 0x4bU;
    const uint8_t after_guard = 0xb4U;
    uint8_t initial[AI_MICRO_US_REPLAY_BYTES];
    uint8_t first_src[AI_MICRO_US_REPLAY_BYTES];
    uint8_t retry_src[AI_MICRO_US_REPLAY_BYTES];
    uint8_t expected_after_fault[AI_MICRO_US_REPLAY_BYTES];
    uint8_t expected_final[AI_MICRO_US_REPLAY_BYTES];
    uint64_t observed_vstart = 0ULL;

    for (unsigned i = 0; i < AI_MICRO_US_REPLAY_BYTES; ++i) {
        initial[i] = (uint8_t)(0x20U + i);
        first_src[i] = (uint8_t)(0x80U + i);
        retry_src[i] = (uint8_t)(0xc0U + i);
        expected_after_fault[i] = (i < 16U) ? first_src[i] : initial[i];
        expected_final[i] = (i < 16U) ? first_src[i] : retry_src[i];
    }

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    ai_micro_write_bytes(low_phys, initial, 19U);
    ai_micro_write_bytes(high_phys, initial + 19U, 13U);
    ai_micro_write_bytes(src_phys, first_src, sizeof(first_src));
    write8(before_guard_addr, before_guard);
    write8(after_guard_addr, after_guard);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, AI_MICRO_US_REPLAY_VL);
    TEST_ASSERT("P1-228 setup loads the first e32 payload before the misalign-prefix plus late-page-fault vector store",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_US_REPLAY_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)store_vaddr, AI_MICRO_US_REPLAY_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("P1-228 first store faults at element4 after an earlier element0 cross16 misalign flow has already completed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 4ULL,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, high_fault_vaddr,
        observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("P1-228 fault image commits only completed elements0-3, keeps the faulting element4 low bytes and high-page suffix old, and preserves guards",
        ai_micro_bytes_match(low_phys, expected_after_fault, 19U) &&
        ai_micro_bytes_match(high_phys, expected_after_fault + 19U, 13U) &&
        read8(before_guard_addr) == before_guard &&
        read8(after_guard_addr) == after_guard,
        "low0=0x%x low16=0x%x high0=0x%x",
        read8(low_phys), read8(low_phys + 16U), read8(high_phys)
    );

    ai_micro_write_bytes(src_phys, retry_src, sizeof(retry_src));
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, AI_MICRO_US_REPLAY_VL);
    TEST_ASSERT("P1-228 retry setup loads a different payload so any stale completed-flow replay would overwrite the prefix visibly",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_US_REPLAY_VL
    );

    TEST_SETUP_EXCEPT();
    ai_micro_us_replay_vse32_vstart_from_v6(store_vaddr,
        AI_MICRO_US_REPLAY_VL, (unsigned)observed_vstart);
    TEST_ASSERT("P1-228 repaired retry from vstart=4 completes without replaying already completed prefix elements0-3",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, CSRR(CSR_VSTART)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("P1-228 final image keeps prefix elements0-3 from the first episode and writes only suffix elements4-7 from the repaired retry payload",
        ai_micro_bytes_match(low_phys, expected_final, 19U) &&
        ai_micro_bytes_match(high_phys, expected_final + 19U, 13U) &&
        read8(before_guard_addr) == before_guard &&
        read8(after_guard_addr) == after_guard,
        "prefix0=0x%x expected0=0x%x fault_low=0x%x expected_fault_low=0x%x high0=0x%x expected_high0=0x%x",
        read8(low_phys), expected_final[0], read8(low_phys + 16U),
        expected_final[16], read8(high_phys), expected_final[19]
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vector_store_first_page_fault_blocks_later_misalign_ghost_flow_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH2;
    const uintptr_t store_vaddr = ai_micro_cross_page_addr(low_page, 3U);
    const uintptr_t low_phys = phys_page_base(low_page) + PAGE_SIZE - 3U;
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x720U;
    const uintptr_t src_phys = phys_page_base(src_page) + 0x720U;
    const uintptr_t before_guard_addr = low_phys - 1U;
    const uintptr_t after_guard_addr = high_phys + 37U;
    const uintptr_t marker_vaddr = hs_page_base(high_page) + 48U;
    const uintptr_t marker_phys = high_phys + 48U;
    const uint8_t before_guard = 0x57U;
    const uint8_t after_guard = 0x75U;
    const uint64_t marker_after_fault = 0x112255aacc33dd44ULL;
    const uint64_t marker_after_retry = 0xeedd7711448833ccULL;
    uint8_t initial[AI_MICRO_US_REPLAY_BYTES];
    uint8_t first_src[AI_MICRO_US_REPLAY_BYTES];
    uint8_t retry_src[AI_MICRO_US_REPLAY_BYTES];
    uint64_t observed_vstart = 0ULL;

    for (unsigned i = 0; i < AI_MICRO_US_REPLAY_BYTES; ++i) {
        initial[i] = (uint8_t)(0x31U + i);
        first_src[i] = (uint8_t)(0x91U + i);
        retry_src[i] = (uint8_t)(0xd1U + i);
    }

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    ai_micro_write_bytes(low_phys, initial, 3U);
    ai_micro_write_bytes(high_phys, initial + 3U, 29U);
    ai_micro_write_bytes(src_phys, first_src, sizeof(first_src));
    write8(before_guard_addr, before_guard);
    write8(after_guard_addr, after_guard);
    write64(marker_phys, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_X);
    ai_micro_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, AI_MICRO_US_REPLAY_VL);
    TEST_ASSERT("P1-229 setup loads the first e32 payload while the destination first element still points at a no-write low page",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_US_REPLAY_VL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)store_vaddr, AI_MICRO_US_REPLAY_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("P1-229 first store faults at element0 before later high-page same-uop misalign flows are allowed to create side effects",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr &&
        observed_vstart == 0ULL,
        "triggered=%d cause=0x%lx tval=0x%lx expected_tval=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, store_vaddr,
        observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("P1-229 before repair, neither the low-page faulting bytes nor the later writable high-page misalign/suffix bytes may contain first-episode ghost data",
        ai_micro_bytes_match(low_phys, initial, 3U) &&
        ai_micro_bytes_match(high_phys, initial + 3U, 29U) &&
        read8(before_guard_addr) == before_guard &&
        read8(after_guard_addr) == after_guard,
        "low0=0x%x high0=0x%x high13=0x%x high28=0x%x",
        read8(low_phys), read8(high_phys), read8(high_phys + 13U),
        read8(high_phys + 28U)
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(marker_vaddr, marker_after_fault);
    TEST_ASSERT("P1-229 a younger scalar marker store can reuse store resources after the fault without being poisoned by the old vector-store episode",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("P1-229 marker after first fault is visible and the old faulting vector store still has not leaked into the high-page footprint",
        read64(marker_phys) == marker_after_fault &&
        ai_micro_bytes_match(low_phys, initial, 3U) &&
        ai_micro_bytes_match(high_phys, initial + 3U, 29U),
        "marker=0x%lx high1=0x%x high17=0x%x",
        read64(marker_phys), read8(high_phys + 1U), read8(high_phys + 17U)
    );

    ai_micro_write_bytes(src_phys, retry_src, sizeof(retry_src));
    ai_micro_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, AI_MICRO_US_REPLAY_VL);
    TEST_ASSERT("P1-229 retry setup loads a different payload so stale high-page flows from the first faulting episode are distinguishable",
        excpt.triggered == false &&
        CSRR(CSR_VL) == AI_MICRO_US_REPLAY_VL
    );

    TEST_SETUP_EXCEPT();
    ai_micro_us_replay_vse32_vstart_from_v6(store_vaddr,
        AI_MICRO_US_REPLAY_VL, (unsigned)observed_vstart);
    TEST_ASSERT("P1-229 repaired retry from vstart=0 completes the full footprint from the retry payload",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu",
        excpt.triggered, excpt.cause, excpt.tval, CSRR(CSR_VSTART)
    );

    TEST_SETUP_EXCEPT();
    sd(marker_vaddr, marker_after_retry);
    TEST_ASSERT("P1-229 marker after retry stays ordered after the repaired vector store and checks for stale SQ-generation overwrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("P1-229 final image comes only from retry payload, proving the first TLB-miss episode did not pre-update later misalign flow data",
        ai_micro_bytes_match(low_phys, retry_src, 3U) &&
        ai_micro_bytes_match(high_phys, retry_src + 3U, 29U) &&
        read64(marker_phys) == marker_after_retry &&
        read8(before_guard_addr) == before_guard &&
        read8(after_guard_addr) == after_guard,
        "low0=0x%x high0=0x%x high13=0x%x marker=0x%lx",
        read8(low_phys), read8(high_phys), read8(high_phys + 13U),
        read64(marker_phys)
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
