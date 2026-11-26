#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_STORE_TLB_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_STORE_TLB_REDIRECT_TRAIN_ROUNDS 23U
#define AI_MEMBLOCK_STORE_TLB_REDIRECT_PROBE_ROUNDS 7U

static inline pte_t *ai_memblock_store_tlb_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_store_tlb_set_hs_leaf_to_phys(
    enum test_page va_page, enum test_page phys_page, uint64_t flags)
{
    *ai_memblock_store_tlb_hs_leaf_entry(va_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static __attribute__((noinline)) void
ai_memblock_store_tlb_wrong_path_branch_window(
    uintptr_t cond_vaddr,
    uintptr_t wrong_store_vaddr,
    uintptr_t good_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t wrong_value,
    uint64_t good_value,
    uint64_t marker_value,
    uintptr_t rounds)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 1\n\t"
        "ld t1, 0(%[cond_addr])\n\t"
        "beqz t1, 2f\n\t"
        "sd %[wrong_value], 0(%[wrong_addr])\n\t"
        "2:\n\t"
        "sd %[good_value], 0(%[good_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [cond_addr] "r"(cond_vaddr),
          [wrong_addr] "r"(wrong_store_vaddr),
          [good_addr] "r"(good_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [wrong_value] "r"(wrong_value),
          [good_value] "r"(good_value),
          [marker_value] "r"(marker_value),
          [rounds] "r"(rounds)
        : "t0", "t1", "memory");
}

bool ai_micro_hs_store_pf_then_same_va_remap_store_no_stale_tlb_paddr_corner()
{
    TEST_START();

    const enum test_page store_va_page = SWITCH1;
    const enum test_page old_phys_page = VSRWX_GRWX;
    const enum test_page new_phys_page = VSRW_GRW;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t store_vaddr = hs_page_base(store_va_page) + 0x6c0U;
    const uintptr_t old_paddr = phys_page_base(old_phys_page) + 0x6c0U;
    const uintptr_t new_paddr = phys_page_base(new_phys_page) + 0x6c0U;
    const uintptr_t old_guard_paddr = old_paddr + sizeof(uint64_t);
    const uintptr_t new_guard_paddr = new_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x6c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x6c0U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t old_seeds[] = {
        0x1011223344556677ULL,
        0x2021334455667788ULL,
        0x3031445566778899ULL,
    };
    const uint64_t new_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t fault_store_values[] = {
        0x1111111122222222ULL,
        0x3333333344444444ULL,
        0x5555555566666666ULL,
    };
    const uint64_t valid_store_values[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t old_guard_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t new_guard_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guard_values[] = {
        0xfdb97531eca86420ULL,
        0x1020304050607080ULL,
        0x55aa55aa33cc33ccULL,
    };

    for (size_t round = 0;
         round < AI_MEMBLOCK_STORE_TLB_ARRAY_SIZE(old_seeds);
         ++round) {
        goto_priv(PRIV_M);
        write64(old_paddr, old_seeds[round]);
        write64(new_paddr, new_seeds[round]);
        write64(old_guard_paddr, old_guard_values[round]);
        write64(new_guard_paddr, new_guard_values[round]);
        write64(marker_paddr, 0ULL);
        write64(marker_guard_paddr, marker_guard_values[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_store_tlb_set_hs_leaf_to_phys(store_va_page,
            old_phys_page, PTE_V | PTE_AD | PTE_R);
        ai_memblock_store_tlb_set_hs_leaf_to_phys(marker_page,
            marker_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t warmed_old = ld(store_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the killed-store TLB response probe, the same VA first translates to the old read-only physical page",
            warmed_old == old_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(store_vaddr, fault_store_values[round]);
        TEST_ASSERT("the first store to the read-only HS leaf is killed by a store page fault and must not update the old physical page",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SPF &&
            excpt.tval == store_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the killed store leaves the old and new physical targets untouched before remapping the same VA",
            read64(old_paddr) == old_seeds[round] &&
            read64(new_paddr) == new_seeds[round] &&
            read64(old_guard_paddr) == old_guard_values[round] &&
            read64(new_guard_paddr) == new_guard_values[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_store_tlb_set_hs_leaf_to_phys(store_va_page,
            new_phys_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t remapped_before_store = ld(store_vaddr);
        sd(store_vaddr, valid_store_values[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t remapped_after_store = ld(store_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after the killed store, the same VA remapped to a new physical page uses the current translation for both load and store",
            remapped_before_store == new_seeds[round] &&
            remapped_after_store == valid_store_values[round] &&
            observed_marker == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the killed-store TLB response must not leak its old paddr into the next same-VA store after remap",
            read64(old_paddr) == old_seeds[round] &&
            read64(new_paddr) == valid_store_values[round] &&
            read64(old_guard_paddr) == old_guard_values[round] &&
            read64(new_guard_paddr) == new_guard_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guard_values[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_wrong_path_store_redirect_then_same_va_remap_no_ghost_tlb_request_corner()
{
    TEST_START();

    const enum test_page store_va_page = SWITCH1;
    const enum test_page old_phys_page = VSRWX_GRWX;
    const enum test_page new_phys_page = VSRW_GRWX;
    const enum test_page good_page = SWITCH2;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t wrong_vaddr = hs_page_base(store_va_page) + 0x740U;
    const uintptr_t old_paddr = phys_page_base(old_phys_page) + 0x740U;
    const uintptr_t new_paddr = phys_page_base(new_phys_page) + 0x740U;
    const uintptr_t good_vaddr = hs_page_base(good_page) + 0x740U;
    const uintptr_t good_paddr = phys_page_base(good_page) + 0x740U;
    const uintptr_t control_vaddr = hs_page_base(marker_page) + 0x700U;
    const uintptr_t control_paddr = phys_page_base(marker_page) + 0x700U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x780U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x780U;
    const uintptr_t old_guard_paddr = old_paddr + sizeof(uint64_t);
    const uintptr_t new_guard_paddr = new_paddr + sizeof(uint64_t);
    const uintptr_t good_guard_paddr = good_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t old_seed = 0x1001020304050607ULL;
    const uint64_t new_seed = 0x2001020304050607ULL;
    const uint64_t good_seed = 0x3001020304050607ULL;
    const uint64_t training_wrong = 0x4142434445464748ULL;
    const uint64_t training_good = 0x5152535455565758ULL;
    const uint64_t training_marker = 0x6162636465666768ULL;
    const uint64_t probe_wrong_poison = 0x7172737475767778ULL;
    const uint64_t probe_good = 0x8182838485868788ULL;
    const uint64_t probe_marker = 0x9192939495969798ULL;
    const uint64_t remap_store = 0xa1a2a3a4a5a6a7a8ULL;
    const uint64_t remap_marker = 0xb1b2b3b4b5b6b7b8ULL;
    const uint64_t old_guard = 0xc1c2c3c4c5c6c7c8ULL;
    const uint64_t new_guard = 0xd1d2d3d4d5d6d7d8ULL;
    const uint64_t good_guard = 0xe1e2e3e4e5e6e7e8ULL;
    const uint64_t marker_guard = 0xf1f2f3f4f5f6f7f8ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(new_paddr, new_seed);
    write64(good_paddr, good_seed);
    write64(control_paddr, 1ULL);
    write64(marker_paddr, 0ULL);
    write64(old_guard_paddr, old_guard);
    write64(new_guard_paddr, new_guard);
    write64(good_guard_paddr, good_guard);
    write64(marker_guard_paddr, marker_guard);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_memblock_store_tlb_set_hs_leaf_to_phys(store_va_page,
        old_phys_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_store_tlb_wrong_path_branch_window(control_vaddr,
        wrong_vaddr, good_vaddr, marker_vaddr, training_wrong,
        training_good, training_marker,
        AI_MEMBLOCK_STORE_TLB_REDIRECT_TRAIN_ROUNDS);
    AI_ASSERT_NO_EXCEPT_AND("fall-through store-TLB redirect training commits both the old-PA store and the independent good-path store before the taken probe",
        ld(wrong_vaddr) == training_wrong &&
        ld(good_vaddr) == training_good &&
        ld(marker_vaddr) == training_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("training writes only the selected old physical page, good page, and marker while preserving all guards",
        read64(old_paddr) == training_wrong &&
        read64(new_paddr) == new_seed &&
        read64(good_paddr) == training_good &&
        read64(marker_paddr) == training_marker &&
        read64(old_guard_paddr) == old_guard &&
        read64(new_guard_paddr) == new_guard &&
        read64(good_guard_paddr) == good_guard &&
        read64(marker_guard_paddr) == marker_guard
    );

    write64(old_paddr, old_seed);
    write64(good_paddr, good_seed);
    write64(marker_paddr, 0ULL);
    write64(control_paddr, 0ULL);

    goto_priv(PRIV_HS);
    ai_memblock_store_tlb_set_hs_leaf_to_phys(store_va_page,
        old_phys_page, PTE_V | PTE_AD | PTE_R);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_memblock_store_tlb_wrong_path_branch_window(control_vaddr,
        wrong_vaddr, good_vaddr, marker_vaddr, probe_wrong_poison,
        probe_good, probe_marker,
        AI_MEMBLOCK_STORE_TLB_REDIRECT_PROBE_ROUNDS);
    AI_ASSERT_NO_EXCEPT_AND("taken redirect kills the wrong-path no-W store before a StoreUnit ghost TLB request can raise SPF or poison the next accepted store",
        ld(good_vaddr) == probe_good &&
        ld(marker_vaddr) == probe_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the taken redirect probe, the skipped no-W store leaves the old physical page untouched while the correct-path store and marker commit once",
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_seed &&
        read64(good_paddr) == probe_good &&
        read64(marker_paddr) == probe_marker &&
        read64(old_guard_paddr) == old_guard &&
        read64(new_guard_paddr) == new_guard &&
        read64(good_guard_paddr) == good_guard &&
        read64(marker_guard_paddr) == marker_guard
    );

    goto_priv(PRIV_HS);
    ai_memblock_store_tlb_set_hs_leaf_to_phys(store_va_page,
        new_phys_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    const uint64_t remap_before = ld(wrong_vaddr);
    sd(wrong_vaddr, remap_store);
    sd(marker_vaddr, remap_marker);
    fence_iorw_iorw();
    const uint64_t remap_after = ld(wrong_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the wrong-path store is killed and the same VA is remapped, the next accepted store uses only the fresh PA and not the old ghost request",
        remap_before == new_seed &&
        remap_after == remap_store &&
        ld(marker_vaddr) == remap_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the StoreUnit ghost-TLB request quarantine leaves the old PA unchanged and updates only the remapped PA, marker, and independent good store",
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == remap_store &&
        read64(good_paddr) == probe_good &&
        read64(marker_paddr) == remap_marker &&
        read64(old_guard_paddr) == old_guard &&
        read64(new_guard_paddr) == new_guard &&
        read64(good_guard_paddr) == good_guard &&
        read64(marker_guard_paddr) == marker_guard
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
