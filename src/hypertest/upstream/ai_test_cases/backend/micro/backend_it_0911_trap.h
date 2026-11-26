#ifndef BACKEND_IT_0911_TRAP_H
#define BACKEND_IT_0911_TRAP_H
#include "backend_it_0911.h"

struct be_it_trap_record { uint64_t cause, epc, tval, vstart; };
struct be_it_trap_state {
    uint64_t save_t1, save_t2, save_t3, count, resume;
    struct be_it_trap_record event[4];
    uint64_t return_m;
    uint64_t defer_irq_clear;
};
extern volatile struct be_it_trap_state be_it_traps;
bool be_it_trap_install(void);
bool be_it_trap_restore(void);

/* The caller is M-mode, with an identity-mapped S-mode data environment. */
static inline void be_it_mprv_s(void)
{
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, (1ULL << 11) | MSTATUS_MPRV);
}
#endif
