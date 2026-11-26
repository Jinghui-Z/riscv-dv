#ifndef AI_LR_SC_COMMON_H
#define AI_LR_SC_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static uint32_t ai_lr_sc_words[4] __attribute__((aligned(64)));

#define AI_LR_SC_TIMEOUT_CYCLES 64U
#define AI_LR_SC_LONG_GAP_BURN_ROUNDS (AI_LR_SC_TIMEOUT_CYCLES * 16U)
#define AI_LR_SC_ALIAS_DIFF_SETIDX_PAGE SWITCH1
#define AI_LR_SC_ALIAS_SAME_SETIDX_PAGE VSRWX_GX
/* Use a virtual alias that can hit the existing DCache line; LR/SC matching is still paddr+size. */
#define AI_LR_SC_ALIAS_CACHE_HIT_PAGE VSRWX_GX
#define AI_LR_SC_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_LR_SC_MCONTROL6_STORE 0x2ULL
#define AI_LR_SC_MCONTROL6_EXECUTE 0x4ULL

static inline pte_t *ai_lr_sc_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_lr_sc_map_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_lr_sc_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline uintptr_t ai_lr_sc_addr(size_t index)
{
    return (uintptr_t)&ai_lr_sc_words[index];
}

static inline uintptr_t ai_lr_sc_setidx(uintptr_t vaddr)
{
    return (vaddr >> 6) & 0xffU;
}

static inline void ai_lr_sc_prepare_words(uint32_t first, uint32_t second)
{
    goto_priv(PRIV_M);
    ai_lr_sc_words[0] = first;
    ai_lr_sc_words[1] = second;
    ai_lr_sc_words[2] = 0U;
    ai_lr_sc_words[3] = 0U;
}

static inline uint64_t ai_lr_sc_lr_d64(uintptr_t addr)
{
    uint64_t value;
    asm volatile(
        "lr.d %0, 0(%1)\n\t"
        : "=r"(value) : "r"(addr): "memory"
    );
    return value;
}

static inline uint32_t ai_lr_sc_sc_d_status(uintptr_t addr, uint64_t value)
{
    asm volatile(
        "sc.d %0, %0, 0(%1)\n\t"
        : "+r"(value) : "r"(addr): "memory"
    );
    return (uint32_t)value;
}

static inline void ai_lr_sc_burn_integer_ops(unsigned rounds)
{
    uintptr_t scratch0 = 1U;
    uintptr_t scratch1 = 3U;

    for (unsigned i = 0; i < rounds; ++i) {
        asm volatile(
            "add %[a], %[a], %[b]\n\t"
            "xor %[b], %[b], %[a]\n\t"
            "addi %[a], %[a], 1\n\t"
            "addi %[b], %[b], 3\n\t"
            "sub %[a], %[a], %[b]\n\t"
            "xori %[b], %[b], 0x15\n\t"
            "add %[a], %[a], %[b]\n\t"
            "andi %[b], %[b], 0xff\n\t"
            : [a] "+r"(scratch0), [b] "+r"(scratch1)
            :
            : "memory"
        );
    }
}

static inline uint64_t ai_lr_sc_wait_cycle_gap_gt(uint64_t min_cycles)
{
    uint64_t start = RDCYCLE();
    uint64_t now = start;

    while ((now - start) <= min_cycles) {
        ai_lr_sc_burn_integer_ops(1U);
        now = RDCYCLE();
    }

    return now - start;
}

static inline void ai_lr_sc_force_long_gap_without_counters(void)
{
    /*
     * Current Spike plus this HS harness can still trap on rdcycle in some
     * alias-path cases, so use a conservative integer-op burn when the test
     * only needs a gap that is clearly much longer than the 64-cycle window.
     */
    asm volatile(
        "mv t0, %0\n\t"
        "li t1, 1\n\t"
        "li t2, 3\n\t"
        "1:\n\t"
        "add t1, t1, t2\n\t"
        "xor t2, t2, t1\n\t"
        "addi t1, t1, 1\n\t"
        "addi t2, t2, 3\n\t"
        "sub t1, t1, t2\n\t"
        "xori t2, t2, 0x15\n\t"
        "add t1, t1, t2\n\t"
        "andi t2, t2, 0xff\n\t"
        "addi t0, t0, -1\n\t"
        "bnez t0, 1b\n\t"
        :
        : "r"((uintptr_t)AI_LR_SC_LONG_GAP_BURN_ROUNDS)
        : "t0", "t1", "t2", "memory"
    );
}

static inline void ai_lr_sc_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_lr_sc_clear_mcontrol6_0(void)
{
    ai_lr_sc_clear_mcontrol6_slot(0);
}

static inline uint64_t ai_lr_sc_mcontrol6_tdata1(uint64_t access_bits)
{
    return AI_LR_SC_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_lr_sc_arm_mcontrol6_slot(size_t slot, uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_lr_sc_mcontrol6_tdata1(access_bits));
}

static inline void ai_lr_sc_arm_mcontrol6_0(uintptr_t addr, uint64_t access_bits)
{
    ai_lr_sc_arm_mcontrol6_slot(0, addr, access_bits);
}

static inline uint64_t ai_lr_sc_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_lr_sc_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

__attribute__((noinline)) static uint32_t ai_lr_sc_sc_w_helper(uintptr_t addr,
    uint32_t value)
{
    asm volatile(
        "sc.w %0, %0, 0(%1)\n\t"
        : "+r"(value) : "r"(addr): "memory"
    );
    return value;
}

#endif /* AI_LR_SC_COMMON_H */
