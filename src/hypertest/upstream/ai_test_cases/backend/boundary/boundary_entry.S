.section .text.boundary_entry,"ax",@progbits
.option push
.option norvc
.option norelax
.balign 4
.global boundary_enter
boundary_enter:
    addi sp,sp,-112
    sd ra,0(sp)
    sd s0,8(sp)
    sd s1,16(sp)
    sd s2,24(sp)
    sd s3,32(sp)
    sd s4,40(sp)
    sd s5,48(sp)
    sd s6,56(sp)
    sd s7,64(sp)
    sd s8,72(sp)
    sd s9,80(sp)
    sd s10,88(sp)
    sd s11,96(sp)
    la t6,boundary_context
    la t0,.Lboundary_resume
    sd t0,0(t6)
    csrw mscratch,t6
    ld t0,96(t6)
    csrw mepc,t0
    csrw sepc,t0
    ld a1,104(t6)
    ld a2,112(t6)
    ld a3,120(t6)
    ld a4,128(t6)
    ld a5,136(t6)
    ld t1,144(t6)
    ld t2,152(t6)
    li s0,85
    li s1,0
    li s2,0
    li s3,0
    li s4,0
    li s6,0
    li t0,0x21888
    csrc mstatus,t0
    slli t1,t1,11
    csrs mstatus,t1
    li t0,1
    beq t2,t0,.Lboundary_sret
    beqz t2,.Lboundary_mret
    li t0,0x20000
    csrs mstatus,t0
.Lboundary_mret:
    mret
.Lboundary_sret:
    li t0,0x120
    csrs mstatus,t0
    sret
.Lboundary_resume:
    ld ra,0(sp)
    ld s0,8(sp)
    ld s1,16(sp)
    ld s2,24(sp)
    ld s3,32(sp)
    ld s4,40(sp)
    ld s5,48(sp)
    ld s6,56(sp)
    ld s7,64(sp)
    ld s8,72(sp)
    ld s9,80(sp)
    ld s10,88(sp)
    ld s11,96(sp)
    addi sp,sp,112
    ret

.balign 4
.global boundary_mtrap
boundary_mtrap:
    /* Disable MPRV before the first memory access of the recovery handler. */
    li t0,0x20000
    csrc mstatus,t0
    csrr t6,mscratch
    csrr t0,mcause
    sd t0,8(t6)
    csrr t0,mepc
    sd t0,16(t6)
    csrr t0,mtval
    sd t0,24(t6)
    csrr t0,mstatus
    sd t0,32(t6)
    csrr t0,mip
    sd t0,40(t6)
    sd s0,48(t6)
    sd s1,56(t6)
    csrr t0,scause
    sd t0,64(t6)
    csrr t0,sepc
    sd t0,72(t6)
    csrr t0,stval
    sd t0,80(t6)
    sd s6,88(t6)
    csrw mie,zero
    csrci mip,2
    ld t0,0(t6)
    csrw mepc,t0
    li t0,0x2188a
    csrc mstatus,t0
    li t0,0x1800
    csrs mstatus,t0
    mret
.option pop
