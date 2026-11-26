#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CMO_BLOCK_WORDS 8U
#define AI_MEMBLOCK_CMO_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CMO_REDIRECT_TRAIN_ROUNDS 17U
#define AI_MEMBLOCK_CMO_REDIRECT_PROBE_ROUNDS 5U

static inline pte_t *ai_memblock_cmo_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_cmo_set_hs_leaf_to_phys(
    enum test_page va_page,
    enum test_page phys_page,
    uint64_t flags)
{
    *ai_memblock_cmo_hs_leaf_entry(va_page) =
        (phys_page_base(phys_page) >> 2) | flags;
}

static inline void ai_memblock_cmo_prepare_hs_cbom_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_reset_hs_cbom_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cmo_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cmo_block_matches_seed(
    uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) !=
            seed + i * 0x1111111111111111ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cmo_block_matches_two_updates(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word,
    uint64_t first_value,
    size_t second_word,
    uint64_t second_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == first_word) {
            expected = first_value;
        } else if (i == second_word) {
            expected = second_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static __attribute__((noinline)) void
ai_memblock_cmo_wrong_path_cbom_branch_window(
    uintptr_t cond_vaddr,
    uintptr_t cbom_vaddr,
    uintptr_t poison_vaddr,
    uintptr_t marker_vaddr,
    uint64_t poison_value,
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
        ".insn i 0x0f, 0x2, x0, %[cbom_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[cbom_addr], 2\n\t"
        "sd %[poison_value], 0(%[poison_addr])\n\t"
        "2:\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [cond_addr] "r"(cond_vaddr),
          [cbom_addr] "r"(cbom_vaddr),
          [poison_addr] "r"(poison_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [poison_value] "r"(poison_value),
          [marker_value] "r"(marker_value),
          [rounds] "r"(rounds)
        : "t0", "t1", "memory");
}

static __attribute__((noinline)) void
ai_memblock_cmo_wrong_path_cbozero_branch_window(
    uintptr_t cond_vaddr,
    uintptr_t cbozero_vaddr,
    uintptr_t poison_vaddr,
    uintptr_t marker_vaddr,
    uint64_t poison_value,
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
        ".insn i 0x0f, 0x2, x0, %[cbozero_addr], 4\n\t"
        "sd %[poison_value], 0(%[poison_addr])\n\t"
        "2:\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [cond_addr] "r"(cond_vaddr),
          [cbozero_addr] "r"(cbozero_vaddr),
          [poison_addr] "r"(poison_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [poison_value] "r"(poison_value),
          [marker_value] "r"(marker_value),
          [rounds] "r"(rounds)
        : "t0", "t1", "memory");
}

bool ai_micro_hs_cbom_pending_clean_flush_followup_store_no_stale_sqhascmo_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page follow_page = SWITCH2;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t follow_page_vaddr = hs_page_base(follow_page);
    const uintptr_t follow_page_paddr = phys_page_base(follow_page);
    const uintptr_t target_base_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t target_base_paddr = target_page_paddr + 0x180U;
    const uintptr_t target_cbom_head_vaddr = target_base_vaddr + 8U;
    const uintptr_t target_cbom_tail_vaddr =
        target_base_vaddr + (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t follow_vaddr = follow_page_vaddr + 0x280U;
    const uintptr_t follow_paddr = follow_page_paddr + 0x280U;
    const uintptr_t follow_guard_vaddr = follow_vaddr + sizeof(uint64_t);
    const uintptr_t follow_guard_paddr = follow_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x0102030405060708ULL,
        0x1122334455667788ULL,
    };
    const uint64_t older_target_updates[] = {
        0x8877665544332211ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t younger_target_updates[] = {
        0x0fedcba987654321ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t follow_initials[] = {
        0x55aa55aa33cc33ccULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t follow_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t follow_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
    };

    ai_memblock_cmo_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_ARRAY_SIZE(target_seeds); ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_seed_block(target_base_paddr, target_seeds[round]);
        write64(follow_paddr, follow_initials[round]);
        write64(follow_guard_paddr, follow_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target_head = ld(target_base_vaddr);
        const uint64_t warmed_target_tail = ld(target_cbom_tail_vaddr);
        const uint64_t warmed_follow = ld(follow_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM pending probe starts, target block and follow-up store location are readable with seeded values",
            warmed_target_head == target_seeds[round] &&
            warmed_target_tail == target_seeds[round] +
                (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) * 0x1111111111111111ULL &&
            warmed_follow == follow_initials[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_base_vaddr, older_target_updates[round]);
        cbo_clean(target_cbom_head_vaddr);
        cbo_flush(target_cbom_tail_vaddr);
        sd(target_base_vaddr + 3U * sizeof(uint64_t),
            younger_target_updates[round]);
        sd(follow_vaddr, follow_updates[round]);
        const uint64_t observed_younger =
            ld(target_base_vaddr + 3U * sizeof(uint64_t));
        const uint64_t observed_follow = ld(follow_vaddr);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("clean+flush CBOM pending handoff does not hide the younger same-block store or block the independent follow-up store",
            observed_younger == younger_target_updates[round] &&
            observed_follow == follow_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the CBOM pending handoff drains, target updates, follow-up store, and guard word remain architecturally visible",
            ld(target_base_vaddr) == older_target_updates[round] &&
            ld(target_base_vaddr + 3U * sizeof(uint64_t)) ==
                younger_target_updates[round] &&
            ld(follow_vaddr) == follow_updates[round] &&
            ld(follow_guard_vaddr) == follow_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory keeps both same-block target updates plus the independent follow-up store after the CBOM pending handoff",
            ai_memblock_cmo_block_matches_two_updates(
                target_base_paddr,
                target_seeds[round],
                0U,
                older_target_updates[round],
                3U,
                younger_target_updates[round]) &&
            read64(follow_paddr) == follow_updates[round] &&
            read64(follow_guard_paddr) == follow_guards[round]
        );
    }

    ai_memblock_cmo_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_wrong_path_cbom_redirect_no_stale_cmo_req_or_spurious_fault_corner()
{
    TEST_START();

    const enum test_page cbom_page = SWITCH1;
    const enum test_page control_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t cbom_base_vaddr = hs_page_base(cbom_page) + 0x680U;
    const uintptr_t cbom_base_paddr = phys_page_base(cbom_page) + 0x680U;
    const uintptr_t cbom_fault_vaddr =
        cbom_base_vaddr + (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) *
            sizeof(uint64_t);
    const uintptr_t control_vaddr = hs_page_base(control_page) + 0x280U;
    const uintptr_t control_paddr = phys_page_base(control_page) + 0x280U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x500U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x500U;
    const uintptr_t poison_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t poison_paddr = marker_paddr + sizeof(uint64_t);
    const uintptr_t guard_paddr = poison_paddr + sizeof(uint64_t);
    const uint64_t cbom_seeds[] = {
        0x1021324354657687ULL,
        0x9182736455463728ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
    };
    const uint64_t marker_training[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t poison_seeds[] = {
        0xdeadc0def00dbabeULL,
        0xfeedfacecafebeefULL,
    };
    const uint64_t poison_training[] = {
        0x55aa55aa33cc33ccULL,
        0xaa55aa55cc33cc33ULL,
    };
    const uint64_t poison_wrong_path[] = {
        0x0badf00d5a5aa5a5ULL,
        0x3141592653589793ULL,
    };
    const uint64_t guard_values[] = {
        0xabcdef0123456789ULL,
        0x9876543210fedcbaULL,
    };

    ai_memblock_cmo_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_ARRAY_SIZE(cbom_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_seed_block(cbom_base_paddr, cbom_seeds[round]);
        write64(control_paddr, 1ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(poison_paddr, poison_seeds[round]);
        write64(guard_paddr, guard_values[round]);
        ai_memblock_cmo_set_hs_leaf_to_phys(cbom_page, cbom_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_wrong_path_cbom_branch_window(control_vaddr,
            cbom_fault_vaddr, poison_vaddr, marker_vaddr,
            poison_training[round], marker_training[round],
            AI_MEMBLOCK_CMO_REDIRECT_TRAIN_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("fall-through CBOM training retires cleanly before flipping the branch to the redirect path",
            ld(marker_vaddr) == marker_training[round] &&
            ld(poison_vaddr) == poison_training[round]
        );

        goto_priv(PRIV_M);
        write64(control_paddr, 0ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(poison_paddr, poison_seeds[round]);
        ai_memblock_cmo_set_hs_leaf_to_phys(cbom_page, cbom_page,
            PTE_V | PTE_A | PTE_X);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_wrong_path_cbom_branch_window(control_vaddr,
            cbom_fault_vaddr, poison_vaddr, marker_vaddr,
            poison_wrong_path[round], marker_updates[round],
            AI_MEMBLOCK_CMO_REDIRECT_PROBE_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("taken-branch redirect must kill wrong-path cbo.clean/cbo.flush before they report a page fault or poison the following store",
            ld(marker_vaddr) == marker_updates[round] &&
            ld(poison_vaddr) == poison_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("wrong-path CBOM requests leave the denied target block unchanged and do not leak a stale CMO request into following stores",
            ai_memblock_cmo_block_matches_seed(cbom_base_paddr,
                cbom_seeds[round]) &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(poison_paddr) == poison_seeds[round] &&
            read64(guard_paddr) == guard_values[round]
        );
    }

    ai_memblock_cmo_reset_hs_cbom_env();

    TEST_END();
}

bool ai_micro_hs_wrong_path_cbozero_redirect_no_stale_zero_or_store_wb_corner()
{
    TEST_START();

    const enum test_page cbozero_page = SWITCH1;
    const enum test_page control_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t cbozero_base_vaddr =
        hs_page_base(cbozero_page) + 0x700U;
    const uintptr_t cbozero_base_paddr =
        phys_page_base(cbozero_page) + 0x700U;
    const uintptr_t cbozero_subblock_vaddr =
        cbozero_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        cbozero_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        cbozero_base_paddr +
        AI_MEMBLOCK_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t control_vaddr = hs_page_base(control_page) + 0x2c0U;
    const uintptr_t control_paddr = phys_page_base(control_page) + 0x2c0U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x580U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x580U;
    const uintptr_t poison_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t poison_paddr = marker_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = poison_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x1021324354657687ULL,
        0x9182736455463728ULL,
    };
    const uint64_t before_guards[] = {
        0xaaaabbbbccccddddULL,
        0xddddeeeeffff0001ULL,
    };
    const uint64_t after_guards[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
    };
    const uint64_t marker_training[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_updates[] = {
        0x55aa55aa33cc33ccULL,
        0xaa55aa55cc33cc33ULL,
    };
    const uint64_t poison_seeds[] = {
        0xdeadc0def00dbabeULL,
        0xfeedfacecafebeefULL,
    };
    const uint64_t poison_training[] = {
        0x0badf00d5a5aa5a5ULL,
        0x3141592653589793ULL,
    };
    const uint64_t poison_wrong_path[] = {
        0x45670123cafe9876ULL,
        0x76543210faceba5eULL,
    };
    const uint64_t marker_guards[] = {
        0xabcdef0123456789ULL,
        0x9876543210fedcbaULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_ARRAY_SIZE(target_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_seed_block(cbozero_base_paddr, target_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(control_paddr, 1ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(poison_paddr, poison_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);
        ai_memblock_cmo_set_hs_leaf_to_phys(cbozero_page, cbozero_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_wrong_path_cbozero_branch_window(control_vaddr,
            cbozero_subblock_vaddr, poison_vaddr, marker_vaddr,
            poison_training[round], marker_training[round],
            AI_MEMBLOCK_CMO_REDIRECT_TRAIN_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("fall-through CBO.ZERO training retires and zeros the target block before the redirect probe",
            ld(marker_vaddr) == marker_training[round] &&
            ld(poison_vaddr) == poison_training[round] &&
            ld(cbozero_base_vaddr) == 0ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("training CBO.ZERO zeros only the target block and leaves both adjacent guards intact",
            ai_memblock_cmo_block_all_zero(cbozero_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round]
        );

        ai_memblock_cmo_seed_block(cbozero_base_paddr, target_seeds[round]);
        write64(control_paddr, 0ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(poison_paddr, poison_seeds[round]);
        ai_memblock_cmo_set_hs_leaf_to_phys(cbozero_page, cbozero_page,
            PTE_V | PTE_A | PTE_X);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_wrong_path_cbozero_branch_window(control_vaddr,
            cbozero_subblock_vaddr, poison_vaddr, marker_vaddr,
            poison_wrong_path[round], marker_updates[round],
            AI_MEMBLOCK_CMO_REDIRECT_PROBE_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("taken-branch redirect must kill wrong-path CBO.ZERO before it reports SPF, zeros the denied target, or writes the poison store",
            ld(marker_vaddr) == marker_updates[round] &&
            ld(poison_vaddr) == poison_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("wrong-path CBO.ZERO leaves the denied target block, adjacent guards, marker, and poison data unchanged except for the taken-path marker",
            ai_memblock_cmo_block_matches_seed(cbozero_base_paddr,
                target_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(poison_paddr) == poison_seeds[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_cmo_set_hs_leaf_to_phys(cbozero_page, cbozero_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        cbo_zero(cbozero_subblock_vaddr);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("after repairing the leaf, a real CBO.ZERO uses fresh state and zeros the target block without stale redirect metadata",
            true
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("valid repaired CBO.ZERO zeros exactly the target block while preserving guards and follow-up state",
            ai_memblock_cmo_block_all_zero(cbozero_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(poison_paddr) == poison_seeds[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cbom_two_block_finish_pending_no_cross_release_stores_corner()
{
    TEST_START();

    const enum test_page block0_page = SWITCH1;
    const enum test_page block1_page = SWITCH2;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t block0_base_vaddr = hs_page_base(block0_page) + 0x300U;
    const uintptr_t block0_base_paddr = phys_page_base(block0_page) + 0x300U;
    const uintptr_t block1_base_vaddr = hs_page_base(block1_page) + 0x500U;
    const uintptr_t block1_base_paddr = phys_page_base(block1_page) + 0x500U;
    const uintptr_t block0_guard_vaddr = block0_base_vaddr - sizeof(uint64_t);
    const uintptr_t block0_guard_paddr = block0_base_paddr - sizeof(uint64_t);
    const uintptr_t block1_guard_vaddr =
        block1_base_vaddr + AI_MEMBLOCK_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t block1_guard_paddr =
        block1_base_paddr + AI_MEMBLOCK_CMO_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x380U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x380U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[] = {
        0x1011121314151617ULL,
        0x2021222324252627ULL,
        0x3031323334353637ULL,
    };
    const uint64_t block1_seeds[] = {
        0x5152535455565758ULL,
        0x6162636465666768ULL,
        0x7172737475767778ULL,
    };
    const uint64_t block0_older_updates[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t block0_younger_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t block1_older_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a697887ULL,
        0x2d3c4b5a69788796ULL,
    };
    const uint64_t block1_younger_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_initials[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0303030303030303ULL,
    };
    const uint64_t marker_updates[] = {
        0x55aa55aa33cc33ccULL,
        0xaa55aa55cc33cc33ULL,
        0x5aa55aa53cc33cc3ULL,
    };
    const uint64_t guard0_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0x1123581321345589ULL,
    };
    const uint64_t guard1_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_guards[] = {
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL,
    };

    ai_memblock_cmo_prepare_hs_cbom_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_ARRAY_SIZE(block0_seeds); ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_seed_block(block0_base_paddr, block0_seeds[round]);
        ai_memblock_cmo_seed_block(block1_base_paddr, block1_seeds[round]);
        write64(block0_guard_paddr, guard0_values[round]);
        write64(block1_guard_paddr, guard1_values[round]);
        write64(marker_paddr, marker_initials[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the two-block CBOM finish-pending probe starts, both CMO blocks, guards, and marker are readable with seeded values",
            ld(block0_base_vaddr) == block0_seeds[round] &&
            ld(block0_base_vaddr +
                (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)) ==
                block0_seeds[round] +
                    (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) *
                        0x1111111111111111ULL &&
            ld(block1_base_vaddr) == block1_seeds[round] &&
            ld(block1_base_vaddr +
                (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t)) ==
                block1_seeds[round] +
                    (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) *
                        0x1111111111111111ULL &&
            ld(block0_guard_vaddr) == guard0_values[round] &&
            ld(block1_guard_vaddr) == guard1_values[round] &&
            ld(marker_vaddr) == marker_initials[round]
        );

        TEST_SETUP_EXCEPT();
        sd(block0_base_vaddr, block0_older_updates[round]);
        cbo_clean(block0_base_vaddr + sizeof(uint64_t));
        sd(block1_base_vaddr + sizeof(uint64_t),
            block1_older_updates[round]);
        cbo_flush(block1_base_vaddr +
            (AI_MEMBLOCK_CMO_BLOCK_WORDS - 1U) * sizeof(uint64_t));
        sd(block0_base_vaddr + 3U * sizeof(uint64_t),
            block0_younger_updates[round]);
        sd(block1_base_vaddr + 4U * sizeof(uint64_t),
            block1_younger_updates[round]);
        sd(marker_vaddr, marker_updates[round]);
        const uint64_t observed_block0 =
            ld(block0_base_vaddr + 3U * sizeof(uint64_t));
        const uint64_t observed_block1 =
            ld(block1_base_vaddr + 4U * sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("two different-block CBOM requests do not let cmoFinish or cbomfinish release younger stores against the wrong pending request",
            observed_block0 == block0_younger_updates[round] &&
            observed_block1 == block1_younger_updates[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after both CBOM requests drain, older and younger stores for each block plus the independent marker remain visible",
            ld(block0_base_vaddr) == block0_older_updates[round] &&
            ld(block0_base_vaddr + 3U * sizeof(uint64_t)) ==
                block0_younger_updates[round] &&
            ld(block1_base_vaddr + sizeof(uint64_t)) ==
                block1_older_updates[round] &&
            ld(block1_base_vaddr + 4U * sizeof(uint64_t)) ==
                block1_younger_updates[round] &&
            ld(marker_vaddr) == marker_updates[round] &&
            ld(block0_guard_vaddr) == guard0_values[round] &&
            ld(block1_guard_vaddr) == guard1_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory keeps the two independent CMO-block store streams and does not cross-release either block's younger store",
            ai_memblock_cmo_block_matches_two_updates(
                block0_base_paddr,
                block0_seeds[round],
                0U,
                block0_older_updates[round],
                3U,
                block0_younger_updates[round]) &&
            ai_memblock_cmo_block_matches_two_updates(
                block1_base_paddr,
                block1_seeds[round],
                1U,
                block1_older_updates[round],
                4U,
                block1_younger_updates[round]) &&
            read64(block0_guard_paddr) == guard0_values[round] &&
            read64(block1_guard_paddr) == guard1_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_reset_hs_cbom_env();

    TEST_END();
}
