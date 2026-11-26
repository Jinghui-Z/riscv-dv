/* One binary per POINT; no data access precedes boot_memory_instruction. */
#ifndef POINT
#error POINT must name one boundary test
#endif
.section .text.boot,"ax",@progbits
.option push
.option norvc
.option norelax
.global boot_boundary_entry
boot_boundary_entry:
#if POINT == 101 || POINT == 106
    addi s0,zero,42
#elif POINT == 102
    .2byte 0x0001
    auipc s1,0
#elif POINT == 103
    jal s1,boot_jump_target
    j boot_fail
boot_jump_target:
    la t0,boot_boundary_entry
    addi t0,t0,4
    bne s1,t0,boot_fail
#elif POINT == 104
    .word 0xffffffff
    j boot_fail
#elif POINT == 105
    /* The agent corrupts the real noncacheable flash response. */
    addi s0,zero,42
    j boot_fail
#elif POINT >= 201 && POINT <= 206
    la t0,boot_fault_handler
    csrw mtvec,t0
    li s0,85
    li t0,0x80010000
    li t1,7
#if POINT == 205 || POINT == 206
    srli t2,t0,2
    ori t2,t2,511
    csrw pmpaddr0,t2
#if POINT == 205
    li t2,0x98
#else
    li t2,0x99
#endif
    csrw pmpcfg0,t2
#endif
.global boot_memory_instruction
boot_memory_instruction:
#if POINT == 201 || POINT == 205
    ld s0,0(t0)
#elif POINT == 202 || POINT == 206
    sd t1,0(t0)
#elif POINT == 203
    amoadd.d s0,t1,(t0)
#elif POINT == 204
    sc.d s0,t1,(t0)
#endif
#if POINT == 205 || POINT == 206
    j boot_fail
#elif POINT == 201 || POINT == 203
    li t2,0x1122334455667788
    bne s0,t2,boot_fail
#if POINT == 203
    addi t2,t2,7
#endif
    ld t3,0(t0)
    bne t3,t2,boot_fail
#elif POINT == 202
    ld t3,0(t0)
    bne t3,t1,boot_fail
#elif POINT == 204
    beqz s0,boot_fail
    ld t3,0(t0)
    li t2,0x1122334455667788
    bne t3,t2,boot_fail
#endif
#else
#error Unsupported POINT
#endif

#if POINT == 101 || POINT == 106
    li t0,42
    bne s0,t0,boot_fail
#elif POINT == 102
    la t0,boot_boundary_entry
    addi t0,t0,2
    bne s1,t0,boot_fail
#endif
#if POINT == 106
    csrr t0,mstatus
    andi t0,t0,8
    bnez t0,boot_fail
    csrr t0,mip
    li t1,0x800
    and t0,t0,t1
    beqz t0,boot_fail
#endif
    j boot_pass

.balign 4
boot_fault_handler:
#if POINT == 205 || POINT == 206
    csrr t0,mcause
    li t1,(5 + (POINT-205)*2)
    bne t0,t1,boot_fail
    csrr t0,mepc
    la t1,boot_memory_instruction
    bne t0,t1,boot_fail
    csrr t0,mtval
    li t1,0x80010000
    bne t0,t1,boot_fail
    li t0,85
    bne s0,t0,boot_fail
#if POINT == 206
    ld t0,0(t1)
    li t1,0x1122334455667788
    bne t0,t1,boot_fail
#endif
    j boot_pass
#else
    j boot_fail
#endif

.global boot_pass
boot_pass:
#ifdef BOUNDARY_FLASH_SPIKE
    la t0,tohost
    li t1,1
    sd t1,0(t0)
#else
    li t0,0x40600004
    li t1,66
    sb t1,0(t0)
    li t1,80
    sb t1,0(t0)
    li t1,10
    sb t1,0(t0)
    li t1,0x80
    sb t1,0(t0)
#endif
    j .
.global boot_fail
boot_fail:
#ifdef BOUNDARY_FLASH_SPIKE
    la t0,tohost
    li t1,3
    sd t1,0(t0)
#else
    li t0,0x40600004
    li t1,0x81
    sb t1,0(t0)
#endif
    j .
.option pop

.section .data.boot_seed,"aw",@progbits
.global boot_seed
boot_seed:
    .dword 0x1122334455667788
    .dword 0xa55a5aa55aa55aa5
.section .tohost,"aw",@progbits
.balign 64
.global tohost
tohost: .dword 0
.global fromhost
fromhost: .dword 0
