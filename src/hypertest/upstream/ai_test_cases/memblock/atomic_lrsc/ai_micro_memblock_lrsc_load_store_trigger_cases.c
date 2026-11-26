#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MEMBLOCK_LRSC_LS_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MEMBLOCK_LRSC_LS_TRIGGER_LOAD 0x1ULL
#define AI_MEMBLOCK_LRSC_LS_TRIGGER_STORE 0x2ULL

static inline void ai_memblock_lrsc_ls_clear_mcontrol6_0(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, 0);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_memblock_lrsc_ls_arm_mcontrol6_0(uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, 0);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1,
        AI_MEMBLOCK_LRSC_LS_MCONTROL6_MSU_BASE | access_bits);
}

static inline uint64_t ai_memblock_lrsc_ls_lr_d64(uintptr_t addr)
{
    uint64_t value;
    asm volatile(
        "lr.d %0, 0(%1)\n\t"
        : "=r"(value) : "r"(addr) : "memory"
    );
    return value;
}

static inline uint32_t ai_memblock_lrsc_ls_sc_d_status(uintptr_t addr,
    uint64_t value)
{
    asm volatile(
        "sc.d %0, %0, 0(%1)\n\t"
        : "+r"(value) : "r"(addr) : "memory"
    );
    return (uint32_t)value;
}

static inline uint64_t ai_memblock_lrsc_ls_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_memblock_lrsc_ls_reenable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
}

static inline void ai_memblock_lrsc_ls_restore_machine_status(
    uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

bool ai_micro_memblock_lr_w_lower_word_load_store_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xbc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xbc0U;
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
    const uint64_t guard_seed = 0x6c72776c73303030ULL;
    const uint32_t lower_initial = 0x11223344U;
    const uint32_t upper_initial = 0xa5b6c7d8U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0102030405060708ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint32_t no_reservation_value = 0x55667788U;
    const uint32_t final_lower_value = 0x99aabbccU;
    const uint32_t stale_lower_value = 0xee11dd22U;
    const uint64_t final_value =
        ((uint64_t)upper_initial << 32) | final_lower_value;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_ls_clear_mcontrol6_0();
    ai_memblock_lrsc_ls_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_LS_TRIGGER_LOAD |
        AI_MEMBLOCK_LRSC_LS_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_ls_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)lr_w(lower_word_vaddr);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on lower-word lr.w through the load side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_ls_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_ls_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store lower-word lr.w breakpoint leaves the full target doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        sc_w(lower_word_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the combined trigger, a same lower-word sc.w without a fresh lr.w fails because the breakpoint-suppressed lr.w did not create a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(lower_word_vaddr);
    uint32_t final_status = sc_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("a fresh lower-word lr.w/sc.w after the combined-trigger breakpoint path succeeds and updates only the low 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == lower_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(lower_word_vaddr, stale_lower_value);
    TEST_ASSERT("the successful lower-word sc.w consumes the fresh reservation, so a second naked sc.w fails and preserves the committed low lane",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit lr.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh lower-word sc.w lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_d_load_store_trigger_no_reservation_then_fresh_lrsc_updates_doubleword_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c72646c73303030ULL;
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;
    const uint64_t marker_seed = 0xfedcba6547382910ULL;
    const uint64_t marker_final = 0x8877665544332211ULL;
    const uint64_t no_reservation_value = 0xaabbccddeeff0011ULL;
    const uint64_t final_value = 0x0123456789abcdefULL;
    const uint64_t stale_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_ls_clear_mcontrol6_0();
    ai_memblock_lrsc_ls_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_LS_TRIGGER_LOAD |
        AI_MEMBLOCK_LRSC_LS_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_ls_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_memblock_lrsc_ls_lr_d64(target_vaddr);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on lr.d through the load side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_ls_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_ls_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool breakpoint_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store lr.d breakpoint leaves the target doubleword, guard, adjacent, and marker words unchanged",
        breakpoint_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        ai_memblock_lrsc_ls_sc_d_status(target_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the combined trigger, a same-address sc.d without a fresh lr.d fails because the breakpoint-suppressed lr.d did not create a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved = ai_memblock_lrsc_ls_lr_d64(target_vaddr);
    uint32_t final_status =
        ai_memblock_lrsc_ls_sc_d_status(target_vaddr, final_value);
    TEST_ASSERT("a fresh lr.d/sc.d after the combined-trigger breakpoint path succeeds and updates the full doubleword",
        excpt.triggered == false &&
        fresh_reserved == initial_value &&
        final_status == 0U &&
        read64(target_vaddr) == final_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status =
        ai_memblock_lrsc_ls_sc_d_status(target_vaddr, stale_value);
    TEST_ASSERT("the successful sc.d consumes the fresh reservation, so a second naked sc.d fails and preserves the committed doubleword",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit lr.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh sc.d doubleword update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_d_load_store_trigger_repeated_breakpoint_no_reservation_then_fresh_lrsc_updates_doubleword_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x726c726570303030ULL;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t adjacent_seed = 0x123456789abcdef0ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x7766554433221100ULL;
    const uint64_t no_reservation_value = 0xfeedfacecafebeefULL;
    const uint64_t final_value = 0x1020304050607080ULL;
    const uint64_t stale_value = 0x9988776655443322ULL;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_ls_clear_mcontrol6_0();
    ai_memblock_lrsc_ls_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_LS_TRIGGER_LOAD |
        AI_MEMBLOCK_LRSC_LS_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_ls_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_memblock_lrsc_ls_lr_d64(target_vaddr);
    TEST_ASSERT("the first repeated same-slot load-store mcontrol6 hit on lr.d reports breakpoint through the load side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_ls_reenable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)ai_memblock_lrsc_ls_lr_d64(target_vaddr);
    TEST_ASSERT("a second consecutive same-slot load-store mcontrol6 hit on lr.d still reports breakpoint instead of leaking a completed LR",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_ls_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_ls_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool repeated_breakpoints_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("two repeated combined-trigger lr.d breakpoint hits leave the target doubleword, guard, adjacent, and marker words unchanged",
        repeated_breakpoints_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        ai_memblock_lrsc_ls_sc_d_status(target_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the repeated combined trigger, a same-address sc.d without a fresh lr.d still fails because neither trapped lr.d created a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t fresh_reserved = ai_memblock_lrsc_ls_lr_d64(target_vaddr);
    uint32_t final_status =
        ai_memblock_lrsc_ls_sc_d_status(target_vaddr, final_value);
    TEST_ASSERT("a fresh lr.d/sc.d after the repeated-breakpoint path succeeds and updates the full doubleword exactly once",
        excpt.triggered == false &&
        fresh_reserved == initial_value &&
        final_status == 0U &&
        read64(target_vaddr) == final_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status =
        ai_memblock_lrsc_ls_sc_d_status(target_vaddr, stale_value);
    TEST_ASSERT("the successful post-clear sc.d consumes the fresh reservation, so a second naked sc.d fails and preserves the committed doubleword",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove repeated combined-trigger lr.d traps did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh sc.d doubleword update plus marker progress after repeated lr.d breakpoints",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_lr_w_upper_word_load_store_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc00U;
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
    const uint64_t guard_seed = 0x6c72777573303031ULL;
    const uint32_t lower_initial = 0x55667788U;
    const uint32_t upper_initial = 0x11223344U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1020304050607080ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x2211443366558877ULL;
    const uint32_t no_reservation_value = 0xaabbccddU;
    const uint32_t final_upper_value = 0x66778899U;
    const uint32_t stale_upper_value = 0xee11dd22U;
    const uint64_t final_value =
        ((uint64_t)final_upper_value << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_ls_clear_mcontrol6_0();
    ai_memblock_lrsc_ls_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_LS_TRIGGER_LOAD |
        AI_MEMBLOCK_LRSC_LS_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_ls_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)lr_w(upper_word_vaddr);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on upper-word lr.w through the load side",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_ls_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_ls_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store upper-word lr.w breakpoint leaves the full target doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t no_reservation_status =
        sc_w(upper_word_vaddr, no_reservation_value);
    TEST_ASSERT("after clearing the combined trigger, a same upper-word sc.w without a fresh lr.w fails because the breakpoint-suppressed lr.w did not create a reservation",
        excpt.triggered == false &&
        no_reservation_status != 0U &&
        read64(target_vaddr) == initial_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == upper_initial
    );

    TEST_SETUP_EXCEPT();
    uint32_t fresh_reserved = lr_w(upper_word_vaddr);
    uint32_t final_status = sc_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("a fresh upper-word lr.w/sc.w after the combined-trigger breakpoint path succeeds and updates only the high 32-bit lane",
        excpt.triggered == false &&
        fresh_reserved == upper_initial &&
        final_status == 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    TEST_SETUP_EXCEPT();
    uint32_t stale_status = sc_w(upper_word_vaddr, stale_upper_value);
    TEST_ASSERT("the successful upper-word sc.w consumes the fresh reservation, so a second naked sc.w fails and preserves the committed high lane",
        excpt.triggered == false &&
        stale_status != 0U &&
        read64(target_vaddr) == final_value &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit upper-word lr.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear fresh upper-word sc.w lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == final_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
