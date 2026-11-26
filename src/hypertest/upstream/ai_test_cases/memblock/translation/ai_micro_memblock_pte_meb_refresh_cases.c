#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT(message, expected_cause, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        excpt.tval == (vaddr))

#define AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline pte_t *ai_memblock_meb_refresh_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_meb_refresh_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_meb_refresh_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_meb_refresh_prepare_hs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();
}

bool ai_micro_hs_memblock_pte_fault_repair_load_store_bidirectional_refreshes_exception_metadata_corner()
{
    TEST_START();

    const enum test_page load_page = SWITCH1;
    const enum test_page store_page = SWITCH2;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x288U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x288U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x388U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x388U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x488U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x488U;
    const uint64_t load_initial[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
    };
    const uint64_t store_initial[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x7766554433221100ULL,
    };
    const uint64_t store_update[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t guard_initial[] = {
        0x55aa66bb77cc88ddULL,
        0xdd88cc77bb66aa55ULL,
    };
    const uint64_t load_fault_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t store_fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;

    ai_memblock_meb_refresh_prepare_hs();

    for (unsigned round = 0; round < 2U; ++round) {
        const bool load_first = (round == 0U);

        goto_priv(PRIV_M);
        write64(load_paddr, load_initial[round]);
        write64(store_paddr, store_initial[round]);
        write64(guard_paddr, guard_initial[round]);
        ai_memblock_meb_refresh_set_hs_leaf(load_page, load_fault_leaf);
        ai_memblock_meb_refresh_set_hs_leaf(store_page, store_fault_leaf);
        ai_memblock_meb_refresh_set_hs_leaf(guard_page, legal_leaf);
        const uint64_t load_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(load_page);
        const uint64_t store_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(store_page);
        const uint64_t guard_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page);
        fence_iorw_iorw();
        sfence_vma();
        TEST_ASSERT("the bidirectional MEB refresh probe starts from one no-read leaf, one no-write leaf, and one legal guard leaf",
            (load_leaf_before & (PTE_V | PTE_AD | PTE_X)) ==
                (PTE_V | PTE_AD | PTE_X) &&
            (load_leaf_before & PTE_R) == 0 &&
            (store_leaf_before & (PTE_V | PTE_AD | PTE_R)) ==
                (PTE_V | PTE_AD | PTE_R) &&
            (store_leaf_before & PTE_W) == 0 &&
            (guard_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
                (PTE_V | PTE_AD | PTE_RWX)
        );

        goto_priv(PRIV_HS);
        if (load_first) {
            TEST_SETUP_EXCEPT();
            (void)ld(load_vaddr);
            AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT("round0 first HS 4K load PTE fault reports MEBLoadPort load page fault with its own VA",
                CAUSE_LPF, load_vaddr);

            goto_priv(PRIV_M);
            ai_memblock_meb_refresh_set_hs_leaf(load_page, legal_leaf);
            sfence_vma();

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            uint64_t recovered_load = ld(load_vaddr);
            AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("round0 repairing only the load leaf clears the prior MEBLoadPort fault before the recovery load",
                recovered_load == load_initial[round]);

            TEST_SETUP_EXCEPT();
            sd(store_vaddr, store_update[round]);
            AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT("round0 later HS 4K store PTE fault refreshes cause and tval to the MEBStorePort store VA",
                CAUSE_SPF, store_vaddr);
        } else {
            TEST_SETUP_EXCEPT();
            sd(store_vaddr, store_update[round]);
            AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT("round1 first HS 4K store PTE fault reports MEBStorePort store page fault with its own VA",
                CAUSE_SPF, store_vaddr);

            goto_priv(PRIV_M);
            ai_memblock_meb_refresh_set_hs_leaf(store_page, legal_leaf);
            sfence_vma();

            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            sd(store_vaddr, store_update[round]);
            AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("round1 repairing only the store leaf clears the prior MEBStorePort fault before the recovery store",
                true);

            TEST_SETUP_EXCEPT();
            (void)ld(load_vaddr);
            AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT("round1 later HS 4K load PTE fault refreshes cause and tval to the MEBLoadPort load VA",
                CAUSE_LPF, load_vaddr);
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("before final repair, each round preserves the denied target image and the legal guard while allowing only the repaired producer to change state",
            read64(load_paddr) == load_initial[round] &&
            read64(store_paddr) ==
                (load_first ? store_initial[round] : store_update[round]) &&
            read64(guard_paddr) == guard_initial[round] &&
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page) ==
                guard_leaf_before
        );

        ai_memblock_meb_refresh_set_hs_leaf(load_page, legal_leaf);
        ai_memblock_meb_refresh_set_hs_leaf(store_page, legal_leaf);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t final_load = ld(load_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("after final repair, the load side retires normally without stale exception metadata",
            final_load == load_initial[round]);

        TEST_SETUP_EXCEPT();
        sd(store_vaddr, store_update[round]);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("after final repair, the store side retires normally without stale exception metadata",
            true);

        TEST_SETUP_EXCEPT();
        uint64_t final_store_load = ld(store_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the repaired store payload is readable through the same HS 4K alias",
            final_store_load == store_update[round]);

        TEST_SETUP_EXCEPT();
        uint64_t guard_load = ld(guard_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the unrelated legal guard mapping remains readable after the MEB load/store fault sequence",
            guard_load == guard_initial[round]);

        goto_priv(PRIV_M);
        TEST_ASSERT("the bidirectional MEB load/store refresh probe ends with both leaves legal, correct data, and no guard corruption",
            *ai_memblock_meb_refresh_hs_leaf_entry(load_page) ==
                ((phys_page_base(load_page) >> 2) | legal_leaf) &&
            *ai_memblock_meb_refresh_hs_leaf_entry(store_page) ==
                ((phys_page_base(store_page) >> 2) | legal_leaf) &&
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page) ==
                guard_leaf_before &&
            read64(load_paddr) == load_initial[round] &&
            read64(store_paddr) == store_update[round] &&
            read64(guard_paddr) == guard_initial[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_memblock_faulting_scalar_store_then_same_addr_load_sees_old_after_exception_return_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x5c0U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x5c0U;
    const uintptr_t target_tail_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x6c0U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x6c0U;
    const uint64_t initial_values[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t fault_store_values[] = {
        0xa5a55a5adeadc0deULL,
        0x123456789abcdef0ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t repaired_store_values[] = {
        0x0102030405060708ULL,
        0x1828384858687888ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t tail_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t guard_values[] = {
        0x55aa66bb77cc88ddULL,
        0xdd88cc77bb66aa55ULL,
        0x3141592653589793ULL,
    };
    const uint64_t store_fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;

    ai_memblock_meb_refresh_prepare_hs();

    for (unsigned round = 0; round < 3U; ++round) {
        goto_priv(PRIV_M);
        write64(target_paddr, initial_values[round]);
        write64(target_tail_paddr, tail_values[round]);
        write64(guard_paddr, guard_values[round]);
        ai_memblock_meb_refresh_set_hs_leaf(target_page, store_fault_leaf);
        ai_memblock_meb_refresh_set_hs_leaf(guard_page, legal_leaf);
        const uint64_t target_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(target_page);
        const uint64_t guard_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page);
        fence_iorw_iorw();
        sfence_vma();

        TEST_ASSERT("the scalar faulting-store forward probe starts each round from a readable no-write HS leaf and a legal guard leaf",
            (target_leaf_before & (PTE_V | PTE_AD | PTE_R)) ==
                (PTE_V | PTE_AD | PTE_R) &&
            (target_leaf_before & PTE_W) == 0 &&
            (guard_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
                (PTE_V | PTE_AD | PTE_RWX)
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t warm_value = ld(target_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the target HS leaf is readable before the same-address faulting-store probe",
            warm_value == initial_values[round]);

        TEST_SETUP_EXCEPT();
        sd(target_vaddr, fault_store_values[round]);
        AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT("a scalar store to the readable no-write HS leaf reports a precise store page fault on the target VA",
            CAUSE_SPF, target_vaddr);

        TEST_SETUP_EXCEPT();
        uint64_t after_fault_load = ld(target_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the first same-address load after exception return sees only the old committed word, not the killed store payload",
            after_fault_load == initial_values[round]);

        TEST_SETUP_EXCEPT();
        uint8_t after_fault_byte = lbu(target_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("a byte load after the killed scalar store also observes the old low byte instead of any forwarded faulting-store byte",
            after_fault_byte == (uint8_t)initial_values[round]);

        TEST_SETUP_EXCEPT();
        uint64_t guard_load = ld(guard_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("an unrelated legal guard load still retires after the faulting-store same-address observer sequence",
            guard_load == guard_values[round]);

        goto_priv(PRIV_M);
        TEST_ASSERT("the faulting scalar store leaves the target word, adjacent word, guard data, and PTE words unchanged before repair",
            read64(target_paddr) == initial_values[round] &&
            read64(target_tail_paddr) == tail_values[round] &&
            read64(guard_paddr) == guard_values[round] &&
            *ai_memblock_meb_refresh_hs_leaf_entry(target_page) ==
                target_leaf_before &&
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page) ==
                guard_leaf_before
        );

        ai_memblock_meb_refresh_set_hs_leaf(target_page, legal_leaf);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t repaired_before = ld(target_vaddr);
        sd(target_vaddr, repaired_store_values[round]);
        fence_iorw_iorw();
        uint64_t repaired_after = ld(target_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("after repairing the HS leaf, the same address accepts a fresh scalar store and readback without stale killed-store state",
            repaired_before == initial_values[round] &&
            repaired_after == repaired_store_values[round]);

        TEST_SETUP_EXCEPT();
        uint64_t final_guard_load = ld(guard_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the legal guard remains readable after the repaired same-address store replay",
            final_guard_load == guard_values[round]);

        goto_priv(PRIV_M);
        TEST_ASSERT("the repaired replay updates only the current target word while preserving the adjacent word and guard page",
            read64(target_paddr) == repaired_store_values[round] &&
            read64(target_tail_paddr) == tail_values[round] &&
            read64(guard_paddr) == guard_values[round] &&
            *ai_memblock_meb_refresh_hs_leaf_entry(target_page) ==
                ((phys_page_base(target_page) >> 2) | legal_leaf) &&
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page) ==
                guard_leaf_before
        );
    }

    TEST_END();
}

static inline uint64_t ai_memblock_meb_refresh_masked_merge(uint64_t old_value,
    unsigned byte_offset, uint64_t new_value, unsigned byte_count)
{
    const uint64_t mask = ((1ULL << (byte_count * 8U)) - 1ULL) <<
        (byte_offset * 8U);

    return (old_value & ~mask) |
        ((new_value << (byte_offset * 8U)) & mask);
}

static inline void ai_memblock_meb_refresh_issue_narrow_store(
    uintptr_t addr, unsigned width, uint64_t value)
{
    if (width == 1U) {
        sb(addr, (uint8_t)value);
    } else if (width == 2U) {
        sh(addr, (uint16_t)value);
    } else {
        sw(addr, (uint32_t)value);
    }
}

static inline uint64_t ai_memblock_meb_refresh_load_narrow_unsigned(
    uintptr_t addr, unsigned width)
{
    if (width == 1U) {
        return (uint64_t)lbu(addr);
    } else if (width == 2U) {
        return (uint64_t)lhu(addr);
    }
    return (uint64_t)lwu(addr);
}

bool ai_micro_hs_memblock_faulting_narrow_store_masks_do_not_forward_after_exception_return_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x740U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x740U;
    const uintptr_t guard_vaddr = hs_page_base(guard_page) + 0x7c0U;
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0x7c0U;
    const uint64_t initial_values[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const unsigned store_offsets[] = { 3U, 4U, 0U };
    const unsigned store_widths[] = { 1U, 2U, 4U };
    const uint64_t killed_store_values[] = {
        0xaaULL,
        0xbbccULL,
        0xddeeff00ULL,
    };
    const uint64_t repaired_store_values[] = {
        0x5aULL,
        0x6b7cULL,
        0x89abcdefULL,
    };
    const uint64_t tail_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t guard_values[] = {
        0x55aa66bb77cc88ddULL,
        0xdd88cc77bb66aa55ULL,
        0x3141592653589793ULL,
    };
    const uint64_t store_fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;

    ai_memblock_meb_refresh_prepare_hs();

    for (unsigned round = 0; round < 3U; ++round) {
        const unsigned offset = store_offsets[round];
        const unsigned width = store_widths[round];
        const uintptr_t store_vaddr = target_base_vaddr + offset;
        const uintptr_t observe_vaddr = store_vaddr;
        const uint64_t old_narrow =
            (initial_values[round] >> (offset * 8U)) &
            ((1ULL << (width * 8U)) - 1ULL);
        const uint64_t repaired_merged =
            ai_memblock_meb_refresh_masked_merge(initial_values[round],
                offset, repaired_store_values[round], width);

        goto_priv(PRIV_M);
        write64(target_base_paddr, initial_values[round]);
        write64(target_base_paddr + sizeof(uint64_t), tail_values[round]);
        write64(guard_paddr, guard_values[round]);
        ai_memblock_meb_refresh_set_hs_leaf(target_page, store_fault_leaf);
        ai_memblock_meb_refresh_set_hs_leaf(guard_page, legal_leaf);
        const uint64_t target_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(target_page);
        const uint64_t guard_leaf_before =
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page);
        fence_iorw_iorw();
        sfence_vma();

        TEST_ASSERT("the narrow faulting-store mask probe starts from a readable no-write HS leaf and a legal guard leaf",
            (target_leaf_before & (PTE_V | PTE_AD | PTE_R)) ==
                (PTE_V | PTE_AD | PTE_R) &&
            (target_leaf_before & PTE_W) == 0 &&
            (guard_leaf_before & (PTE_V | PTE_AD | PTE_RWX)) ==
                (PTE_V | PTE_AD | PTE_RWX)
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t warm_full = ld(target_base_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the target HS leaf is readable before each narrow killed-store mask probe",
            warm_full == initial_values[round]);

        TEST_SETUP_EXCEPT();
        ai_memblock_meb_refresh_issue_narrow_store(store_vaddr, width,
            killed_store_values[round]);
        AI_MEMBLOCK_MEB_ASSERT_PAGE_FAULT("a byte/halfword/word store to the readable no-write HS leaf reports a precise store page fault on its own VA",
            CAUSE_SPF, store_vaddr);

        TEST_SETUP_EXCEPT();
        uint64_t after_fault_narrow =
            ai_memblock_meb_refresh_load_narrow_unsigned(observe_vaddr, width);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the first same-address unsigned narrow load after exception return sees the old bytes instead of the killed store mask payload",
            after_fault_narrow == old_narrow);

        TEST_SETUP_EXCEPT();
        uint64_t after_fault_full = ld(target_base_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the full doubleword load after a killed narrow store does not pick up stale forwarded byte-lane data",
            after_fault_full == initial_values[round]);

        TEST_SETUP_EXCEPT();
        uint64_t guard_load = ld(guard_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("an unrelated legal guard load still retires after the narrow killed-store observer sequence",
            guard_load == guard_values[round]);

        goto_priv(PRIV_M);
        TEST_ASSERT("the byte/halfword/word killed store leaves the target word, adjacent word, guard data, and PTE words unchanged before repair",
            read64(target_base_paddr) == initial_values[round] &&
            read64(target_base_paddr + sizeof(uint64_t)) == tail_values[round] &&
            read64(guard_paddr) == guard_values[round] &&
            *ai_memblock_meb_refresh_hs_leaf_entry(target_page) ==
                target_leaf_before &&
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page) ==
                guard_leaf_before
        );

        ai_memblock_meb_refresh_set_hs_leaf(target_page, legal_leaf);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_meb_refresh_issue_narrow_store(store_vaddr, width,
            repaired_store_values[round]);
        fence_iorw_iorw();
        uint64_t repaired_full = ld(target_base_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("after repairing the HS leaf, the same narrow store width merges only its own byte lanes into the target word",
            repaired_full == repaired_merged);

        TEST_SETUP_EXCEPT();
        uint64_t final_guard_load = ld(guard_vaddr);
        AI_MEMBLOCK_MEB_ASSERT_NO_FAULT_AND("the legal guard remains readable after the repaired narrow store replay",
            final_guard_load == guard_values[round]);

        goto_priv(PRIV_M);
        TEST_ASSERT("the repaired narrow replay updates only its current byte lanes while preserving the adjacent word and guard page",
            read64(target_base_paddr) == repaired_merged &&
            read64(target_base_paddr + sizeof(uint64_t)) == tail_values[round] &&
            read64(guard_paddr) == guard_values[round] &&
            *ai_memblock_meb_refresh_hs_leaf_entry(target_page) ==
                ((phys_page_base(target_page) >> 2) | legal_leaf) &&
            *ai_memblock_meb_refresh_hs_leaf_entry(guard_page) ==
                guard_leaf_before
        );
    }

    TEST_END();
}
