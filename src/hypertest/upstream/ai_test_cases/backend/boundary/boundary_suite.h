#ifndef BOUNDARY_SUITE_H
#define BOUNDARY_SUITE_H
#include <rvh_test.h>
#include <page_tables.h>

/* Each descriptor line has a single writer: program or IT monitor. */
struct boundary_control {
    uint64_t magic, point, epoch, phase, target, origin, memory_pc, wrong_pa;
    uint64_t ready, done, errors, evidence, reserved[4];
};
extern volatile struct boundary_control boundary_control;

/* Offsets are shared with boundary_entry.S. All scratch values are captured
 * before the trap returns to C; none of the tested handlers uses a C prologue. */
struct boundary_context {
    uint64_t resume, cause, epc, tval, status, pending;
    uint64_t result, link, scause, sepc, stval, irq_count;
    uint64_t entry, data, value, new_satp, jump, other, privilege, sret;
};
extern volatile struct boundary_context boundary_context;
void boundary_enter(void);
void boundary_mtrap(void);
bool boundary_run(unsigned point, bool monitor_required);
#endif
