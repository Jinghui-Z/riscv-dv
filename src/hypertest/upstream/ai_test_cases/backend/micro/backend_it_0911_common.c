#include "backend_it_0911.h"

volatile struct backend_it_control backend_it_control
    __attribute__((aligned(64)));

bool be_it_enable(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY | (3ULL << 9));
    CSRW(CSR_VSTART, 0);
    CSRW(CSR_FCSR, 0);
    backend_it_control.target_pc = 0;
    backend_it_control.target_addr = 0;
    backend_it_control.auxiliary = 0;
    return true;
}

static void control_publish(void)
{
#ifndef BACKEND_IT_ARCH_SMOKE
    asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(&backend_it_control) : "memory");
#endif
}

static void control_poll(void)
{
#ifndef BACKEND_IT_ARCH_SMOKE
    asm volatile("cbo.inval 0(%0)\nfence rw,rw"
        :: "r"(&backend_it_control.ready_epoch) : "memory");
#endif
}

static uint64_t be_it_required_capabilities(unsigned scenario)
{
    switch (scenario) {
    case BE_IT_SC_DEBUG_WFI_HALTREQ:
    case BE_IT_SC_DEBUG_WFI_TRIGGER:
    case BE_IT_SC_DEBUG_WFI_STEP:
    case BE_IT_SC_DEBUG_BOUNDARY_TRIGGER:
    case BE_IT_SC_DEBUG_BOUNDARY_STEP:
    case BE_IT_SC_DEBUG_BOUNDARY_HALTREQ:
        return BE_IT_CAP_MONITOR | BE_IT_CAP_DEBUG;
    case BE_IT_SC_RESET_FTQ:
    case BE_IT_SC_RESET_ICACHE_MSHR:
    case BE_IT_SC_RESET_INSTR_UNCACHE:
    case BE_IT_SC_RESET_IBUFFER:
    case BE_IT_SC_RESET_RENAME:
    case BE_IT_SC_RESET_IQ_FU:
    case BE_IT_SC_RESET_LSU:
    case BE_IT_SC_RESET_EXCEPTION:
    case BE_IT_SC_RESET_FULL_PIPE:
        return BE_IT_CAP_MONITOR | BE_IT_CAP_RESET;
    case BE_IT_SC_BOOT_BPU_VECTOR:
    case BE_IT_SC_BOOT_ICACHE_RESET:
    case BE_IT_SC_BOOT_CSR_RESET:
        return BE_IT_CAP_MONITOR | BE_IT_CAP_BOOT;
    case BE_IT_SC_BOUNDARY_IRQ_TARGET:
    case BE_IT_SC_BOUNDARY_IRQ_VECTOR:
    case BE_IT_SC_BOUNDARY_IRQ_LATE:
    case BE_IT_SC_BOUNDARY_IRQ_DISABLE:
    case BE_IT_SC_BOUNDARY_IRQ_CLEAR:
    case BE_IT_SC_BOUNDARY_IRQ_ACCEPT_CLEAR:
    case BE_IT_SC_BOUNDARY_BREAKPOINT_IRQ:
    case BE_IT_SC_BOUNDARY_RNMI:
    case BE_IT_SC_BOUNDARY_IRQ_M_IPF_S:
    case BE_IT_SC_BOUNDARY_IRQ_S_IPF_M:
    case BE_IT_SC_BOUNDARY_IRQ_DIRECT:
    case BE_IT_SC_BOUNDARY_IRQ_VECTORED:
    case BE_IT_SC_BOUNDARY_IRQ_VECTOR_IPF:
    case BE_IT_SC_BOUNDARY_HANDLER_IAF:
    case BE_IT_SC_BOUNDARY_HANDLER_ILLEGAL:
    case BE_IT_SC_BOUNDARY_RETURN_SV48:
    case BE_IT_SC_BOUNDARY_RETURN_NEW_IPF:
    case BE_IT_SC_BOUNDARY_TARGET_NOT_ROB:
    case BE_IT_SC_BOUNDARY_HANDLER_OLD_REFILL:
    case BE_IT_SC_BOUNDARY_OLDER_LOAD_IRQ:
        return BE_IT_CAP_MONITOR | BE_IT_CAP_EVENT;
    case BE_IT_SC_CMO_COMPLETION:
        return BE_IT_CAP_MONITOR | BE_IT_CAP_CMO;
    default:
        return BE_IT_CAP_MONITOR;
    }
}

bool be_it_prepare_scenario(unsigned row, unsigned scenario, unsigned iteration)
{
    backend_it_control.phase = 0;
    backend_it_control.version = 1;
    backend_it_control.row = row;
    backend_it_control.iteration = ((uint64_t)scenario << 32) | iteration;
    backend_it_control.epoch++;
    backend_it_control.errors = 0;
    backend_it_control.capabilities = 0;
    asm volatile("fence rw,rw" ::: "memory");
    backend_it_control.phase = 1;
#ifndef BACKEND_IT_ARCH_SMOKE
    /* Response fields occupy their own cache line. Publish its initial
     * contents before the host writes it; polls must not reuse a stale line. */
    asm volatile("cbo.flush 0(%0)\nfence rw,rw"
        :: "r"(&backend_it_control.ready_epoch) : "memory");
    control_publish();
    unsigned timeout = 100000;
    do { control_poll(); }
    while (backend_it_control.ready_epoch != backend_it_control.epoch && --timeout);
    if (!timeout) {
        printf("\nBACKEND_IT row=%u: missing IT monitor acknowledgement; require +backend_it_enable=1 and ELF control address\n", row);
        backend_it_control.phase = 0;
        return false;
    }
    uint64_t required = be_it_required_capabilities(scenario);
    uint64_t capabilities = backend_it_control.capabilities;
    if ((capabilities & ~63ULL) != BE_IT_CAP_TAG || (capabilities & required) != required) {
        printf("\nBACKEND_IT row=%u: monitor opt-in/capabilities missing: %lx\n", row, capabilities);
        backend_it_control.phase = 0;
        control_publish();
        return false;
    }
#endif
    return true;
}

void be_it_activate(void)
{
    backend_it_control.phase = 2;
    asm volatile("fence rw,rw" ::: "memory");
    control_publish();
}

bool be_it_begin_scenario(unsigned row, unsigned scenario, unsigned iteration)
{
    if (!be_it_prepare_scenario(row, scenario, iteration)) return false;
    be_it_activate();
    return true;
}

bool be_it_begin(unsigned row, unsigned iteration)
{
    return be_it_begin_scenario(row, BE_IT_SC_NONE, iteration);
}

bool be_it_finish_timeout(unsigned timeout)
{
    asm volatile("fence rw,rw" ::: "memory");
    backend_it_control.phase = 3;
    control_publish();
#ifdef BACKEND_IT_ARCH_SMOKE
    /* This build checks the software oracle only; it is never an IT gate. */
    backend_it_control.phase = 0;
    return true;
#else
    do { control_poll(); }
    while (backend_it_control.observed_epoch != backend_it_control.epoch && --timeout);
    bool ok = timeout && backend_it_control.errors == 0;
    if (!ok)
        printf("\nBACKEND_IT row=%lu epoch=%lu: missing coverage or monitor error=%lu\n",
            backend_it_control.row, backend_it_control.epoch, backend_it_control.errors);
    backend_it_control.phase = 0;
    control_publish();
    return ok;
#endif
}

bool be_it_finish(void)
{
    return be_it_finish_timeout(100000);
}
