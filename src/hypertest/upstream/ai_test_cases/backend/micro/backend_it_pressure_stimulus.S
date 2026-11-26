/* Ordinary instructions only. a0=data, a1=results, a2=branch training. */
.section .text
.option push
.option norvc
.option norelax
.macro PRESSURE_FINISH
    sd t0,0(a1)
    sd t3,8(a1)
    sd t4,16(a1)
    add a0,t0,t3
    ret
.endm
.macro DEPENDENT_WORK count,src
    .rept \count
    mul t2,\src,t6
    addi t1,t1,1
    .endr
.endm

.balign 64
.global be_pressure_down
be_pressure_down:
    li t6,3
    li t1,0
    li t4,0
.global be_pressure_down_old
be_pressure_down_old:
    ld t0,0(a0)
    DEPENDENT_WORK 160,t0
.global be_pressure_down_young
be_pressure_down_young:
    ld t3,64(a0)
    PRESSURE_FINISH

.balign 64
.global be_pressure_up
be_pressure_up:
    li t6,3
    li t1,0
    li t4,0
.global be_pressure_up_old
be_pressure_up_old:
    ld t0,0(a0)
    DEPENDENT_WORK 48,t0
    j be_pressure_up_ic
.balign 4096
.global be_pressure_up_ic
be_pressure_up_ic:
    DEPENDENT_WORK 160,t0
.global be_pressure_up_young
be_pressure_up_young:
    ld t3,64(a0)
    PRESSURE_FINISH

.balign 64
.global be_pressure_middle
be_pressure_middle:
    li t6,3
    li t5,-1
    li t1,0
.global be_pressure_middle_old
be_pressure_middle_old:
    ld t0,0(a0)
    .rept 8
    divu t5,t5,t6
    .endr
    DEPENDENT_WORK 8,t0
    ld t2,128(a0)
.global be_pressure_middle_alu
be_pressure_middle_alu:
    mul t4,t5,t6
    ld t2,128(a0)
    DEPENDENT_WORK 96,t0
.global be_pressure_middle_young
be_pressure_middle_young:
    ld t3,64(a0)
    PRESSURE_FINISH

.balign 64
.global be_pressure_rob
be_pressure_rob:
    li t6,3
    li t1,0
.global be_pressure_rob_old
be_pressure_rob_old:
    ld t0,0(a0)
.global be_pressure_rob_prefix
be_pressure_rob_prefix:
    mul t4,t6,t6
    .rept 16
    ld t2,128(a0)
    addi t1,t1,1
    .endr
.global be_pressure_rob_young
be_pressure_rob_young:
    ld t3,64(a0)
    DEPENDENT_WORK 128,t3
    PRESSURE_FINISH

.balign 64
.global be_pressure_load
be_pressure_load:
    li t6,3
    li t5,-1
    .rept 15
    divu t5,t5,t6
    .endr
    ld t2,128(a0)
.global be_pressure_load_old
be_pressure_load_old:
    divu t4,t5,t6
.global be_pressure_load_young
be_pressure_load_young:
    ld t0,0(a0)
    .rept 80
    ld t2,128(a0)
    addi t1,t2,1
    .endr
    ld t3,64(a0)
    PRESSURE_FINISH

.balign 64
.global be_pressure_repeat
be_pressure_repeat:
    li t6,3
    li t1,0
    li t4,0
.global be_pressure_repeat_old
be_pressure_repeat_old:
    ld t0,0(a0)
    DEPENDENT_WORK 160,t0
    li t5,-1
    .rept 8
    divu t5,t5,t6
    .endr
    /* The target remains dependent on the real divide chain. */
    sltiu t5,t5,1
    ld t6,8(a0)
    add t6,t6,t5
.global be_pressure_repeat_branch
be_pressure_repeat_branch:
    jalr zero,0(t6)
.global be_pressure_repeat_wrong
be_pressure_repeat_wrong:
    sd t0,24(a1)
2:
.global be_pressure_repeat_young
be_pressure_repeat_young:
    ld t3,64(a0)
    li t6,3
    DEPENDENT_WORK 160,t3
    PRESSURE_FINISH
.balign 64
.global be_pressure_repeat_targets
be_pressure_repeat_targets:
    .rept 8
    j be_pressure_repeat_young
    .balign 64
    .endr
.option pop
