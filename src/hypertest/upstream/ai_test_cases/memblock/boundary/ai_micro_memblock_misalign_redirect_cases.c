#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET 0x100U
#define AI_MEMBLOCK_MISALIGN_REDIRECT_ROUNDS 37UL
#define AI_MEMBLOCK_MISALIGN_REDIRECT_TRAINING_ROUNDS 29U
#define AI_MEMBLOCK_MISALIGN_REDIRECT_STALE_ROUNDS 17U

static inline uint8_t ai_memblock_misalign_redirect_byte(uint64_t value,
    unsigned byte)
{
    return (uint8_t)(value >> (8U * byte));
}

static inline void ai_memblock_misalign_redirect_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline pte_t *ai_memblock_misalign_redirect_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
    enum test_page va_page,
    enum test_page phys_page,
    uint64_t flags)
{
    *ai_memblock_misalign_redirect_hs_leaf_entry(va_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline void ai_memblock_misalign_redirect_wrong_path_probe(
    uintptr_t base_vaddr,
    uint64_t poison_value,
    uint64_t middle_seed,
    uint64_t high_seed,
    uintptr_t rounds)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 1\n\t"
        "sltu t1, t0, %[rounds]\n\t"
        "beqz t1, 2f\n\t"
        "sd %[poison], 9(%[base])\n\t"
        "fence rw, rw\n\t"
        "sd %[middle], 8(%[base])\n\t"
        "sd %[high], 16(%[base])\n\t"
        "fence rw, rw\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "fence rw, rw\n\t"
        "3:\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base_vaddr),
          [poison] "r"(poison_value),
          [middle] "r"(middle_seed),
          [high] "r"(high_seed),
          [rounds] "r"(rounds)
        : "t0", "t1", "memory");
}

static __attribute__((noinline)) void
ai_memblock_misalign_redirect_single_cross_page_branch(
    uintptr_t store_vaddr,
    uint64_t poison_value,
    uintptr_t do_store)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "beqz %[do_store], 1f\n\t"
        "sd %[poison], 0(%[addr])\n\t"
        "fence rw, rw\n\t"
        "1:\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(store_vaddr),
          [poison] "r"(poison_value),
          [do_store] "r"(do_store)
        : "memory");
}

static __attribute__((noinline)) void
ai_memblock_misalign_redirect_single_cross_page_load_branch(
    uintptr_t load_vaddr,
    uintptr_t marker_vaddr,
    uintptr_t do_load)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "beqz %[do_load], 1f\n\t"
        "ld t0, 0(%[addr])\n\t"
        "sd t0, 0(%[marker])\n\t"
        "fence rw, rw\n\t"
        "1:\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(load_vaddr),
          [marker] "r"(marker_vaddr),
          [do_load] "r"(do_load)
        : "t0", "memory");
}

static __attribute__((noinline)) void
ai_memblock_misalign_redirect_single_cross_page_vector_load_branch(
    uintptr_t load_vaddr,
    uintptr_t seed_vaddr,
    uintptr_t marker_vaddr,
    uintptr_t do_load)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, 1\n\t"
        "vsetvli t0, t1, e64, m1, ta, ma\n\t"
        "vle64.v v8, (%[seed])\n\t"
        "beqz %[do_load], 1f\n\t"
        "vle64.v v8, (%[addr])\n\t"
        "fence rw, rw\n\t"
        "1:\n\t"
        "vse64.v v8, (%[marker])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(load_vaddr),
          [seed] "r"(seed_vaddr),
          [marker] "r"(marker_vaddr),
          [do_load] "r"(do_load)
        : "t0", "t1", "memory");
}

static __attribute__((noinline)) void
ai_memblock_misalign_redirect_single_cross_page_vector_store_branch(
    uintptr_t store_vaddr,
    uintptr_t seed_vaddr,
    uintptr_t do_store)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, 1\n\t"
        "vsetvli t0, t1, e64, m1, ta, ma\n\t"
        "vle64.v v8, (%[seed])\n\t"
        "beqz %[do_store], 1f\n\t"
        "vse64.v v8, (%[addr])\n\t"
        "fence rw, rw\n\t"
        "1:\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(store_vaddr),
          [seed] "r"(seed_vaddr),
          [do_store] "r"(do_store)
        : "t0", "t1", "memory");
}

bool ai_micro_hs_store_cross_16b_wrong_path_redirect_no_misalign_buffer_leak_corner()
{
    TEST_START();

    const uintptr_t paddr = phys_page_base(VSRWX_GRWX) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET;
    const uintptr_t vaddr = hs_page_base(VSRWX_GRWX) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET;
    const uint64_t low_guard_seed = 0x1122334455667788ULL;
    const uint64_t middle_seed = 0x99aabbccddeeff00ULL;
    const uint64_t high_seed = 0x0123456789abcdefULL;
    const uint64_t after_guard_seed = 0xfedcba9876543210ULL;
    const uint64_t poison_value = 0xdeadbeefcafebabeULL;

    goto_priv(PRIV_M);
    write64(paddr, low_guard_seed);
    write64(paddr + 8U, middle_seed);
    write64(paddr + 16U, high_seed);
    write64(paddr + 24U, after_guard_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    const uint64_t warm_low = ld(vaddr);
    const uint64_t warm_middle = ld(vaddr + 8U);
    const uint64_t warm_high = ld(vaddr + 16U);
    TEST_ASSERT("the cacheable HS window is readable before the wrong-path cross-16B store redirect probe",
        excpt.triggered == false &&
        warm_low == low_guard_seed &&
        warm_middle == middle_seed &&
        warm_high == high_seed
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_wrong_path_probe(
        vaddr,
        poison_value,
        middle_seed,
        high_seed,
        AI_MEMBLOCK_MISALIGN_REDIRECT_ROUNDS);
    TEST_ASSERT("the branch-redirect wrong-path cross-16B store probe itself must retire without architectural exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a wrong-path 7B+1B cross-16B store must not leak from StoreMisalignBuffer after redirect",
        read64(paddr) == low_guard_seed &&
        read64(paddr + 8U) == middle_seed &&
        read64(paddr + 16U) == high_seed &&
        read64(paddr + 24U) == after_guard_seed
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_wrong_path_redirect_no_spurious_lpf_or_marker_write_corner()
{
    TEST_START();

    const enum test_page low_va_page = SWITCH1;
    const enum test_page high_va_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t load_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 7U;
    const uintptr_t low_tail_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_va_page);
    const uintptr_t marker_paddr = phys_page_base(marker_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x40U;
    const uintptr_t low_tail_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_vaddr = hs_page_base(high_va_page);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x40U;
    const uint64_t low_tail_seed = 0x1122334455667788ULL;
    const uint64_t high_head_seed = 0xaabbccddeeff0099ULL;
    const uint64_t marker_seed = 0x13579bdf2468ace0ULL;
    const uint64_t expected_load = 0x9911223344556677ULL;
    uint64_t marker_observed = 0ULL;

    goto_priv(PRIV_M);
    write64(low_tail_paddr, low_tail_seed);
    write64(high_head_paddr, high_head_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    const uint64_t warm_low_tail = ld(low_tail_vaddr);
    const uint64_t warm_high_head = ld(high_head_vaddr);
    TEST_ASSERT("the cross-page HS load window is readable before training the fall-through split-load branch",
        excpt.triggered == false &&
        warm_low_tail == low_tail_seed &&
        warm_high_head == high_head_seed
    );

    for (unsigned round = 0;
         round < AI_MEMBLOCK_MISALIGN_REDIRECT_TRAINING_ROUNDS;
         ++round) {
        goto_priv(PRIV_M);
        write64(marker_paddr, marker_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_load_branch(
            load_vaddr, marker_vaddr, 1U);
        marker_observed = ld(marker_vaddr);
        TEST_ASSERT("the fall-through training cross-page load retires and writes only its merged 7B+1B value into the marker",
            excpt.triggered == false &&
            marker_observed == expected_load
        );
    }

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_load_branch(
        load_vaddr, marker_vaddr, 0U);
    TEST_ASSERT("a taken branch that skips a cross-page load must not raise a spurious load page fault from a wrong-path LoadMisalignBuffer entry",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the skipped wrong-path cross-page load must not write its merged value into the marker or disturb backing data",
        read64(marker_paddr) == marker_seed &&
        read64(low_tail_paddr) == low_tail_seed &&
        read64(high_head_paddr) == high_head_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_wrong_path_redirect_no_spurious_spf_or_partial_write_corner()
{
    TEST_START();

    const enum test_page low_va_page = SWITCH1;
    const enum test_page high_va_page = SWITCH2;
    const uintptr_t store_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 7U;
    const uintptr_t low_tail_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_va_page);
    const uintptr_t low_guard_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 16U;
    const uintptr_t high_guard_paddr = phys_page_base(high_va_page) + 8U;
    const uintptr_t low_tail_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_vaddr = hs_page_base(high_va_page);
    const uint64_t low_guard_seed = 0x1021324354657687ULL;
    const uint64_t low_tail_seed = 0x98a9babbdcedfe0fULL;
    const uint64_t high_head_seed = 0x0123456789abcdefULL;
    const uint64_t high_guard_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t poison_value = 0x55aa66bb77cc88ddULL;

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_tail_paddr, low_tail_seed);
    write64(high_head_paddr, high_head_seed);
    write64(high_guard_paddr, high_guard_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    const uint64_t warm_low_tail = ld(low_tail_vaddr);
    const uint64_t warm_high_head = ld(high_head_vaddr);
    TEST_ASSERT("the cross-page HS window is readable before training the fall-through split-store branch",
        excpt.triggered == false &&
        warm_low_tail == low_tail_seed &&
        warm_high_head == high_head_seed
    );

    for (unsigned round = 0;
         round < AI_MEMBLOCK_MISALIGN_REDIRECT_TRAINING_ROUNDS;
         ++round) {
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_branch(
            store_vaddr, poison_value, 1U);
        TEST_ASSERT("the fall-through training cross-page store retires without exception while both pages are writable",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        write64(low_tail_paddr, low_tail_seed);
        write64(high_head_paddr, high_head_seed);
    }

    goto_priv(PRIV_HS);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_branch(
        store_vaddr, poison_value, 0U);
    TEST_ASSERT("a taken branch that skips a cross-page store must not raise a spurious store page fault from a wrong-path StoreMisalignBuffer entry",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the skipped wrong-path cross-page store must not partially update either side of the page boundary",
        read64(low_guard_paddr) == low_guard_seed &&
        read64(low_tail_paddr) == low_tail_seed &&
        read64(high_head_paddr) == high_head_seed &&
        read64(high_guard_paddr) == high_guard_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap59_load_cross_page_wrong_path_repeated_redirect_then_repaired_retry_no_stale_mab_state_manual_corner()
{
    TEST_START();

    const enum test_page low_va_page = SWITCH1;
    const enum test_page high_va_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t load_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 7U;
    const uintptr_t low_tail_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_va_page);
    const uintptr_t marker_paddr = phys_page_base(marker_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x80U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x80U;
    const uint64_t low_tail_seed = 0x7766554433221100ULL;
    const uint64_t high_head_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x0badf00d13579bdfULL;
    const uint64_t expected_load = 0x8777665544332211ULL;

    goto_priv(PRIV_M);
    write64(low_tail_paddr, low_tail_seed);
    write64(high_head_paddr, high_head_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_load_branch(
        load_vaddr, marker_vaddr, 1U);
    TEST_ASSERT("AP-59 load repeated-redirect setup first proves the 7B+1B split load can complete and write its merged marker",
        excpt.triggered == false &&
        ld(marker_vaddr) == expected_load
    );

    for (unsigned round = 0;
         round < AI_MEMBLOCK_MISALIGN_REDIRECT_STALE_ROUNDS;
         ++round) {
        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_M);
        write64(marker_paddr, marker_seed ^ (uint64_t)round);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_load_branch(
            load_vaddr, marker_vaddr, 1U);
        TEST_ASSERT("AP-59 each load retry round first lets a clean split request retire before the next redirect window",
            excpt.triggered == false &&
            ld(marker_vaddr) == expected_load
        );

        goto_priv(PRIV_M);
        write64(marker_paddr, marker_seed ^ (uint64_t)(round + 0x40U));

        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_load_branch(
            load_vaddr, marker_vaddr, 0U);
        TEST_ASSERT("AP-59 repeated taken-branch redirects must not let a wrong-path LoadMisalignBuffer resident request report a high-page LPF",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-59 repeated wrong-path split-load redirects must not leave a stale response marker or disturb either backing page",
            read64(marker_paddr) == (marker_seed ^ (uint64_t)(round + 0x40U)) &&
            read64(low_tail_paddr) == low_tail_seed &&
            read64(high_head_paddr) == high_head_seed
        );
    }

    goto_priv(PRIV_HS);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_load_branch(
        load_vaddr, marker_vaddr, 1U);
    TEST_ASSERT("AP-59 after repeated redirect flushes, a repaired clean cross-page split load must still retire with the fresh merged marker",
        excpt.triggered == false &&
        ld(marker_vaddr) == expected_load
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap59_store_cross_page_wrong_path_repeated_redirect_then_repaired_retry_no_partial_or_stale_mab_state_manual_corner()
{
    TEST_START();

    const enum test_page low_va_page = SWITCH1;
    const enum test_page high_va_page = SWITCH2;
    const uintptr_t store_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 7U;
    const uintptr_t low_guard_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 16U;
    const uintptr_t low_tail_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_va_page);
    const uintptr_t high_guard_paddr = phys_page_base(high_va_page) + 8U;
    const uint64_t low_guard_seed = 0x1020304050607080ULL;
    const uint64_t low_tail_seed = 0x8877665544332211ULL;
    const uint64_t high_head_seed = 0x99aabbccddeeff00ULL;
    const uint64_t high_guard_seed = 0x0ff1ce123456789aULL;
    const uint64_t poison_value = 0xca11ab1edeadbeefULL;
    const uint64_t final_value = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_tail_paddr, low_tail_seed);
    write64(high_head_paddr, high_head_seed);
    write64(high_guard_paddr, high_guard_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    for (unsigned round = 0;
         round < AI_MEMBLOCK_MISALIGN_REDIRECT_STALE_ROUNDS;
         ++round) {
        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_branch(
            store_vaddr, poison_value ^ (uint64_t)(round + 0x20U), 1U);
        TEST_ASSERT("AP-59 each store retry round first lets a clean split store retire before the next redirect window",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        write64(low_tail_paddr, low_tail_seed);
        write64(high_head_paddr, high_head_seed);

        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD | PTE_R);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_branch(
            store_vaddr, poison_value ^ (uint64_t)round, 0U);
        TEST_ASSERT("AP-59 repeated taken-branch redirects must not let a wrong-path StoreMisalignBuffer request report a high-page SPF",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-59 repeated wrong-path split-store redirects must not partially update low or high page bytes",
            read64(low_guard_paddr) == low_guard_seed &&
            read64(low_tail_paddr) == low_tail_seed &&
            read64(high_head_paddr) == high_head_seed &&
            read64(high_guard_paddr) == high_guard_seed
        );
    }

    goto_priv(PRIV_HS);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_branch(
        store_vaddr, final_value, 1U);
    TEST_ASSERT("AP-59 after repeated redirect flushes, a repaired clean cross-page split store must retire without stale MAB state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-59 repaired split store writes exactly the 7 low-tail bytes plus 1 high-head byte and preserves both guards",
        read64(low_guard_paddr) == low_guard_seed &&
        read8(low_tail_paddr + 0U) ==
            ai_memblock_misalign_redirect_byte(low_tail_seed, 0U) &&
        read8(low_tail_paddr + 1U) ==
            ai_memblock_misalign_redirect_byte(final_value, 0U) &&
        read8(low_tail_paddr + 2U) ==
            ai_memblock_misalign_redirect_byte(final_value, 1U) &&
        read8(low_tail_paddr + 3U) ==
            ai_memblock_misalign_redirect_byte(final_value, 2U) &&
        read8(low_tail_paddr + 4U) ==
            ai_memblock_misalign_redirect_byte(final_value, 3U) &&
        read8(low_tail_paddr + 5U) ==
            ai_memblock_misalign_redirect_byte(final_value, 4U) &&
        read8(low_tail_paddr + 6U) ==
            ai_memblock_misalign_redirect_byte(final_value, 5U) &&
        read8(low_tail_paddr + 7U) ==
            ai_memblock_misalign_redirect_byte(final_value, 6U) &&
        read8(high_head_paddr + 0U) ==
            ai_memblock_misalign_redirect_byte(final_value, 7U) &&
        read8(high_head_paddr + 1U) ==
            ai_memblock_misalign_redirect_byte(high_head_seed, 1U) &&
        read64(high_guard_paddr) == high_guard_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_ap59_vector_store_cross_page_wrong_path_repeated_redirect_then_repaired_retry_no_suffix_leak_manual_corner()
{
    TEST_START();

    const enum test_page low_va_page = SWITCH1;
    const enum test_page high_va_page = SWITCH2;
    const enum test_page seed_page = SCRATCHPAD;
    const uintptr_t store_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 7U;
    const uintptr_t seed_vaddr = hs_page_base(seed_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x1c0U;
    const uintptr_t low_guard_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 16U;
    const uintptr_t low_tail_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_va_page);
    const uintptr_t high_guard_paddr = phys_page_base(high_va_page) + 8U;
    const uintptr_t seed_paddr = phys_page_base(seed_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x1c0U;
    const uint64_t low_guard_seed = 0x66778899aabbccddULL;
    const uint64_t low_tail_seed = 0x0011223344556677ULL;
    const uint64_t high_head_seed = 0x8899aabbccddeeffULL;
    const uint64_t high_guard_seed = 0x0123456789abcdefULL;
    const uint64_t vector_poison = 0x5aa55aa5c33cc33cULL;
    const uint64_t final_vector = 0x0f1e2d3c4b5a6978ULL;

    ai_memblock_misalign_redirect_enable_vector_state();

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_tail_paddr, low_tail_seed);
    write64(high_head_paddr, high_head_seed);
    write64(high_guard_paddr, high_guard_seed);
    write64(seed_paddr, vector_poison);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    for (unsigned round = 0;
         round < AI_MEMBLOCK_MISALIGN_REDIRECT_STALE_ROUNDS;
         ++round) {
        goto_priv(PRIV_M);
        write64(seed_paddr, vector_poison ^ (uint64_t)(round + 0x10U));

        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_vector_store_branch(
            store_vaddr, seed_vaddr, 1U);
        TEST_ASSERT("AP-59 each vector-store retry round first lets a clean split vector store retire before the next redirect window",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        write64(low_tail_paddr, low_tail_seed);
        write64(high_head_paddr, high_head_seed);
        write64(seed_paddr, vector_poison ^ (uint64_t)round);

        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD | PTE_R);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_vector_store_branch(
            store_vaddr, seed_vaddr, 0U);
        TEST_ASSERT("AP-59 repeated taken-branch redirects must not let a wrong-path vector StoreMisalignBuffer request report a high-page SPF",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-59 repeated wrong-path vector split-store redirects must not leak active element bytes into either side of the boundary",
            read64(low_guard_paddr) == low_guard_seed &&
            read64(low_tail_paddr) == low_tail_seed &&
            read64(high_head_paddr) == high_head_seed &&
            read64(high_guard_paddr) == high_guard_seed
        );
    }

    goto_priv(PRIV_M);
    write64(seed_paddr, final_vector);

    goto_priv(PRIV_HS);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_vector_store_branch(
        store_vaddr, seed_vaddr, 1U);
    TEST_ASSERT("AP-59 after repeated vector-store redirect flushes, a repaired clean split vector store must retire",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-59 repaired vector split store writes exactly the active 7B+1B element and preserves both guards",
        read64(low_guard_paddr) == low_guard_seed &&
        read8(low_tail_paddr + 0U) ==
            ai_memblock_misalign_redirect_byte(low_tail_seed, 0U) &&
        read8(low_tail_paddr + 1U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 0U) &&
        read8(low_tail_paddr + 2U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 1U) &&
        read8(low_tail_paddr + 3U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 2U) &&
        read8(low_tail_paddr + 4U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 3U) &&
        read8(low_tail_paddr + 5U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 4U) &&
        read8(low_tail_paddr + 6U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 5U) &&
        read8(low_tail_paddr + 7U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 6U) &&
        read8(high_head_paddr + 0U) ==
            ai_memblock_misalign_redirect_byte(final_vector, 7U) &&
        read8(high_head_paddr + 1U) ==
            ai_memblock_misalign_redirect_byte(high_head_seed, 1U) &&
        read64(high_guard_paddr) == high_guard_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}

bool ai_micro_ap86_load_replay_wrong_path_split_response_then_clean_retry_marker_no_stale_vecwb_manual_corner()
{
    TEST_START();

    const enum test_page low_va_page = SWITCH1;
    const enum test_page high_va_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const enum test_page seed_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(low_va_page) + PAGE_SIZE - 7U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0xc0U;
    const uintptr_t seed_vaddr = hs_page_base(seed_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x180U;
    const uintptr_t low_tail_paddr = phys_page_base(low_va_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_va_page);
    const uintptr_t marker_paddr = phys_page_base(marker_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0xc0U;
    const uintptr_t seed_paddr = phys_page_base(seed_page) +
        AI_MEMBLOCK_MISALIGN_REDIRECT_BASE_OFFSET + 0x180U;
    const uint64_t low_tail_seed = 0x4433221100ffeeddULL;
    const uint64_t high_head_seed = 0x8877665544332211ULL;
    const uint64_t vector_seed = 0xfeedfacecafebeefULL;
    const uint64_t expected_load = 0x114433221100ffeeULL;
    const uint64_t marker_poison = 0xdeadbad0abcddcbaULL;

    ai_memblock_misalign_redirect_enable_vector_state();

    goto_priv(PRIV_M);
    write64(low_tail_paddr, low_tail_seed);
    write64(high_head_paddr, high_head_seed);
    write64(seed_paddr, vector_seed);
    write64(marker_paddr, marker_poison);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        low_va_page, low_va_page, PTE_V | PTE_AD | PTE_RWX);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_vector_load_branch(
        load_vaddr, seed_vaddr, marker_vaddr, 1U);
    TEST_ASSERT("AP-86 vector setup first proves the cross-page split vector load can write the merged value to the marker",
        excpt.triggered == false &&
        ld(marker_vaddr) == expected_load
    );

    for (unsigned round = 0;
         round < AI_MEMBLOCK_MISALIGN_REDIRECT_STALE_ROUNDS;
         ++round) {
        goto_priv(PRIV_M);
        write64(marker_paddr, marker_poison ^ (uint64_t)round);

        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_vector_load_branch(
            load_vaddr, seed_vaddr, marker_vaddr, 1U);
        TEST_ASSERT("AP-86 each vector retry round first lets a clean split vector load update the marker before the next redirect window",
            excpt.triggered == false &&
            ld(marker_vaddr) == expected_load
        );

        goto_priv(PRIV_M);
        write64(marker_paddr, marker_poison ^ (uint64_t)(round + 0x80U));

        goto_priv(PRIV_HS);
        ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
            high_va_page, high_va_page, PTE_V | PTE_AD);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_misalign_redirect_single_cross_page_vector_load_branch(
            load_vaddr, seed_vaddr, marker_vaddr, 0U);
        TEST_ASSERT("AP-86 repeated wrong-path staged vector MAB candidates must not raise a spurious high-page load fault",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("AP-86 killed staged vector MAB candidates must not later produce stale vecWriteBack data into the marker",
            read64(marker_paddr) == vector_seed &&
            read64(low_tail_paddr) == low_tail_seed &&
            read64(high_head_paddr) == high_head_seed
        );
    }

    goto_priv(PRIV_HS);
    ai_memblock_misalign_redirect_set_hs_leaf_to_phys(
        high_va_page, high_va_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_poison);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_memblock_misalign_redirect_single_cross_page_vector_load_branch(
        load_vaddr, seed_vaddr, marker_vaddr, 1U);
    TEST_ASSERT("AP-86 after repeated killed staged candidates, a clean vector split-load retry still writes the fresh merged value",
        excpt.triggered == false &&
        ld(marker_vaddr) == expected_load
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);

    TEST_END();
}
