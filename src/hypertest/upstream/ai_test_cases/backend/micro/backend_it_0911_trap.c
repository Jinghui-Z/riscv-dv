#include "backend_it_0911_trap.h"

volatile struct be_it_trap_state be_it_traps __attribute__((aligned(64)));
static uint64_t trap_mtvec, trap_mscratch, trap_medeleg, trap_mideleg, trap_mie, trap_status;
extern void be_it_trap_vector(void);
asm(".pushsection .text.be_it_trap,\"ax\",@progbits\n"
    BE_ASM_BEGIN ".balign 4\n.global be_it_trap_vector\nbe_it_trap_vector:\n"
    "csrrw t0,mscratch,t0\nsd t1,0(t0)\nsd t2,8(t0)\nsd t3,16(t0)\n"
    "li t1,0x20000\ncsrc mstatus,t1\nld t1,24(t0)\nli t2,4\nbgeu t1,t2,2f\n"
    "slli t2,t1,5\nadd t2,t2,t0\ncsrr t3,mcause\nsd t3,40(t2)\n"
    "csrr t3,mepc\nsd t3,48(t2)\ncsrr t3,mtval\nsd t3,56(t2)\n"
    "csrr t3,vstart\nsd t3,64(t2)\n2: addi t1,t1,1\nsd t1,24(t0)\n"
    "csrr t1,mcause\nbgez t1,5f\nld t1,176(t0)\nbeqz t1,7f\n"
    /* Preserve local enable/pending for a WFI that has not executed yet.
     * Clear MPIE so mret cannot immediately service the level again. */
    "li t1,0x80\ncsrc mstatus,t1\nj 3f\n7: csrw mie,zero\n"
#if VMODULE
    /* All scratch registers used here are saved above. Cancel the injection
     * delay first, then clear the level source before returning from trap. */
    "li t1,0xffff000100001028\nli t2,0xffff000000000000\n"
    "fence.i\nadd zero,zero,t1\nfence.i\nadd zero,zero,t2\nfence.i\n"
    "li t1,0xffff000100001000\n"
    "add zero,zero,t1\nfence.i\nadd zero,zero,t2\nfence.i\n"
#endif
    "j 3f\n"
    "5: ld t1,168(t0)\nbeqz t1,6f\nli t1,0x1800\ncsrs mstatus,t1\n"
    "6: ld t1,32(t0)\nbnez t1,4f\n"
    "csrr t1,mepc\naddi t1,t1,4\n4: csrw mepc,t1\n3:\n"
    "ld t1,0(t0)\nld t2,8(t0)\nld t3,16(t0)\ncsrrw t0,mscratch,t0\nmret\n"
    BE_ASM_END ".popsection\n");

bool be_it_trap_install(void)
{
    trap_status = CSRR(CSR_MSTATUS);
    CSRC(CSR_MSTATUS,MSTATUS_MIE);
    trap_mtvec = CSRR(CSR_MTVEC);
    trap_mscratch = CSRR(CSR_MSCRATCH);
    trap_medeleg = CSRR(CSR_MEDELEG);
    trap_mideleg = CSRR(CSR_MIDELEG);
    trap_mie = CSRR(CSR_MIE);
    be_it_traps.count = be_it_traps.resume = 0;
    be_it_traps.return_m = 0;
    be_it_traps.defer_irq_clear = 0;
    for (unsigned i = 0; i < 4; ++i) {
        be_it_traps.event[i].cause = UINT64_MAX;
        be_it_traps.event[i].epc = be_it_traps.event[i].tval = be_it_traps.event[i].vstart = 0;
    }
    CSRW(CSR_MEDELEG,0);
    CSRW(CSR_MIDELEG,0);
    CSRW(CSR_MIE,0);
    CSRW(CSR_MSCRATCH,(uintptr_t)&be_it_traps);
    CSRW(CSR_MTVEC,(uintptr_t)be_it_trap_vector);
    return true;
}

bool be_it_trap_restore(void)
{
    CSRC(CSR_MSTATUS,MSTATUS_MIE | MSTATUS_MPRV);
    CSRW(CSR_MTVEC,trap_mtvec);
    CSRW(CSR_MSCRATCH,trap_mscratch);
    CSRW(CSR_MEDELEG,trap_medeleg);
    CSRW(CSR_MIDELEG,trap_mideleg);
    CSRW(CSR_MIE,trap_mie);
    CSRW(CSR_MSTATUS,trap_status);
    return true;
}
