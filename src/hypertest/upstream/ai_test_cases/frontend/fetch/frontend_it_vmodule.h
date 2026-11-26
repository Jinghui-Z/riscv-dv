#ifndef FRONTEND_IT_VMODULE_H
#define FRONTEND_IT_VMODULE_H
#include <stdint.h>

/* Atomic ALU command: opcode[31:24], payload[23:0]. Unlike the legacy paired
 * transport, the arm marker must preserve warmed instruction-cache lines. */
static inline void fe_vm_raw(uint32_t opcode, uint32_t payload)
{
    uint64_t command = 0xffff000200000000ULL | ((uint64_t)opcode << 24) |
        (payload & 0xffffffU);
    asm volatile(
        ".option push\n\t.option norvc\n\t"
        "add x0, x0, %[command]\n\t"
        ".rept 32\n\tnop\n\t.endr\n\t.option pop\n\t"
        : : [command] "r"(command) : "memory");
}

static inline void fe_vm_config(uint32_t opcode, uint32_t payload)
{
    asm volatile("fence.i" ::: "memory");
    fe_vm_raw(opcode, payload);
    asm volatile("fence.i" ::: "memory");
}

static inline void fe_vm_addresses(uint64_t target, uint64_t clean, uint64_t fetch_pc)
{
    fe_vm_config(1, 0);
    fe_vm_config(2, target);
    fe_vm_config(3, target >> 24);
    fe_vm_config(4, clean);
    fe_vm_config(5, clean >> 24);
    fe_vm_config(8, fetch_pc);
    fe_vm_config(9, fetch_pc >> 24);
}
#endif
