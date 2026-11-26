#include "ai_micro_overlap_common.h"

bool ai_micro_hs_store_then_load_same_vaddr_latest()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x48U;
    const uintptr_t paddr = phys_page_base(page) + 0x48U;
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t stored_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS cacheable virtual address first observes the original payload before the overlapping store-load sequence",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("an HS cacheable store completes normally before the younger overlapping load on the same virtual address",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS load on the same virtual address observes the latest payload written by the older store",
        observed == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the overlapping same-virtual-address store-load sequence updates backing memory with the stored payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_then_load_latest()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x88U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x88U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x88U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t stored_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_overlap_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second HS cacheable virtual alias first observes the shared physical payload before the overlapping alias sequence",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("an HS cacheable store through the first virtual alias completes normally before the younger load on the second alias",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS load through the second virtual alias observes the latest payload written through the first alias",
        observed_alias == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS virtual alias also reads back the same shared latest payload afterwards",
        observed_base == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the overlapping different-virtual-address alias sequence updates the shared backing memory with the stored payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_store_then_load_same_vaddr_partial_merge_latest()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x120U;
    const uintptr_t paddr = phys_page_base(page) + 0x120U;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t expected_value = 0x11223344aa667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS cacheable virtual address first observes the original aligned payload before the overlapping partial-store sequence",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sb(vaddr + 3U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("an older HS byte store on the same virtual address retires without surfacing an exception before the younger full-width load",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width load on the same virtual address observes the merged latest bytes from the older partial store",
        observed == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-virtual-address partial-store overlap sequence updates backing memory with the merged latest bytes",
        read64(paddr) == expected_value
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_consecutive_partial_stores_merge_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x160U;
    const uintptr_t paddr = phys_page_base(page) + 0x160U;
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t expected_value = 0x1021beef5465aa87ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS cacheable virtual address first observes the original payload before the consecutive partial-store merge corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sb(vaddr + 1U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS byte store on the same virtual address completes normally in the consecutive partial-store merge corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 4U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the second HS halfword store on the same virtual address also completes normally in the consecutive partial-store merge corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a younger HS full-width load on the same virtual address observes the cumulative merged latest bytes from both older partial stores",
        observed == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-virtual-address consecutive partial-store merge corner updates backing memory with the cumulatively merged latest bytes",
        read64(paddr) == expected_value
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_partial_store_merge_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x168U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + 0x168U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t expected_value = 0x1021beef5465aa87ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target HS word first observes the original payload before the same-VA adjacent-preservation partial-store corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the same-VA adjacent-preservation partial-store corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sb(vaddr + 1U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS byte store on the target word completes normally before the adjacent-preservation check",
        true
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 4U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the second HS halfword store on the target word also completes normally before the adjacent-preservation check",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS word reads back the cumulatively merged latest bytes after the two partial stores",
        observed == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the two partial stores targeting the previous word",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-VA partial-store merge corner updates only the target backing word while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_full_then_partial_merge_latest_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x1e0U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + 0x1e0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t first_store = 0xaabbccddeeff0011ULL;
    const uint64_t expected_value = 0xaabbcc4455ff0011ULL;
    const uint64_t adjacent = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS cacheable virtual address first observes the original payload before the full-then-partial same-VA merge corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the full-then-partial same-VA merge corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the older HS full-width store on the same virtual address completes normally before the younger partial store",
        true
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 3U, 0x4455U);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS halfword store on the same virtual address completes normally and merges into the earlier full-width store payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width load on the same virtual address observes the merged latest payload after the full-store then partial-store sequence",
        observed == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the same-VA full-store then partial-store sequence",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-VA full-then-partial merge corner updates only the target backing word while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_partial_then_full_last_writer_wins_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page) + 0x240U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + 0x240U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t final_store = 0x1021324354657687ULL;
    const uint64_t adjacent = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS cacheable virtual address first observes the original payload before the partial-then-full same-VA last-writer-wins corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the partial-then-full same-VA last-writer-wins corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the older HS partial store on the same virtual address completes normally before the younger full-width store",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, final_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width store on the same virtual address completes normally and becomes the architectural last writer",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width load on the same virtual address observes the final full-width store payload after the earlier partial store",
        observed == final_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the same-VA partial-then-full last-writer-wins sequence",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-VA partial-then-full last-writer-wins corner leaves the target backing word with the final full-width store payload while preserving the adjacent word",
        read64(paddr) == final_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_partial_merge_latest()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x1a0U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x1a0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x1a0U;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88776655beef2211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_micro_overlap_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS cacheable virtual alias observes the original shared payload before the overlapping partial-store alias sequence",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second HS cacheable virtual alias also observes the original shared payload before the overlapping partial-store alias sequence",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sh(base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("an older HS halfword store through the first virtual alias retires without surfacing an exception before the younger alias load",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width load through the second virtual alias observes the merged latest bytes from the older partial store",
        observed_alias == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS virtual alias also reads back the merged latest bytes afterwards",
        observed_base == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the different-virtual-address partial-store alias sequence updates the shared backing memory with the merged latest bytes",
        read64(paddr) == expected_value
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_partial_merge_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x1b0U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x1b0U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x1b0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88776655beef2211ULL;
    const uint64_t adjacent = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS alias observes the original target payload before the alias adjacent-preservation partial-store corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the alias adjacent-preservation partial-store corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(alias_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the HS halfword store through the second virtual alias completes normally before checking adjacent-word preservation",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias reads back the merged latest bytes after the alias partial store",
        observed_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the store alias itself also reads back the merged latest bytes after the alias partial store",
        observed_alias == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the alias partial store to the previous shared word",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias partial-store merge corner updates only the shared target word while preserving the adjacent backing word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_partial_then_full_last_writer_wins_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page partial_store_page = SWITCH1;
    const enum test_page full_store_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x2a0U;
    const uintptr_t partial_store_vaddr = hs_page_base(partial_store_page) + 0x2a0U;
    const uintptr_t full_store_vaddr = hs_page_base(full_store_page) + 0x2a0U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x2a0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t final_store = 0xaabbccddeeff0011ULL;
    const uint64_t adjacent = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(partial_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(full_store_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS alias observes the original target payload before the alias partial-then-full last-writer-wins corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the alias partial-then-full last-writer-wins corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(partial_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the older HS partial store through one alias completes normally before the younger full-width store through another alias",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(full_store_vaddr, final_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width store through the second alias completes normally and becomes the architectural last writer",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias reads back the final full-width store payload after the alias partial-then-full sequence",
        observed_base == final_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_partial_alias = ld(partial_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the partial-store alias also reads back the final full-width store payload after the alias partial-then-full sequence",
        observed_partial_alias == final_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_full_alias = ld(full_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger full-store alias reads back its own final payload after the alias partial-then-full sequence",
        observed_full_alias == final_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the alias partial-then-full last-writer-wins sequence",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias partial-then-full last-writer-wins corner leaves the shared backing word with the final full-width store payload while preserving the adjacent word",
        read64(paddr) == final_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_full_then_partial_merge_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x220U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x220U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x220U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t first_store = 0x0123456789abcdefULL;
    const uint64_t expected_value = 0x0199456789abcdefULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS alias observes the original target payload before the alias full-then-partial merge corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the alias full-then-partial merge corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the older HS full-width store through the base alias completes normally before the younger alias partial store",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(alias_vaddr + 6U, 0x99U);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS byte store through the second virtual alias completes normally and merges into the earlier full-width store payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias reads back the merged latest payload after the alias full-store then partial-store sequence",
        observed_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second HS alias also reads back the merged latest payload after the alias full-store then partial-store sequence",
        observed_alias == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the alias full-store then partial-store sequence",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias full-then-partial merge corner updates only the shared target backing word while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_triple_alias_then_load_latest()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x260U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x260U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x260U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x260U;
    const uint64_t initial_value = 0x01030507090b0d0fULL;
    const uint64_t stored_value = 0xf0e0d0c0b0a09080ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS cacheable alias first observes the shared payload before the triple-alias overlapping store-load sequence",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger-load HS alias also first observes the same shared payload before the triple-alias overlapping sequence",
        warmed_load == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("an older HS store through the second virtual alias completes normally before the younger load on the third alias",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS load through the third virtual alias observes the latest payload written through the second alias",
        observed_load == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias also reads back the same latest payload after the triple-alias overlapping sequence",
        observed_base == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_store = ld(store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the store alias itself also returns the shared latest payload afterwards",
        observed_store == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the triple-alias overlapping store-load sequence updates the shared backing memory with the stored payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_load_alias_pair_cold_consistent()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x2e0U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x2e0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x2e0U;
    const uint64_t initial_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_observed = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first cold HS load through the original alias completes and returns the shared payload",
        first_observed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_observed = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the immediately following cold HS load through the second alias also returns the same shared payload",
        second_observed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t third_observed = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the original alias after the back-to-back alias loads remains stable and returns the same payload",
        third_observed == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the back-to-back alias load sequence leaves the shared backing memory unchanged",
        read64(paddr) == initial_value
    );

    TEST_END();
}

bool ai_micro_hs_ld_st_ld_ld_distinct_vaddr_store_visible()
{
    TEST_START();

    const enum test_page ld0_page = VSRWX_GRWX;
    const enum test_page st0_page = SWITCH1;
    const enum test_page ld1_page = SWITCH2;
    const enum test_page ld2_page = SCRATCHPAD;
    const uintptr_t ld0_vaddr = hs_page_base(ld0_page) + 0x340U;
    const uintptr_t st0_vaddr = hs_page_base(st0_page) + 0x340U;
    const uintptr_t ld1_vaddr = hs_page_base(ld1_page) + 0x340U;
    const uintptr_t ld2_vaddr = hs_page_base(ld2_page) + 0x180U;
    const uintptr_t shared_paddr = phys_page_base(ld0_page) + 0x340U;
    const uintptr_t unrelated_paddr = phys_page_base(ld2_page) + 0x180U;
    const uint64_t initial_shared = 0x1021324354657687ULL;
    const uint64_t stored_shared = 0x8796a5b4c3d2e1f0ULL;
    const uint64_t unrelated_value = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(shared_paddr, initial_shared);
    write64(unrelated_paddr, unrelated_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(st0_page, ld0_page);
    ai_micro_overlap_map_alias_to_base(ld1_page, ld0_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_ld0 = ld(ld0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ld0 through the first virtual address first observes the original shared payload before the mixed alias sequence",
        observed_ld0 == initial_shared
    );

    TEST_SETUP_EXCEPT();
    sd(st0_vaddr, stored_shared);
    AI_ASSERT_NO_EXCEPT_AND("st0 through the second virtual address completes normally before the younger alias and unrelated loads",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_ld1 = ld(ld1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ld1 through the third virtual address observes the latest payload written by st0 on the shared physical page",
        observed_ld1 == stored_shared
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_ld2 = ld(ld2_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("ld2 through the fourth virtual address still observes the unrelated payload on its own physical page",
        observed_ld2 == unrelated_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_ld0_again = ld(ld0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading ld0 after the full ld-st-ld-ld mixed sequence returns the shared latest payload",
        observed_ld0_again == stored_shared
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the mixed four-virtual-address sequence updates only the shared backing page while leaving the unrelated page unchanged",
        read64(shared_paddr) == stored_shared &&
        read64(unrelated_paddr) == unrelated_value
    );

    TEST_END();
}

bool ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page older_page = SWITCH1;
    const enum test_page younger_page = SWITCH2;
    const enum test_page nearby_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x3c0U;
    const uintptr_t older_vaddr = hs_page_base(older_page) + 0x3c0U;
    const uintptr_t younger_vaddr = hs_page_base(younger_page) + 0x3c0U;
    const uintptr_t nearby_vaddr = hs_page_base(nearby_page) + 0x200U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(base_page) + 0x3c0U;
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t nearby_paddr = phys_page_base(nearby_page) + 0x200U;
    const uint64_t target_value = 0x1021324354657687ULL;
    const uint64_t adjacent_value = 0x8899aabbccddeeffULL;
    const uint64_t nearby_value = 0x55aa33cc77ee11ffULL;
    uint64_t observed_older = 0ULL;
    uint64_t observed_younger = 0ULL;
    uint64_t observed_nearby = 0ULL;

    goto_priv(PRIV_M);
    write64(target_paddr, target_value);
    write64(adjacent_paddr, adjacent_value);
    write64(nearby_paddr, nearby_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(older_page, base_page);
    ai_micro_overlap_map_alias_to_base(younger_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base HS alias first observes the target payload before the ld-ld replay no-stuck regression probe",
        warmed_base == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_nearby = ld(nearby_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the nearby HS load also observes its independent payload before the ld-ld replay no-stuck regression probe",
        warmed_nearby == nearby_value
    );

    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the target line retires without exception before the delayed-older-load ld-ld replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the target line retires without exception before the delayed-older-load ld-ld replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_then_younger_loads_capture(
        older_vaddr, younger_vaddr, nearby_vaddr,
        &observed_older, &observed_younger, &observed_nearby);
    TEST_ASSERT("with an older same-PA alias load delayed behind a cold-line replay window, the younger same-PA alias load and a nearby independent load still retire with the correct payloads and the sequence makes forward progress instead of getting stuck",
        excpt.triggered == false &&
        observed_older == target_value &&
        observed_younger == target_value &&
        observed_nearby == nearby_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the ld-ld replay no-stuck probe still returns the target payload",
        observed_base == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(younger_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the younger HS alias after the ld-ld replay no-stuck probe also returns the target payload",
        observed_alias == target_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent target word remains unchanged across the ld-ld replay no-stuck probe",
        observed_adjacent == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_nearby_again = ld(nearby_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the nearby HS word remains unchanged across the ld-ld replay no-stuck probe",
        observed_nearby_again == nearby_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the ld-ld replay no-stuck regression corner leaves backing memory unchanged because the probe only performs loads after creating the replay window",
        read64(target_paddr) == target_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(nearby_paddr) == nearby_value
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_ldld_replay_fetch_flush_no_duplicate_writeback_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page older_page = SWITCH1;
    const enum test_page younger_page = SWITCH2;
    const enum test_page nearby_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x400U;
    const uintptr_t older_vaddr = hs_page_base(older_page) + 0x400U;
    const uintptr_t younger_vaddr = hs_page_base(younger_page) + 0x400U;
    const uintptr_t nearby_vaddr = hs_page_base(nearby_page) + 0x240U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(base_page) + 0x400U;
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t nearby_paddr = phys_page_base(nearby_page) + 0x240U;
    const uint64_t first_target = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_target = 0x99aabbccddeeff00ULL;
    const uint64_t first_nearby = 0x0123456789abcdefULL;
    const uint64_t second_nearby = 0xfedcba9876543210ULL;
    const uint64_t adjacent_value = 0x13579bdf2468ace0ULL;
    uint64_t first_older = 0ULL;
    uint64_t first_younger = 0ULL;
    uint64_t first_nearby_observed = 0ULL;
    uint64_t second_older = 0ULL;
    uint64_t second_younger = 0ULL;
    uint64_t second_nearby_observed = 0ULL;

    goto_priv(PRIV_M);
    write64(target_paddr, first_target);
    write64(adjacent_paddr, adjacent_value);
    write64(nearby_paddr, first_nearby);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(older_page, base_page);
    ai_micro_overlap_map_alias_to_base(younger_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base HS alias first observes the initial target payload before the repeated ld-ld replay fetch-flush probe",
        warmed_base == first_target
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_nearby = ld(nearby_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the nearby HS load first observes the initial nearby payload before the repeated ld-ld replay fetch-flush probe",
        warmed_nearby == first_nearby
    );

    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the target line retires without exception before the first delayed-older-load fetch-flush probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the target line retires without exception before the first delayed-older-load fetch-flush probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_then_younger_loads_capture(
        older_vaddr, younger_vaddr, nearby_vaddr,
        &first_older, &first_younger, &first_nearby_observed);
    TEST_ASSERT("in the first delayed-older-load fetch-flush window, both same-PA loads and the nearby load still retire with the initial payloads",
        excpt.triggered == false &&
        first_older == first_target &&
        first_younger == first_target &&
        first_nearby_observed == first_nearby
    );

    goto_priv(PRIV_M);
    write64(target_paddr, second_target);
    write64(nearby_paddr, second_nearby);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the same target line retires without exception before the repeated delayed-older-load fetch-flush probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the second invalidation retires without exception before the repeated delayed-older-load fetch-flush probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_then_younger_loads_capture(
        older_vaddr, younger_vaddr, nearby_vaddr,
        &second_older, &second_younger, &second_nearby_observed);
    TEST_ASSERT("after repeating the delayed-older-load fetch-flush window with updated backing values, both same-PA loads and the nearby load retire only the new payloads so no stale duplicate writeback is visible",
        excpt.triggered == false &&
        second_older == second_target &&
        second_younger == second_target &&
        second_nearby_observed == second_nearby
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the repeated fetch-flush probe returns only the second target payload",
        observed_base == second_target
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(younger_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the younger HS alias after the repeated fetch-flush probe also returns only the second target payload",
        observed_alias == second_target
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent target word remains unchanged across both delayed-older-load fetch-flush windows",
        observed_adjacent == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_nearby = ld(nearby_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the nearby HS word returns only the second payload after the repeated fetch-flush probe",
        observed_nearby == second_nearby
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated delayed-older-load fetch-flush corner leaves backing memory at only the second target and nearby payloads while preserving the adjacent word",
        read64(target_paddr) == second_target &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(nearby_paddr) == second_nearby
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_ldld_same_line_different_word_release_no_false_replay_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page older_page = SWITCH1;
    const enum test_page younger_page = SWITCH2;
    const enum test_page nearby_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x500U;
    const uintptr_t older_vaddr = hs_page_base(older_page) + 0x500U;
    const uintptr_t younger_vaddr = hs_page_base(younger_page) + 0x508U;
    const uintptr_t nearby_vaddr = hs_page_base(nearby_page) + 0x2c0U;
    const uintptr_t target0_paddr = phys_page_base(base_page) + 0x500U;
    const uintptr_t target1_paddr = phys_page_base(base_page) + 0x508U;
    const uintptr_t guard_paddr = phys_page_base(base_page) + 0x510U;
    const uintptr_t nearby_paddr = phys_page_base(nearby_page) + 0x2c0U;
    const uint64_t first_word0 = 0x1111222233334444ULL;
    const uint64_t first_word1 = 0x5555666677778888ULL;
    const uint64_t first_nearby = 0x9999aaaabbbbccccULL;
    const uint64_t second_word0 = 0x0102030405060708ULL;
    const uint64_t second_word1 = 0x1122334455667788ULL;
    const uint64_t second_nearby = 0x8877665544332211ULL;
    const uint64_t guard = 0xf0e1d2c3b4a59687ULL;
    uint64_t first_older = 0ULL;
    uint64_t first_younger = 0ULL;
    uint64_t first_nearby_observed = 0ULL;
    uint64_t second_older = 0ULL;
    uint64_t second_younger = 0ULL;
    uint64_t second_nearby_observed = 0ULL;

    goto_priv(PRIV_M);
    write64(target0_paddr, first_word0);
    write64(target1_paddr, first_word1);
    write64(guard_paddr, guard);
    write64(nearby_paddr, first_nearby);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(older_page, base_page);
    ai_micro_overlap_map_alias_to_base(younger_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();

    TEST_SETUP_EXCEPT();
    uint64_t warmed0 = ld(base_vaddr);
    uint64_t warmed1 = ld(base_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("both words in the shared cache line are readable before the same-line different-word ld-ld release probe",
        warmed0 == first_word0 &&
        warmed1 == first_word1
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the shared line retires before the first same-line different-word ld-ld release probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the first shared-line invalidation retires before the same-line different-word ld-ld release probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_then_younger_loads_capture(
        older_vaddr, younger_vaddr, nearby_vaddr,
        &first_older, &first_younger, &first_nearby_observed);
    TEST_ASSERT("in the first same-line different-word ld-ld release window, the delayed older load returns word0 and the younger same-line load returns only word1 without false replay cross-binding",
        excpt.triggered == false &&
        first_older == first_word0 &&
        first_younger == first_word1 &&
        first_nearby_observed == first_nearby
    );

    goto_priv(PRIV_M);
    write64(target0_paddr, second_word0);
    write64(target1_paddr, second_word1);
    write64(nearby_paddr, second_nearby);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the same shared line retires before the repeated same-line different-word ld-ld release probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the second shared-line invalidation retires before the repeated same-line different-word ld-ld release probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_then_younger_loads_capture(
        older_vaddr, younger_vaddr, nearby_vaddr,
        &second_older, &second_younger, &second_nearby_observed);
    TEST_ASSERT("after refreshing both words, the repeated same-line different-word ld-ld release window returns only the second-round word0, word1, and nearby payloads",
        excpt.triggered == false &&
        second_older == second_word0 &&
        second_younger == second_word1 &&
        second_nearby_observed == second_nearby
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_word0 = ld(base_vaddr);
    uint64_t final_word1 = ld(base_vaddr + sizeof(uint64_t));
    uint64_t final_guard = ld(base_vaddr + 2U * sizeof(uint64_t));
    uint64_t final_nearby = ld(nearby_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("final reloads after the same-line different-word ld-ld release probe keep word0, word1, guard, and nearby payloads bound to their own addresses",
        final_word0 == second_word0 &&
        final_word1 == second_word1 &&
        final_guard == guard &&
        final_nearby == second_nearby
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-line different-word ld-ld release probe leaves backing memory with the latest per-word payloads and does not corrupt the same-line guard",
        read64(target0_paddr) == second_word0 &&
        read64(target1_paddr) == second_word1 &&
        read64(guard_paddr) == guard &&
        read64(nearby_paddr) == second_nearby
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page older_page = SWITCH1;
    const enum test_page younger_page = SWITCH2;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x580U;
    const uintptr_t older_vaddr = hs_page_base(older_page) + 0x580U;
    const uintptr_t younger_vaddr = hs_page_base(younger_page) + 0x580U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x380U;
    const uintptr_t target_paddr = phys_page_base(base_page) + 0x580U;
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x380U;
    const uint64_t first_target = 0x3141592653589793ULL;
    const uint64_t second_target = 0x2718281828459045ULL;
    const uint64_t adjacent = 0x0badc0ffee0ddf00ULL;
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t first_marker = 0xa55a966969965aa5ULL;
    const uint64_t second_marker = 0x5aa569969669a55aULL;
    uint64_t first_older = 0ULL;
    uint64_t first_younger = 0ULL;
    uint64_t first_marker_observed = 0ULL;
    uint64_t second_older = 0ULL;
    uint64_t second_younger = 0ULL;
    uint64_t second_marker_observed = 0ULL;

    goto_priv(PRIV_M);
    write64(target_paddr, first_target);
    write64(adjacent_paddr, adjacent);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(older_page, base_page);
    ai_micro_overlap_map_alias_to_base(younger_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    uint64_t warmed_marker = ld(marker_vaddr);
    TEST_ASSERT("before the ld-ld flushAfter follower-store probe starts, the target line and independent marker line are readable with their first-round payloads",
        excpt.triggered == false &&
        warmed_base == first_target &&
        warmed_marker == marker_seed
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the target line retires before the first ld-ld flushAfter follower-store probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the first target invalidation retires before the first ld-ld flushAfter follower-store probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_younger_load_then_marker_store_capture(
        older_vaddr, younger_vaddr, marker_vaddr, first_marker,
        &first_older, &first_younger, &first_marker_observed);
    TEST_ASSERT("in the first ld-ld flushAfter window, the delayed older load and younger same-PA load both return the target payload and the immediately following independent marker store is not lost",
        excpt.triggered == false &&
        first_older == first_target &&
        first_younger == first_target &&
        first_marker_observed == first_marker
    );

    goto_priv(PRIV_M);
    write64(target_paddr, second_target);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the target line retires before the repeated ld-ld flushAfter follower-store probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the second target invalidation retires before the repeated ld-ld flushAfter follower-store probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_older_load_younger_load_then_marker_store_capture(
        older_vaddr, younger_vaddr, marker_vaddr, second_marker,
        &second_older, &second_younger, &second_marker_observed);
    TEST_ASSERT("after refreshing the target payload, the repeated ld-ld flushAfter window still keeps both loads on the second target payload and re-executes the follower marker store exactly to the second marker value",
        excpt.triggered == false &&
        second_older == second_target &&
        second_younger == second_target &&
        second_marker_observed == second_marker
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_base = ld(base_vaddr);
    uint64_t final_younger = ld(younger_vaddr);
    uint64_t final_adjacent = ld(adjacent_base_vaddr);
    uint64_t final_marker = ld(marker_vaddr);
    TEST_ASSERT("final reloads after the repeated ld-ld flushAfter follower-store probe keep the target aliases, adjacent word, and marker line bound to their latest expected payloads",
        excpt.triggered == false &&
        final_base == second_target &&
        final_younger == second_target &&
        final_adjacent == adjacent &&
        final_marker == second_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the ld-ld flushAfter follower-store probe leaves backing memory with the second target payload, preserves the adjacent word, and commits only the latest marker store",
        read64(target_paddr) == second_target &&
        read64(adjacent_paddr) == adjacent &&
        read64(marker_paddr) == second_marker
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_store_triple_alias_partial_merge_latest_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x3c0U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x3c0U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x3c0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x3c0U;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88beef5544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the shared payload before the triple-alias partial-store merge corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the third HS alias also observes the original shared payload before the triple-alias partial-store merge corner",
        warmed_load == initial_value
    );

    TEST_SETUP_EXCEPT();
    sh(store_vaddr + 5U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("an HS halfword store through the second virtual alias completes normally in the triple-alias partial-store merge corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the third HS alias observes the merged latest bytes after the triple-alias partial store",
        observed_load == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias also reads back the same merged latest bytes after the triple-alias partial store",
        observed_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_store = ld(store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the store alias itself also returns the merged latest bytes in the triple-alias partial-store corner",
        observed_store == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the triple-alias partial-store merge corner updates the shared backing memory with the merged latest bytes",
        read64(paddr) == expected_value
    );

    TEST_END();
}

bool ai_micro_hs_triple_alias_consecutive_stores_last_writer_wins_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x420U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x420U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x420U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x420U;
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t first_store = 0xaabbccddeeff0011ULL;
    const uint64_t second_store = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the seeded payload before the consecutive triple-alias stores corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(first_store_vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the first HS store through its alias completes normally before the later alias store in the last-writer-wins corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_store_vaddr, second_store);
    AI_ASSERT_NO_EXCEPT_AND("the second HS store through a different alias also completes normally and becomes the architectural last writer",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after consecutive triple-alias stores, the original HS alias observes the last writer payload",
        observed_base == second_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_store_alias = ld(first_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after consecutive triple-alias stores, the first store alias also observes the final last writer payload",
        observed_first_store_alias == second_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_store_alias = ld(second_store_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after consecutive triple-alias stores, the second store alias observes its own final payload",
        observed_second_store_alias == second_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the consecutive triple-alias stores corner leaves the shared backing memory with the last writer payload",
        read64(paddr) == second_store
    );

    TEST_END();
}

bool ai_micro_hs_triple_alias_multi_partial_store_merge_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x4a0U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x4a0U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x4a0U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x4a0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x4a0U;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88aa6655beef2211ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the shared payload before the triple-alias multi-partial-store merge corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger load alias also observes the original shared payload before the triple-alias multi-partial-store merge corner",
        warmed_load == initial_value
    );

    TEST_SETUP_EXCEPT();
    sh(first_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS partial store through its alias completes normally in the triple-alias multi-partial-store merge corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(second_store_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second HS partial store through a different alias also completes normally in the triple-alias multi-partial-store merge corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger load alias observes the cumulative merged latest bytes after both triple-alias partial stores",
        observed_load == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original alias also reads back the same cumulative merged latest bytes after both triple-alias partial stores",
        observed_base == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the triple-alias multi-partial-store merge corner updates the shared backing memory with the cumulatively merged latest bytes",
        read64(paddr) == expected_value
    );

    TEST_END();
}

bool ai_micro_hs_triple_alias_multi_partial_store_merge_adjacent_preserved_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x620U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x620U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x620U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x620U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x620U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88aa6655beef2211ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the shared payload before the triple-alias multi-partial-store adjacent-preservation corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the triple-alias multi-partial-store adjacent-preservation corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(first_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS partial store through its alias completes normally in the triple-alias multi-partial-store adjacent-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(second_store_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second HS partial store through a different alias also completes normally in the triple-alias multi-partial-store adjacent-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger load alias observes the cumulative merged latest bytes after both triple-alias partial stores in the adjacent-preservation corner",
        observed_load == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original alias also reads back the same cumulative merged latest bytes after both triple-alias partial stores in the adjacent-preservation corner",
        observed_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the triple-alias multi-partial-store merge corner",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the triple-alias multi-partial-store adjacent-preservation corner updates only the shared target word while preserving the adjacent backing word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_triple_alias_full_then_partial_merge_latest_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page full_store_page = SWITCH1;
    const enum test_page partial_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x520U;
    const uintptr_t full_store_vaddr = hs_page_base(full_store_page) + 0x520U;
    const uintptr_t partial_store_vaddr = hs_page_base(partial_store_page) + 0x520U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x520U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x520U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t first_store = 0xaabbccddeeff0011ULL;
    const uint64_t expected_value = 0xaabb44ddeeff0011ULL;
    const uint64_t adjacent = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(full_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(partial_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the seeded payload before the triple-alias full-then-partial merge corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the triple-alias full-then-partial merge corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(full_store_vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the older HS full-width store through its alias completes normally before the younger partial store",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(partial_store_vaddr + 5U, 0x44U);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS byte store through a different alias completes normally and merges into the earlier full-width store payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS load through the fourth alias observes the merged latest payload after the full-store then partial-store sequence",
        observed_load == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias also reads back the same merged latest payload after the full-store then partial-store sequence",
        observed_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the triple-alias full-store then partial-store sequence",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the triple-alias full-then-partial merge corner leaves the shared backing word updated with the merged latest payload while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_triple_alias_partial_then_full_last_writer_wins_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page partial_store_page = SWITCH1;
    const enum test_page full_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x5a0U;
    const uintptr_t partial_store_vaddr = hs_page_base(partial_store_page) + 0x5a0U;
    const uintptr_t full_store_vaddr = hs_page_base(full_store_page) + 0x5a0U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x5a0U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x5a0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t final_store = 0x1021324354657687ULL;
    const uint64_t adjacent = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(partial_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(full_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the seeded payload before the triple-alias partial-then-full last-writer-wins corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the triple-alias partial-then-full last-writer-wins corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(partial_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the older HS partial store through its alias completes normally before the younger full-width store",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(full_store_vaddr, final_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width store through another alias completes normally and becomes the architectural last writer",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS load through the fourth alias observes the final full-width store payload after the earlier partial-store alias write",
        observed_load == final_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias also reads back the last writer payload after the triple-alias partial-then-full sequence",
        observed_base == final_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the triple-alias partial-then-full last-writer-wins sequence",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the triple-alias partial-then-full last-writer-wins corner leaves the shared backing word with the final full-width store payload while preserving the adjacent word",
        read64(paddr) == final_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_load_alias_partial_mask_matchinvalid_sqidx_correct_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x6a0U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x6a0U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x6a0U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x6a0U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x6a0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_after_first = 0x88776655beef2211ULL;
    const uint64_t expected_final = 0x88aa6655beef2211ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the shared payload before the alias partial-mask forward-selection corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the alias partial-mask forward-selection corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(first_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS partial store through its alias completes normally before the younger alias load checks the current blocking store bytes",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after_first = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the first alias partial store, the younger alias load observes exactly the merged latest bytes from that first store without wrong matchInvalid selection",
        observed_after_first == expected_after_first
    );

    TEST_SETUP_EXCEPT();
    sb(second_store_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second HS partial store through another alias also completes normally before the final alias load",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after both alias partial stores, the younger alias load observes the cumulative merged latest bytes instead of stale or cross-store wrong bytes",
        observed_final == expected_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original alias also reads back the same cumulative merged latest bytes after the alias partial-mask sequence",
        observed_base == expected_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the alias partial-mask forward-selection corner",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias partial-mask forward-selection corner updates only the shared backing word with the correct byte-age merge while preserving the adjacent backing word",
        read64(paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_load_alias_same_line_different_word_mask_no_wrong_forward_bytes_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_line_vaddr = hs_page_base(base_page) + 0x6c0U;
    const uintptr_t first_store_line_vaddr = hs_page_base(first_store_page) + 0x6c0U;
    const uintptr_t second_store_line_vaddr = hs_page_base(second_store_page) + 0x6c0U;
    const uintptr_t load_line_vaddr = hs_page_base(load_page) + 0x6c0U;
    const uintptr_t word0_vaddr = load_line_vaddr;
    const uintptr_t word1_vaddr = load_line_vaddr + sizeof(uint64_t);
    const uintptr_t word2_vaddr = load_line_vaddr + (2U * sizeof(uint64_t));
    const uintptr_t paddr = phys_page_base(base_page) + 0x6c0U;
    const uintptr_t word0_paddr = paddr;
    const uintptr_t word1_paddr = paddr + sizeof(uint64_t);
    const uintptr_t word2_paddr = paddr + (2U * sizeof(uint64_t));
    const uint64_t word0_initial = 0x1021324354657687ULL;
    const uint64_t word1_initial = 0x8877665544332211ULL;
    const uint64_t word2_initial = 0xdeadbeefcafed00dULL;
    const uint64_t word0_expected = 0x10213243beef7687ULL;
    const uint64_t word1_expected = 0x8877aa5544332211ULL;

    goto_priv(PRIV_M);
    write64(word0_paddr, word0_initial);
    write64(word1_paddr, word1_initial);
    write64(word2_paddr, word2_initial);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_word0 = ld(base_line_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first word in the shared cache line is readable before the same-line different-word alias forward-byte corner",
        warmed_word0 == word0_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_word1 = ld(base_line_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("the second word in the shared cache line is also readable before the same-line different-word alias forward-byte corner",
        warmed_word1 == word1_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_word2 = ld(base_line_vaddr + (2U * sizeof(uint64_t)));
    AI_ASSERT_NO_EXCEPT_AND("the next word in the same shared cache line is readable before the same-line different-word alias forward-byte corner",
        warmed_word2 == word2_initial
    );

    TEST_SETUP_EXCEPT();
    sh(first_store_line_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first partial store through one alias updates the first shared word without exception before the line-wide forward-byte check",
        true
    );

    TEST_SETUP_EXCEPT();
    sb(second_store_line_vaddr + sizeof(uint64_t) + 5U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the second partial store through another alias updates a different word in the same shared line without exception before the line-wide forward-byte check",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word0 = ld(word0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after same-line different-word alias partial stores, the first word reads back only its own merged bytes with no wrong forward from the second word",
        observed_word0 == word0_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word1 = ld(word1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after same-line different-word alias partial stores, the second word reads back only its own merged bytes with no wrong forward from the first word",
        observed_word1 == word1_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_word2 = ld(word2_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after same-line different-word alias partial stores, the untouched following word in the same line remains unchanged",
        observed_word2 == word2_initial
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-line different-word alias forward-byte corner updates each shared backing word with only its intended mask bytes while preserving the untouched following word",
        read64(word0_paddr) == word0_expected &&
        read64(word1_paddr) == word1_expected &&
        read64(word2_paddr) == word2_initial
    );

    TEST_END();
}

bool ai_micro_hs_store_load_alias_sq_wrap_no_wrong_forward_bytes_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x700U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x700U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x700U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x700U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t filler_base_vaddr = hs_page_base(filler_page);
    const uintptr_t paddr = phys_page_base(base_page) + 0x700U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t filler_sample0_paddr = phys_page_base(filler_page);
    const uintptr_t filler_sample31_paddr = phys_page_base(filler_page) + 31U * 64U;
    const uintptr_t filler_sample63_paddr = phys_page_base(filler_page) + 63U * 64U;
    static const uint64_t initial_values[] = {
        0x8877665544332211ULL,
        0x1021324354657687ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint16_t half_updates[] = {
        0xbeefU,
        0x1234U,
        0xabcdU,
    };
    static const uint8_t byte_updates[] = {
        0xaaU,
        0x55U,
        0x44U,
    };
    static const uint64_t expected_values[] = {
        0x88aa6655beef2211ULL,
        0x1055324312347687ULL,
        0x0f442d3cabcd6978ULL,
    };
    static const uint64_t adjacent_values[] = {
        0x55aa33cc77ee11ffULL,
        0x66778899aabbccddULL,
        0x123456789abcdef0ULL,
    };
    static const uint64_t pressure_first_seeds[] = {
        0x0123456789abcdefULL,
        0x89abcdef01234567ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t pressure_second_seeds[] = {
        0xfedcba9876543210ULL,
        0x76543210fedcba98ULL,
        0xa5b4c3d2e1f00112ULL,
    };
    static const uint64_t pressure_third_seeds[] = {
        0x13579bdf2468ace0ULL,
        0xcafebabedeadbeefULL,
        0x1122334455667788ULL,
    };
    static const uint64_t pressure_fourth_seeds[] = {
        0x02468ace13579bdfULL,
        0x0badf00ddeadbeefULL,
        0x8877665544332211ULL,
    };
    const size_t rounds = sizeof(initial_values) / sizeof(initial_values[0]);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    for (size_t round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        write64(paddr, initial_values[round]);
        write64(adjacent_paddr, adjacent_values[round]);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        uint64_t warmed_base = ld(base_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the shared alias target word is readable before the SQ-wrap alias-forward pressure round begins",
            warmed_base == initial_values[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word is also readable before the SQ-wrap alias-forward pressure round begins",
            warmed_adjacent == adjacent_values[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_overlap_dense_store_pressure_window(filler_base_vaddr, 0U, 64U,
            pressure_first_seeds[round], pressure_second_seeds[round]);
        AI_ASSERT_NO_EXCEPT_AND("a full-page dense store-pressure burst completes before the first alias partial store in the SQ-wrap approximation round",
            ld(filler_base_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(pressure_second_seeds[round], 0U) &&
            ld(filler_base_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(pressure_second_seeds[round], 31U) &&
            ld(filler_base_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(pressure_second_seeds[round], 63U)
        );

        TEST_SETUP_EXCEPT();
        sh(first_store_vaddr + 2U, half_updates[round]);
        AI_ASSERT_NO_EXCEPT_AND("the first alias partial store retires normally before dense younger-store pressure approximates SQ wrap",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_overlap_dense_store_pressure_window(filler_base_vaddr, 0U, 64U,
            pressure_third_seeds[round], pressure_fourth_seeds[round]);
        AI_ASSERT_NO_EXCEPT_AND("a second full-page dense store-pressure burst completes between the two alias partial stores in the SQ-wrap approximation round",
            ld(filler_base_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(pressure_fourth_seeds[round], 0U) &&
            ld(filler_base_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(pressure_fourth_seeds[round], 31U) &&
            ld(filler_base_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(pressure_fourth_seeds[round], 63U)
        );

        TEST_SETUP_EXCEPT();
        sb(second_store_vaddr + 6U, byte_updates[round]);
        AI_ASSERT_NO_EXCEPT_AND("the second alias partial store also retires normally after dense younger-store pressure in the SQ-wrap approximation round",
            true
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_load = ld(load_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after alias partial stores surrounded by full-page younger-store pressure, the alias load still observes only the correct merged latest bytes",
            observed_load == expected_values[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_base = ld(base_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the original alias also reads back the same merged latest bytes after the SQ-wrap alias-forward pressure round",
            observed_base == expected_values[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_adjacent = ld(adjacent_base_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word remains unchanged after the SQ-wrap alias-forward pressure round",
            observed_adjacent == adjacent_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the SQ-wrap alias-forward pressure round leaves the shared backing word with only the intended byte-age merge while preserving the adjacent shared word and representative pressure lines",
            read64(paddr) == expected_values[round] &&
            read64(adjacent_paddr) == adjacent_values[round] &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(pressure_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(pressure_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(pressure_fourth_seeds[round], 63U)
        );
    }

    TEST_END();
}

bool ai_micro_hs_sq_forward_one_cycle_late_result_matches_query_epoch_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x700U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x700U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x700U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x700U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x700U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_after_first = 0x88776655beef2211ULL;
    const uint64_t expected_final = 0x88aa6655beef2211ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the shared payload before the one-cycle-late forward query-epoch corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the one-cycle-late forward query-epoch corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(first_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS partial store through its alias completes normally before the older load captures the current forward-query epoch",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_query_epoch = ld(load_vaddr);
    bool query_epoch_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sb(second_store_vaddr + 6U, 0xaaU);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS partial store through another alias also completes normally immediately after the older load issues in the query-epoch corner",
        true
    );

    TEST_ASSERT("the older HS load keeps the first query epoch's forwarded bytes and does not leak the next store's data into its one-cycle-late return value",
        query_epoch_no_except && observed_query_epoch == expected_after_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later HS load after the younger partial store observes the cumulative final merged bytes in the query-epoch corner",
        observed_final == expected_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias also reads back the final merged bytes after the one-cycle-late forward query-epoch corner",
        observed_base == expected_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the one-cycle-late forward query-epoch corner",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the one-cycle-late forward query-epoch corner preserves the older load result from the first store epoch while backing memory ends with the later cumulative merged bytes and the adjacent word stays intact",
        read64(paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_sq_forward_younger_full_store_no_next_epoch_data_leak_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page first_store_page = SWITCH1;
    const enum test_page second_store_page = SWITCH2;
    const enum test_page load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x720U;
    const uintptr_t first_store_vaddr = hs_page_base(first_store_page) + 0x720U;
    const uintptr_t second_store_vaddr = hs_page_base(second_store_page) + 0x720U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x720U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x720U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t expected_after_first = 0x10213243beef7687ULL;
    const uint64_t younger_full_store = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0xdeadbeefcafed00dULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(first_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias first observes the shared payload before the younger-full-store next-epoch leak corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the younger-full-store next-epoch leak corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(first_store_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the first HS partial store through its alias completes normally before the older load captures the forward-query epoch in the younger-full-store leak corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_query_epoch = ld(load_vaddr);
    bool query_epoch_no_except = (excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    sd(second_store_vaddr, younger_full_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width store through another alias also completes normally immediately after the older load issues in the younger-full-store leak corner",
        true
    );

    TEST_ASSERT("the older HS load keeps the pre-full-store query epoch result and does not return the younger full store's next-epoch payload",
        query_epoch_no_except && observed_query_epoch == expected_after_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_final = ld(load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a later HS load after the younger full-width store observes the final full-width store payload in the younger-full-store leak corner",
        observed_final == younger_full_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original HS alias also reads back the younger full-width store payload after the younger-full-store next-epoch leak corner",
        observed_base == younger_full_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged after the younger-full-store next-epoch leak corner",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger-full-store next-epoch leak corner keeps the older load bound to the first forward-query epoch while backing memory ends with the later full-width store and the adjacent word stays intact",
        read64(paddr) == younger_full_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_loadqueue_raw_late_revoke_no_free_of_reallocated_slot_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x780U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x780U;
    const uintptr_t load0_vaddr = hs_page_base(load0_page) + 0x780U;
    const uintptr_t load1_vaddr = hs_page_base(load1_page) + 0x780U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x780U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t delayed_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;
    uint64_t observed_load0 = 0ULL;
    uint64_t observed_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the target line retires without exception before the delayed-store RAW late-revoke probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the target line retires without exception before the delayed-store RAW late-revoke probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        load0_vaddr, load1_vaddr, delayed_store_value,
        &observed_load0, &observed_load1);
    TEST_ASSERT("under delayed older-store plus cold-load replay pressure, both younger alias loads still retire with the delayed store's final value so a late revoke cannot free a recycled RAW tracker slot",
        excpt.triggered == false &&
        observed_load0 == delayed_store_value &&
        observed_load1 == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the delayed-store RAW late-revoke probe returns the final delayed store payload",
        observed_base == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load1_again = ld(load1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the second younger-load alias after the delayed-store RAW late-revoke probe also returns the final delayed store payload",
        observed_load1_again == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the delayed-store RAW late-revoke probe",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the delayed-store RAW late-revoke corner leaves backing memory with only the delayed store update while preserving the adjacent word",
        read64(paddr) == delayed_store_value &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_loadqueue_raw_flushed_store_no_stale_rollback_redirect_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xb80U;
    const uintptr_t fault_store_vaddr = hs_page_base(fault_store_page) + 0xb80U;
    const uintptr_t load0_vaddr = hs_page_base(load0_page) + 0xb80U;
    const uintptr_t load1_vaddr = hs_page_base(load1_page) + 0xb80U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(base_page) + 0xbc0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xb80U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(base_page) + 0xbc0U;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t recovery_value = 0x99aabbccddeeff00ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;
    uint64_t observed_load0 = 0ULL;
    uint64_t observed_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(marker_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(fault_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    ai_micro_overlap_make_leaf_access_fault(fault_store_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the RAW target line is readable through the base HS alias before the flushed-store stale-rollback probe",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the flushed-store stale-rollback probe",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_faulting_store_then_two_loads_capture(
        fault_store_vaddr, load0_vaddr, load1_vaddr, marker_vaddr,
        fault_store_value, &observed_load0, &observed_load1);
    TEST_ASSERT("when an older delayed store alias is killed by a leaf access fault before RAW select finishes, both younger valid alias loads still retire with the original payload instead of being flushed by a stale rollback redirect",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_store_vaddr &&
        observed_load0 == initial_value &&
        observed_load1 == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the killed delayed store leaves the target and adjacent backing words unchanged while the marker confirms the younger loads continued executing after the fault",
        read64(paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(marker_paddr) == 1ULL
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(base_vaddr, recovery_value);
    AI_ASSERT_NO_EXCEPT_AND("after the faulting delayed store is gone, a later aligned HS store to the same target still retires normally with no stale rollback side effect",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(load1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through a younger alias after the recovery store returns the new payload immediately, showing no delayed stale redirect survived the killed store",
        observed_recovery == recovery_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the flushed-store stale-rollback probe and later recovery store",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the flushed-store stale-rollback corner leaves backing memory with only the later aligned recovery store while preserving the adjacent word",
        read64(paddr) == recovery_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}
