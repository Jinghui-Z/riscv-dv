#include "ai_micro_overlap_common.h"

#define AI_ARCH_NEWMDP_PRESSURE_WORDS 32U

__attribute__((naked, noinline, aligned(4))) static void
ai_arch_newmdp_delayed_store_then_same_pc_load(uintptr_t store_vaddr,
    uintptr_t load_vaddr, uint64_t store_value, uintptr_t marker_vaddr,
    uintptr_t guard_vaddr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a2, 0(t2)\n"
        "ld t3, 0(a1)\n"
        "sd t3, 0(a3)\n"
        "ld t4, 0(a4)\n"
        "sd t4, 8(a3)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

static inline void ai_arch_newmdp_store_pressure(uintptr_t pressure_vaddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_ARCH_NEWMDP_PRESSURE_WORDS; ++i) {
        *(volatile uint64_t *)(pressure_vaddr + (uintptr_t)i *
            sizeof(uint64_t)) = seed ^ ((uint64_t)i << 40) ^
            (0x9e3779b97f4a7c15ULL * (uint64_t)(i + 1U));
    }
}

static inline bool ai_arch_newmdp_pressure_matches(uintptr_t pressure_paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_ARCH_NEWMDP_PRESSURE_WORDS; ++i) {
        const uint64_t expected = seed ^ ((uint64_t)i << 40) ^
            (0x9e3779b97f4a7c15ULL * (uint64_t)(i + 1U));

        if (read64(pressure_paddr + (uintptr_t)i * sizeof(uint64_t)) !=
            expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_arch_newmdp_map_store_alias(enum test_page store_page,
    enum test_page load_page, enum test_page backing_page)
{
    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, backing_page);
    ai_micro_overlap_map_alias_to_base(load_page, VSRWX_GRWX);
    sfence_vma();
    goto_priv(PRIV_HS);
}

bool ai_arch_newmdp_same_entry_update_collision_no_stale_payload_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page unrelated_page = VSRW_GRWX;
    const enum test_page store_alias_page = SWITCH1;
    const enum test_page load_alias_page = SWITCH2;
    const enum test_page side_page = SCRATCHPAD;
    const uintptr_t off = 0xa00U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + off;
    const uintptr_t target_paddr = phys_page_base(target_page) + off;
    const uintptr_t unrelated_vaddr = hs_page_base(unrelated_page) + off;
    const uintptr_t unrelated_paddr = phys_page_base(unrelated_page) + off;
    const uintptr_t store_alias_vaddr = hs_page_base(store_alias_page) + off;
    const uintptr_t load_alias_vaddr = hs_page_base(load_alias_page) + off;
    const uintptr_t guard_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t guard_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t unrelated_guard_paddr =
        unrelated_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(side_page) + 0xa00U;
    const uintptr_t marker_paddr = phys_page_base(side_page) + 0xa00U;
    const uintptr_t pressure_vaddr = hs_page_base(side_page) + 0xb00U;
    const uintptr_t pressure_paddr = phys_page_base(side_page) + 0xb00U;
    const uint64_t target_initial0 = 0x1021324354657687ULL;
    const uint64_t target_initial1 = 0x8877665544332211ULL;
    const uint64_t target_initial2 = 0x3141592653589793ULL;
    const uint64_t target_initial3 = 0x2718281828459045ULL;
    const uint64_t unrelated_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t unrelated_store0 = 0xc001cafe00000011ULL;
    const uint64_t unrelated_store1 = 0xc001cafe00000022ULL;
    const uint64_t dependent_store0 = 0xa1b2c3d4e5f60718ULL;
    const uint64_t dependent_store1 = 0xfedcba9876543210ULL;
    const uint64_t guard_value = 0x55aa33cc77ee11ffULL;
    const uint64_t unrelated_guard = 0x66bb44dd88ee22ccULL;
    const uint64_t marker_seed = 0xdeadbeefcafebabeULL;
    const uint64_t pressure_seed0 = 0x123456789abcdef0ULL;
    const uint64_t pressure_seed1 = 0x0badf00d13579bdfULL;
    const uint64_t pressure_seed2 = 0x2468ace013579bdfULL;

    goto_priv(PRIV_M);
    write64(target_paddr, target_initial0);
    write64(target_paddr + sizeof(uint64_t), guard_value);
    write64(unrelated_paddr, unrelated_initial);
    write64(unrelated_guard_paddr, unrelated_guard);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_arch_newmdp_map_store_alias(store_alias_page, load_alias_page,
        unrelated_page);

    TEST_SETUP_EXCEPT();
    uint64_t warm_target = ld(load_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the NewMDP same-entry conflict probe starts with the load alias mapped to the target payload",
        warm_target == target_initial0);

    TEST_SETUP_EXCEPT();
    uint64_t warm_unrelated = ld(unrelated_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the unrelated store-training line is readable before the no-dependence update round",
        warm_unrelated == unrelated_initial);

    TEST_SETUP_EXCEPT();
    ai_arch_newmdp_delayed_store_then_same_pc_load(store_alias_vaddr,
        load_alias_vaddr, unrelated_store0, marker_vaddr, guard_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("round0 same static load PC sees the target payload while the delayed older store updates only an unrelated page",
        ld(marker_vaddr) == target_initial0 &&
        ld(marker_vaddr + sizeof(uint64_t)) == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("round0 leaves the target line untouched and updates only the unrelated store line",
        read64(target_paddr) == target_initial0 &&
        read64(unrelated_paddr) == unrelated_store0 &&
        read64(guard_paddr) == guard_value &&
        read64(unrelated_guard_paddr) == unrelated_guard);
    write64(target_paddr, target_initial1);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), marker_seed);

    ai_arch_newmdp_map_store_alias(store_alias_page, load_alias_page,
        target_page);
    ai_arch_newmdp_store_pressure(pressure_vaddr, pressure_seed0);

    TEST_SETUP_EXCEPT();
    ai_arch_newmdp_delayed_store_then_same_pc_load(store_alias_vaddr,
        load_alias_vaddr, dependent_store0, marker_vaddr, guard_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("round1 same static load PC must observe the delayed older same-PA store payload after redirect/NewMDP repair training",
        ld(marker_vaddr) == dependent_store0 &&
        ld(marker_vaddr + sizeof(uint64_t)) == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("round1 commits only the dependent store payload on the target line and preserves unrelated state",
        read64(target_paddr) == dependent_store0 &&
        read64(unrelated_paddr) == unrelated_store0 &&
        read64(guard_paddr) == guard_value &&
        ai_arch_newmdp_pressure_matches(pressure_paddr, pressure_seed0));
    write64(target_paddr, target_initial2);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), marker_seed);

    ai_arch_newmdp_map_store_alias(store_alias_page, load_alias_page,
        unrelated_page);
    ai_arch_newmdp_store_pressure(pressure_vaddr, pressure_seed1);

    TEST_SETUP_EXCEPT();
    ai_arch_newmdp_delayed_store_then_same_pc_load(store_alias_vaddr,
        load_alias_vaddr, unrelated_store1, marker_vaddr, guard_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("round2 same static load PC must refresh to a no-dependence update without waiting for or reading the unrelated older store",
        ld(marker_vaddr) == target_initial2 &&
        ld(marker_vaddr + sizeof(uint64_t)) == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("round2 no-dependence update changes only the unrelated store line and keeps the target payload current",
        read64(target_paddr) == target_initial2 &&
        read64(unrelated_paddr) == unrelated_store1 &&
        read64(guard_paddr) == guard_value &&
        ai_arch_newmdp_pressure_matches(pressure_paddr, pressure_seed1));
    write64(target_paddr, target_initial3);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), marker_seed);

    ai_arch_newmdp_map_store_alias(store_alias_page, load_alias_page,
        target_page);
    ai_arch_newmdp_store_pressure(pressure_vaddr, pressure_seed2);

    TEST_SETUP_EXCEPT();
    ai_arch_newmdp_delayed_store_then_same_pc_load(store_alias_vaddr,
        load_alias_vaddr, dependent_store1, marker_vaddr, guard_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("round3 must-wait same static load PC observes the current dependent store payload after the intervening fail/success-style update round",
        ld(marker_vaddr) == dependent_store1 &&
        ld(marker_vaddr + sizeof(uint64_t)) == guard_value);

    TEST_SETUP_EXCEPT();
    uint64_t final_alias_load = ld(load_alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("final same-PC target alias reload sees the newest dependent store payload rather than an old NewMDP distance generation",
        final_alias_load == dependent_store1);

    goto_priv(PRIV_M);
    TEST_ASSERT("NewMDP update-collision symptom probe leaves only the latest target and unrelated payloads visible with guards and pressure stores intact",
        read64(target_paddr) == dependent_store1 &&
        read64(unrelated_paddr) == unrelated_store1 &&
        read64(guard_paddr) == guard_value &&
        read64(unrelated_guard_paddr) == unrelated_guard &&
        read64(marker_paddr) == dependent_store1 &&
        read64(marker_paddr + sizeof(uint64_t)) == guard_value &&
        ai_arch_newmdp_pressure_matches(pressure_paddr, pressure_seed2));

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
