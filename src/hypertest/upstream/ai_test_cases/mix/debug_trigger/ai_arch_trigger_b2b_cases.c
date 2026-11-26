#include <rvh_test.h>
#include <stdint.h>

#define B2B_TYPE  0x6000000000000000ULL
#define B2B_M     0x40ULL
#define B2B_EXEC  0x4ULL
#define B2B_LOAD  0x1ULL
#define B2B_STORE 0x2ULL
#define B2B_CHAIN 0x800ULL
#define B2B_CFG(access) (B2B_TYPE | B2B_M | (access))
#define B2B_SEED  0x1122334455667788ULL
#define B2B_GUARD 0x8877665544332211ULL
#define B2B_VALUE 0x23456789abcdef01ULL

static volatile uint64_t b2b_mem[3] __attribute__((aligned(64)));

typedef uint64_t (*b2b_probe_t)(uint64_t, uint64_t,
    volatile uint64_t *, uint64_t);

/* a0/a1 carry CSR operands; a2 is memory; a3 is the store value/rd sentinel.
 * Keep CSR writes and the target in one assembler fragment. No compiler code,
 * fence, call or branch can be inserted into the adjacency under test.
 */
#define B2B_PROBE(name, padding, writes, target) \
    extern uint64_t name(uint64_t, uint64_t, volatile uint64_t *, uint64_t); \
    extern char name##_target[]; \
    asm(".pushsection .text." #name ",\"ax\",@progbits\n" \
        ".option push\n.option norvc\n.option norelax\n" \
        ".balign 64\n.global " #name "\n.type " #name ",@function\n" \
        #name ":\n" \
        "mv t0, a3\nli t1, 0\n" \
        padding \
        writes \
        ".global " #name "_target\n" #name "_target:\n" \
        target \
        "addi t1, t1, 1\nsd t1, 16(a2)\n" \
        "csrw tdata1, zero\nfence.i\nmv a0, t0\nret\n" \
        ".size " #name ", .-" #name "\n.option pop\n.popsection\n");

B2B_PROBE(b2b_write_store, "", "csrw tdata1, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_write_store_32, ".rept 5\nnop\n.endr\n",
    "csrw tdata1, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_write_store_64, ".rept 13\nnop\n.endr\n",
    "csrw tdata1, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_write_store_split, ".rept 12\nnop\n.endr\n.2byte 0x0001\n",
    "csrw tdata1, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_write_compressed, ".rept 4\nnop\n.endr\n.2byte 0x0001\n",
    "csrw tdata1, a0\n", ".option rvc\nc.addi t0, 1\n.option norvc\n")
B2B_PROBE(b2b_write_load, "", "csrw tdata1, a0\n", "ld t0, 0(a2)\n")
B2B_PROBE(b2b_addr_store, "", "csrw tdata2, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_addr_load, "", "csrw tdata2, a0\n", "ld t0, 0(a2)\n")
B2B_PROBE(b2b_set_store, "", "csrs tdata1, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_clear_store, "", "csrc tdata1, a0\n", "sd a3, 0(a2)\n")
B2B_PROBE(b2b_select_write_store, "", "csrw tselect, a1\ncsrw tdata1, a0\n",
    "sd a3, 0(a2)\n")
B2B_PROBE(b2b_write_twice_store, "", "csrw tdata1, a0\ncsrw tdata1, a1\n",
    "sd a3, 0(a2)\n")
B2B_PROBE(b2b_write_select_store, "", "csrw tdata1, a0\ncsrw tselect, a1\n",
    "sd a3, 0(a2)\n")

enum b2b_operation { B2B_OP_STORE, B2B_OP_LOAD, B2B_OP_ADDI };

struct b2b_layout {
    b2b_probe_t run;
    const char *pc;
    enum b2b_operation op;
    unsigned offset;
};

static const struct b2b_layout b2b_layouts[] = {
    {b2b_write_store, b2b_write_store_target, B2B_OP_STORE, 12},
    {b2b_write_store_32, b2b_write_store_32_target, B2B_OP_STORE, 32},
    {b2b_write_store_64, b2b_write_store_64_target, B2B_OP_STORE, 0},
    {b2b_write_store_split, b2b_write_store_split_target, B2B_OP_STORE, 62},
    {b2b_write_compressed, b2b_write_compressed_target, B2B_OP_ADDI, 30},
};

struct b2b_saved_csrs {
    uint64_t mstatus;
    uint64_t mie;
};

static void b2b_clear_all(void)
{
    for (unsigned slot = 0; slot < 4; ++slot) {
        CSRW(CSR_TSELECT, slot);
        CSRW(CSR_TDATA1, 0);
        CSRW(CSR_TDATA2, 0);
    }
    CSRW(CSR_TSELECT, 0);
}

static struct b2b_saved_csrs b2b_begin(void)
{
    goto_priv(PRIV_M);
    struct b2b_saved_csrs saved = {CSRR(CSR_MSTATUS), CSRR(CSR_MIE)};
    CSRW(CSR_MIE, 0);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    b2b_clear_all();
    return saved;
}

static void b2b_prepare(uint64_t config, uintptr_t address)
{
    b2b_clear_all();
    b2b_mem[0] = B2B_SEED;
    b2b_mem[1] = B2B_GUARD;
    b2b_mem[2] = 0;
    CSRW(CSR_TDATA2, address);
    CSRW(CSR_TDATA1, config);
    /* Establish the OLD configuration before the probe. There is no fence
     * between the probe's configuration update and its target instruction.
     */
    asm volatile("fence.i" ::: "memory");
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
}

static void b2b_finish(struct b2b_saved_csrs saved)
{
    b2b_clear_all();
    CSRW(CSR_MSTATUS, saved.mstatus);
    CSRW(CSR_MIE, saved.mie);
}

static bool b2b_observed(bool hit, uintptr_t pc, uintptr_t tval,
    enum b2b_operation op, uint64_t rd)
{
    bool trap_ok = hit ? (excpt.triggered && excpt.cause == CAUSE_BKP &&
        excpt.epc == pc && excpt.tval == tval) : !excpt.triggered;
    uint64_t expected_mem = !hit && op == B2B_OP_STORE ? B2B_VALUE : B2B_SEED;
    uint64_t expected_rd = B2B_VALUE;
    if (!hit && op == B2B_OP_LOAD) expected_rd = B2B_SEED;
    if (!hit && op == B2B_OP_ADDI) expected_rd = B2B_VALUE + 1;
    return trap_ok && b2b_mem[0] == expected_mem && rd == expected_rd &&
        b2b_mem[1] == B2B_GUARD && b2b_mem[2] == 1;
}

bool ai_arch_trigger_b2b_execute_enable(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    for (unsigned i = 0; i < sizeof(b2b_layouts) / sizeof(b2b_layouts[0]); ++i) {
        const struct b2b_layout *layout = &b2b_layouts[i];
        uintptr_t pc = (uintptr_t)layout->pc;
        b2b_prepare(0, pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = layout->run(B2B_CFG(B2B_EXEC), 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("enabling execute trigger catches the adjacent instruction, with exact PC and no target side effect",
            b2b_observed(true, pc, pc, layout->op, rd) && (pc & 63) == layout->offset,
            "layout=%u pc=0x%lx rd=0x%lx", i, pc, rd);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_execute_disable(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    for (unsigned i = 0; i < sizeof(b2b_layouts) / sizeof(b2b_layouts[0]); ++i) {
        const struct b2b_layout *layout = &b2b_layouts[i];
        uintptr_t pc = (uintptr_t)layout->pc;
        b2b_prepare(B2B_CFG(B2B_EXEC), pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = layout->run(0, 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("disabling execute trigger removes any old hit on the adjacent instruction and preserves its result",
            b2b_observed(false, pc, pc, layout->op, rd) && (pc & 63) == layout->offset,
            "layout=%u pc=0x%lx rd=0x%lx", i, pc, rd);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_execute_retarget(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t pc = (uintptr_t)b2b_addr_store_target;
    for (unsigned hit = 0; hit < 2; ++hit) {
        b2b_prepare(B2B_CFG(B2B_EXEC), hit ? 0 : pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_addr_store(hit ? pc : 0, 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("tdata2 immediately moves execute matching to the new PC and removes matching at the old PC",
            b2b_observed(hit, pc, pc, B2B_OP_STORE, rd), "expect_hit=%u", hit);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_execute_set_clear(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    b2b_probe_t probes[] = {b2b_clear_store, b2b_set_store};
    const char *pcs[] = {b2b_clear_store_target, b2b_set_store_target};
    for (unsigned hit = 0; hit < 2; ++hit) {
        uintptr_t pc = (uintptr_t)pcs[hit];
        b2b_prepare(B2B_CFG(hit ? 0 : B2B_EXEC), pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = probes[hit](B2B_EXEC, 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("csrs/csrc execute-bit updates take effect on the immediately following store PC",
            b2b_observed(hit, pc, pc, B2B_OP_STORE, rd), "expect_hit=%u", hit);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_load_enable_disable(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t address = (uintptr_t)&b2b_mem[0];
    for (unsigned hit = 0; hit < 2; ++hit) {
        b2b_prepare(hit ? 0 : B2B_CFG(B2B_LOAD), address);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_write_load(hit ? B2B_CFG(B2B_LOAD) : 0, 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("load-trigger enable/disable affects the adjacent ld and suppresses rd only on a breakpoint",
            b2b_observed(hit, (uintptr_t)b2b_write_load_target, address, B2B_OP_LOAD, rd),
            "expect_hit=%u rd=0x%lx", hit, rd);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_store_enable_disable(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t address = (uintptr_t)&b2b_mem[0];
    for (unsigned hit = 0; hit < 2; ++hit) {
        b2b_prepare(hit ? 0 : B2B_CFG(B2B_STORE), address);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_write_store(hit ? B2B_CFG(B2B_STORE) : 0, 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("store-trigger enable/disable affects the adjacent sd without an early write or stale breakpoint",
            b2b_observed(hit, (uintptr_t)b2b_write_store_target, address, B2B_OP_STORE, rd),
            "expect_hit=%u", hit);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_address_retarget(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t address = (uintptr_t)&b2b_mem[0];
    uintptr_t other = (uintptr_t)&b2b_mem[1];
    b2b_probe_t probes[] = {b2b_addr_store, b2b_addr_load};
    const char *pcs[] = {b2b_addr_store_target, b2b_addr_load_target};
    for (unsigned load = 0; load < 2; ++load) {
        for (unsigned hit = 0; hit < 2; ++hit) {
            b2b_prepare(B2B_CFG(load ? B2B_LOAD : B2B_STORE), hit ? other : address);
            TEST_SETUP_EXCEPT();
            uint64_t rd = probes[load](hit ? address : other, 0, b2b_mem, B2B_VALUE);
            TEST_ASSERT("address-trigger tdata2 updates affect the next load/store at the new address and stop old-address hits",
                b2b_observed(hit, (uintptr_t)pcs[load], address,
                    load ? B2B_OP_LOAD : B2B_OP_STORE, rd),
                "load=%u expect_hit=%u", load, hit);
        }
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_access_type_switch(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t address = (uintptr_t)&b2b_mem[0];
    b2b_probe_t probes[] = {b2b_write_store, b2b_write_load};
    const char *pcs[] = {b2b_write_store_target, b2b_write_load_target};
    for (unsigned load = 0; load < 2; ++load) {
        uint64_t matching = load ? B2B_LOAD : B2B_STORE;
        uint64_t other = load ? B2B_STORE : B2B_LOAD;
        for (unsigned hit = 0; hit < 2; ++hit) {
            b2b_prepare(B2B_CFG(hit ? other : matching), address);
            TEST_SETUP_EXCEPT();
            uint64_t rd = probes[load](B2B_CFG(hit ? matching : other), 0, b2b_mem, B2B_VALUE);
            TEST_ASSERT("replacing load/store selection uses only the new access type for the adjacent memory instruction",
                b2b_observed(hit, (uintptr_t)pcs[load], address,
                    load ? B2B_OP_LOAD : B2B_OP_STORE, rd),
                "load=%u expect_hit=%u", load, hit);
        }
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_tselect_write(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t pc = (uintptr_t)b2b_select_write_store_target;
    for (unsigned slot = 0; slot < 4; ++slot) {
        b2b_prepare(0, 0);
        for (unsigned i = 0; i < 4; ++i) {
            CSRW(CSR_TSELECT, i);
            CSRW(CSR_TDATA2, i == slot ? pc : 0);
            CSRW(CSR_TDATA1, i == slot ? 0 : B2B_CFG(B2B_EXEC));
        }
        CSRW(CSR_TSELECT, (slot + 1) % 4);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_select_write_store(B2B_CFG(B2B_EXEC), slot, b2b_mem, B2B_VALUE);
        TEST_ASSERT("tselect followed by tdata1 writes the new slot and its adjacent target traps precisely",
            b2b_observed(true, pc, pc, B2B_OP_STORE, rd), "slot=%u", slot);
        bool others_unchanged = true;
        for (unsigned i = 0; i < 4; ++i) {
            if (i == slot) continue;
            CSRW(CSR_TSELECT, i);
            others_unchanged &= CSRR(CSR_TDATA2) == 0 &&
                (CSRR(CSR_TDATA1) & (B2B_M | B2B_EXEC)) == (B2B_M | B2B_EXEC);
        }
        TEST_ASSERT("adjacent tselect/tdata1 writes leave the three other trigger slots unchanged", others_unchanged);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_last_write_wins(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t pc = (uintptr_t)b2b_write_twice_store_target;
    for (unsigned hit = 0; hit < 2; ++hit) {
        b2b_prepare(0, pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_write_twice_store(hit ? 0 : B2B_CFG(B2B_EXEC),
            hit ? B2B_CFG(B2B_EXEC) : 0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("two adjacent tdata1 writes apply the last value to the next instruction, without stale enable or disable",
            b2b_observed(hit, pc, pc, B2B_OP_STORE, rd), "expect_hit=%u", hit);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_machine_enable(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t pc = (uintptr_t)b2b_write_store_target;
    for (unsigned hit = 0; hit < 2; ++hit) {
        b2b_prepare(B2B_TYPE | B2B_EXEC | (hit ? 0 : B2B_M), pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_write_store(B2B_TYPE | B2B_EXEC | (hit ? B2B_M : 0),
            0, b2b_mem, B2B_VALUE);
        TEST_ASSERT("changing only mcontrol6.M updates next-instruction eligibility even when EXECUTE stays set",
            b2b_observed(hit, pc, pc, B2B_OP_STORE, rd), "expect_hit=%u", hit);
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_execute_chain_update(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t pc = (uintptr_t)b2b_addr_store_target;
    for (unsigned first = 0; first < 4; first += 2) {
        for (unsigned hit = 0; hit < 2; ++hit) {
            b2b_prepare(0, 0);
            CSRW(CSR_TSELECT, first);
            CSRW(CSR_TDATA2, pc);
            CSRW(CSR_TDATA1, B2B_CFG(B2B_EXEC) | B2B_CHAIN);
            CSRW(CSR_TSELECT, first + 1);
            CSRW(CSR_TDATA2, hit ? 0 : pc);
            CSRW(CSR_TDATA1, B2B_CFG(B2B_EXEC));
            asm volatile("fence.i" ::: "memory");
            TEST_SETUP_EXCEPT();
            uint64_t rd = b2b_addr_store(hit ? pc : 0, 0, b2b_mem, B2B_VALUE);
            TEST_ASSERT("updating the second execute-chain address immediately closes or breaks the next instruction's match",
                b2b_observed(hit, pc, pc, B2B_OP_STORE, rd),
                "first_slot=%u expect_hit=%u", first, hit);
        }
    }
    b2b_finish(saved);
    TEST_END();
}

bool ai_arch_trigger_b2b_write_then_tselect(void)
{
    TEST_START();
    struct b2b_saved_csrs saved = b2b_begin();
    uintptr_t pc = (uintptr_t)b2b_write_select_store_target;
    for (unsigned slot = 0; slot < 4; ++slot) {
        b2b_prepare(0, 0);
        CSRW(CSR_TSELECT, slot);
        CSRW(CSR_TDATA2, pc);
        TEST_SETUP_EXCEPT();
        uint64_t rd = b2b_write_select_store(B2B_CFG(B2B_EXEC), (slot + 1) % 4,
            b2b_mem, B2B_VALUE);
        TEST_ASSERT("tselect immediately after tdata1 cannot misroute the pending trigger update to another slot",
            b2b_observed(true, pc, pc, B2B_OP_STORE, rd), "written_slot=%u", slot);
        CSRW(CSR_TSELECT, slot);
        TEST_ASSERT("the just-written trigger remains in its original slot after switching tselect",
            CSRR(CSR_TDATA2) == pc &&
            (CSRR(CSR_TDATA1) & (B2B_M | B2B_EXEC)) == (B2B_M | B2B_EXEC));
    }
    b2b_finish(saved);
    TEST_END();
}
