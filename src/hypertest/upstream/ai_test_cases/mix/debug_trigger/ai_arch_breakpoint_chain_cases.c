#include <rvh_test.h>
#include <stdint.h>

#define AI_ARCH_CHAIN_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_ARCH_CHAIN_MCONTROL6_LOAD 0x1ULL
#define AI_ARCH_CHAIN_MCONTROL6_STORE 0x2ULL
#define AI_ARCH_CHAIN_MCONTROL6_EXECUTE 0x4ULL
#define AI_ARCH_CHAIN_MCONTROL6_CHAIN 0x800ULL
#define AI_ARCH_CHAIN_MCONTROL6_U 0x8ULL
#define AI_ARCH_CHAIN_MCONTROL6_S 0x10ULL
#define AI_ARCH_CHAIN_MCONTROL6_M 0x40ULL
#define AI_ARCH_CHAIN_MCONTROL6_MSU \
    (AI_ARCH_CHAIN_MCONTROL6_M | AI_ARCH_CHAIN_MCONTROL6_S | \
     AI_ARCH_CHAIN_MCONTROL6_U)
#define AI_ARCH_CHAIN_MCONTROL6_MATCH_SHIFT 7U
#define AI_ARCH_CHAIN_MCONTROL6_ACTION_SHIFT 12U
#define AI_ARCH_CHAIN_MCONTROL6_DMODE 0x0800000000000000ULL
#define AI_ARCH_CHAIN_MCONTROL6_TYPE_SHIFT 60U
#define AI_ARCH_CHAIN_MCONTROL6_TYPE_MCONTROL6 6ULL
#define AI_ARCH_CHAIN_TDATA1_TYPE_UNSUPPORTED_TMEXTTRIGGER 7ULL
#define AI_ARCH_CHAIN_TDATA1_TYPE_DISABLED 15ULL
#define AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ 0ULL
#define AI_ARCH_CHAIN_MCONTROL6_MATCH_GE 2ULL
#define AI_ARCH_CHAIN_MCONTROL6_MATCH_LT 3ULL
#define AI_ARCH_CHAIN_TRIGGER_NUM 4ULL
#define AI_ARCH_CHAIN_TINFO_MCONTROL6_BIT (1ULL << 6)
#define AI_ARCH_CHAIN_MSTATUS_MPIE (1ULL << 7)
#define AI_ARCH_CHAIN_CLEAR_SLOTS01_RAW() do { \
    asm volatile( \
        "li t0, 0\n\t" \
        "csrw tselect, t0\n\t" \
        "csrw tdata1, x0\n\t" \
        "csrw tdata2, x0\n\t" \
        "li t0, 1\n\t" \
        "csrw tselect, t0\n\t" \
        "csrw tdata1, x0\n\t" \
        "csrw tdata2, x0\n\t" \
        ::: "t0", "memory"); \
} while (0)

#ifndef CSR_TINFO
#define CSR_TINFO 0x7a4
#endif

static uint64_t ai_arch_chain_mem[3] __attribute__((aligned(16)));

static inline void ai_arch_chain_clear_mcontrol6_slot_raw(size_t slot)
{
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_arch_chain_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    ai_arch_chain_clear_mcontrol6_slot_raw(slot);
}

static inline uint64_t ai_arch_chain_mcontrol6_tdata1_match(
    uint64_t access_bits,
    bool chain,
    uint64_t match)
{
    uint64_t value = AI_ARCH_CHAIN_MCONTROL6_MSU_BASE | access_bits;
    value |= (match & 0xfULL) << AI_ARCH_CHAIN_MCONTROL6_MATCH_SHIFT;
    if (chain) {
        value |= AI_ARCH_CHAIN_MCONTROL6_CHAIN;
    }
    return value;
}

static inline uint64_t ai_arch_chain_mcontrol6_tdata1_match_priv(
    uint64_t access_bits,
    uint64_t priv_bits,
    bool chain,
    uint64_t match)
{
    uint64_t value = ai_arch_chain_mcontrol6_tdata1_match(access_bits, chain,
        match);
    value &= ~AI_ARCH_CHAIN_MCONTROL6_MSU;
    value |= priv_bits & AI_ARCH_CHAIN_MCONTROL6_MSU;
    return value;
}

static inline uint64_t ai_arch_chain_mcontrol6_tdata1(uint64_t access_bits,
    bool chain)
{
    return ai_arch_chain_mcontrol6_tdata1_match(
        access_bits, chain, AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
}

static inline void ai_arch_chain_arm_mcontrol6_slot_match(size_t slot,
    uintptr_t addr,
    uint64_t access_bits,
    bool chain,
    uint64_t match)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1,
        ai_arch_chain_mcontrol6_tdata1_match(access_bits, chain, match));
}

static inline void ai_arch_chain_arm_mcontrol6_slot_match_priv(size_t slot,
    uintptr_t addr,
    uint64_t access_bits,
    uint64_t priv_bits,
    bool chain,
    uint64_t match)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_arch_chain_mcontrol6_tdata1_match_priv(access_bits,
        priv_bits, chain, match));
}

static inline void ai_arch_chain_arm_mcontrol6_slot(size_t slot, uintptr_t addr,
    uint64_t access_bits, bool chain)
{
    ai_arch_chain_arm_mcontrol6_slot_match(
        slot, addr, access_bits, chain, AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
}

static inline uint64_t ai_arch_chain_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
    return saved_mstatus;
}

static inline void ai_arch_chain_reenable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);
}

static inline void ai_arch_chain_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline uint64_t ai_arch_chain_mcontrol6_type(uint64_t tdata1)
{
    return (tdata1 >> AI_ARCH_CHAIN_MCONTROL6_TYPE_SHIFT) & 0xfULL;
}

static inline uint64_t ai_arch_chain_mcontrol6_match(uint64_t tdata1)
{
    return (tdata1 >> AI_ARCH_CHAIN_MCONTROL6_MATCH_SHIFT) & 0xfULL;
}

static inline uint64_t ai_arch_chain_mcontrol6_action(uint64_t tdata1)
{
    return (tdata1 >> AI_ARCH_CHAIN_MCONTROL6_ACTION_SHIFT) & 0xfULL;
}

static inline bool ai_arch_chain_trigger_tval_is_arch_visible(uintptr_t pc)
{
    return excpt.tval == 0ULL || excpt.tval == pc;
}

__attribute__((naked, noinline, aligned(4)))
static void ai_arch_chain_store_once(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4)))
static uint64_t ai_arch_chain_load_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld a0, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

bool ai_arch_store_mcontrol6_double_execute_chain_breakpoint_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_arch_chain_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_arch_chain_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_arch_chain_mem[2];
    const uintptr_t trigger_pc = (uintptr_t)ai_arch_chain_store_once;
    const uintptr_t nonmatching_pc = trigger_pc + 4U;
    const uint64_t guard_value = 0x0123456789abcdefULL;
    const uint64_t initial_target = 0x8877665544332211ULL;
    const uint64_t initial_other = 0xfedcba9876543210ULL;
    const uint64_t other_store = 0x13579bdf2468ace0ULL;
    const uint64_t first_target_store = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_target_store = 0x0fedcba987654321ULL;
    const uint64_t final_target_store = 0x55aa33cc77ee11ffULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_value);
    write64(target_addr, initial_target);
    write64(other_addr, initial_other);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    ai_arch_chain_arm_mcontrol6_slot(0, trigger_pc,
        AI_ARCH_CHAIN_MCONTROL6_EXECUTE, true);
    ai_arch_chain_arm_mcontrol6_slot(1, nonmatching_pc,
        AI_ARCH_CHAIN_MCONTROL6_EXECUTE, false);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(other_addr, other_store);
    TEST_ASSERT("with mcontrol6 slot0 chained to a nonmatching execute slot1, an execute-PC match in slot0 alone must not raise breakpoint",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == initial_target &&
        read64(other_addr) == other_store
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_arm_mcontrol6_slot(1, trigger_pc,
        AI_ARCH_CHAIN_MCONTROL6_EXECUTE, false);
    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, first_target_store);
    TEST_ASSERT("when both mcontrol6 execute-PC slots in the chain match the same store helper, the chain raises breakpoint and suppresses the store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == initial_target &&
        read64(other_addr) == other_store
    );
    ai_arch_chain_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, second_target_store);
    TEST_ASSERT("a repeated hit on the same mcontrol6 double-execute chain still reports breakpoint and leaves the target word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == initial_target &&
        read64(other_addr) == other_store
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_clear_mcontrol6_slot(1);
    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, first_target_store);
    TEST_ASSERT("after clearing only slot1, the remaining chained execute-PC slot no longer fires by itself and the store commits",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == first_target_store &&
        read64(other_addr) == other_store
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_clear_mcontrol6_slot(0);
    write64(target_addr, initial_target);
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, final_target_store);
    TEST_ASSERT("after clearing both mcontrol6 chain slots, the same store completes normally and still preserves adjacent words",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == final_target_store &&
        read64(other_addr) == other_store
    );

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_arch_trigger_mcontrol6_chain_mixed_access_bits_no_cross_fire_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_arch_chain_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_arch_chain_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_arch_chain_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_arch_chain_store_once;
    const uintptr_t load_pc = (uintptr_t)ai_arch_chain_load_once;
    const uint64_t guard_value = 0x5566778899aabbccULL;
    const uint64_t initial_target = 0x1123581321345589ULL;
    const uint64_t other_value = 0x0badf00d12345678ULL;
    const uint64_t store_payload = 0xc001d00dcafefeedULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_value);
    write64(target_addr, initial_target);
    write64(other_addr, other_value);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    ai_arch_chain_arm_mcontrol6_slot_match(0, store_pc,
        AI_ARCH_CHAIN_MCONTROL6_EXECUTE, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
    ai_arch_chain_arm_mcontrol6_slot_match(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE | AI_ARCH_CHAIN_MCONTROL6_LOAD, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, store_payload);
    TEST_ASSERT("a chain with slot0 execute-only and slot1 load/store-only must not cross-close on a store helper fetch plus store address match",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == store_payload &&
        read64(other_addr) == other_value
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    write64(target_addr, initial_target);
    ai_arch_chain_arm_mcontrol6_slot_match(0, load_pc,
        AI_ARCH_CHAIN_MCONTROL6_EXECUTE, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
    ai_arch_chain_arm_mcontrol6_slot_match(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE | AI_ARCH_CHAIN_MCONTROL6_LOAD, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    uint64_t loaded = ai_arch_chain_load_once(target_addr);
    TEST_ASSERT("the same mixed execute versus load/store access-bit chain does not cross-close on a load helper fetch plus load address match",
        excpt.triggered == false &&
        loaded == initial_target &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == initial_target &&
        read64(other_addr) == other_value
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, store_payload);
    uint64_t loaded_after_clear = ai_arch_chain_load_once(target_addr);
    TEST_ASSERT("after clearing the mixed access-bit chain, ordinary store and load complete with no stale trigger state",
        excpt.triggered == false &&
        loaded_after_clear == store_payload &&
        read64(guard_addr) == guard_value &&
        read64(other_addr) == other_value
    );

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_arch_trigger_mcontrol6_chain_match_modes_store_addr_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_arch_chain_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_arch_chain_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_arch_chain_mem[2];
    const uintptr_t trigger_pc = (uintptr_t)ai_arch_chain_store_once;
    const uint64_t guard_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t target_initial = 0x1122334455667788ULL;
    const uint64_t other_initial = 0x99aabbccddeeff00ULL;
    const uint64_t eq_store = 0x0102030405060708ULL;
    const uint64_t ge_store = 0x1020304050607080ULL;
    const uint64_t lt_store = 0x8877665544332211ULL;
    const uint64_t gated_store = 0xa5a55a5ac3c33c3cULL;
    const uint64_t final_store = 0xfeedfacecafebeefULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_value);
    write64(target_addr, target_initial);
    write64(other_addr, other_initial);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    ai_arch_chain_arm_mcontrol6_slot_match(0, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
    ai_arch_chain_arm_mcontrol6_slot_match(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, eq_store);
    TEST_ASSERT("a two-slot store-address mcontrol6 chain with match=EQ in both slots raises breakpoint at the store instruction and suppresses the store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == trigger_pc &&
        ai_arch_chain_trigger_tval_is_arch_visible(target_addr) &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == target_initial &&
        read64(other_addr) == other_initial
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_arm_mcontrol6_slot_match(0, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_GE);
    ai_arch_chain_arm_mcontrol6_slot_match(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_GE);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, ge_store);
    TEST_ASSERT("a two-slot store-address mcontrol6 chain with match=GE in both slots treats the accessed bytes as greater-or-equal than tdata2 and suppresses the store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == trigger_pc &&
        ai_arch_chain_trigger_tval_is_arch_visible(target_addr) &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == target_initial &&
        read64(other_addr) == other_initial
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_arm_mcontrol6_slot_match(0, target_addr + sizeof(uint64_t),
        AI_ARCH_CHAIN_MCONTROL6_STORE, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_LT);
    ai_arch_chain_arm_mcontrol6_slot_match(1, target_addr + sizeof(uint64_t),
        AI_ARCH_CHAIN_MCONTROL6_STORE, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_LT);

    ai_arch_chain_store_once(target_addr, lt_store);
    AI_ARCH_CHAIN_CLEAR_SLOTS01_RAW();
    CSRW(CSR_MSTATUS, saved_mstatus);
    TEST_ASSERT("a two-slot store-address mcontrol6 chain with match=LT in both slots treats every accessed byte as less-than tdata2 and suppresses the store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == trigger_pc &&
        ai_arch_chain_trigger_tval_is_arch_visible(target_addr) &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == target_initial &&
        read64(other_addr) == other_initial
    );

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_arm_mcontrol6_slot_match(0, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_LT);
    ai_arch_chain_arm_mcontrol6_slot_match(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    ai_arch_chain_store_once(target_addr, gated_store);
    AI_ARCH_CHAIN_CLEAR_SLOTS01_RAW();
    CSRW(CSR_MSTATUS, saved_mstatus);
    TEST_ASSERT("when the chained slot0 match=LT address condition is false, a matching non-chained slot1 store trigger remains gated and the store commits",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == gated_store &&
        read64(other_addr) == other_initial
    );

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, final_store);
    TEST_ASSERT("after clearing the EQ/GE/LT store-address chain slots, the same helper executes normally and adjacent words are still preserved",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == final_store &&
        read64(other_addr) == other_initial
    );

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_arch_trigger_csr_privilege_warl_matrix_corner()
{
    TEST_START();

    const uint64_t eq_tdata2 = 0x0000000080000120ULL;
    const uint64_t ge_tdata2 = 0x0000000080000100ULL;
    const uint64_t lt_tdata2 = 0x0000000080000200ULL;
    const uint64_t slot1_marker = 0x13572468abcdef00ULL;
    const uint64_t slot2_marker = 0x24681357fedcba00ULL;
    const uint64_t legal_eq =
        ai_arch_chain_mcontrol6_tdata1_match(AI_ARCH_CHAIN_MCONTROL6_EXECUTE,
            false, AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
    const uint64_t legal_ge =
        ai_arch_chain_mcontrol6_tdata1_match(AI_ARCH_CHAIN_MCONTROL6_EXECUTE,
            false, AI_ARCH_CHAIN_MCONTROL6_MATCH_GE);
    const uint64_t legal_lt =
        ai_arch_chain_mcontrol6_tdata1_match(AI_ARCH_CHAIN_MCONTROL6_EXECUTE,
            false, AI_ARCH_CHAIN_MCONTROL6_MATCH_LT);
    const uint64_t dmode_attempt = legal_eq | AI_ARCH_CHAIN_MCONTROL6_DMODE;
    const uint64_t action1_attempt = legal_eq |
        (1ULL << AI_ARCH_CHAIN_MCONTROL6_ACTION_SHIFT);
    const uint64_t unsupported_type_attempt =
        AI_ARCH_CHAIN_TDATA1_TYPE_UNSUPPORTED_TMEXTTRIGGER <<
            AI_ARCH_CHAIN_MCONTROL6_TYPE_SHIFT;

    goto_priv(PRIV_M);
    for (size_t slot = 0; slot < AI_ARCH_CHAIN_TRIGGER_NUM; slot++) {
        ai_arch_chain_clear_mcontrol6_slot(slot);
    }

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TSELECT, 0);
    CSRW(CSR_TDATA2, eq_tdata2);
    CSRW(CSR_TDATA1, legal_eq);
    uint64_t slot0_tdata1 = CSRR(CSR_TDATA1);
    uint64_t slot0_tdata2 = CSRR(CSR_TDATA2);
    TEST_ASSERT("M-mode can write/read an mcontrol6 slot with match=EQ and exact tdata2 readback",
        excpt.triggered == false &&
        ai_arch_chain_mcontrol6_type(slot0_tdata1) ==
            AI_ARCH_CHAIN_MCONTROL6_TYPE_MCONTROL6 &&
        ai_arch_chain_mcontrol6_match(slot0_tdata1) ==
            AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ &&
        ai_arch_chain_mcontrol6_action(slot0_tdata1) == 0ULL &&
        (slot0_tdata1 & AI_ARCH_CHAIN_MCONTROL6_EXECUTE) != 0ULL &&
        slot0_tdata2 == eq_tdata2
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TDATA1, action1_attempt);
    uint64_t action1_readback = CSRR(CSR_TDATA1);
    TEST_ASSERT("when dmode is not set outside Debug Mode, action=1 is WARL-normalized back to breakpoint action=0",
        excpt.triggered == false &&
        (action1_readback & AI_ARCH_CHAIN_MCONTROL6_DMODE) == 0ULL &&
        ai_arch_chain_mcontrol6_action(action1_readback) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TDATA1, dmode_attempt);
    uint64_t dmode_readback = CSRR(CSR_TDATA1);
    TEST_ASSERT("a non-Debug-Mode M-mode write cannot leave tdata1.dmode set for the selected trigger slot",
        excpt.triggered == false &&
        (dmode_readback & AI_ARCH_CHAIN_MCONTROL6_DMODE) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TDATA1, unsupported_type_attempt);
    uint64_t unsupported_type_readback = CSRR(CSR_TDATA1);
    TEST_ASSERT("an unsupported tdata1.type WARL-normalizes to a disabled trigger without trapping",
        excpt.triggered == false &&
        ai_arch_chain_mcontrol6_type(unsupported_type_readback) ==
            AI_ARCH_CHAIN_TDATA1_TYPE_DISABLED
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TSELECT, 1);
    CSRW(CSR_TDATA2, ge_tdata2);
    CSRW(CSR_TDATA1, legal_ge);
    uint64_t slot1_tdata1 = CSRR(CSR_TDATA1);
    uint64_t slot1_tdata2 = CSRR(CSR_TDATA2);
    CSRW(CSR_TSELECT, 2);
    CSRW(CSR_TDATA2, lt_tdata2);
    CSRW(CSR_TDATA1, legal_lt);
    uint64_t slot2_tdata1 = CSRR(CSR_TDATA1);
    uint64_t slot2_tdata2 = CSRR(CSR_TDATA2);
    TEST_ASSERT("M-mode tdata2 remains writable and independent while legal mcontrol6 match=GE/LT values read back per slot",
        excpt.triggered == false &&
        ai_arch_chain_mcontrol6_match(slot1_tdata1) ==
            AI_ARCH_CHAIN_MCONTROL6_MATCH_GE &&
        ai_arch_chain_mcontrol6_match(slot2_tdata1) ==
            AI_ARCH_CHAIN_MCONTROL6_MATCH_LT &&
        slot1_tdata2 == ge_tdata2 &&
        slot2_tdata2 == lt_tdata2
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TSELECT, 1);
    CSRW(CSR_TDATA2, slot1_marker);
    CSRW(CSR_TSELECT, 2);
    CSRW(CSR_TDATA2, slot2_marker);
    CSRW(CSR_TSELECT, 1);
    uint64_t slot1_after_marker = CSRR(CSR_TDATA2);
    CSRW(CSR_TSELECT, 2);
    uint64_t slot2_after_marker = CSRR(CSR_TDATA2);
    TEST_ASSERT("switching tselect selects independent trigger storage for tdata2 writes",
        excpt.triggered == false &&
        slot1_after_marker == slot1_marker &&
        slot2_after_marker == slot2_marker
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_TSELECT, 1);
    CSRW(CSR_TSELECT, AI_ARCH_CHAIN_TRIGGER_NUM);
    uint64_t after_tselect_4 = CSRR(CSR_TSELECT);
    CSRW(CSR_TSELECT, ~0ULL);
    uint64_t after_tselect_all_ones = CSRR(CSR_TSELECT);
    TEST_ASSERT("out-of-range tselect writes are WARL no-effect and keep the previously selected trigger index",
        excpt.triggered == false &&
        after_tselect_4 == 1ULL &&
        after_tselect_all_ones == 1ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t tinfo_before = CSRR(CSR_TINFO);
    CSRW(CSR_TINFO, 0ULL);
    uint64_t tinfo_after_zero = CSRR(CSR_TINFO);
    CSRW(CSR_TINFO, ~0ULL);
    uint64_t tinfo_after_ones = CSRR(CSR_TINFO);
    TEST_ASSERT("tinfo is read-only and advertises mcontrol6 support",
        excpt.triggered == false &&
        (tinfo_before & AI_ARCH_CHAIN_TINFO_MCONTROL6_BIT) != 0ULL &&
        tinfo_after_zero == tinfo_before &&
        tinfo_after_ones == tinfo_before
    );

    for (size_t slot = 0; slot < AI_ARCH_CHAIN_TRIGGER_NUM; slot++) {
        ai_arch_chain_clear_mcontrol6_slot(slot);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)CSRR(CSR_TSELECT);
    TEST_ASSERT("HS-mode reading tselect raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRW(CSR_TDATA1, legal_eq);
    TEST_ASSERT("HS-mode writing tdata1 raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)CSRR(CSR_TINFO);
    TEST_ASSERT("HS-mode reading tinfo raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    (void)CSRR(CSR_TDATA2);
    TEST_ASSERT("HU-mode reading tdata2 raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRW(CSR_TSELECT, 0ULL);
    TEST_ASSERT("HU-mode writing tselect raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_M);
    for (size_t slot = 0; slot < AI_ARCH_CHAIN_TRIGGER_NUM; slot++) {
        ai_arch_chain_clear_mcontrol6_slot(slot);
    }

    TEST_END();
}

bool ai_arch_trigger_mcontrol6_chain_privilege_bits_no_cross_privilege_fire_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_arch_chain_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_arch_chain_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_arch_chain_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_arch_chain_store_once;
    const uint64_t guard_value = 0x1029384756abcdefULL;
    const uint64_t target_initial = 0x7766554433221100ULL;
    const uint64_t other_initial = 0x0f0e0d0c0b0a0908ULL;
    const uint64_t hs_store_payload = 0xa55aa55a11223344ULL;
    const uint64_t m_store_payload = 0xcc33cc3377889900ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_value);
    write64(target_addr, target_initial);
    write64(other_addr, other_initial);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    ai_arch_chain_arm_mcontrol6_slot_match_priv(0, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, AI_ARCH_CHAIN_MCONTROL6_M, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
    ai_arch_chain_arm_mcontrol6_slot_match_priv(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, AI_ARCH_CHAIN_MCONTROL6_S, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, hs_store_payload);
    TEST_ASSERT("a two-slot mcontrol6 store-address chain with slot0 enabled only for M and slot1 enabled only for S must not cross-close on an HS store",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == hs_store_payload &&
        read64(other_addr) == other_initial
    );
    goto_priv(PRIV_M);
    ai_arch_chain_restore_machine_status(saved_mstatus);

    write64(target_addr, target_initial);
    ai_arch_chain_arm_mcontrol6_slot_match_priv(0, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, AI_ARCH_CHAIN_MCONTROL6_M, true,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);
    ai_arch_chain_arm_mcontrol6_slot_match_priv(1, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, AI_ARCH_CHAIN_MCONTROL6_M, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, m_store_payload);
    TEST_ASSERT("when both mcontrol6 store-address chain slots are enabled for M, the same store helper raises breakpoint and suppresses the M-mode store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == store_pc &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == target_initial &&
        read64(other_addr) == other_initial
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_clear_mcontrol6_slot(1);
    saved_mstatus = ai_arch_chain_enable_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, m_store_payload);
    TEST_ASSERT("after clearing the second privilege-qualified chain slot, the M-only chained store-address slot does not fire alone and the store commits",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == m_store_payload &&
        read64(other_addr) == other_initial
    );
    ai_arch_chain_restore_machine_status(saved_mstatus);

    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);

    TEST_END();
}

bool ai_arch_trigger_hs_store_breakpoint_mstatus_mie0_still_sync_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_arch_chain_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_arch_chain_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_arch_chain_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_arch_chain_store_once;
    const uint64_t guard_value = 0x8877665544332211ULL;
    const uint64_t target_initial = 0x0123fedc4567ba98ULL;
    const uint64_t other_initial = 0xa5a5c3c35a5a3c3cULL;
    const uint64_t blocked_payload = 0x13579bdf2468ace0ULL;
    const uint64_t clear_payload = 0xf0e1d2c3b4a59687ULL;
    uint64_t saved_mstatus;
    uint64_t saved_medeleg;
    uint64_t mie0_mstatus;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_value);
    write64(target_addr, target_initial);
    write64(other_addr, other_initial);

    saved_medeleg = CSRR(CSR_MEDELEG);
    CSRW(CSR_MEDELEG, saved_medeleg | (1ULL << CAUSE_BKP));
    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    ai_arch_chain_arm_mcontrol6_slot_match_priv(0, target_addr,
        AI_ARCH_CHAIN_MCONTROL6_STORE, AI_ARCH_CHAIN_MCONTROL6_S, false,
        AI_ARCH_CHAIN_MCONTROL6_MATCH_EQ);

    saved_mstatus = CSRR(CSR_MSTATUS);
    mie0_mstatus = (saved_mstatus | MSTATUS_SIE) &
        ~(MSTATUS_MIE | AI_ARCH_CHAIN_MSTATUS_MPIE);
    CSRW(CSR_MSTATUS, mie0_mstatus);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, blocked_payload);
    TEST_ASSERT("with mstatus.MIE cleared, a delegated HS mcontrol6 store-address trigger remains a synchronous breakpoint and suppresses the matching store",
        excpt.triggered == true &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == store_pc &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == target_initial &&
        read64(other_addr) == other_initial
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    CSRW(CSR_MEDELEG, saved_medeleg);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_chain_store_once(target_addr, clear_payload);
    TEST_ASSERT("after clearing the HS store-address trigger and restoring delegation state, the same store commits while adjacent words stay unchanged",
        excpt.triggered == false &&
        read64(guard_addr) == guard_value &&
        read64(target_addr) == clear_payload &&
        read64(other_addr) == other_initial
    );

    goto_priv(PRIV_M);
    ai_arch_chain_clear_mcontrol6_slot(0);
    ai_arch_chain_clear_mcontrol6_slot(1);
    CSRW(CSR_MSTATUS, saved_mstatus);
    CSRW(CSR_MEDELEG, saved_medeleg);

    TEST_END();
}
