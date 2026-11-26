#include "../boundary/ai_micro_cross_page_fault_common.h"

#define AI_MICRO_LOAD_FIRSTISSUE_VL 4U

static inline void ai_micro_load_firstissue_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_micro_load_firstissue_ld_rd_probe(
    uintptr_t addr, uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value)
        : "r"(addr)
        : "memory");

    return value;
}

static inline void ai_micro_load_firstissue_write64s(uintptr_t paddr,
    const uint64_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write64(paddr + i * sizeof(uint64_t), words[i]);
    }
}

static inline void ai_micro_load_firstissue_vloxei64_seeded(
    const uint64_t *seed, const uint64_t *offsets, const uint8_t *base,
    unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v6, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei64.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seed] "r"(seed),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_MICRO_LOAD_FIRSTISSUE_VL)
        : "t0", "memory");
}

static inline void ai_micro_load_firstissue_vloxei64_resume(
    const uint64_t *offsets, const uint8_t *base, unsigned start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei64.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [offsets] "r"(offsets),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_MICRO_LOAD_FIRSTISSUE_VL)
        : "t0", "memory");
}

static inline void ai_micro_load_firstissue_dump_v6(uint64_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        "vse64.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dump] "r"(dump),
          [vl] "r"((unsigned)AI_MICRO_LOAD_FIRSTISSUE_VL)
        : "t0", "memory");
}

bool ai_micro_hs_load_firstissue_scalar_vector_retry_no_stale_source_corner()
{
    TEST_START();

    const enum test_page scalar_page = VSR_GRWX;
    const enum test_page scalar_guard_page = VSRW_GRW;
    const enum test_page vector_base_page = VSRWX_GRWX;
    const enum test_page vector_fault_page = VSRWX_GRW;
    const enum test_page offsets_page = SWITCH2;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_page) + 0x340U;
    const uintptr_t scalar_paddr = phys_page_base(scalar_page) + 0x340U;
    const uintptr_t scalar_guard_paddr =
        phys_page_base(scalar_guard_page) + 0x3c0U;
    const uintptr_t vector_base_vaddr = hs_page_base(vector_base_page);
    const uintptr_t elem0_offset = 0x240U;
    const uintptr_t elem1_offset = 0x248U;
    const uintptr_t elem3_offset = 0x258U;
    const uintptr_t fault_offset = 0x280U;
    const uintptr_t elem0_paddr = phys_page_base(vector_base_page) +
        elem0_offset;
    const uintptr_t elem1_paddr = phys_page_base(vector_base_page) +
        elem1_offset;
    const uintptr_t elem3_paddr = phys_page_base(vector_base_page) +
        elem3_offset;
    const uintptr_t fault_vaddr = hs_page_base(vector_fault_page) +
        fault_offset;
    const uintptr_t fault_paddr = phys_page_base(vector_fault_page) +
        fault_offset;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x180U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x180U;
    const uintptr_t seed_vaddr = hs_page_base(scratch_page) + 0x200U;
    const uintptr_t seed_paddr = phys_page_base(scratch_page) + 0x200U;
    const uintptr_t dump_vaddr = hs_page_base(scratch_page) + 0x280U;
    const uintptr_t dump_paddr = phys_page_base(scratch_page) + 0x280U;
    const uint64_t fault_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t scalar_payload = 0x88776655c43382f1ULL;
    const uint64_t scalar_guard = 0x13579bdf2468ace0ULL;
    const uint64_t scalar_sentinel = 0xa55a00000000f104ULL;
    const uint64_t elem0_value = 0x1011223344556677ULL;
    const uint64_t elem1_value = 0x2021324354657687ULL;
    const uint64_t elem3_value = 0x3031425364758697ULL;
    const uint64_t fault_value = 0x40415263748596a7ULL;
    const uint64_t elem0_poison = 0xdead0000000000a0ULL;
    const uint64_t elem1_poison = 0xdead0000000000b1ULL;
    const uint64_t seed[AI_MICRO_LOAD_FIRSTISSUE_VL] = {
        0x0bad000000000000ULL,
        0x0bad000000000001ULL,
        0x0bad000000000002ULL,
        0x0bad000000000003ULL
    };
    const uint64_t offsets[AI_MICRO_LOAD_FIRSTISSUE_VL] = {
        elem0_offset,
        elem1_offset,
        (uint64_t)(fault_vaddr - vector_base_vaddr),
        elem3_offset
    };
    uint64_t scalar_after_repair = 0ULL;
    unsigned vector_fault_vstart = 0U;
    unsigned vector_retry_vstart = 0U;

    ai_micro_load_firstissue_enable_vector_state();

    goto_priv(PRIV_M);
    write64(scalar_paddr, scalar_payload);
    write64(scalar_guard_paddr, scalar_guard);
    write64(elem0_paddr, elem0_value);
    write64(elem1_paddr, elem1_value);
    write64(elem3_paddr, elem3_value);
    write64(fault_paddr, fault_value);
    ai_micro_load_firstissue_write64s(offsets_paddr, offsets,
        AI_MICRO_LOAD_FIRSTISSUE_VL);
    ai_micro_load_firstissue_write64s(seed_paddr, seed,
        AI_MICRO_LOAD_FIRSTISSUE_VL);
    for (unsigned i = 0; i < AI_MICRO_LOAD_FIRSTISSUE_VL; ++i) {
        write64(dump_paddr + i * sizeof(uint64_t),
            0xfeed000000000000ULL + i);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_set_hs_leaf(scalar_page, fault_leaf);
    ai_micro_set_hs_leaf(scalar_guard_page, legal_leaf);
    ai_micro_set_hs_leaf(vector_base_page, legal_leaf);
    ai_micro_set_hs_leaf(vector_fault_page, fault_leaf);
    ai_micro_set_hs_leaf(offsets_page, legal_leaf);
    ai_micro_set_hs_leaf(scratch_page, legal_leaf);
    sfence_vma();
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);

    TEST_SETUP_EXCEPT();
    uint64_t scalar_after_fault =
        ai_micro_load_firstissue_ld_rd_probe(scalar_vaddr, scalar_sentinel);
    TEST_ASSERT("load-firstIssue quarantine scalar leg first reports a real no-read LPF and suppresses rd before any retry-like source can reuse the episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == scalar_vaddr &&
        scalar_after_fault == scalar_sentinel
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(scalar_vaddr);
    TEST_ASSERT("a software prefetch to the same no-read page stays silent between the scalar fault and repaired demand load",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    prefetch_r(scalar_vaddr);
    TEST_ASSERT("a repeated software prefetch still does not become the demand-load retry owner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("scalar fault plus silent prefetch pressure leaves the target word and guard word unchanged before repair",
        read64(scalar_paddr) == scalar_payload &&
        read64(scalar_guard_paddr) == scalar_guard
    );

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(scalar_page, legal_leaf);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    scalar_after_repair = ld(scalar_vaddr);
    TEST_ASSERT("after scalar leaf repair, the real demand load returns the seeded payload instead of stale sentinel or prefetch-owned data",
        excpt.triggered == false &&
        scalar_after_repair == scalar_payload
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_load_firstissue_vloxei64_seeded((const uint64_t *)seed_vaddr,
        (const uint64_t *)offsets_vaddr, (const uint8_t *)vector_base_vaddr,
        0U);
    vector_fault_vstart = (unsigned)CSRR(CSR_VSTART);
    TEST_ASSERT("load-firstIssue quarantine vector leg reports lane2 LPF with precise tval/vstart after the scalar replay-like episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr &&
        vector_fault_vstart == 2U,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%u",
        excpt.cause, excpt.tval, fault_vaddr, vector_fault_vstart
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_load_firstissue_dump_v6((uint64_t *)dump_vaddr);
    TEST_ASSERT("dumping v6 after the vector fault succeeds after clearing vstart so prefix and suffix ownership can be checked",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before vector repair, only lanes before the fault are committed while fault/tail lanes keep their seed values",
        read64(dump_paddr + 0U * sizeof(uint64_t)) == elem0_value &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == elem1_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == seed[2] &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == seed[3]
    );

    write64(elem0_paddr, elem0_poison);
    write64(elem1_paddr, elem1_poison);

    goto_priv(PRIV_HS);
    ai_micro_set_hs_leaf(vector_fault_page, legal_leaf);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_micro_load_firstissue_vloxei64_resume((const uint64_t *)offsets_vaddr,
        (const uint8_t *)vector_base_vaddr, vector_fault_vstart);
    vector_retry_vstart = (unsigned)CSRR(CSR_VSTART);
    TEST_ASSERT("after vector leaf repair, retry from vstart=2 completes without replaying poisoned prefix addresses",
        excpt.triggered == false &&
        vector_retry_vstart == 0U,
        "retry_vstart=%u", vector_retry_vstart
    );

    TEST_SETUP_EXCEPT();
    ai_micro_load_firstissue_dump_v6((uint64_t *)dump_vaddr);
    TEST_ASSERT("dumping the repaired vector image completes without stale load exception metadata",
        excpt.triggered == false &&
        CSRR(CSR_VSTART) == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("final vector image keeps pre-vstart prefix values, loads the repaired fault lane and suffix lane, and proves poisoned prefix memory was not replayed",
        read64(dump_paddr + 0U * sizeof(uint64_t)) == elem0_value &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == elem1_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == fault_value &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == elem3_value &&
        read64(elem0_paddr) == elem0_poison &&
        read64(elem1_paddr) == elem1_poison &&
        read64(scalar_paddr) == scalar_payload &&
        read64(scalar_guard_paddr) == scalar_guard
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}
