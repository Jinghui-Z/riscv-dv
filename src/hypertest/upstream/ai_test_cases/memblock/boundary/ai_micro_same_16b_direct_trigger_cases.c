#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_SAME_16B_LOW_OFFSET 16U
#define AI_MICRO_SAME_16B_BAD_OFFSET 17U
#define AI_MICRO_SAME_16B_HIGH_OFFSET 24U
#define AI_MICRO_SAME_16B_WORD_BAD_OFFSET 21U
#define AI_MICRO_SAME_16B_HALF_BAD_OFFSET 23U

#define AI_MICRO_SAME_16B_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MICRO_SAME_16B_MCONTROL6_LOAD 0x1ULL
#define AI_MICRO_SAME_16B_MCONTROL6_STORE 0x2ULL
#define AI_MICRO_SAME_16B_MCONTROL6_EXECUTE 0x4ULL

static inline uintptr_t ai_micro_same_16b_low_paddr(void)
{
    return phys_page_base(VSRWX_GRWX) + AI_MICRO_SAME_16B_LOW_OFFSET;
}

static inline uintptr_t ai_micro_same_16b_high_paddr(void)
{
    return phys_page_base(VSRWX_GRWX) + AI_MICRO_SAME_16B_HIGH_OFFSET;
}

static inline uintptr_t ai_micro_same_16b_low_vaddr(void)
{
    return hs_page_base(VSRWX_GRWX) + AI_MICRO_SAME_16B_LOW_OFFSET;
}

static inline uintptr_t ai_micro_same_16b_bad_vaddr(void)
{
    return hs_page_base(VSRWX_GRWX) + AI_MICRO_SAME_16B_BAD_OFFSET;
}

static inline uintptr_t ai_micro_same_16b_word_bad_vaddr(void)
{
    return hs_page_base(VSRWX_GRWX) + AI_MICRO_SAME_16B_WORD_BAD_OFFSET;
}

static inline uintptr_t ai_micro_same_16b_half_bad_vaddr(void)
{
    return hs_page_base(VSRWX_GRWX) + AI_MICRO_SAME_16B_HALF_BAD_OFFSET;
}

static inline uintptr_t ai_micro_same_16b_window_paddr(enum test_page page,
    uintptr_t offset)
{
    return phys_page_base(page) + offset;
}

static inline uintptr_t ai_micro_same_16b_window_vaddr(enum test_page page,
    uintptr_t offset)
{
    return hs_page_base(page) + offset;
}

static inline uintptr_t ai_micro_same_16b_mcontrol6_tdata1(uint64_t access_bits)
{
    return AI_MICRO_SAME_16B_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_micro_same_16b_clear_mcontrol6_slot(unsigned slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_micro_same_16b_arm_mcontrol6_slot(unsigned slot,
    uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_micro_same_16b_mcontrol6_tdata1(access_bits));
}

static inline uint64_t ai_micro_same_16b_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_micro_same_16b_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_micro_same_16b_prepare_hs_cacheable_page(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_same_16b_prepare_hs_access_fault_page(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();
    goto_priv(PRIV_HS);
}

static inline uint64_t ai_micro_same_16b_expected_ld_value(uint64_t low_pattern,
    uint64_t high_pattern)
{
    return (low_pattern >> 8) | ((high_pattern & 0xffULL) << 56);
}

static inline void ai_micro_same_16b_apply_misaligned_store(uint64_t low_before,
    uint64_t high_before,
    uint64_t store_value,
    uint64_t *low_after,
    uint64_t *high_after)
{
    uint8_t low_bytes[8];
    uint8_t high_bytes[8];

    for (unsigned i = 0; i < 8U; ++i) {
        low_bytes[i] = (uint8_t)((low_before >> (8U * i)) & 0xffU);
        high_bytes[i] = (uint8_t)((high_before >> (8U * i)) & 0xffU);
    }

    for (unsigned i = 0; i < 8U; ++i) {
        uint8_t byte = (uint8_t)((store_value >> (8U * i)) & 0xffU);

        if ((1U + i) < 8U) {
            low_bytes[1U + i] = byte;
        } else {
            high_bytes[(1U + i) - 8U] = byte;
        }
    }

    *low_after = 0ULL;
    *high_after = 0ULL;
    for (unsigned i = 0; i < 8U; ++i) {
        *low_after |= ((uint64_t)low_bytes[i]) << (8U * i);
        *high_after |= ((uint64_t)high_bytes[i]) << (8U * i);
    }
}

static inline uint64_t ai_micro_same_16b_extract_window(uint64_t low_pattern,
    uint64_t high_pattern,
    unsigned byte_offset,
    unsigned size_bytes)
{
    uint8_t bytes[16];
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < 8U; ++i) {
        bytes[i] = (uint8_t)((low_pattern >> (8U * i)) & 0xffU);
        bytes[8U + i] = (uint8_t)((high_pattern >> (8U * i)) & 0xffU);
    }

    for (unsigned i = 0; i < size_bytes; ++i) {
        value |= ((uint64_t)bytes[byte_offset + i]) << (8U * i);
    }

    return value;
}

static inline uint64_t ai_micro_same_16b_sign_extend(uint64_t value,
    unsigned size_bytes)
{
    const unsigned shift = 64U - (8U * size_bytes);
    return (uint64_t)(((int64_t)(value << shift)) >> shift);
}

static inline void ai_micro_same_16b_apply_narrow_store(uint64_t low_before,
    uint64_t high_before,
    uint64_t store_value,
    unsigned byte_offset,
    unsigned size_bytes,
    uint64_t *low_after,
    uint64_t *high_after)
{
    uint8_t low_bytes[8];
    uint8_t high_bytes[8];

    for (unsigned i = 0; i < 8U; ++i) {
        low_bytes[i] = (uint8_t)((low_before >> (8U * i)) & 0xffU);
        high_bytes[i] = (uint8_t)((high_before >> (8U * i)) & 0xffU);
    }

    for (unsigned i = 0; i < size_bytes; ++i) {
        uint8_t byte = (uint8_t)((store_value >> (8U * i)) & 0xffU);
        const unsigned target = byte_offset + i;

        if (target < 8U) {
            low_bytes[target] = byte;
        } else {
            high_bytes[target - 8U] = byte;
        }
    }

    *low_after = 0ULL;
    *high_after = 0ULL;
    for (unsigned i = 0; i < 8U; ++i) {
        *low_after |= ((uint64_t)low_bytes[i]) << (8U * i);
        *high_after |= ((uint64_t)high_bytes[i]) << (8U * i);
    }
}

static inline bool ai_micro_same_16b_words_match(uintptr_t base_paddr,
    const uint64_t *expected_words,
    unsigned word_count)
{
    for (unsigned i = 0; i < word_count; ++i) {
        if (read64(base_paddr + (i << 3)) != expected_words[i]) {
            return false;
        }
    }
    return true;
}

__attribute__((naked, noinline, aligned(4))) static uint64_t
ai_micro_same_16b_execute_load_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld a0, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_same_16b_execute_store_once(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

bool ai_micro_hs_load_same_16b_cacheable_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t bad_vaddr = ai_micro_same_16b_bad_vaddr();
    const uint64_t low_pattern = 0x7766554433221100ULL;
    const uint64_t high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_value =
        ai_micro_same_16b_expected_ld_value(low_pattern, high_pattern);
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the address-trigger probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, bad_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("with a load-address trigger armed on the cacheable same-16B direct-misalign path, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first same-16B direct-misalign load breakpoint leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("with the same load-address trigger still armed on the cacheable same-16B direct-misalign path, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second same-16B direct-misalign load breakpoint also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("after clearing the load-address trigger, the same cacheable same-16B direct-misalign load completes and returns the merged payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered cacheable same-16B direct-misalign load, both participating backing doublewords still match the original payload",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );
    ai_micro_same_16b_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t bad_vaddr = ai_micro_same_16b_bad_vaddr();
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_same_16b_execute_load_once;
    const uint64_t low_pattern = 0xfedcba9876543210ULL;
    const uint64_t high_pattern = 0x0123456789abcdefULL;
    const uint64_t expected_value =
        ai_micro_same_16b_expected_ld_value(low_pattern, high_pattern);
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the dual-trigger probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_SAME_16B_MCONTROL6_EXECUTE);
    ai_micro_same_16b_arm_mcontrol6_slot(1, bad_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("with both execute-PC and load-address trigger slots armed on the cacheable same-16B direct-misalign load, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first dual-trigger same-16B direct-misalign load breakpoint leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("with the same dual-trigger setup still armed on the cacheable same-16B direct-misalign load, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second dual-trigger same-16B direct-misalign load breakpoint also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining load-address trigger still reports breakpoint on the same cacheable same-16B direct-misalign load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing only the execute-PC slot, the address-triggered same-16B direct-misalign load still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_same_16b_execute_load_once(bad_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cacheable same-16B direct-misalign load completes and returns the merged payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered dual-trigger cacheable same-16B direct-misalign load, both participating backing doublewords still match the original payload",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t bad_vaddr = ai_micro_same_16b_bad_vaddr();
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;
    const uint64_t first_store = 0x0123456789abcdefULL;
    const uint64_t second_store = 0xa1b2c3d4e5f60718ULL;
    uint64_t expected_low = 0ULL;
    uint64_t expected_high = 0ULL;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the store address-trigger probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, bad_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, first_store);
    TEST_ASSERT("with a store-address trigger armed on the cacheable same-16B direct-misalign path, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first same-16B direct-misalign store breakpoint leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, second_store);
    TEST_ASSERT("with the same store-address trigger still armed on the cacheable same-16B direct-misalign path, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second same-16B direct-misalign store breakpoint also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, second_store);
    TEST_ASSERT("after clearing the store-address trigger, the same cacheable same-16B direct-misalign store completes normally",
        excpt.triggered == false
    );

    ai_micro_same_16b_apply_misaligned_store(low_pattern, high_pattern, second_store,
        &expected_low, &expected_high);
    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered cacheable same-16B direct-misalign store, the two participating backing doublewords reflect only the final byte merge",
        read64(low_paddr) == expected_low &&
        read64(high_paddr) == expected_high
    );
    ai_micro_same_16b_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t bad_vaddr = ai_micro_same_16b_bad_vaddr();
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_same_16b_execute_store_once;
    const uint64_t low_pattern = 0x1021324354657687ULL;
    const uint64_t high_pattern = 0x98badcfe10325476ULL;
    const uint64_t first_store = 0x1122334455667788ULL;
    const uint64_t second_store = 0x02468ace13579bdfULL;
    uint64_t expected_low = 0ULL;
    uint64_t expected_high = 0ULL;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the dual-trigger store probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_SAME_16B_MCONTROL6_EXECUTE);
    ai_micro_same_16b_arm_mcontrol6_slot(1, bad_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, first_store);
    TEST_ASSERT("with both execute-PC and store-address trigger slots armed on the cacheable same-16B direct-misalign store, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first dual-trigger same-16B direct-misalign store breakpoint leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, second_store);
    TEST_ASSERT("with the same dual-trigger setup still armed on the cacheable same-16B direct-misalign store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second dual-trigger same-16B direct-misalign store breakpoint also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, second_store);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining store-address trigger still reports breakpoint on the same cacheable same-16B direct-misalign store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing only the execute-PC slot, the address-triggered same-16B direct-misalign store still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(bad_vaddr, second_store);
    TEST_ASSERT("after clearing both trigger slots, the same cacheable same-16B direct-misalign store completes normally",
        excpt.triggered == false
    );

    ai_micro_same_16b_apply_misaligned_store(low_pattern, high_pattern, second_store,
        &expected_low, &expected_high);
    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered dual-trigger cacheable same-16B direct-misalign store, the two participating backing doublewords reflect only the final byte merge",
        read64(low_paddr) == expected_low &&
        read64(high_paddr) == expected_high
    );

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_BAD_OFFSET;
    const uintptr_t good_vaddr = ai_micro_same_16b_bad_vaddr();
    const uint64_t low_pattern = 0x6f5e4d3c2b1a0908ULL;
    const uint64_t high_pattern = 0x8070605040302010ULL;
    const uint64_t expected_value =
        ai_micro_same_16b_expected_ld_value(low_pattern, high_pattern);
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the trigger-versus-page-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_arm_mcontrol6_slot(0, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with a load-address trigger armed on the same cacheable same-16B direct-misalign byte+1 shape, the first page-faulting probe still reports breakpoint before the later page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first same-16B direct-misalign trigger-plus-page-fault load probe leaves the later recovery backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with the same load-address trigger still armed on the same cacheable same-16B direct-misalign page-faulting probe, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second same-16B direct-misalign trigger-plus-page-fault load probe also leaves the later recovery backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after clearing the load-address trigger, the same same-16B direct-misalign probe falls back to load page fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_same_16b_execute_load_once(good_vaddr);
    TEST_ASSERT("after the repeated breakpoint hits and fallback page fault, a valid cacheable same-16B direct-misalign load still completes and returns the merged payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign load following the trigger-plus-page-fault sequence, both participating backing doublewords still match the original payload",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = ai_micro_same_16b_bad_vaddr();
    const uint64_t low_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_pattern = 0xf0e1d2c3b4a59687ULL;
    const uint64_t expected_value =
        ai_micro_same_16b_expected_ld_value(low_pattern, high_pattern);
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the same byte+1 shape is remapped onto the leaf-overflow access-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_arm_mcontrol6_slot(0, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with a load-address trigger armed on that same byte+1 shape after the leaf-overflow remap, the first probe still reports breakpoint before the later load access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first same-16B direct-misalign trigger-plus-leaf-overflow-access-fault load probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with the same load-address trigger still armed on that same byte+1 shape after the leaf-overflow remap, the second probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second same-16B direct-misalign trigger-plus-leaf-overflow-access-fault load probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after clearing the load-address trigger, the same same-16B direct-misalign probe falls back to load access fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after the repeated breakpoint hits and fallback leaf-overflow access fault, restoring the cacheable translation lets the same byte+1 same-16B load complete and return the merged payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign load following the trigger-plus-leaf-overflow-access-fault sequence, both participating backing doublewords still match the original payload",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_BAD_OFFSET;
    const uintptr_t good_vaddr = ai_micro_same_16b_bad_vaddr();
    const uint64_t low_pattern = 0x5a4b3c2d1e0f9081ULL;
    const uint64_t high_pattern = 0x1029384756abcedfULL;
    const uint64_t first_store = 0x89abcdef01234567ULL;
    const uint64_t second_store = 0xfedcba9876543210ULL;
    uint64_t expected_low = 0ULL;
    uint64_t expected_high = 0ULL;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the store trigger-versus-page-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_arm_mcontrol6_slot(0, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, first_store);
    TEST_ASSERT("with a store-address trigger armed on the same cacheable same-16B direct-misalign byte+1 shape, the first page-faulting store probe still reports breakpoint before the later store page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first same-16B direct-misalign trigger-plus-page-fault store probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("with the same store-address trigger still armed on the same cacheable same-16B direct-misalign page-faulting store probe, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second same-16B direct-misalign trigger-plus-page-fault store probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after clearing the store-address trigger, the same same-16B direct-misalign store probe falls back to store page fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback same-16B direct-misalign store page-fault path still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(good_vaddr, second_store);
    TEST_ASSERT("after the repeated breakpoint hits and fallback store page fault, a valid cacheable same-16B direct-misalign store still completes normally",
        excpt.triggered == false
    );

    ai_micro_same_16b_apply_misaligned_store(low_pattern, high_pattern, second_store,
        &expected_low, &expected_high);
    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign store following the trigger-plus-page-fault sequence, the two participating backing doublewords reflect only the final byte merge",
        read64(low_paddr) == expected_low &&
        read64(high_paddr) == expected_high
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = ai_micro_same_16b_bad_vaddr();
    const uint64_t low_pattern = 0x7f6e5d4c3b2a1908ULL;
    const uint64_t high_pattern = 0x90a1b2c3d4e5f607ULL;
    const uint64_t first_store = 0x0011223344556677ULL;
    const uint64_t second_store = 0x8899aabbccddeeffULL;
    uint64_t expected_low = 0ULL;
    uint64_t expected_high = 0ULL;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the same byte+1 store shape is remapped onto the leaf-overflow access-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_arm_mcontrol6_slot(0, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, first_store);
    TEST_ASSERT("with a store-address trigger armed on that same byte+1 store shape after the leaf-overflow remap, the first probe still reports breakpoint before the later store access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first same-16B direct-misalign trigger-plus-leaf-overflow-access-fault store probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("with the same store-address trigger still armed on that same byte+1 store shape after the leaf-overflow remap, the second probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second same-16B direct-misalign trigger-plus-leaf-overflow-access-fault store probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after clearing the store-address trigger, the same same-16B direct-misalign store probe falls back to store access fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback same-16B direct-misalign store access-fault path still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after the repeated breakpoint hits and fallback leaf-overflow store access fault, restoring the cacheable translation lets the same byte+1 same-16B store complete normally",
        excpt.triggered == false
    );

    ai_micro_same_16b_apply_misaligned_store(low_pattern, high_pattern, second_store,
        &expected_low, &expected_high);
    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign store following the trigger-plus-leaf-overflow-access-fault sequence, the two participating backing doublewords reflect only the final byte merge",
        read64(low_paddr) == expected_low &&
        read64(high_paddr) == expected_high
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_BAD_OFFSET;
    const uintptr_t good_vaddr = ai_micro_same_16b_bad_vaddr();
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_same_16b_execute_store_once;
    const uint64_t low_pattern = 0x31425364758697a8ULL;
    const uint64_t high_pattern = 0xe0d1c2b3a4958677ULL;
    const uint64_t first_store = 0x1021324354657687ULL;
    const uint64_t second_store = 0x98badcfe10325476ULL;
    uint64_t expected_low = 0ULL;
    uint64_t expected_high = 0ULL;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the dual-trigger store-versus-page-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_SAME_16B_MCONTROL6_EXECUTE);
    ai_micro_same_16b_arm_mcontrol6_slot(1, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, first_store);
    TEST_ASSERT("with both execute-PC and store-address trigger slots armed on the same cacheable same-16B direct-misalign page-faulting store, the first access still reports breakpoint before the later store page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first dual-trigger same-16B direct-misalign trigger-plus-page-fault store probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("with both trigger slots still armed on the same cacheable same-16B direct-misalign page-faulting store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second dual-trigger same-16B direct-misalign trigger-plus-page-fault store probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after two prior dual-trigger store breakpoint hits, clearing only the execute-PC slot still leaves the same page-faulting same-16B direct-misalign store reporting breakpoint through the address trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the store-address trigger slot armed, the same same-16B direct-misalign trigger-plus-page-fault store probe still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after clearing both trigger slots, the same same-16B direct-misalign store probe falls back to store page fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback same-16B direct-misalign dual-trigger store page-fault path still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(good_vaddr, second_store);
    TEST_ASSERT("after the repeated dual-trigger breakpoint hits and fallback store page fault, a valid cacheable same-16B direct-misalign store still completes normally",
        excpt.triggered == false
    );

    ai_micro_same_16b_apply_misaligned_store(low_pattern, high_pattern, second_store,
        &expected_low, &expected_high);
    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign store following the dual-trigger-plus-page-fault sequence, the two participating backing doublewords reflect only the final byte merge",
        read64(low_paddr) == expected_low &&
        read64(high_paddr) == expected_high
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = ai_micro_same_16b_bad_vaddr();
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_same_16b_execute_store_once;
    const uint64_t low_pattern = 0x425364758697a8b9ULL;
    const uint64_t high_pattern = 0xf1e2d3c4b5a69788ULL;
    const uint64_t first_store = 0x89abcdef01234567ULL;
    const uint64_t second_store = 0x76543210fedcba98ULL;
    uint64_t expected_low = 0ULL;
    uint64_t expected_high = 0ULL;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the same byte+1 store shape is remapped onto the dual-trigger leaf-overflow access-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_SAME_16B_MCONTROL6_EXECUTE);
    ai_micro_same_16b_arm_mcontrol6_slot(1, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, first_store);
    TEST_ASSERT("with both execute-PC and store-address trigger slots armed on that same byte+1 store shape after the leaf-overflow remap, the first access still reports breakpoint before the later store access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first dual-trigger same-16B direct-misalign trigger-plus-leaf-overflow-access-fault store probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("with both trigger slots still armed on that same byte+1 store shape after the leaf-overflow remap, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second dual-trigger same-16B direct-misalign trigger-plus-leaf-overflow-access-fault store probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after two prior dual-trigger store breakpoint hits, clearing only the execute-PC slot still leaves that same byte+1 leaf-overflow store shape reporting breakpoint through the address trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the store-address trigger slot armed, the same same-16B direct-misalign trigger-plus-leaf-overflow-access-fault store probe still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after clearing both trigger slots, the same same-16B direct-misalign store probe falls back to store access fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback same-16B direct-misalign dual-trigger store access-fault path still leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_store_once(fault_vaddr, second_store);
    TEST_ASSERT("after the repeated dual-trigger breakpoint hits and fallback leaf-overflow store access fault, restoring the cacheable translation lets the same byte+1 same-16B store complete normally",
        excpt.triggered == false
    );

    ai_micro_same_16b_apply_misaligned_store(low_pattern, high_pattern, second_store,
        &expected_low, &expected_high);
    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign store following the dual-trigger-plus-leaf-overflow-access-fault sequence, the two participating backing doublewords reflect only the final byte merge",
        read64(low_paddr) == expected_low &&
        read64(high_paddr) == expected_high
    );

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_BAD_OFFSET;
    const uintptr_t good_vaddr = ai_micro_same_16b_bad_vaddr();
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_same_16b_execute_load_once;
    const uint64_t low_pattern = 0x5364758697a8b9caULL;
    const uint64_t high_pattern = 0xfedcba9876543210ULL;
    const uint64_t expected_value =
        ai_micro_same_16b_expected_ld_value(low_pattern, high_pattern);
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the dual-trigger load-versus-page-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_SAME_16B_MCONTROL6_EXECUTE);
    ai_micro_same_16b_arm_mcontrol6_slot(1, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both execute-PC and load-address trigger slots armed on the same cacheable same-16B direct-misalign page-faulting load, the first access still reports breakpoint before the later load page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first dual-trigger same-16B direct-misalign trigger-plus-page-fault load probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both trigger slots still armed on the same cacheable same-16B direct-misalign page-faulting load, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second dual-trigger same-16B direct-misalign trigger-plus-page-fault load probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after two prior dual-trigger load breakpoint hits, clearing only the execute-PC slot still leaves the same page-faulting same-16B direct-misalign load reporting breakpoint through the address trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same same-16B direct-misalign load probe falls back to load page fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_same_16b_execute_load_once(good_vaddr);
    TEST_ASSERT("after the repeated dual-trigger breakpoint hits and fallback load page fault, a valid cacheable same-16B direct-misalign load still completes and returns the merged payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign load following the dual-trigger-plus-page-fault sequence, both participating backing doublewords still match the original payload",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t fault_vaddr = ai_micro_same_16b_bad_vaddr();
    const uintptr_t trigger_pc = (uintptr_t)ai_micro_same_16b_execute_load_once;
    const uint64_t low_pattern = 0x64758697a8b9cadbULL;
    const uint64_t high_pattern = 0xe1d2c3b4a5968778ULL;
    const uint64_t expected_value =
        ai_micro_same_16b_expected_ld_value(low_pattern, high_pattern);
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the same byte+1 load shape is remapped onto the dual-trigger leaf-overflow access-fault probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_SAME_16B_MCONTROL6_EXECUTE);
    ai_micro_same_16b_arm_mcontrol6_slot(1, fault_vaddr,
        AI_MICRO_SAME_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();

    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both execute-PC and load-address trigger slots armed on that same byte+1 load shape after the leaf-overflow remap, the first access still reports breakpoint before the later load access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first dual-trigger same-16B direct-misalign trigger-plus-leaf-overflow-access-fault load probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both trigger slots still armed on that same byte+1 load shape after the leaf-overflow remap, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second dual-trigger same-16B direct-misalign trigger-plus-leaf-overflow-access-fault load probe also leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_same_16b_enable_machine_breakpoint_delivery();
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after two prior dual-trigger load breakpoint hits, clearing only the execute-PC slot still leaves that same byte+1 leaf-overflow load shape reporting breakpoint through the address trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_same_16b_restore_machine_status(saved_mstatus);

    ai_micro_same_16b_clear_mcontrol6_slot(1);
    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same same-16B direct-misalign load probe falls back to load access fault with the original byte+1 virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_same_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after the repeated dual-trigger breakpoint hits and fallback leaf-overflow load access fault, restoring the cacheable translation lets the same byte+1 same-16B load complete and return the merged payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered same-16B direct-misalign load following the dual-trigger-plus-leaf-overflow-access-fault sequence, both participating backing doublewords still match the original payload",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_narrow_cross_dw_signed_unsigned_lane_select_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t word_bad_vaddr = ai_micro_same_16b_word_bad_vaddr();
    const uintptr_t half_bad_vaddr = ai_micro_same_16b_half_bad_vaddr();
    const uint64_t low_pattern = 0x7faa555443322110ULL;
    const uint64_t high_pattern = 0xf7e6d5c4b3a29180ULL;
    const uint64_t expected_lhu =
        ai_micro_same_16b_extract_window(low_pattern, high_pattern, 7U, 2U);
    const uint64_t expected_lh =
        ai_micro_same_16b_sign_extend(expected_lhu, 2U);
    const uint64_t expected_lwu =
        ai_micro_same_16b_extract_window(low_pattern, high_pattern, 5U, 4U);
    const uint64_t expected_lw =
        ai_micro_same_16b_sign_extend(expected_lwu, 4U);

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the narrow-width data-lane probe",
        excpt.triggered == false &&
        warm == low_pattern
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_lhu = lhu(half_bad_vaddr);
    TEST_ASSERT("a cacheable same-16B direct-misalign lhu that crosses the internal doubleword boundary still returns the exact unsigned halfword lane",
        excpt.triggered == false &&
        observed_lhu == expected_lhu
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_lh = (uint64_t)(int64_t)(int16_t)lh(half_bad_vaddr);
    TEST_ASSERT("reissuing the same cacheable same-16B direct-misalign halfword access as lh still returns the exact sign-extended halfword lane",
        excpt.triggered == false &&
        observed_lh == expected_lh
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_lwu = lwu(word_bad_vaddr);
    TEST_ASSERT("a cacheable same-16B direct-misalign lwu that crosses the internal doubleword boundary still returns the exact unsigned word lane",
        excpt.triggered == false &&
        observed_lwu == expected_lwu
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_lw = (uint64_t)(int64_t)(int32_t)lw(word_bad_vaddr);
    TEST_ASSERT("reissuing the same cacheable same-16B direct-misalign word access as lw still returns the exact sign-extended word lane",
        excpt.triggered == false &&
        observed_lw == expected_lw
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-16B direct-misalign narrow load lane-select probe leaves both participating backing doublewords unchanged",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_narrow_cross_dw_byte_merge_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t word_bad_vaddr = ai_micro_same_16b_word_bad_vaddr();
    const uintptr_t half_bad_vaddr = ai_micro_same_16b_half_bad_vaddr();
    static const unsigned sizes[] = {2U, 4U};
    static const unsigned byte_offsets[] = {7U, 5U};
    static const uint64_t low_patterns[] = {
        0x8899aabbccddeeffULL,
        0x1021324354657687ULL,
    };
    static const uint64_t high_patterns[] = {
        0x7766554433221100ULL,
        0x98badcfe10325476ULL,
    };
    static const uint64_t store_values[] = {
        0x0000000000005aa5ULL,
        0x00000000c3d2e1f0ULL,
    };

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned size_bytes = sizes[round];
        const unsigned byte_offset = byte_offsets[round];
        const uintptr_t target_vaddr = (round == 0U) ? half_bad_vaddr : word_bad_vaddr;
        const uint64_t low_pattern = low_patterns[round];
        const uint64_t high_pattern = high_patterns[round];
        const uint64_t store_value = store_values[round];
        uint64_t expected_low = 0ULL;
        uint64_t expected_high = 0ULL;

        goto_priv(PRIV_M);
        write64(low_paddr, low_pattern);
        write64(high_paddr, high_pattern);

        ai_micro_same_16b_prepare_hs_cacheable_page();

        TEST_SETUP_EXCEPT();
        uint64_t warm = ld(low_vaddr);
        TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before the narrow-width byte-merge store probe",
            excpt.triggered == false &&
            warm == low_pattern
        );

        TEST_SETUP_EXCEPT();
        if (size_bytes == 2U) {
            sh(target_vaddr, (uint16_t)store_value);
        } else {
            sw(target_vaddr, (uint32_t)store_value);
        }
        TEST_ASSERT("a cacheable same-16B direct-misalign narrow store that crosses the internal doubleword boundary completes without exception",
            excpt.triggered == false
        );

        fence_iorw_iorw();
        ai_micro_same_16b_apply_narrow_store(low_pattern, high_pattern, store_value,
            byte_offset, size_bytes, &expected_low, &expected_high);

        goto_priv(PRIV_M);
        TEST_ASSERT("after the cacheable same-16B direct-misalign narrow store, the two participating backing doublewords reflect only the intended byte-lane merge",
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high
        );
    }

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_access_fault_then_different_shape_recovery_lane_refresh_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t word_bad_vaddr = ai_micro_same_16b_word_bad_vaddr();
    const uintptr_t half_bad_vaddr = ai_micro_same_16b_half_bad_vaddr();
    const uint64_t low_pattern = 0x7faa555443322110ULL;
    const uint64_t high_pattern = 0xf7e6d5c4b3a29180ULL;
    const uint64_t expected_lh =
        ai_micro_same_16b_sign_extend(
            ai_micro_same_16b_extract_window(low_pattern, high_pattern, 7U, 2U),
            2U);
    const uint64_t expected_lw =
        ai_micro_same_16b_sign_extend(
            ai_micro_same_16b_extract_window(low_pattern, high_pattern, 5U, 4U),
            4U);

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before switching narrow fault and recovery shapes",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    (void)lwu(word_bad_vaddr);
    TEST_ASSERT("after remapping to the leaf-overflow access-fault page, a same-16B direct-misalign lwu at the cross-doubleword word shape reports load access fault on its own word alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == word_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting same-16B direct-misalign word load leaves both participating backing doublewords unchanged before the narrower recovery probe",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t observed_lh = (uint64_t)(int64_t)(int16_t)lh(half_bad_vaddr);
    TEST_ASSERT("after the faulting word-shaped same-16B direct-misalign load, recovering with lh on the halfword alias refreshes lane ownership and returns the correct sign-extended halfword",
        excpt.triggered == false &&
        observed_lh == expected_lh
    );

    ai_micro_same_16b_prepare_hs_access_fault_page();
    TEST_SETUP_EXCEPT();
    (void)lhu(half_bad_vaddr);
    TEST_ASSERT("reversing the order, a same-16B direct-misalign lhu on the leaf-overflow access-fault page reports its own load access fault VA instead of inheriting the older word alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == half_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting same-16B direct-misalign halfword load also leaves both participating backing doublewords unchanged before the wider recovery probe",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t observed_lw = (uint64_t)(int64_t)(int32_t)lw(word_bad_vaddr);
    TEST_ASSERT("after the faulting halfword-shaped same-16B direct-misalign load, recovering with lw on the word alias refreshes width and sign ownership and returns the correct sign-extended word",
        excpt.triggered == false &&
        observed_lw == expected_lw
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-16B direct-misalign narrow fault-then-different-shape recovery load sequence keeps both backing doublewords unchanged throughout",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_access_fault_then_different_shape_recovery_mask_refresh_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t word_bad_vaddr = ai_micro_same_16b_word_bad_vaddr();
    const uintptr_t half_bad_vaddr = ai_micro_same_16b_half_bad_vaddr();
    static const unsigned fault_sizes[] = {4U, 2U};
    static const unsigned recover_sizes[] = {2U, 4U};
    static const unsigned recover_offsets[] = {7U, 5U};
    static const uint64_t low_patterns[] = {
        0x8899aabbccddeeffULL,
        0x1021324354657687ULL,
    };
    static const uint64_t high_patterns[] = {
        0x7766554433221100ULL,
        0x98badcfe10325476ULL,
    };
    static const uint64_t fault_values[] = {
        0x00000000c3d2e1f0ULL,
        0x0000000000005aa5ULL,
    };
    static const uint64_t recover_values[] = {
        0x0000000000005aa5ULL,
        0x00000000c3d2e1f0ULL,
    };

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned fault_size = fault_sizes[round];
        const unsigned recover_size = recover_sizes[round];
        const uintptr_t fault_vaddr =
            (round == 0U) ? word_bad_vaddr : half_bad_vaddr;
        const uintptr_t recover_vaddr =
            (round == 0U) ? half_bad_vaddr : word_bad_vaddr;
        const uint64_t low_pattern = low_patterns[round];
        const uint64_t high_pattern = high_patterns[round];
        const uint64_t fault_value = fault_values[round];
        const uint64_t recover_value = recover_values[round];
        uint64_t expected_low = 0ULL;
        uint64_t expected_high = 0ULL;

        goto_priv(PRIV_M);
        write64(low_paddr, low_pattern);
        write64(high_paddr, high_pattern);

        ai_micro_same_16b_prepare_hs_cacheable_page();

        TEST_SETUP_EXCEPT();
        uint64_t warm = ld(low_vaddr);
        TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before switching narrow store fault and recovery shapes",
            excpt.triggered == false &&
            warm == low_pattern
        );

        ai_micro_same_16b_prepare_hs_access_fault_page();
        TEST_SETUP_EXCEPT();
        if (fault_size == 2U) {
            sh(fault_vaddr, (uint16_t)fault_value);
        } else {
            sw(fault_vaddr, (uint32_t)fault_value);
        }
        TEST_ASSERT("after remapping to the leaf-overflow access-fault page, a same-16B direct-misalign narrow store that faults first reports store access fault on its own shape-specific alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the faulting same-16B direct-misalign narrow store leaves both participating backing doublewords unchanged before the different-shape recovery store",
            read64(low_paddr) == low_pattern &&
            read64(high_paddr) == high_pattern
        );

        ai_micro_same_16b_prepare_hs_cacheable_page();
        TEST_SETUP_EXCEPT();
        if (recover_size == 2U) {
            sh(recover_vaddr, (uint16_t)recover_value);
        } else {
            sw(recover_vaddr, (uint32_t)recover_value);
        }
        TEST_ASSERT("after the faulting same-16B direct-misalign narrow store, the different-shape recovery store completes normally and refreshes the active byte mask",
            excpt.triggered == false
        );

        fence_iorw_iorw();
        ai_micro_same_16b_apply_narrow_store(low_pattern, high_pattern, recover_value,
            recover_offsets[round], recover_size, &expected_low, &expected_high);

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-16B direct-misalign narrow store fault-then-different-shape recovery sequence commits only the current shape's byte merge",
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high
        );
    }

    TEST_END();
}

bool ai_micro_hs_load_same_16b_cacheable_page_fault_then_different_shape_recovery_lane_refresh_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t word_fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_WORD_BAD_OFFSET;
    const uintptr_t half_fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_HALF_BAD_OFFSET;
    const uintptr_t word_good_vaddr = ai_micro_same_16b_word_bad_vaddr();
    const uintptr_t half_good_vaddr = ai_micro_same_16b_half_bad_vaddr();
    const uint64_t low_pattern = 0x7faa555443322110ULL;
    const uint64_t high_pattern = 0xf7e6d5c4b3a29180ULL;
    const uint64_t expected_lh =
        ai_micro_same_16b_sign_extend(
            ai_micro_same_16b_extract_window(low_pattern, high_pattern, 7U, 2U),
            2U);
    const uint64_t expected_lw =
        ai_micro_same_16b_sign_extend(
            ai_micro_same_16b_extract_window(low_pattern, high_pattern, 5U, 4U),
            4U);

    goto_priv(PRIV_M);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);

    ai_micro_same_16b_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(low_vaddr);
    TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before switching narrow page-fault and recovery shapes",
        excpt.triggered == false &&
        warm == low_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    (void)lwu(word_fault_vaddr);
    TEST_ASSERT("a cacheable same-16B direct-misalign lwu that first page-faults at the cross-doubleword word shape reports load page fault on its own word alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == word_fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-faulting same-16B direct-misalign word load leaves both participating backing doublewords unchanged before the narrower recovery probe",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t observed_lh = (uint64_t)(int64_t)(int16_t)lh(half_good_vaddr);
    TEST_ASSERT("after the page-faulting word-shaped same-16B direct-misalign load, recovering with lh on the halfword alias refreshes lane ownership and returns the correct sign-extended halfword",
        excpt.triggered == false &&
        observed_lh == expected_lh
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    (void)lhu(half_fault_vaddr);
    TEST_ASSERT("reversing the order, a cacheable same-16B direct-misalign lhu page fault on the halfword alias reports its own load page fault VA instead of inheriting the older word alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == half_fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-faulting same-16B direct-misalign halfword load also leaves both participating backing doublewords unchanged before the wider recovery probe",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    ai_micro_same_16b_prepare_hs_cacheable_page();
    TEST_SETUP_EXCEPT();
    uint64_t observed_lw = (uint64_t)(int64_t)(int32_t)lw(word_good_vaddr);
    TEST_ASSERT("after the page-faulting halfword-shaped same-16B direct-misalign load, recovering with lw on the word alias refreshes width and sign ownership and returns the correct sign-extended word",
        excpt.triggered == false &&
        observed_lw == expected_lw
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-16B direct-misalign narrow page-fault-then-different-shape recovery load sequence keeps both backing doublewords unchanged throughout",
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_page_fault_then_different_shape_recovery_mask_refresh_corner()
{
    TEST_START();

    const uintptr_t low_paddr = ai_micro_same_16b_low_paddr();
    const uintptr_t high_paddr = ai_micro_same_16b_high_paddr();
    const uintptr_t low_vaddr = ai_micro_same_16b_low_vaddr();
    const uintptr_t word_fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_WORD_BAD_OFFSET;
    const uintptr_t half_fault_vaddr = hs_page_base(VSI_GI) + AI_MICRO_SAME_16B_HALF_BAD_OFFSET;
    const uintptr_t word_good_vaddr = ai_micro_same_16b_word_bad_vaddr();
    const uintptr_t half_good_vaddr = ai_micro_same_16b_half_bad_vaddr();
    static const unsigned fault_sizes[] = {4U, 2U};
    static const unsigned recover_sizes[] = {2U, 4U};
    static const unsigned recover_offsets[] = {7U, 5U};
    static const uint64_t low_patterns[] = {
        0x8899aabbccddeeffULL,
        0x1021324354657687ULL,
    };
    static const uint64_t high_patterns[] = {
        0x7766554433221100ULL,
        0x98badcfe10325476ULL,
    };
    static const uint64_t fault_values[] = {
        0x00000000c3d2e1f0ULL,
        0x0000000000005aa5ULL,
    };
    static const uint64_t recover_values[] = {
        0x0000000000005aa5ULL,
        0x00000000c3d2e1f0ULL,
    };

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned fault_size = fault_sizes[round];
        const unsigned recover_size = recover_sizes[round];
        const uintptr_t fault_vaddr =
            (round == 0U) ? word_fault_vaddr : half_fault_vaddr;
        const uintptr_t recover_vaddr =
            (round == 0U) ? half_good_vaddr : word_good_vaddr;
        const uint64_t low_pattern = low_patterns[round];
        const uint64_t high_pattern = high_patterns[round];
        const uint64_t fault_value = fault_values[round];
        const uint64_t recover_value = recover_values[round];
        uint64_t expected_low = 0ULL;
        uint64_t expected_high = 0ULL;

        goto_priv(PRIV_M);
        write64(low_paddr, low_pattern);
        write64(high_paddr, high_pattern);

        ai_micro_same_16b_prepare_hs_cacheable_page();

        TEST_SETUP_EXCEPT();
        uint64_t warm = ld(low_vaddr);
        TEST_ASSERT("an aligned HS load first warms the cacheable same-16B direct-misalign page before switching narrow store page-fault and recovery shapes",
            excpt.triggered == false &&
            warm == low_pattern
        );

        ai_micro_same_16b_prepare_hs_cacheable_page();
        TEST_SETUP_EXCEPT();
        if (fault_size == 2U) {
            sh(fault_vaddr, (uint16_t)fault_value);
        } else {
            sw(fault_vaddr, (uint32_t)fault_value);
        }
        TEST_ASSERT("a cacheable same-16B direct-misalign narrow store that first page-faults reports store page fault on its own shape-specific alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SPF &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the page-faulting same-16B direct-misalign narrow store leaves both participating backing doublewords unchanged before the different-shape recovery store",
            read64(low_paddr) == low_pattern &&
            read64(high_paddr) == high_pattern
        );

        ai_micro_same_16b_prepare_hs_cacheable_page();
        TEST_SETUP_EXCEPT();
        if (recover_size == 2U) {
            sh(recover_vaddr, (uint16_t)recover_value);
        } else {
            sw(recover_vaddr, (uint32_t)recover_value);
        }
        TEST_ASSERT("after the page-faulting same-16B direct-misalign narrow store, the different-shape recovery store completes normally and refreshes the active byte mask",
            excpt.triggered == false
        );

        fence_iorw_iorw();
        ai_micro_same_16b_apply_narrow_store(low_pattern, high_pattern, recover_value,
            recover_offsets[round], recover_size, &expected_low, &expected_high);

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-16B direct-misalign narrow store page-fault-then-different-shape recovery sequence commits only the current shape's byte merge",
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high
        );
    }

    TEST_END();
}

static bool ai_micro_same_16b_store_fault_then_sq_wrap_direct_sd_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_FAULT_SD_CASES = 2,
        AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS = 8,
        AI_MICRO_SAME_16B_FAULT_SD_WRAP_STORES = 64
    };
    const bool use_access_fault[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        false,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        VSI_GI,
        VSRWX_GRWX,
    };
    const enum test_page recover_pages[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        VSRWX_GRWX,
        SWITCH1,
    };
    const enum test_page scratch_pages[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        SWITCH1,
        VSRW_GRW,
    };
    const uintptr_t fault_offsets[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        AI_MICRO_SAME_16B_BAD_OFFSET,
        22U,
    };
    const uintptr_t recover_offsets[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        22U,
        AI_MICRO_SAME_16B_BAD_OFFSET,
    };
    const unsigned recover_byte_offsets[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        6U,
        1U,
    };
    const uint64_t fault_values[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t recover_values[AI_MICRO_SAME_16B_FAULT_SD_CASES] = {
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t fault_patterns[AI_MICRO_SAME_16B_FAULT_SD_CASES][4] = {
        {
            0x1021324354657687ULL,
            0x8899aabbccddeeffULL,
            0x7766554433221100ULL,
            0x98badcfe10325476ULL,
        },
        {
            0x3141592653589793ULL,
            0xdeadbeefcafef00dULL,
            0x0badf00dfacefeedULL,
            0x2718281828459045ULL,
        },
    };
    const uint64_t recover_patterns[AI_MICRO_SAME_16B_FAULT_SD_CASES][4] = {
        {
            0xa5b4c3d2e1f00112ULL,
            0x1122334455667788ULL,
            0x99aabbccddeeff00ULL,
            0x8877665544332211ULL,
        },
        {
            0x5566778899aabbccULL,
            0xfedcba9876543210ULL,
            0x0123456789abcdefULL,
            0xc3d2e1f0a5968778ULL,
        },
    };
    const uint64_t scratch_bases[AI_MICRO_SAME_16B_FAULT_SD_CASES][AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS] = {
        {
            0x1111222233334444ULL,
            0x5555666677778888ULL,
            0x9999aaaabbbbccccULL,
            0xddddeeeeffff0001ULL,
            0x0123456789abcdefULL,
            0xfedcba9876543210ULL,
            0x13579bdf2468ace0ULL,
            0x0badf00dfacefeedULL,
        },
        {
            0x1021324354657687ULL,
            0x98badcfe10325476ULL,
            0x3141592653589793ULL,
            0x2718281828459045ULL,
            0x89abcdef01234567ULL,
            0x7766554433221100ULL,
            0x8877665544332211ULL,
            0xa5a55a5ac3c33c3cULL,
        },
    };
    bool test_status = true;

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_FAULT_SD_CASES; ++round) {
        const uintptr_t scratch_base_phys = phys_page_base(scratch_pages[round]);
        const uintptr_t fault_vaddr =
            ai_micro_same_16b_window_vaddr(fault_pages[round], fault_offsets[round]);
        const uintptr_t recover_vaddr =
            ai_micro_same_16b_window_vaddr(recover_pages[round],
                recover_offsets[round]);
        const uintptr_t recover_low_vaddr =
            ai_micro_same_16b_window_vaddr(recover_pages[round],
                AI_MICRO_SAME_16B_LOW_OFFSET);
        const uintptr_t scratch_base_vaddr = hs_page_base(scratch_pages[round]);
        const uintptr_t fault_leading_phys =
            ai_micro_same_16b_window_paddr(fault_pages[round], 8U);
        const uintptr_t fault_low_phys =
            ai_micro_same_16b_window_paddr(fault_pages[round],
                AI_MICRO_SAME_16B_LOW_OFFSET);
        const uintptr_t fault_high_phys =
            ai_micro_same_16b_window_paddr(fault_pages[round],
                AI_MICRO_SAME_16B_HIGH_OFFSET);
        const uintptr_t fault_trailing_phys =
            ai_micro_same_16b_window_paddr(fault_pages[round], 32U);
        const uintptr_t recover_leading_phys =
            ai_micro_same_16b_window_paddr(recover_pages[round], 8U);
        const uintptr_t recover_low_phys =
            ai_micro_same_16b_window_paddr(recover_pages[round],
                AI_MICRO_SAME_16B_LOW_OFFSET);
        const uintptr_t recover_high_phys =
            ai_micro_same_16b_window_paddr(recover_pages[round],
                AI_MICRO_SAME_16B_HIGH_OFFSET);
        const uintptr_t recover_trailing_phys =
            ai_micro_same_16b_window_paddr(recover_pages[round], 32U);
        const uint64_t fault_leading = fault_patterns[round][0];
        const uint64_t fault_low = fault_patterns[round][1];
        const uint64_t fault_high = fault_patterns[round][2];
        const uint64_t fault_trailing = fault_patterns[round][3];
        const uint64_t recover_leading = recover_patterns[round][0];
        const uint64_t recover_low = recover_patterns[round][1];
        const uint64_t recover_high = recover_patterns[round][2];
        const uint64_t recover_trailing = recover_patterns[round][3];
        uint64_t expected_recover_low = 0ULL;
        uint64_t expected_recover_high = 0ULL;
        uint64_t expected_scratch[AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS];
        bool wrap_ok = true;
        const char *fault_desc =
            use_access_fault[round] ?
            "a same-16B direct-misalign sd on the leaf-overflow access-fault path reports store access fault before the later SQ-wrap recovery probe" :
            "a cacheable same-16B direct-misalign sd that faults first on the page-fault path reports store page fault before the later SQ-wrap recovery probe";
        const char *unchanged_desc =
            use_access_fault[round] ?
            "the access-faulting same-16B direct-misalign sd leaves both the fault window and the later recovery window plus scratch slots unchanged before cleanup" :
            "the page-faulting same-16B direct-misalign sd leaves both the fault window and the later recovery window plus scratch slots unchanged before cleanup";
        const char *wrap_desc =
            use_access_fault[round] ?
            "after clearing the same-16B direct-store access fault, the aligned scalar SQ-wrap stress stores all retire normally before recovery" :
            "after clearing the same-16B direct-store page fault, the aligned scalar SQ-wrap stress stores all retire normally before recovery";
        const char *recover_desc =
            use_access_fault[round] ?
            "after the same-16B direct-store access-fault episode and SQ-wrap stress window, the valid same-16B direct sd recovery commits without exception" :
            "after the same-16B direct-store page-fault episode and SQ-wrap stress window, the valid same-16B direct sd recovery commits without exception";
        const char *final_desc =
            check_adjacent ?
            "after the same-16B direct sd fault-then-SQ-wrap recovery sequence, only the recovery window changes while both fault and recovery guards plus scratch slots stay exact" :
            "after the same-16B direct sd fault-then-SQ-wrap recovery sequence, only the recovery window changes while the fault window and scratch slots stay exact";

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading);
        write64(fault_low_phys, fault_low);
        write64(fault_high_phys, fault_high);
        write64(fault_trailing_phys, fault_trailing);
        write64(recover_leading_phys, recover_leading);
        write64(recover_low_phys, recover_low);
        write64(recover_high_phys, recover_high);
        write64(recover_trailing_phys, recover_trailing);
        for (unsigned slot = 0;
             slot < AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS;
             ++slot) {
            expected_scratch[slot] = scratch_bases[round][slot];
            write64(scratch_base_phys + (slot << 3), expected_scratch[slot]);
        }

        ai_micro_same_16b_prepare_hs_cacheable_page();

        TEST_SETUP_EXCEPT();
        uint64_t warm = ld(recover_low_vaddr);
        TEST_ASSERT("an aligned HS load first warms the valid same-16B direct-store recovery page before the fault and SQ-wrap probe",
            excpt.triggered == false &&
            warm == recover_low
        );

        if (use_access_fault[round]) {
            ai_micro_same_16b_prepare_hs_access_fault_page();
        } else {
            ai_micro_same_16b_prepare_hs_cacheable_page();
        }

        TEST_SETUP_EXCEPT();
        sd(fault_vaddr, fault_values[round]);
        TEST_ASSERT(fault_desc,
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[round] ? CAUSE_SAF : CAUSE_SPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(unchanged_desc,
            read64(fault_leading_phys) == fault_leading &&
            read64(fault_low_phys) == fault_low &&
            read64(fault_high_phys) == fault_high &&
            read64(fault_trailing_phys) == fault_trailing &&
            read64(recover_leading_phys) == recover_leading &&
            read64(recover_low_phys) == recover_low &&
            read64(recover_high_phys) == recover_high &&
            read64(recover_trailing_phys) == recover_trailing &&
            ai_micro_same_16b_words_match(scratch_base_phys, expected_scratch,
                AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS)
        );

        ai_micro_same_16b_prepare_hs_cacheable_page();

        goto_priv(PRIV_HS);
        for (unsigned j = 0; j < AI_MICRO_SAME_16B_FAULT_SD_WRAP_STORES; ++j) {
            const unsigned slot =
                j & (AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS - 1U);
            const uint64_t value = scratch_bases[round][slot] +
                (uint64_t)(j + 1U) * 0x0101010101010101ULL;

            expected_scratch[slot] = value;
            TEST_SETUP_EXCEPT();
            sd(scratch_base_vaddr + (slot << 3), value);
            fence_iorw_iorw();
            wrap_ok = wrap_ok && (excpt.triggered == false);
        }
        TEST_ASSERT(wrap_desc,
            wrap_ok
        );

        TEST_SETUP_EXCEPT();
        sd(recover_vaddr, recover_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT(recover_desc,
            excpt.triggered == false
        );

        ai_micro_same_16b_apply_narrow_store(recover_low, recover_high,
            recover_values[round], recover_byte_offsets[round], 8U,
            &expected_recover_low, &expected_recover_high);

        goto_priv(PRIV_M);
        TEST_ASSERT(final_desc,
            read64(fault_leading_phys) == fault_leading &&
            read64(fault_low_phys) == fault_low &&
            read64(fault_high_phys) == fault_high &&
            read64(fault_trailing_phys) == fault_trailing &&
            read64(recover_low_phys) == expected_recover_low &&
            read64(recover_high_phys) == expected_recover_high &&
            ai_micro_same_16b_words_match(scratch_base_phys, expected_scratch,
                AI_MICRO_SAME_16B_FAULT_SD_SCRATCH_SLOTS) &&
            (!check_adjacent ||
                (read64(recover_leading_phys) == recover_leading &&
                 read64(recover_trailing_phys) == recover_trailing))
        );
    }

    ai_micro_same_16b_prepare_hs_cacheable_page();

    return test_status;
}

bool ai_micro_hs_store_same_16b_cacheable_fault_then_sq_wrap_direct_sd_recovery_no_fake_cross16_state_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_fault_then_sq_wrap_direct_sd_window(false);
    TEST_END();
}

bool ai_micro_hs_store_same_16b_cacheable_fault_then_sq_wrap_direct_sd_recovery_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_same_16b_store_fault_then_sq_wrap_direct_sd_window(true);
    TEST_END();
}
