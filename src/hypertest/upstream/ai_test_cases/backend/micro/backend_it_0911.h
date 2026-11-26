#ifndef BACKEND_IT_0911_H
#define BACKEND_IT_0911_H

#include <rvh_test.h>
#include <page_tables.h>

/* ABI for an external IT monitor, not a hardware injection register.
 * See docs/backend_it_0911_notes.md. All fields are 64-bit little endian.
 */
struct backend_it_control {
    uint64_t version, row, epoch, phase;
    uint64_t iteration, target_pc, target_addr, auxiliary;
    uint64_t ready_epoch, observed_epoch, errors, capabilities;
};
/* The monitor advertises these only after explicit runtime opt-in. */
#define BE_IT_CAP_TAG 0x4245495400000000ULL
#define BE_IT_CAP_MONITOR 1ULL
#define BE_IT_CAP_DEBUG 2ULL
#define BE_IT_CAP_RESET 4ULL
#define BE_IT_CAP_EVENT 8ULL
#define BE_IT_CAP_BOOT 16ULL
#define BE_IT_CAP_CMO 32ULL

/* Stable stimulus selectors. Excel rows remain reporting/witness IDs and may
 * move when the review table changes; agents must never key behavior off them.
 * The selector is carried in iteration[63:32], preserving the version-1 ABI.
 */
enum backend_it_scenario {
    BE_IT_SC_NONE = 0,

    BE_IT_SC_DEBUG_WFI_HALTREQ = 0x1001,
    BE_IT_SC_DEBUG_WFI_TRIGGER = 0x1002,
    BE_IT_SC_DEBUG_WFI_STEP = 0x1003,
    BE_IT_SC_DEBUG_BOUNDARY_TRIGGER = 0x1011,
    BE_IT_SC_DEBUG_BOUNDARY_STEP = 0x1012,
    BE_IT_SC_DEBUG_BOUNDARY_HALTREQ = 0x1013,

    BE_IT_SC_RESET_FTQ = 0x2001,
    BE_IT_SC_RESET_ICACHE_MSHR = 0x2002,
    BE_IT_SC_RESET_INSTR_UNCACHE = 0x2003,
    BE_IT_SC_RESET_IBUFFER = 0x2004,
    BE_IT_SC_RESET_RENAME = 0x2005,
    BE_IT_SC_RESET_IQ_FU = 0x2006,
    BE_IT_SC_RESET_LSU = 0x2007,
    BE_IT_SC_RESET_EXCEPTION = 0x2008,
    BE_IT_SC_RESET_FULL_PIPE = 0x2009,
    BE_IT_SC_BOOT_BPU_VECTOR = 0x2101,
    BE_IT_SC_BOOT_ICACHE_RESET = 0x2102,
    BE_IT_SC_BOOT_CSR_RESET = 0x2103,

    BE_IT_SC_REFILL_FENCEI_EARLY = 0x3001,
    BE_IT_SC_REFILL_FENCEI_OVERLAP = 0x3002,
    BE_IT_SC_REFILL_FENCEI_LATE = 0x3003,
    BE_IT_SC_REFILL_FENCEI_CROSSLINE = 0x3004,
    BE_IT_SC_FLUSH_OLD_RESPONSE_EARLY = 0x3011,
    BE_IT_SC_FLUSH_OLD_RESPONSE_SAME = 0x3012,
    BE_IT_SC_FLUSH_OLD_RESPONSE_LATE = 0x3013,
    BE_IT_SC_BOUNDARY_FENCEI_REFILL_EARLY = 0x3021,
    BE_IT_SC_BOUNDARY_FENCEI_REFILL_LATE = 0x3022,

    BE_IT_SC_BOUNDARY_IRQ_TARGET = 0x4001,
    BE_IT_SC_BOUNDARY_IRQ_VECTOR = 0x4002,
    BE_IT_SC_BOUNDARY_IRQ_LATE = 0x4003,
    BE_IT_SC_BOUNDARY_IRQ_ENABLE = 0x4004,
    BE_IT_SC_BOUNDARY_IRQ_DISABLE = 0x4005,
    BE_IT_SC_BOUNDARY_IRQ_CLEAR = 0x4006,
    BE_IT_SC_BOUNDARY_IRQ_ACCEPT_CLEAR = 0x4007,
    BE_IT_SC_BOUNDARY_BREAKPOINT_IRQ = 0x4008,
    BE_IT_SC_BOUNDARY_RNMI = 0x5001,

    BE_IT_SC_BOUNDARY_IRQ_M_IPF_S = 0x5101,
    BE_IT_SC_BOUNDARY_IRQ_S_IPF_M = 0x5102,
    BE_IT_SC_BOUNDARY_IRQ_DIRECT = 0x5103,
    BE_IT_SC_BOUNDARY_IRQ_VECTORED = 0x5104,
    BE_IT_SC_BOUNDARY_IRQ_VECTOR_IPF = 0x5105,
    BE_IT_SC_BOUNDARY_HANDLER_IAF = 0x5106,
    BE_IT_SC_BOUNDARY_HANDLER_ILLEGAL = 0x5107,
    BE_IT_SC_BOUNDARY_RETURN_SV48 = 0x5108,
    BE_IT_SC_BOUNDARY_RETURN_NEW_IPF = 0x5109,
    BE_IT_SC_BOUNDARY_TARGET_NOT_ROB = 0x5110,
    BE_IT_SC_BOUNDARY_HANDLER_OLD_REFILL = 0x5111,
    BE_IT_SC_BOUNDARY_OLDER_LOAD_IRQ = 0x5112,

    BE_IT_SC_CMO_COMPLETION = 0x6001,
    BE_IT_SC_WFI_FRONTEND_FIRST = 0x7001,
    BE_IT_SC_WFI_MEMORY_FIRST = 0x7002,
    BE_IT_SC_WFI_L2_TAIL_SCAN = 0x7003
};

extern volatile struct backend_it_control backend_it_control;
bool be_it_begin(unsigned row, unsigned iteration);
bool be_it_begin_scenario(unsigned row, unsigned scenario, unsigned iteration);
bool be_it_prepare_scenario(unsigned row, unsigned scenario, unsigned iteration);
void be_it_activate(void);
bool be_it_finish(void);
bool be_it_finish_timeout(unsigned timeout);

bool be_it_enable(void);

#define BE_ASM_BEGIN ".option push\n.option norvc\n.option norelax\n"
#define BE_ASM_END ".option pop\n"
#define BE_DELAY "li t5, -1\nli t6, 3\n.rept 8\ndivu t5,t5,t6\n.endr\n"

#endif
