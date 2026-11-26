#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include "frontend_it_vmodule.h"

/* AP-IT special runs only. No ordinary Spike/difftest gate.
 * TB contract, scenario IDs and acceptance checks: docs/frontend_it_case_mapping.md.
 * The descriptor is an observation interface, not a software error injector.
 * The TB must record a matching injected transaction and the required fcov hit.
 * All scenarios program vmodule_frontend_stimulus directly. Do not also
 * enable a descriptor-driven injector or the legacy 0x1098 corrupt injector.
 */
#ifndef CSR_SFETCHCTL
#define CSR_SFETCHCTL 0x9e0
#endif

enum fe_it_scenario {
    FE_IT_PAGE_CORRUPT = 1,
    FE_IT_SPLIT_ECC0,
    FE_IT_SPLIT_ECC1,
    FE_IT_SINGLE_ECC,
    FE_IT_FIRST_CORRUPT,
    FE_IT_LAST_CORRUPT,
    FE_IT_LINE1_CORRUPT,
    FE_IT_REDIRECT_LATE,
    FE_IT_FENCEI_LATE,
    FE_IT_ECC_STALL,
    FE_IT_CORRUPT_CLEAN
};

volatile struct {
    uint64_t scenario;
    uint64_t phase; /* 0: setup/off; 1: warming; 2: inject window; 3: recovery */
    uint64_t target_pc;
    uint64_t target_pa;
    uint64_t clean_pc;
    uint64_t clean_pa;
} fe_it_inject __attribute__((aligned(64)));

static uint8_t fe_it_code[3 * PAGE_SIZE] __attribute__((aligned(4096)));

/* Same command transport as ai_frontend_it_vmodule_write_reg. */
static void fi_put32(uintptr_t pa, uint32_t inst)
{
    write16(pa, inst);
    write16(pa + 2, inst >> 16);
}

static uint32_t fi_jal(int32_t offset)
{
    uint32_t u = (uint32_t)offset;
    return ((u >> 20 & 1) << 31) | ((u >> 1 & 1023) << 21) |
        ((u >> 11 & 1) << 20) | (u & 0xff000U) | 0x6fU;
}

static uint64_t fi_call(uintptr_t pc, uint64_t seed)
{
    uint64_t value;
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "lla t0, 1f\n\tsd t0, %[fault]\n\tmv a0, %[seed]\n\t"
        "1: jalr ra, 0(%[pc])\n\tmv %[value], a0\n\t.option pop\n\t"
        : [value] "=r"(value), [fault] "=m"(excpt.fault_inst)
        : [pc] "r"(pc), [seed] "r"(seed)
        : "t0", "ra", "a0", "memory");
    excpt.fault_inst = 0;
    return value;
}

extern uint64_t fi_redirect(uintptr_t target, uint64_t taken, uint64_t seed);
extern uint64_t fi_fencei(uintptr_t target, uint64_t seed);
extern const uint8_t fi_redirect_recover[], fi_fencei_recover[];
asm(
    ".pushsection .text.fi_late,\"ax\",@progbits\n"
    ".option push\n.option norvc\n.option norelax\n"
    ".balign 64\n.global fi_redirect\nfi_redirect:\n"
    "addi sp,sp,-16\nsd ra,8(sp)\nmv t2,a0\nmv a0,a2\nli t3,3\nli t4,-1\n"
    ".rept 8\ndivu t4,t4,t3\n.endr\n"
    "sltu t4,t4,zero\nor a1,a1,t4\n"
    "bnez a1,1f\njalr ra,0(t2)\nj 2f\n1: addi a0,a0,2\n2:\n"
    ".global fi_redirect_recover\nfi_redirect_recover:\n"
    "nop\nld ra,8(sp)\naddi sp,sp,16\nret\n"
    ".balign 64\n.global fi_fencei\nfi_fencei:\n"
    "addi sp,sp,-16\nsd ra,8(sp)\nmv t2,a0\nmv a0,a1\nli t3,3\nli t4,-1\n"
    ".rept 32\ndivu t4,t4,t3\n.endr\n"
    "fence.i\njal ra,fe_it_code+128\n"
    ".global fi_fencei_recover\nfi_fencei_recover:\n"
    "nop\nld ra,8(sp)\naddi sp,sp,16\nret\n"
    ".option pop\n.popsection\n");

static bool fi_is_ecc(unsigned id)
{
    return id == FE_IT_SPLIT_ECC0 || id == FE_IT_SPLIT_ECC1 ||
        id == FE_IT_SINGLE_ECC || id == FE_IT_ECC_STALL;
}

static bool fi_run(unsigned id)
{
    goto_priv(PRIV_M);
    uint64_t saved_fetchctl = CSRR(CSR_SFETCHCTL);
    uint64_t saved_pfctl = CSRR(0x5c1);
    if (id == FE_IT_PAGE_CORRUPT || id == FE_IT_LINE1_CORRUPT)
        CSRC(0x5c1, 1); /* Keep the second line cold during first-line warmup. */
    uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    uint64_t saved_mncause = CSRR(CSR_MNCAUSE);
    uint64_t saved_mnepc = CSRR(CSR_MNEPC);
    bool saved_rnmi = rnmi_handler_enabled;
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    CSRS(CSR_SFETCHCTL, 1);
    bool split = id == FE_IT_PAGE_CORRUPT || id == FE_IT_SPLIT_ECC0 ||
        id == FE_IT_SPLIT_ECC1 || id == FE_IT_LINE1_CORRUPT || id == FE_IT_ECC_STALL;
    uintptr_t base = (uintptr_t)fe_it_code;
    uintptr_t pc = base + (id == FE_IT_PAGE_CORRUPT ? PAGE_SIZE - 2 : split ? 62 : 128);
    uintptr_t clean = base + 2 * PAGE_SIZE;
    bool second = id == FE_IT_PAGE_CORRUPT || id == FE_IT_SPLIT_ECC1 ||
        id == FE_IT_LINE1_CORRUPT || id == FE_IT_ECC_STALL;
    uintptr_t entry = id == FE_IT_ECC_STALL ? pc - 36 : second ? pc - 4 : pc;
    fe_vm_addresses(second ? pc + 2 : pc, clean, second ? pc - 4 : pc);
    fe_it_inject.phase = 0;
    if (split)
        fi_put32(pc & ~63ULL, 0x00008067U); /* Single-line warmup entry. */
    if (id == FE_IT_ECC_STALL)
        for (uintptr_t at = entry; at < pc - 4; at += 4)
            fi_put32(at, 0x00000013U);
    fi_put32(pc - 4, 0x00150513U);
    fi_put32(pc, 0x00150513U);
    fi_put32(pc + 4, 0x00008067U);
    fi_put32(clean, 0x00750513U);
    fi_put32(clean + 4, 0x00008067U);
    if (id == FE_IT_CORRUPT_CLEAN)
        fi_put32(pc + 4, fi_jal((int32_t)(clean - (pc + 4))));
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
    fe_it_inject.scenario = id;
    fe_it_inject.target_pc = pc;
    fe_it_inject.target_pa = pc; /* Bare M-mode executable main memory. */
    fe_it_inject.clean_pc = clean;
    fe_it_inject.clean_pa = clean;
    printf("FE_IT scenario=%u pc=0x%llx pa=0x%llx clean=0x%llx\n",
        id, (uint64_t)pc, (uint64_t)pc, (uint64_t)clean);

    fe_it_inject.phase = 1;
    TEST_SETUP_EXCEPT();
    uint64_t clean_expected = id == FE_IT_CORRUPT_CLEAN ? 49 : 42;
    bool correct = fi_call(pc, 41) == clean_expected && !excpt.triggered;
    correct &= fi_call(clean, 41) == 48 && !excpt.triggered;
    if (id == FE_IT_PAGE_CORRUPT || id == FE_IT_LINE1_CORRUPT) {
        /* Train this separate warmup RET before invalidation. Otherwise its
         * first unpredicted fallthrough can fetch/poison the second line on
         * a path that is discarded before the actual split instruction. */
        for (unsigned i = 0; i < 64; ++i)
            correct &= fi_call(pc & ~63ULL, 41) == 41 && !excpt.triggered;
    }
    if (id == FE_IT_CORRUPT_CLEAN) {
        /* Train the target-to-clean JAL so both cold lines can be in flight.
         * The responder must verify overlap and schedule adjacent beats. */
        for (unsigned i = 0; i < 64; ++i)
            correct &= fi_call(pc, 41) == 49 && !excpt.triggered;
    }
    if (id == FE_IT_REDIRECT_LATE || id == FE_IT_FENCEI_LATE) {
        for (unsigned i = 0; i < 64; ++i) {
            excpt.fault_inst = (uintptr_t)(id == FE_IT_REDIRECT_LATE ?
                fi_redirect_recover : fi_fencei_recover);
            uint64_t value = id == FE_IT_REDIRECT_LATE ?
                fi_redirect(pc, 0, 41) : fi_fencei(pc, 41);
            correct &= value == 42 && !excpt.triggered;
        }
        excpt.fault_inst = 0;
    }
    if (!fi_is_ecc(id))
        asm volatile("fence.i" ::: "memory"); /* Make the next request miss. */
    else
        correct &= fi_call(entry, second ? 40 : 41) == 42 && !excpt.triggered;
    TEST_SETUP_EXCEPT();
    /* The integrated core wires ICache ECC reporting to RNMI 43. Clear the
     * observation CSRs so an earlier RNMI cannot satisfy this probe. */
    CSRW(CSR_MNCAUSE, 0);
    CSRW(CSR_MNEPC, 0);
    fe_it_inject.phase = 2;
    fence_iorw_iorw();
    if (fi_is_ecc(id))
        fe_vm_raw(6, id);
    else
        fe_vm_config(6, id | 0x10000U);
    if (id == FE_IT_PAGE_CORRUPT || id == FE_IT_LINE1_CORRUPT)
        correct &= fi_call(pc & ~63ULL, 41) == 41 && !excpt.triggered;
    if (id == FE_IT_REDIRECT_LATE || id == FE_IT_FENCEI_LATE) {
        excpt.fault_inst = (uintptr_t)(id == FE_IT_REDIRECT_LATE ?
            fi_redirect_recover : fi_fencei_recover);
        uint64_t value = id == FE_IT_REDIRECT_LATE ?
            fi_redirect(pc, 1, 41) : fi_fencei(pc, 41);
        correct &= !excpt.triggered &&
            value == (id == FE_IT_REDIRECT_LATE ? 43 : 42);
        excpt.fault_inst = 0;
    } else {
        /* The complete preceding RVI must survive a second-line fault. */
        uint64_t value = fi_call(entry, second ? 40 : 41);
        bool access_fault = excpt.triggered && excpt.cause == CAUSE_IAF &&
            excpt.epc == pc && excpt.tval == pc + (second ? 2 : 0) &&
            value == 41;
        /* RNMI may preempt the synchronous fault before this fetch reaches
         * retirement. MNRET then refetches the unmodified instruction data.
         * The VModule separately requires the addressed ECC to be consumed. */
        uint64_t rnmi_pc = CSRR(CSR_MNEPC);
        bool ecc_rnmi = fi_is_ecc(id) && !excpt.triggered &&
            CSRR(CSR_MNCAUSE) == (CAUSE_INT_BIT | 43ULL) &&
            rnmi_pc >= entry && rnmi_pc <= pc + 4 && value == 42;
        correct &= access_fault || ecc_rnmi;
        printf("FE_IT observed cause=0x%llx epc=0x%llx tval=0x%llx value=%llu rnmi=0x%llx/0x%llx\n",
            excpt.cause, excpt.epc, excpt.tval, value, CSRR(CSR_MNCAUSE), CSRR(CSR_MNEPC));
    }
    fe_vm_config(7, 1); /* Injection and the required temporal window are mandatory. */
    fe_vm_config(1, 0);
    fe_it_inject.phase = 3;
    fence_iorw_iorw();
    /* Source-local clean refill check; simultaneous in-flight/adjacent beats
     * for scenario 11 must additionally be witnessed by the IT monitor. */
    TEST_SETUP_EXCEPT();
    correct &= fi_call(clean, 41) == 48 && !excpt.triggered;
    asm volatile("fence.i" ::: "memory");
    TEST_SETUP_EXCEPT();
    correct &= fi_call(pc, 41) == clean_expected && !excpt.triggered;
    fe_it_inject.phase = 0;
    CSRW(CSR_SFETCHCTL, saved_fetchctl);
    CSRW(0x5c1, saved_pfctl);
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    CSRW(CSR_MNCAUSE, saved_mncause);
    CSRW(CSR_MNEPC, saved_mnepc);
    rnmi_handler_enabled = saved_rnmi;
    return correct;
}

bool ai_micro_frontend_it_second_page_corrupt_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_PAGE_CORRUPT);
    TEST_ASSERT("responding second-page corrupt faults the split instruction and permits clean recovery", correct);
    TEST_END();
}
bool ai_micro_frontend_it_split_first_line_ecc_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_SPLIT_ECC0);
    TEST_ASSERT("first-line ECC prevents the split RVI from committing", correct);
    TEST_END();
}
bool ai_micro_frontend_it_split_second_line_ecc_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_SPLIT_ECC1);
    TEST_ASSERT("second-line ECC reports IAF or RNMI 43 and preserves precise execution after recovery", correct);
    TEST_END();
}
bool ai_micro_frontend_it_single_line_ecc_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_SINGLE_ECC);
    TEST_ASSERT("single-line ECC faults the target PC and clean refetch recovers", correct);
    TEST_END();
}
bool ai_micro_frontend_it_first_beat_corrupt_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_FIRST_CORRUPT);
    TEST_ASSERT("first-beat-only corrupt reaches the architectural IAF", correct);
    TEST_END();
}
bool ai_micro_frontend_it_last_beat_corrupt_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_LAST_CORRUPT);
    TEST_ASSERT("last-beat-only corrupt reaches the architectural IAF", correct);
    TEST_END();
}
bool ai_micro_frontend_it_second_line_corrupt_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_LINE1_CORRUPT);
    TEST_ASSERT("second-line refill corrupt preserves earlier work and faults the high halfword", correct);
    TEST_END();
}
bool ai_micro_frontend_it_redirect_late_corrupt_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_REDIRECT_LATE);
    TEST_ASSERT("a late corrupt response from the redirected path cannot poison the new path", correct);
    TEST_END();
}
bool ai_micro_frontend_it_fencei_late_corrupt_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_FENCEI_LATE);
    TEST_ASSERT("fence.i discards the old corrupt refill before clean target execution", correct);
    TEST_END();
}
bool ai_micro_frontend_it_doubleline_ecc_stall_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_ECC_STALL);
    TEST_ASSERT("second-line ECC survives IFU backpressure and reports IAF or RNMI 43 before recovery", correct);
    TEST_END();
}
bool ai_micro_frontend_it_corrupt_then_clean_injected_corner()
{
    TEST_START();
    bool correct = fi_run(FE_IT_CORRUPT_CLEAN);
    TEST_ASSERT("a corrupt refill cannot contaminate a different clean line/source", correct);
    TEST_END();
}
