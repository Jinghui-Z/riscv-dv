#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_ARCH_FENCE_ASSERT_STORE_PAGE_FAULT(message, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == CAUSE_SPF && \
        excpt.tval == (vaddr))

#define AI_ARCH_FENCE_ASSERT_LOAD_PAGE_FAULT(message, vaddr) \
    TEST_ASSERT(message, \
        excpt.triggered == true && \
        excpt.cause == CAUSE_LPF && \
        excpt.tval == (vaddr))

static inline pte_t *ai_micro_fence_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_fence_map_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_micro_fence_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_micro_fence_set_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_micro_fence_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline uint64_t ai_micro_fence_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value) : "r"(addr) : "memory");
    return value;
}

static inline void ai_micro_fence_prepare_env(uint64_t first_seed, uint64_t second_seed)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    write64(phys_page_base(SWITCH1), first_seed);
    write64(phys_page_base(SWITCH2), second_seed);
}

static inline uint64_t ai_micro_fence_amoadd_d_aq(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;
    asm volatile (
        "amoadd.d.aq %0, %2, 0(%1)\n\t"
        : "=r"(old_value)
        : "r"(addr), "r"(value)
        : "memory");
    return old_value;
}

static inline uint64_t ai_micro_fence_amoadd_d_rl(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;
    asm volatile (
        "amoadd.d.rl %0, %2, 0(%1)\n\t"
        : "=r"(old_value)
        : "r"(addr), "r"(value)
        : "memory");
    return old_value;
}

static inline uint64_t ai_micro_fence_amoadd_d_aqrl(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;
    asm volatile (
        "amoadd.d.aqrl %0, %2, 0(%1)\n\t"
        : "=r"(old_value)
        : "r"(addr), "r"(value)
        : "memory");
    return old_value;
}

static inline uint64_t ai_micro_fence_lr_d_aq(uintptr_t addr)
{
    uint64_t value;
    asm volatile (
        "lr.d.aq %0, 0(%1)\n\t"
        : "=r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline uint64_t ai_micro_fence_lr_d_aqrl(uintptr_t addr)
{
    uint64_t value;
    asm volatile (
        "lr.d.aqrl %0, 0(%1)\n\t"
        : "=r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline uint64_t ai_micro_fence_sc_d_rl(uintptr_t addr, uint64_t value)
{
    uint64_t status;
    asm volatile (
        "sc.d.rl %0, %2, 0(%1)\n\t"
        : "=r"(status)
        : "r"(addr), "r"(value)
        : "memory");
    return status;
}

static inline uint64_t ai_micro_fence_sc_d_aqrl(uintptr_t addr, uint64_t value)
{
    uint64_t status;
    asm volatile (
        "sc.d.aqrl %0, %2, 0(%1)\n\t"
        : "=r"(status)
        : "r"(addr), "r"(value)
        : "memory");
    return status;
}

static inline uint64_t ai_micro_fence_amoadd_d_aq_probe_rd(
    uintptr_t addr, uint64_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "amoadd.d.aq %0, %2, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr), "r"(value)
        : "memory");
    return rd;
}

static inline uint64_t ai_micro_fence_amoadd_d_rl_probe_rd(
    uintptr_t addr, uint64_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "amoadd.d.rl %0, %2, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr), "r"(value)
        : "memory");
    return rd;
}

static inline uint64_t ai_micro_fence_amoadd_d_aqrl_probe_rd(
    uintptr_t addr, uint64_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "amoadd.d.aqrl %0, %2, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr), "r"(value)
        : "memory");
    return rd;
}

static inline uint64_t ai_micro_fence_lr_d_aq_probe_rd(
    uintptr_t addr, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "lr.d.aq %0, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr)
        : "memory");
    return rd;
}

static inline uint64_t ai_micro_fence_lr_d_aqrl_probe_rd(
    uintptr_t addr, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "lr.d.aqrl %0, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr)
        : "memory");
    return rd;
}

static inline uint64_t ai_micro_fence_sc_d_rl_probe_rd(
    uintptr_t addr, uint64_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "sc.d.rl %0, %2, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr), "r"(value)
        : "memory");
    return rd;
}

static inline uint64_t ai_micro_fence_sc_d_aqrl_probe_rd(
    uintptr_t addr, uint64_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    asm volatile (
        "sc.d.aqrl %0, %2, 0(%1)\n\t"
        : "+&r"(rd)
        : "r"(addr), "r"(value)
        : "memory");
    return rd;
}

bool ai_micro_hs_store_a_fence_store_b_load_b_latest()
{
    TEST_START();

    const uintptr_t store_a_vaddr = hs_page_base(SWITCH1);
    const uintptr_t store_b_vaddr = hs_page_base(SWITCH2);
    const uint64_t store_a_value = 0x1111222233334444ULL;
    const uint64_t store_b_value = 0xaaaabbbbccccddddULL;

    ai_micro_fence_prepare_env(0x0102030405060708ULL, 0x0807060504030201ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(store_a_vaddr, store_a_value);
    AI_ASSERT_NO_EXCEPT_AND("Store A completes successfully before the ordering fence",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence between Store A and Store B completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(store_b_vaddr, store_b_value);
    AI_ASSERT_NO_EXCEPT_AND("Store B completes successfully after the ordering fence",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_b = ld(store_b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("Store A -> FENCE -> Store B -> Load B returns the payload written by Store B",
        loaded_b == store_b_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("both stores reach backing memory with their own final payloads intact",
        read64(phys_page_base(SWITCH1)) == store_a_value &&
        read64(phys_page_base(SWITCH2)) == store_b_value
    );

    TEST_END();
}

bool ai_micro_hs_store_a_fence_store_b_load_a_b_consistent_corner()
{
    TEST_START();

    const uintptr_t store_a_vaddr = hs_page_base(SWITCH1) + 0x40U;
    const uintptr_t store_b_vaddr = hs_page_base(SWITCH2) + 0x48U;
    const uintptr_t store_a_paddr = phys_page_base(SWITCH1) + 0x40U;
    const uintptr_t store_b_paddr = phys_page_base(SWITCH2) + 0x48U;
    const uint64_t store_a_value = 0x1111222233334444ULL;
    const uint64_t store_b_value = 0xaaaabbbbccccddddULL;

    ai_micro_fence_prepare_env(0x0102030405060708ULL, 0x0807060504030201ULL);

    goto_priv(PRIV_M);
    write64(store_a_paddr, 0x1021324354657687ULL);
    write64(store_b_paddr, 0x8796a5b4c3d2e1f0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(store_a_vaddr, store_a_value);
    AI_ASSERT_NO_EXCEPT_AND("Store A completes successfully before the ordering fence in the dual-readback corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence between Store A and Store B completes without exception in the dual-readback corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(store_b_vaddr, store_b_value);
    AI_ASSERT_NO_EXCEPT_AND("Store B completes successfully after the ordering fence in the dual-readback corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_a = ld(store_a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("Store A -> FENCE -> Store B still lets a later Load A observe Store A's payload",
        loaded_a == store_a_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_b = ld(store_b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("Store A -> FENCE -> Store B also lets a later Load B observe Store B's payload",
        loaded_b == store_b_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-readback fence corner leaves both backing locations updated with their own final payloads",
        read64(store_a_paddr) == store_a_value &&
        read64(store_b_paddr) == store_b_value
    );

    TEST_END();
}

bool ai_micro_hs_store_fence_load_reads_latest()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(SWITCH1);
    const uint64_t stored_value = 0x0123456789abcdefULL;

    ai_micro_fence_prepare_env(0xfedcba9876543210ULL, 0x0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the Store in Store -> FENCE -> Load completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the Store completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("Store -> FENCE -> Load reads back the latest stored payload",
        loaded_value == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the backing memory also contains the latest Store payload after the fence",
        read64(phys_page_base(SWITCH1)) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_store_fence_load_adjacent_preserved_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(SWITCH1) + 0x80U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(SWITCH1) + 0x80U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0xfedcba9876543210ULL;
    const uint64_t stored_value = 0x0123456789abcdefULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the Store in Store -> FENCE -> Load adjacent-preservation corner completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the Store completes without exception in the adjacent-preservation corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("Store -> FENCE -> Load still reads back the latest stored payload in the adjacent-preservation corner",
        loaded_value == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the Store -> FENCE -> Load sequence",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the Store -> FENCE -> Load adjacent-preservation corner updates only the target backing word while preserving the adjacent word",
        read64(paddr) == stored_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_partial_fence_load_merged_latest_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(SWITCH1) + 0x1c0U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(SWITCH1) + 0x1c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t expected_value = 0x11223344beef7788ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target HS word first observes the original payload before the partial-Store -> FENCE -> Load merged-latest corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the partial-Store -> FENCE -> Load merged-latest corner",
        warmed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the HS partial store completes successfully before the ordering fence in the merged-latest corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the partial store completes without exception in the merged-latest corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("partial Store -> FENCE -> Load reads back the merged latest payload on the same address",
        loaded_value == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the partial Store -> FENCE -> Load merged-latest corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the partial Store -> FENCE -> Load merged-latest corner updates only the target backing word while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_partial_fence_store_same_addr_last_writer_wins_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(SWITCH1) + 0x280U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(SWITCH1) + 0x280U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t second_store = 0xdeca1234567890efULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target HS word first observes the original payload before the partial-Store -> FENCE -> Store same-address last-writer-wins corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 4U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the older HS partial store completes successfully before the ordering fence in the same-address last-writer-wins corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence between the older partial store and younger full-width store completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS full-width store completes successfully after the ordering fence in the same-address last-writer-wins corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store -> FENCE -> Store on the same address, a later load observes the younger full-width store as the architectural last writer",
        loaded_value == second_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged in the partial-Store -> FENCE -> Store same-address last-writer-wins corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the partial-Store -> FENCE -> Store same-address last-writer-wins corner leaves the target backing word with the younger full-width store payload while preserving the adjacent word",
        read64(paddr) == second_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_fence_partial_store_same_addr_merged_latest_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(SWITCH1) + 0x2c0U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(SWITCH1) + 0x2c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t first_store = 0xaabbccddeeff0011ULL;
    const uint64_t expected_value = 0xaabbccdd44550011ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target HS word first observes the original payload before the Store -> FENCE -> partial Store same-address merged-latest corner",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the older HS full-width store completes successfully before the ordering fence in the same-address merged-latest corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence between the older full-width store and younger partial store completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sh(vaddr + 2U, 0x4455U);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS partial store completes successfully after the ordering fence and merges into the earlier full-width store payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE -> partial Store on the same address, a later load observes the merged latest payload",
        loaded_value == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged in the Store -> FENCE -> partial Store same-address merged-latest corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the Store -> FENCE -> partial Store same-address merged-latest corner leaves the target backing word with the merged latest payload while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_fence_load_alias_latest_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xc0U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0xc0U;
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0xc0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t stored_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the original shared payload before the alias Store -> FENCE -> Load corner",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the HS store through the base alias completes successfully before the ordering fence in the alias corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence between the base-alias store and alias load completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store(alias0) -> FENCE -> Load(alias1), the alias load observes the latest shared payload",
        loaded_alias == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word also remains unchanged in the alias Store -> FENCE -> Load corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias Store -> FENCE -> Load corner updates only the shared target backing word while preserving the adjacent word",
        read64(paddr) == stored_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_partial_fence_load_alias_merged_latest_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x200U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x200U;
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x200U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88776655beef2211ULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the original shared payload before the alias partial-Store -> FENCE -> Load merged-latest corner",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sh(base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the HS partial store through the base alias completes successfully before the ordering fence in the alias merged-latest corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence between the base-alias partial store and alias load completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store(alias0) -> FENCE -> Load(alias1), the alias load observes the merged latest shared payload",
        loaded_alias == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base alias also reads back the same merged latest shared payload after the alias partial-store fence corner",
        loaded_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word remains unchanged in the alias partial-Store -> FENCE -> Load merged-latest corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias partial-Store -> FENCE -> Load merged-latest corner updates only the shared target backing word while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_partial_fence_partial_store_alias_merged_latest_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x300U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x300U;
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x300U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t expected_value = 0x88774455beef2211ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base alias first observes the original shared payload before the partial Store(alias0) -> FENCE -> partial Store(alias1) merged-latest corner",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the remapped alias also first observes the original shared payload before the alias partial-store merged-latest corner",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sh(base_vaddr + 2U, 0xbeefU);
    AI_ASSERT_NO_EXCEPT_AND("the older HS partial store through the base alias completes successfully before the ordering fence in the alias merged-latest corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence between the two alias partial stores completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sh(alias_vaddr + 4U, 0x4455U);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS partial store through the remapped alias completes successfully after the ordering fence and merges into the earlier shared payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store(alias0) -> FENCE -> partial Store(alias1), the base alias observes the merged latest shared payload",
        loaded_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after partial Store(alias0) -> FENCE -> partial Store(alias1), the remapped alias also observes the merged latest shared payload",
        loaded_alias == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word remains unchanged in the alias partial-Store -> FENCE -> partial-Store merged-latest corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias partial-Store -> FENCE -> partial-Store merged-latest corner leaves the shared backing word with the merged latest payload while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_same_pa_diff_va_alias_triggers_clean_drain_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_first_vaddr = hs_page_base(base_page) + 0x340U;
    const uintptr_t alias_first_vaddr = hs_page_base(alias_page) + 0x340U;
    const uintptr_t base_second_vaddr = hs_page_base(base_page) + 0x348U;
    const uintptr_t alias_second_vaddr = hs_page_base(alias_page) + 0x348U;
    const uintptr_t adjacent_alias_vaddr = hs_page_base(alias_page) + 0x350U;
    const uintptr_t first_paddr = phys_page_base(base_page) + 0x340U;
    const uintptr_t second_paddr = first_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = second_paddr + sizeof(uint64_t);
    const uint64_t initial_first = 0x0102030405060708ULL;
    const uint64_t initial_second = 0x1112131415161718ULL;
    const uint64_t stored_first = 0xa1b2c3d4e5f60718ULL;
    const uint64_t stored_second = 0x0fedcba987654321ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(first_paddr, initial_first);
    write64(second_paddr, initial_second);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_first = ld(alias_first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the original first shared doubleword before the same-PA different-VA sbuffer-drain probe",
        warmed_first == initial_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second = ld(base_second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base HS load first observes the original second shared doubleword before the same-PA different-VA sbuffer-drain probe",
        warmed_second == initial_second
    );

    TEST_SETUP_EXCEPT();
    sd(base_first_vaddr, stored_first);
    AI_ASSERT_NO_EXCEPT_AND("the first HS store through the base VA completes successfully before the same-PA different-VA same-block alias merge probe",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(alias_second_vaddr, stored_second);
    AI_ASSERT_NO_EXCEPT_AND("the second HS store through the alias VA to the same physical cache block also completes successfully before the drain-observing fence",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the same-PA different-VA same-block alias stores completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_first = ld(alias_first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the same-PA different-VA same-block alias stores and the following fence, the alias view observes the first stored payload without stale forwarding",
        loaded_first == stored_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_second = ld(base_second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the same-PA different-VA same-block alias stores and the following fence, the base view observes the second stored payload without stale forwarding",
        loaded_second == stored_second
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared doubleword remains unchanged across the same-PA different-VA sbuffer drain probe",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-PA different-VA same-block alias stores leave backing memory fully merged after the drain point while preserving the adjacent doubleword",
        read64(first_paddr) == stored_first &&
        read64(second_paddr) == stored_second &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_sbuffer_synonym_alias_no_stale_forward_after_drain_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_first_vaddr = hs_page_base(base_page) + 0x3c0U;
    const uintptr_t alias_first_vaddr = hs_page_base(alias_page) + 0x3c0U;
    const uintptr_t base_second_vaddr = hs_page_base(base_page) + 0x3c8U;
    const uintptr_t alias_second_vaddr = hs_page_base(alias_page) + 0x3c8U;
    const uintptr_t adjacent_alias_vaddr = hs_page_base(alias_page) + 0x3d0U;
    const uintptr_t first_paddr = phys_page_base(base_page) + 0x3c0U;
    const uintptr_t second_paddr = first_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = second_paddr + sizeof(uint64_t);
    const uint64_t initial_first = 0x1020304050607080ULL;
    const uint64_t initial_second = 0x0102030405060708ULL;
    const uint64_t first_after_drain_probe = 0x8877665544332211ULL;
    const uint64_t second_after_drain_probe = 0x0fedcba987654321ULL;
    const uint64_t first_after_post_drain_overwrite = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x1122334455667788ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(first_paddr, initial_first);
    write64(second_paddr, initial_second);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_first = ld(alias_first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the original first shared doubleword before the synonym-alias stale-forwarding regression probe",
        warmed_first == initial_first
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second = ld(base_second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base HS load first observes the original second shared doubleword before the synonym-alias stale-forwarding regression probe",
        warmed_second == initial_second
    );

    TEST_SETUP_EXCEPT();
    sd(base_first_vaddr, first_after_drain_probe);
    AI_ASSERT_NO_EXCEPT_AND("the first HS store through the base VA completes successfully before the synonym-alias drain-observing fence",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(alias_second_vaddr, second_after_drain_probe);
    AI_ASSERT_NO_EXCEPT_AND("the second HS store through the alias VA to the same physical cache block also completes successfully before the synonym-alias drain-observing fence",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the synonym-alias same-block stores completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_after_drain = ld(alias_first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the synonym-alias same-block stores and the following drain-observing fence, the alias view observes the first stored payload without stale forwarding",
        observed_first_after_drain == first_after_drain_probe
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_after_drain = ld(base_second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the synonym-alias same-block stores and the following drain-observing fence, the base view observes the second stored payload without stale forwarding",
        observed_second_after_drain == second_after_drain_probe
    );

    TEST_SETUP_EXCEPT();
    sd(alias_first_vaddr, first_after_post_drain_overwrite);
    AI_ASSERT_NO_EXCEPT_AND("after the earlier synonym-alias drain has completed, a later overwrite through the alias VA still completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base_after_post_drain = ld(base_first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the post-drain overwrite through the alias VA, the base view immediately observes the newest payload instead of a stale pre-drain value",
        observed_base_after_post_drain == first_after_post_drain_overwrite
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_after_post_drain = ld(alias_first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the post-drain overwrite through the alias VA, the alias view also observes the newest payload immediately",
        observed_alias_after_post_drain == first_after_post_drain_overwrite
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_after_post_drain = ld(alias_second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second shared doubleword remains at the payload established before the drain point when the post-drain overwrite only targets the first word",
        observed_second_after_post_drain == second_after_drain_probe
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared doubleword remains unchanged across the synonym-alias drain and the later post-drain overwrite",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the synonym-alias drain regression corner leaves backing memory with the post-drain overwrite on the first word, the pre-drain second-word update intact, and the adjacent word preserved",
        read64(first_paddr) == first_after_post_drain_overwrite &&
        read64(second_paddr) == second_after_drain_probe &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_fence_store_same_addr_last_writer_wins_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(SWITCH1) + 0x100U;
    const uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t first_store = 0x1122334455667788ULL;
    const uint64_t second_store = 0xaabbccddeeff0011ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the first HS store to the target address completes successfully before the ordering fence in the same-address last-writer-wins corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence between the two same-address stores completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    AI_ASSERT_NO_EXCEPT_AND("the second HS store to the same address completes successfully after the ordering fence",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_value = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE -> Store on the same address, a later load observes the second store as the architectural last writer",
        loaded_value == second_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged in the same-address Store -> FENCE -> Store last-writer-wins corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-address Store -> FENCE -> Store last-writer-wins corner leaves the target backing word with the second store payload while preserving the adjacent word",
        read64(paddr) == second_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_fence_partial_store_alias_merged_latest_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x240U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x240U;
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x240U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t first_store = 0xaabbccddeeff0011ULL;
    const uint64_t expected_value = 0xaabbcc4455ff0011ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the original shared payload before the Store(alias0) -> FENCE -> partial Store(alias1) merged-latest corner",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the first HS full-width store through the base alias completes successfully before the ordering fence in the alias merged-latest corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence between the base-alias full store and alias partial store completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sh(alias_vaddr + 3U, 0x4455U);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS partial store through the remapped alias completes successfully after the ordering fence and merges into the earlier full-width store payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store(alias0) -> FENCE -> partial Store(alias1), the base alias observes the merged latest shared payload",
        loaded_base == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store(alias0) -> FENCE -> partial Store(alias1), the remapped alias also observes the merged latest shared payload",
        loaded_alias == expected_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word remains unchanged in the alias full-store then partial-store fence corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias full-store then partial-store fence corner leaves the shared backing word with the merged latest payload while preserving the adjacent word",
        read64(paddr) == expected_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_fence_store_alias_last_writer_wins_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x140U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x140U;
    const uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x140U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t first_store = 0x123456789abcdef0ULL;
    const uint64_t second_store = 0x0fedcba987654321ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the original shared payload before the alias Store -> FENCE -> Store last-writer-wins corner",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(base_vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the first HS store through the base alias completes successfully before the ordering fence in the alias last-writer-wins corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence between the two alias stores completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, second_store);
    AI_ASSERT_NO_EXCEPT_AND("the second HS store through the remapped alias completes successfully after the ordering fence",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store(alias0) -> FENCE -> Store(alias1), the base alias observes the second store as the final shared payload",
        loaded_base == second_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store(alias0) -> FENCE -> Store(alias1), the remapped alias also observes the second store as the final shared payload",
        loaded_alias == second_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word remains unchanged in the alias Store -> FENCE -> Store last-writer-wins corner",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias Store -> FENCE -> Store last-writer-wins corner leaves the shared backing word with the second store payload while preserving the adjacent word",
        read64(paddr) == second_store &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_alias_double_fence_loads_stable_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x180U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x180U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x180U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t stored_value = 0x8796a5b4c3d2e1f0ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(base_vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the HS store through the base alias completes successfully before the consecutive fences in the alias stability corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the first fence in the alias double-fence stability corner completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the second fence in the alias double-fence stability corner completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store(alias0) -> FENCE -> FENCE -> Load(alias1), the alias load still observes the latest shared payload",
        loaded_alias == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the original base alias also still observes the same latest shared payload after consecutive fences",
        loaded_base == stored_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent shared word remains unchanged after the alias double-fence sequence",
        loaded_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the alias double-fence stability corner leaves the shared backing word updated with the store payload while preserving the adjacent word",
        read64(paddr) == stored_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_back_to_back_fence_no_exception()
{
    TEST_START();

    const uintptr_t first_vaddr = hs_page_base(SWITCH1);
    const uintptr_t second_vaddr = hs_page_base(SWITCH2);
    const uint64_t first_value = 0x13579bdf2468ace0ULL;
    const uint64_t second_value = 0x0eca8642fdb97531ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(first_vaddr, first_value);
    AI_ASSERT_NO_EXCEPT_AND("the pipeline still accepts a Store before the back-to-back fence sequence",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the first fence in a consecutive sequence completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the second fence in a consecutive sequence completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the third fence in a consecutive sequence completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fourth fence in a consecutive sequence completes without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, second_value);
    AI_ASSERT_NO_EXCEPT_AND("a Store issued after consecutive fences still completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_loaded = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first payload remains readable after multiple consecutive fences",
        first_loaded == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_loaded = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second payload remains readable after multiple consecutive fences",
        second_loaded == second_value
    );

    TEST_END();
}

bool ai_micro_hs_fence_variant_pred_succ_matrix_no_exception_corner()
{
    TEST_START();

    const uintptr_t first_vaddr = hs_page_base(SWITCH1) + 0x440U;
    const uintptr_t second_vaddr = hs_page_base(SWITCH2) + 0x448U;
    const uintptr_t first_paddr = phys_page_base(SWITCH1) + 0x440U;
    const uintptr_t second_paddr = phys_page_base(SWITCH2) + 0x448U;
    const uint64_t first_initial = 0x0102030405060708ULL;
    const uint64_t second_initial = 0x8877665544332211ULL;
    const uint64_t first_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_value = 0x0fedcba987654321ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(first_paddr, first_initial);
    write64(second_paddr, second_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(first_vaddr, first_value);
    AI_ASSERT_NO_EXCEPT_AND("the older HS store completes before the fence variant matrix",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile (
        "fence rw, rw\n\t"
        "fence r, w\n\t"
        "fence w, r\n\t"
        "fence io, io\n\t"
        "fence iorw, iorw\n\t"
        "fence.tso\n\t"
        ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("rw/rw, weak memory, io/io, iorw/iorw, and tso fence variants retire without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, second_value);
    AI_ASSERT_NO_EXCEPT_AND("the younger HS store completes after the fence variant matrix",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_loaded = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the older store payload remains visible after the fence variant matrix",
        first_loaded == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_loaded = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the younger store payload remains visible after the fence variant matrix",
        second_loaded == second_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fence variant matrix leaves both backing locations with their intended payloads",
        read64(first_paddr) == first_value &&
        read64(second_paddr) == second_value
    );

    TEST_END();
}

bool ai_micro_hs_fence_pred_succ_load_store_load_load_matrix_corner()
{
    TEST_START();

    const uintptr_t first_vaddr = hs_page_base(SWITCH1) + 0x4c0U;
    const uintptr_t second_vaddr = hs_page_base(SWITCH2) + 0x4c8U;
    const uintptr_t first_paddr = phys_page_base(SWITCH1) + 0x4c0U;
    const uintptr_t second_paddr = phys_page_base(SWITCH2) + 0x4c8U;
    const uint64_t first_initial = 0x1122334455667788ULL;
    const uint64_t second_initial = 0x8877665544332211ULL;
    const uint64_t first_after_tso = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_after_store = 0x0fedcba987654321ULL;
    const uint64_t second_after_fence_i = 0x13579bdf2468ace0ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(first_paddr, first_initial);
    write64(second_paddr, second_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_observed_before_store = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the older load observes the first seed before the fence r,w Load->Store subcase",
        first_observed_before_store == first_initial
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence r, w" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence r,w retires between an older load and a younger store",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, second_after_store);
    AI_ASSERT_NO_EXCEPT_AND("the younger store after fence r,w commits its payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after_rw = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Load -> FENCE r,w -> Store, a later load observes the younger store payload",
        second_after_rw == second_after_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_load_before_rr = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first load in the fence r,r Load->Load subcase observes the original first seed",
        first_load_before_rr == first_initial
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence r, r" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence r,r retires between two loads",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_load_after_rr = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Load -> FENCE r,r -> Load, the second load observes the current second payload",
        second_load_after_rr == second_after_store
    );

    TEST_SETUP_EXCEPT();
    sd(first_vaddr, first_after_tso);
    AI_ASSERT_NO_EXCEPT_AND("the store before the fence.tso subcase commits successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence.tso" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence.tso retires as a legal ordering instruction in the single-hart subcase",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_load_after_tso = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE.TSO -> Load on the same address, the load observes the latest store payload",
        first_load_after_tso == first_after_tso
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, second_after_fence_i);
    AI_ASSERT_NO_EXCEPT_AND("the store before the fence.i negative-control subcase commits successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence.i" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence.i retires without acting as a data-ordering fence oracle in this single-hart negative-control subcase",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_load_after_fence_i = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE.I -> same-address Load, only ordinary same-hart same-address visibility is checked",
        second_load_after_fence_i == second_after_fence_i
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fence pred/succ load-store/load-load matrix leaves backing memory with the final single-hart payloads",
        read64(first_paddr) == first_after_tso &&
        read64(second_paddr) == second_after_fence_i
    );

    TEST_END();
}

bool ai_micro_hs_fence_pred_succ_full_rw_matrix_corner()
{
    TEST_START();

    const uintptr_t first_vaddr = hs_page_base(SWITCH1) + 0x540U;
    const uintptr_t second_vaddr = hs_page_base(SWITCH2) + 0x548U;
    const uintptr_t first_paddr = phys_page_base(SWITCH1) + 0x540U;
    const uintptr_t second_paddr = phys_page_base(SWITCH2) + 0x548U;
    const uint64_t first_initial = 0x0123456789abcdefULL;
    const uint64_t second_initial = 0xfedcba9876543210ULL;
    const uint64_t wr_value = 0x1111222233334444ULL;
    const uint64_t ww_first_value = 0x5555666677778888ULL;
    const uint64_t ww_second_value = 0x9999aaaabbbbccccULL;
    const uint64_t rw_second_value = 0x0badc0de12345678ULL;
    const uint64_t final_first_value = 0x13579bdf2468ace0ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(first_paddr, first_initial);
    write64(second_paddr, second_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(first_vaddr, wr_value);
    AI_ASSERT_NO_EXCEPT_AND("the older Store in the FENCE w,r subcase commits before the fence",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence w, r" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence w,r retires between an older Store and a younger Load",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t wr_load = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE w,r -> Load, the same-hart load observes the older store payload",
        wr_load == wr_value
    );

    TEST_SETUP_EXCEPT();
    sd(first_vaddr, ww_first_value);
    AI_ASSERT_NO_EXCEPT_AND("the older Store in the FENCE w,w subcase commits before the fence",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence w, w" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence w,w retires between two stores",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, ww_second_value);
    AI_ASSERT_NO_EXCEPT_AND("the younger Store after fence w,w commits its payload",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_after_ww = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE w,w -> Store, the older store payload remains visible",
        first_after_ww == ww_first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after_ww = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Store -> FENCE w,w -> Store, the younger store payload is also visible",
        second_after_ww == ww_second_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_before_rw = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the older Load in the FENCE rw,rw subcase observes the current first payload",
        first_before_rw == ww_first_value
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, rw_second_value);
    AI_ASSERT_NO_EXCEPT_AND("the older Store in the FENCE rw,rw subcase commits before the full RW fence",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fm=0000 fence rw,rw retires as a full ordinary-memory ordering point",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after_rw = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after Load+Store -> FENCE rw,rw -> Load, the load observes the latest second payload",
        second_after_rw == rw_second_value
    );

    TEST_SETUP_EXCEPT();
    sd(first_vaddr, final_first_value);
    AI_ASSERT_NO_EXCEPT_AND("after Load+Store -> FENCE rw,rw -> Store, the younger store commits successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile (".word 0x0030000f" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("a FENCE with empty pred and RW succ retires as a legal weak-ordering boundary",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile (".word 0x0300000f" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("a FENCE with RW pred and empty succ retires as a legal weak-ordering boundary",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence w, w" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("a single-bit ordinary-memory FENCE w,w retires without disturbing final payloads",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_first = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the pred/succ matrix, the first word holds the final younger store payload",
        final_first == final_first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_second = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the pred/succ matrix, the second word holds the final RW-fence store payload",
        final_second == rw_second_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the full pred/succ matrix updates only the intended backing locations",
        read64(first_paddr) == final_first_value &&
        read64(second_paddr) == rw_second_value
    );

    TEST_END();
}

bool ai_micro_hs_fence_single_hart_litmus_degenerate_matrix_corner()
{
    TEST_START();

    const uintptr_t a_vaddr = hs_page_base(SWITCH1) + 0x640U;
    const uintptr_t b_vaddr = hs_page_base(SWITCH1) + 0x648U;
    const uintptr_t c_vaddr = hs_page_base(SWITCH2) + 0x650U;
    const uintptr_t a_paddr = phys_page_base(SWITCH1) + 0x640U;
    const uintptr_t b_paddr = phys_page_base(SWITCH1) + 0x648U;
    const uintptr_t c_paddr = phys_page_base(SWITCH2) + 0x650U;
    const uint64_t a_seed = 0x0101010101010101ULL;
    const uint64_t b_seed = 0x0202020202020202ULL;
    const uint64_t c_seed = 0x0303030303030303ULL;
    const uint64_t mp_a = 0x1111111111111111ULL;
    const uint64_t mp_b = 0x2222222222222222ULL;
    const uint64_t lb_b = 0x3333333333333333ULL;
    const uint64_t wrc_a = 0x4444444444444444ULL;
    const uint64_t iriw_a = 0x5555555555555555ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(a_paddr, a_seed);
    write64(b_paddr, b_seed);
    write64(c_paddr, c_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(a_vaddr, mp_a);
    AI_ASSERT_NO_EXCEPT_AND("MP single-hart producer subcase stores A before fence rw,rw",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("MP single-hart producer subcase retires fence rw,rw before storing B",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(b_vaddr, mp_b);
    AI_ASSERT_NO_EXCEPT_AND("MP single-hart producer subcase stores B after fence rw,rw",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t mp_read_b = ld(b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("MP single-hart degenerate observer reads B=1 after the fenced producer sequence",
        mp_read_b == mp_b
    );

    TEST_SETUP_EXCEPT();
    uint64_t mp_read_a = ld(a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("MP single-hart degenerate observer then reads A=1 after observing B=1",
        mp_read_a == mp_a
    );

    TEST_SETUP_EXCEPT();
    uint64_t lb_read_a = ld(a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("LB single-hart subcase load reads the current A value before fence w,r",
        lb_read_a == mp_a
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence w, r" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("LB single-hart subcase retires fence w,r before the later store/load pair",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(b_vaddr, lb_b);
    AI_ASSERT_NO_EXCEPT_AND("LB single-hart subcase stores B after fence w,r",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t lb_read_b = ld(b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("LB single-hart subcase later reads the B payload it just stored",
        lb_read_b == lb_b
    );

    TEST_SETUP_EXCEPT();
    sd(a_vaddr, wrc_a);
    AI_ASSERT_NO_EXCEPT_AND("WRC single-hart subcase stores A before fence.tso",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence.tso" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("WRC single-hart subcase retires fence.tso as a legal ordering instruction",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t wrc_read_b = ld(b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("WRC single-hart subcase can read the current B value after fence.tso",
        wrc_read_b == lb_b
    );

    TEST_SETUP_EXCEPT();
    uint64_t wrc_read_a = ld(a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("WRC single-hart subcase reads back the A payload stored before fence.tso",
        wrc_read_a == wrc_a
    );

    TEST_SETUP_EXCEPT();
    sd(a_vaddr, iriw_a);
    AI_ASSERT_NO_EXCEPT_AND("IRIW single-hart degenerate subcase stores A before the full fence",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("IRIW single-hart degenerate subcase retires the full fence before independent reads",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t iriw_read_b = ld(b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("IRIW single-hart degenerate subcase reads the current independent B payload",
        iriw_read_b == lb_b
    );

    TEST_SETUP_EXCEPT();
    uint64_t iriw_read_c = ld(c_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("IRIW single-hart degenerate subcase reads the independent C seed",
        iriw_read_c == c_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t iriw_read_a = ld(a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("IRIW single-hart degenerate subcase reads A=1 after the fenced store",
        iriw_read_a == iriw_a
    );

    TEST_SETUP_EXCEPT();
    uint64_t rwc_read_a = ld(a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("RWC single-hart subcase first reads the current A value",
        rwc_read_a == iriw_a
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence r, r" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("RWC single-hart subcase retires fence r,r between two loads",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t rwc_read_b = ld(b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("RWC single-hart subcase then reads the current B value after fence r,r",
        rwc_read_b == lb_b
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the single-hart litmus-degenerate matrix leaves A/B/C in their expected final states",
        read64(a_paddr) == iriw_a &&
        read64(b_paddr) == lb_b &&
        read64(c_paddr) == c_seed
    );

    TEST_END();
}

bool ai_arch_fence_amo_aqrl_pred_succ_overlay_corner()
{
    TEST_START();

    const uintptr_t target_vaddr = hs_page_base(SWITCH1) + 0x700U;
    const uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(SWITCH1) + 0x700U;
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x0000000000000100ULL;
    const uint64_t after_aq = 0x0000000000000110ULL;
    const uint64_t after_rl = 0x0000000000000130ULL;
    const uint64_t after_aqrl = 0x0000000000000160ULL;
    const uint64_t adjacent = 0xf0e1d2c3b4a59687ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(target_paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the full ordinary-memory fence retires before the AMO aq overlay subcase",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_aq = ai_micro_fence_amoadd_d_aq(target_vaddr, 0x10ULL);
    AI_ASSERT_NO_EXCEPT_AND("amoadd.d.aq after fence rw,rw returns the old value and performs the RMW update",
        old_aq == initial
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence r, w" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence r,w retires between the AMO aq result check and the AMO rl update",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after_aq = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the load after FENCE r,w observes the AMO aq update",
        observed_after_aq == after_aq
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_rl = ai_micro_fence_amoadd_d_rl(target_vaddr, 0x20ULL);
    AI_ASSERT_NO_EXCEPT_AND("amoadd.d.rl in the pred/succ overlay sequence keeps normal old-value RMW semantics",
        old_rl == after_aq
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence w, r" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence w,r retires after the AMO rl update and before the next load/AMO pair",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after_rl = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the load after FENCE w,r observes the AMO rl update",
        observed_after_rl == after_rl
    );

    TEST_SETUP_EXCEPT();
    uint64_t old_aqrl = ai_micro_fence_amoadd_d_aqrl(target_vaddr, 0x30ULL);
    AI_ASSERT_NO_EXCEPT_AND("amoadd.d.aqrl after the FENCE sequence keeps normal old-value RMW semantics",
        old_aqrl == after_rl
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the final full fence retires after the AMO aqrl update",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_value = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the final load observes the accumulated AMO aq/rl/aqrl result across the FENCE overlays",
        final_value == after_aqrl
    );

    TEST_SETUP_EXCEPT();
    uint64_t adjacent_value = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word is not modified by the AMO aq/rl plus FENCE overlay sequence",
        adjacent_value == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory reflects only the intended AMO aq/rl/aqrl accumulated update",
        read64(target_paddr) == after_aqrl &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_fence_lrsc_aqrl_pred_succ_overlay_corner()
{
    TEST_START();

    const uintptr_t target_vaddr = hs_page_base(SWITCH1) + 0x780U;
    const uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = phys_page_base(SWITCH1) + 0x780U;
    const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t initial = 0x0102030405060708ULL;
    const uint64_t first_sc_payload = 0x1111222233334444ULL;
    const uint64_t stale_sc_payload = 0x5555666677778888ULL;
    const uint64_t second_sc_payload = 0x9999aaaabbbbccccULL;
    const uint64_t adjacent = 0x8877665544332211ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(target_paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    asm volatile ("fence r, w" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence r,w retires before the first contiguous LR/SC aqrl pair",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_lr;
    uint64_t first_sc_status;
    asm volatile(
        "lr.d.aq %[lr], 0(%[addr])\n\t"
        "sc.d.rl %[status], %[payload], 0(%[addr])\n\t"
        : [lr] "=&r"(first_lr), [status] "=&r"(first_sc_status)
        : [addr] "r"(target_vaddr), [payload] "r"(first_sc_payload)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("contiguous lr.d.aq/sc.d.rl after the FENCE r,w overlay captures the seed and succeeds",
        first_lr == initial &&
        first_sc_status == 0
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the full fence retires after the successful sc.d.rl before the stale-SC fail path",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t stale_sc_status = ai_micro_fence_sc_d_rl(target_vaddr, stale_sc_payload);
    AI_ASSERT_NO_EXCEPT_AND("a stale sc.d.rl without a fresh LR fails after the successful SC consumed the reservation",
        stale_sc_status != 0
    );

    TEST_SETUP_EXCEPT();
    uint64_t after_stale_sc = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the failed stale SC after FENCE leaves the first successful SC payload intact",
        after_stale_sc == first_sc_payload
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence w, r" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence w,r retires before refreshing the LR/SC aqrl sequence",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence r, w" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("fence r,w retires before refreshing the contiguous lr.d.aqrl/sc.d.aqrl pair",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_lr;
    uint64_t second_sc_status;
    asm volatile(
        "lr.d.aqrl %[lr], 0(%[addr])\n\t"
        "sc.d.aqrl %[status], %[payload], 0(%[addr])\n\t"
        : [lr] "=&r"(second_lr), [status] "=&r"(second_sc_status)
        : [addr] "r"(target_vaddr), [payload] "r"(second_sc_payload)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("contiguous lr.d.aqrl/sc.d.aqrl refreshes the reservation after FENCE and succeeds",
        second_lr == first_sc_payload &&
        second_sc_status == 0
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_value = ld(target_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the final load observes the second successful SC payload after the FENCE/LRSC overlay",
        final_value == second_sc_payload
    );

    TEST_SETUP_EXCEPT();
    uint64_t adjacent_value = ld(adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word is not modified by the FENCE plus LR/SC aqrl overlay sequence",
        adjacent_value == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory reflects the successful LR/SC aqrl path while the stale-SC payload never commits",
        read64(target_paddr) == second_sc_payload &&
        read64(target_paddr) != stale_sc_payload &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_fence_atomic_aqrl_single_hart_matrix_corner()
{
    TEST_START();

    const uintptr_t amo_vaddr = hs_page_base(SWITCH1) + 0x740U;
    const uintptr_t lrsc_vaddr = hs_page_base(SWITCH2) + 0x748U;
    const uintptr_t amo_paddr = phys_page_base(SWITCH1) + 0x740U;
    const uintptr_t lrsc_paddr = phys_page_base(SWITCH2) + 0x748U;
    const uint64_t amo_initial = 0x1000000000000010ULL;
    const uint64_t lrsc_initial = 0x2000000000000020ULL;
    const uint64_t amo_addend = 0x0000000000000101ULL;
    const uint64_t lrsc_success_value = 0x3000000000000030ULL;
    const uint64_t lrsc_intervening_value = 0x4000000000000040ULL;
    const uint64_t lrsc_final_value = 0x5000000000000050ULL;

    ai_micro_fence_prepare_env(0x0ULL, 0x0ULL);

    goto_priv(PRIV_M);
    write64(amo_paddr, amo_initial);
    write64(lrsc_paddr, lrsc_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(amo_vaddr, amo_initial);
    AI_ASSERT_NO_EXCEPT_AND("the ordinary store before the AMO aqrl plus fence subcase initializes the target",
        true
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the fence before AMO aqrl retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old;
    asm volatile(
        "amoadd.d.aqrl %0, %2, 0(%1)\n\t"
        : "=r"(amo_old)
        : "r"(amo_vaddr), "r"(amo_addend)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("AMO aqrl after a full fence returns the old value and performs the RMW update",
        amo_old == amo_initial
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the fence after AMO aqrl retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_after = ld(amo_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after FENCE + AMO aqrl + FENCE, the load observes the AMO-updated payload",
        amo_after == amo_initial + amo_addend
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the fence before the LR aq / SC rl success path retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t sc_status = lrsc_success_value;
    uint64_t lr_value;
    asm volatile(
        "lr.d.aq %[lr], 0(%[addr])\n\t"
        "sc.d.rl %[status], %[value], 0(%[addr])\n\t"
        : [lr] "=&r"(lr_value), [status] "=&r"(sc_status)
        : [addr] "r"(lrsc_vaddr), [value] "r"(lrsc_success_value)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("LR aq / SC rl succeeds on the matching reservation when no ordering instruction is inserted inside the pair",
        lr_value == lrsc_initial &&
        sc_status == 0U
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the fence after the successful LR aq / SC rl pair retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t lrsc_after_success = ld(lrsc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after FENCE + LR aq / SC rl + FENCE, the LR/SC target holds the SC payload",
        lrsc_after_success == lrsc_success_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t sc_intervening_status = lrsc_intervening_value;
    asm volatile(
        "lr.d.aq %[lr], 0(%[addr])\n\t"
        "fence rw, rw\n\t"
        "sc.d.rl %[status], %[value], 0(%[addr])\n\t"
        : [lr] "=&r"(lr_value), [status] "=&r"(sc_intervening_status)
        : [addr] "r"(lrsc_vaddr), [value] "r"(lrsc_intervening_value)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("LR aq / FENCE / SC rl observes the committed payload and returns a status without trapping",
        lr_value == lrsc_success_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t lrsc_after_intervening = ld(lrsc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the intra-pair-fence SC status matches the observed memory side effect",
        ((sc_intervening_status == 0U) &&
            (lrsc_after_intervening == lrsc_intervening_value)) ||
        ((sc_intervening_status != 0U) &&
            (lrsc_after_intervening == lrsc_success_value))
    );

    TEST_SETUP_EXCEPT();
    uint64_t final_status = lrsc_final_value;
    asm volatile(
        "lr.d.aq %[lr], 0(%[addr])\n\t"
        "sc.d.rl %[status], %[value], 0(%[addr])\n\t"
        : [lr] "=&r"(lr_value), [status] "=&r"(final_status)
        : [addr] "r"(lrsc_vaddr), [value] "r"(lrsc_final_value)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("after the intra-pair-fence SC, a fresh contiguous LR aq / SC rl pair succeeds again from the current payload",
        lr_value == lrsc_after_intervening &&
        final_status == 0U
    );

    TEST_SETUP_EXCEPT();
    asm volatile ("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the fence after the refreshed LR aq / SC rl pair retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t lrsc_after_refresh = ld(lrsc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the refreshed LR aq / SC rl / FENCE path updates the LR/SC target to the final payload",
        lrsc_after_refresh == lrsc_final_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the FENCE plus atomic aq/rl single-hart matrix leaves only the intended AMO and LR/SC targets updated",
        read64(amo_paddr) == amo_initial + amo_addend &&
        read64(lrsc_paddr) == lrsc_final_value
    );

    TEST_END();
}

bool ai_arch_fence_fault_side_effect_boundary_corner()
{
    TEST_START();

    const enum test_page pre_fault_page = SWITCH1;
    const enum test_page post_store_page = SWITCH2;
    const enum test_page pre_load_fault_page = VSR_GRWX;
    const enum test_page post_load_store_page = VSRW_GRW;
    const enum test_page pre_store_page = VSRW_GRW;
    const enum test_page post_fault_page = SCRATCHPAD;
    const uintptr_t pre_fault_vaddr = hs_page_base(pre_fault_page) + 0x688U;
    const uintptr_t post_store_vaddr = hs_page_base(post_store_page) + 0x688U;
    const uintptr_t pre_load_fault_vaddr =
        hs_page_base(pre_load_fault_page) + 0x690U;
    const uintptr_t post_load_store_vaddr =
        hs_page_base(post_load_store_page) + 0x690U;
    const uintptr_t pre_store_vaddr = hs_page_base(pre_store_page) + 0x688U;
    const uintptr_t post_fault_vaddr = hs_page_base(post_fault_page) + 0x688U;
    const uintptr_t pre_fault_paddr = phys_page_base(pre_fault_page) + 0x688U;
    const uintptr_t post_store_paddr = phys_page_base(post_store_page) + 0x688U;
    const uintptr_t pre_load_fault_paddr =
        phys_page_base(pre_load_fault_page) + 0x690U;
    const uintptr_t post_load_store_paddr =
        phys_page_base(post_load_store_page) + 0x690U;
    const uintptr_t pre_store_paddr = phys_page_base(pre_store_page) + 0x688U;
    const uintptr_t post_fault_paddr = phys_page_base(post_fault_page) + 0x688U;
    const uint64_t pre_fault_initial = 0x66656e63652d6630ULL;
    const uint64_t post_store_initial = 0x66656e63652d7330ULL;
    const uint64_t pre_load_fault_initial = 0x66656e63652d6c30ULL;
    const uint64_t post_load_store_initial = 0x66656e63652d6c31ULL;
    const uint64_t pre_store_initial = 0x66656e63652d7030ULL;
    const uint64_t post_fault_initial = 0x66656e63652d7830ULL;
    const uint64_t denied_pre_payload = 0x66656e63652d6631ULL;
    const uint64_t blocked_post_payload = 0x66656e63652d7331ULL;
    const uint64_t blocked_after_load_payload = 0x66656e63652d6c32ULL;
    const uint64_t committed_pre_payload = 0x66656e63652d7031ULL;
    const uint64_t denied_post_payload = 0x66656e63652d7831ULL;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t store_fault_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t load_fault_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t load_sentinel = 0x66656e63652d6c66ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(pre_fault_paddr, pre_fault_initial);
    write64(post_store_paddr, post_store_initial);
    write64(pre_load_fault_paddr, pre_load_fault_initial);
    write64(post_load_store_paddr, post_load_store_initial);
    write64(pre_store_paddr, pre_store_initial);
    write64(post_fault_paddr, post_fault_initial);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_set_leaf(pre_fault_page, store_fault_leaf);
    ai_micro_fence_set_leaf(post_store_page, legal_leaf);
    ai_micro_fence_set_leaf(pre_load_fault_page, load_fault_leaf);
    ai_micro_fence_set_leaf(post_load_store_page, legal_leaf);
    ai_micro_fence_set_leaf(pre_store_page, legal_leaf);
    ai_micro_fence_set_leaf(post_fault_page, store_fault_leaf);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(pre_fault_vaddr, denied_pre_payload);
    AI_ARCH_FENCE_ASSERT_STORE_PAGE_FAULT("a fault before FENCE reports store page fault and returns past the faulting store",
        pre_fault_vaddr);
    const uint64_t pre_fault_cause = excpt.cause;
    const uint64_t pre_fault_tval = excpt.tval;

    asm volatile("fence rw, rw" ::: "memory");
    TEST_ASSERT("a diagnostic FENCE after the trapped store does not overwrite the precise fault cause/tval metadata",
        excpt.triggered == true &&
        excpt.cause == pre_fault_cause &&
        excpt.tval == pre_fault_tval);

    if (!excpt.triggered) {
        TEST_SETUP_EXCEPT();
        asm volatile("fence rw, rw" ::: "memory");
        (void)excpt.triggered;

        TEST_SETUP_EXCEPT();
        sd(post_store_vaddr, blocked_post_payload);
        (void)excpt.triggered;
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the pre-FENCE store fault blocks the in-program FENCE and younger store from architecturally committing",
        read64(pre_fault_paddr) == pre_fault_initial &&
        read64(post_store_paddr) == post_store_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t faulting_load_rd =
        ai_micro_fence_ld_rd_probe(pre_load_fault_vaddr, load_sentinel);
    AI_ARCH_FENCE_ASSERT_LOAD_PAGE_FAULT("a load fault before FENCE reports load page fault, leaves rd unchanged, and returns past the faulting load",
        pre_load_fault_vaddr);
    TEST_ASSERT("the faulting load before FENCE leaves rd at its sentinel",
        faulting_load_rd == load_sentinel);

    if (!excpt.triggered) {
        TEST_SETUP_EXCEPT();
        asm volatile("fence rw, rw" ::: "memory");
        (void)excpt.triggered;

        TEST_SETUP_EXCEPT();
        sd(post_load_store_vaddr, blocked_after_load_payload);
        (void)excpt.triggered;
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("the pre-FENCE load fault blocks the modeled younger store and preserves both ordinary backing locations",
        read64(pre_load_fault_paddr) == pre_load_fault_initial &&
        read64(post_load_store_paddr) == post_load_store_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(pre_store_vaddr, committed_pre_payload);
    AI_ASSERT_NO_EXCEPT_AND("the ordinary store before the FENCE commits successfully",
        true);

    TEST_SETUP_EXCEPT();
    asm volatile("fence rw, rw" ::: "memory");
    AI_ASSERT_NO_EXCEPT_AND("the FENCE between the committed store and later fault retires without exception",
        true);

    TEST_SETUP_EXCEPT();
    sd(post_fault_vaddr, denied_post_payload);
    AI_ARCH_FENCE_ASSERT_STORE_PAGE_FAULT("a fault after FENCE reports the later store page fault",
        post_fault_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-FENCE fault does not roll back the earlier committed store and has no partial side effect",
        read64(pre_store_paddr) == committed_pre_payload &&
        read64(post_fault_paddr) == post_fault_initial &&
        *ai_micro_fence_hs_leaf_entry(pre_fault_page) ==
            ((phys_page_base(pre_fault_page) >> 2) | store_fault_leaf) &&
        *ai_micro_fence_hs_leaf_entry(post_fault_page) ==
            ((phys_page_base(post_fault_page) >> 2) | store_fault_leaf));

    TEST_END();
}

bool ai_arch_atomic_aqrl_fault_side_effect_boundary_corner()
{
    TEST_START();

    const enum test_page amo_page = SWITCH1;
    const enum test_page lr_page = SWITCH2;
    const enum test_page sc_page = SCRATCHPAD;
    const enum test_page success_page = VSRW_GRW;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x740U;
    const uintptr_t lr_vaddr = hs_page_base(lr_page) + 0x780U;
    const uintptr_t sc_vaddr = hs_page_base(sc_page) + 0x7c0U;
    const uintptr_t success_vaddr = hs_page_base(success_page) + 0x800U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x740U;
    const uintptr_t lr_paddr = phys_page_base(lr_page) + 0x780U;
    const uintptr_t sc_paddr = phys_page_base(sc_page) + 0x7c0U;
    const uintptr_t success_paddr = phys_page_base(success_page) + 0x800U;
    const uint64_t amo_initial = 0x6171726c2d616d6fULL;
    const uint64_t lr_initial = 0x6171726c2d6c7221ULL;
    const uint64_t sc_initial = 0x6171726c2d736321ULL;
    const uint64_t success_initial = 0x6171726c2d676f30ULL;
    const uint64_t amo_addend_aq = 0x11ULL;
    const uint64_t amo_addend_rl = 0x22ULL;
    const uint64_t amo_addend_aqrl = 0x33ULL;
    const uint64_t stale_sc_payload = 0x6171726c2d737461ULL;
    const uint64_t denied_sc_payload = 0x6171726c2d626164ULL;
    const uint64_t success_payload = 0x6171726c2d676f31ULL;
    const uint64_t stale_success_payload = 0x6171726c2d6f6f70ULL;
    const uint64_t sentinel_amo_aq = 0xa0a0000000000001ULL;
    const uint64_t sentinel_amo_rl = 0xa0a0000000000002ULL;
    const uint64_t sentinel_amo_aqrl = 0xa0a0000000000003ULL;
    const uint64_t sentinel_lr_aq = 0xb0b0000000000001ULL;
    const uint64_t sentinel_lr_aqrl = 0xb0b0000000000002ULL;
    const uint64_t sentinel_sc_rl = 0xc0c0000000000001ULL;
    const uint64_t sentinel_sc_aqrl = 0xc0c0000000000002ULL;
    const uint64_t read_only_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(amo_paddr, amo_initial);
    write64(lr_paddr, lr_initial);
    write64(sc_paddr, sc_initial);
    write64(success_paddr, success_initial);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_fence_set_leaf(amo_page, read_only_leaf);
    ai_micro_fence_set_leaf(lr_page, no_read_leaf);
    ai_micro_fence_set_leaf(sc_page, read_only_leaf);
    ai_micro_fence_set_leaf(success_page, legal_leaf);
    const uint64_t amo_fault_leaf =
        *ai_micro_fence_hs_leaf_entry(amo_page);
    const uint64_t lr_fault_leaf =
        *ai_micro_fence_hs_leaf_entry(lr_page);
    const uint64_t sc_fault_leaf =
        *ai_micro_fence_hs_leaf_entry(sc_page);
    const uint64_t success_leaf =
        *ai_micro_fence_hs_leaf_entry(success_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    TEST_ASSERT("the atomic aq/rl fault boundary starts from ordinary DRAM leaves with AMO/SC store faults, LR load faults, and a legal release-store target",
        amo_fault_leaf == ((phys_page_base(amo_page) >> 2) |
            read_only_leaf) &&
        lr_fault_leaf == ((phys_page_base(lr_page) >> 2) |
            no_read_leaf) &&
        sc_fault_leaf == ((phys_page_base(sc_page) >> 2) |
            read_only_leaf) &&
        success_leaf == ((phys_page_base(success_page) >> 2) |
            legal_leaf));

    TEST_SETUP_EXCEPT();
    uint64_t amo_aq_rd = ai_micro_fence_amoadd_d_aq_probe_rd(
        amo_vaddr, amo_addend_aq, sentinel_amo_aq);
    TEST_ASSERT("faulting amoadd.d.aq reports Store/AMO page fault, keeps rd at its sentinel, and creates no acquire observation point",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == amo_vaddr &&
        amo_aq_rd == sentinel_amo_aq);

    TEST_SETUP_EXCEPT();
    uint64_t amo_rl_rd = ai_micro_fence_amoadd_d_rl_probe_rd(
        amo_vaddr, amo_addend_rl, sentinel_amo_rl);
    TEST_ASSERT("faulting amoadd.d.rl independently refreshes Store/AMO page-fault metadata and creates no release observation point",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == amo_vaddr &&
        amo_rl_rd == sentinel_amo_rl);

    TEST_SETUP_EXCEPT();
    uint64_t amo_aqrl_rd = ai_micro_fence_amoadd_d_aqrl_probe_rd(
        amo_vaddr, amo_addend_aqrl, sentinel_amo_aqrl);
    TEST_ASSERT("faulting amoadd.d.aqrl leaves the destination register unchanged and performs no RMW side effect",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == amo_vaddr &&
        amo_aqrl_rd == sentinel_amo_aqrl);

    goto_priv(PRIV_M);
    TEST_ASSERT("none of the faulting AMO aq/rl/aqrl probes updates the target word or adjacent page-table state",
        read64(amo_paddr) == amo_initial &&
        *ai_micro_fence_hs_leaf_entry(amo_page) == amo_fault_leaf);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t lr_aq_rd = ai_micro_fence_lr_d_aq_probe_rd(
        lr_vaddr, sentinel_lr_aq);
    TEST_ASSERT("faulting lr.d.aq reports Load page fault, keeps rd at its sentinel, and does not create a reservation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == lr_vaddr &&
        lr_aq_rd == sentinel_lr_aq);

    TEST_SETUP_EXCEPT();
    uint64_t lr_aqrl_rd = ai_micro_fence_lr_d_aqrl_probe_rd(
        lr_vaddr, sentinel_lr_aqrl);
    TEST_ASSERT("faulting lr.d.aqrl refreshes Load page-fault metadata without updating the reservation set",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == lr_vaddr &&
        lr_aqrl_rd == sentinel_lr_aqrl);

    goto_priv(PRIV_M);
    ai_micro_fence_set_leaf(lr_page, legal_leaf);
    const uint64_t lr_repaired_leaf =
        *ai_micro_fence_hs_leaf_entry(lr_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t stale_lr_sc_status = ai_micro_fence_sc_d_rl(
        lr_vaddr, stale_sc_payload);
    AI_ASSERT_NO_EXCEPT_AND("after repairing the LR target, stale sc.d.rl still fails, proving the faulting LR did not create a release-capable reservation",
        stale_lr_sc_status != 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t after_stale_lr_sc = ld(lr_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the stale SC after faulting LR leaves the repaired LR target unchanged",
        after_stale_lr_sc == lr_initial);

    TEST_SETUP_EXCEPT();
    uint64_t sc_rl_rd = ai_micro_fence_sc_d_rl_probe_rd(
        sc_vaddr, denied_sc_payload, sentinel_sc_rl);
    TEST_ASSERT("faulting sc.d.rl reports Store/AMO page fault, leaves the status rd at its sentinel, and performs no release store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == sc_vaddr &&
        sc_rl_rd == sentinel_sc_rl);

    TEST_SETUP_EXCEPT();
    uint64_t sc_aqrl_rd = ai_micro_fence_sc_d_aqrl_probe_rd(
        sc_vaddr, denied_sc_payload, sentinel_sc_aqrl);
    TEST_ASSERT("faulting sc.d.aqrl refreshes Store/AMO page-fault metadata and leaves the target memory unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == sc_vaddr &&
        sc_aqrl_rd == sentinel_sc_aqrl);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t success_lr;
    uint64_t success_sc_status = success_payload;
    asm volatile(
        "lr.d.aq %[lr], 0(%[addr])\n\t"
        "sc.d.rl %[status], %[value], 0(%[addr])\n\t"
        : [lr] "=&r"(success_lr), [status] "=&r"(success_sc_status)
        : [addr] "r"(success_vaddr), [value] "r"(success_payload)
        : "memory");
    AI_ASSERT_NO_EXCEPT_AND("a fresh lr.d.aq/sc.d.rl pair on the legal target succeeds and distinguishes the real release store from the fault/fail paths",
        success_lr == success_initial &&
        success_sc_status == 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t stale_success_status = ai_micro_fence_sc_d_rl(
        success_vaddr, stale_success_payload);
    AI_ASSERT_NO_EXCEPT_AND("a later stale sc.d.rl without a fresh LR fails and cannot overwrite the successful release-store payload",
        stale_success_status != 0ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("atomic aq/rl fault/fail paths leave backing memory unchanged while only the successful release SC updates its target",
        *ai_micro_fence_hs_leaf_entry(amo_page) == amo_fault_leaf &&
        *ai_micro_fence_hs_leaf_entry(lr_page) == lr_repaired_leaf &&
        *ai_micro_fence_hs_leaf_entry(sc_page) == sc_fault_leaf &&
        *ai_micro_fence_hs_leaf_entry(success_page) == success_leaf &&
        read64(amo_paddr) == amo_initial &&
        read64(lr_paddr) == lr_initial &&
        read64(sc_paddr) == sc_initial &&
        read64(success_paddr) == success_payload);

    TEST_END();
}
