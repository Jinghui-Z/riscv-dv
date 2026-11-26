#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_AMOOR_W_LS_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MEMBLOCK_AMOOR_W_LS_TRIGGER_LOAD 0x1ULL
#define AI_MEMBLOCK_AMOOR_W_LS_TRIGGER_STORE 0x2ULL

static inline void ai_memblock_amoor_w_ls_clear_mcontrol6_0(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, 0);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_memblock_amoor_w_ls_arm_mcontrol6_0(uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, 0);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1,
        AI_MEMBLOCK_AMOOR_W_LS_MCONTROL6_MSU_BASE | access_bits);
}

static inline uint64_t ai_memblock_amoor_w_ls_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_memblock_amoor_w_ls_restore_machine_status(
    uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

bool ai_micro_memblock_amoor_w_lower_word_load_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xac0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xac0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f726c7330303030ULL;
    const uint32_t lower_initial = 0x12003400U;
    const uint32_t upper_initial = 0x89abcdefU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x2436485a6c7e8092ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x78695a4b3c2d1e0fULL;
    const uint32_t blocked_operand = 0x00f0004fU;
    const uint32_t final_operand = 0x0000c0ffU;
    const uint32_t expected_lower = lower_initial | final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_amoor_w_ls_clear_mcontrol6_0();
    ai_memblock_amoor_w_ls_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_AMOOR_W_LS_TRIGGER_LOAD |
        AI_MEMBLOCK_AMOOR_W_LS_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_amoor_w_ls_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on lower-word amoor.w before the logic-or lane can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_amoor_w_ls_restore_machine_status(saved_mstatus);
    ai_memblock_amoor_w_ls_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store lower-word amoor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amoor.w returns the old lower word and OR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit lower-word amoor.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoor.w OR lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_upper_word_load_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f726c7375703030ULL;
    const uint32_t lower_initial = 0x10203040U;
    const uint32_t upper_initial = 0x12003400U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x31425364758697a8ULL;
    const uint64_t marker_seed = 0x13572468ace0bdf1ULL;
    const uint64_t marker_final = 0xf1bde0ac68245713ULL;
    const uint32_t blocked_operand = 0x00f0004fU;
    const uint32_t final_operand = 0x0000c0ffU;
    const uint32_t expected_upper = upper_initial | final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_amoor_w_ls_clear_mcontrol6_0();
    ai_memblock_amoor_w_ls_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_AMOOR_W_LS_TRIGGER_LOAD |
        AI_MEMBLOCK_AMOOR_W_LS_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_amoor_w_ls_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on upper-word amoor.w before the logic-or lane can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_amoor_w_ls_restore_machine_status(saved_mstatus);
    ai_memblock_amoor_w_ls_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store upper-word amoor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amoor.w returns the old upper word and OR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit upper-word amoor.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoor.w OR lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
