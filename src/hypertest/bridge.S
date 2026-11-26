/* Copyright 2026
 * SPDX-License-Identifier: Apache-2.0
 * M-mode bridge. The directed stream saves all outer GPRs and switches to the
 * dedicated stack before calling this standard-ABI function.
 */
.section .text.dv_hypertest_bridge,"ax",@progbits
.option push
.option norvc
.option norelax
.balign 4
.globl dv_hypertest_call
.type dv_hypertest_call,@function
dv_hypertest_call:
    addi sp, sp, -512
    sd ra, 0(sp)
    sd s0, 8(sp)
    sd s1, 16(sp)
    sd s2, 24(sp)
    sd a0, 32(sp)
    mv s0, sp
    csrr t0, mstatus
    sd t0, 40(s0)
    csrci mstatus, 8
    /* MPRV must already be zero: the caller saved its frame physically. */
    li t1, (1 << 17)
    and t0, t0, t1
    bnez t0, dv_hypertest_fail

.macro DV_HYP_SAVE_CSR csr, offset
    csrr t0, \csr
    sd t0, \offset(s0)
.endm
.macro DV_HYP_RESTORE_CSR csr, offset
    ld t0, \offset(s0)
    csrw \csr, t0
.endm
    DV_HYP_SAVE_CSR mtvec, 48
    DV_HYP_SAVE_CSR mepc, 56
    DV_HYP_SAVE_CSR mcause, 64
    DV_HYP_SAVE_CSR mtval, 72
    DV_HYP_SAVE_CSR mscratch, 80
    DV_HYP_SAVE_CSR medeleg, 88
    DV_HYP_SAVE_CSR mideleg, 96
    DV_HYP_SAVE_CSR mie, 104
    DV_HYP_SAVE_CSR mcounteren, 112
    DV_HYP_SAVE_CSR menvcfg, 120
    DV_HYP_SAVE_CSR stvec, 128
    DV_HYP_SAVE_CSR sepc, 136
    DV_HYP_SAVE_CSR scause, 144
    DV_HYP_SAVE_CSR stval, 152
    DV_HYP_SAVE_CSR sscratch, 160
    DV_HYP_SAVE_CSR satp, 168
    DV_HYP_SAVE_CSR scounteren, 176
    DV_HYP_SAVE_CSR senvcfg, 184
    DV_HYP_SAVE_CSR mip, 248
    csrr s1, misa
    sd s1, 240(s0)
    /* Enable register-file access even when the outer state is Off. */
    li t0, 0x6600
    csrs mstatus, t0
    andi t0, s1, (1 << 5)
    beqz t0, .Ldv_hyp_save_vector
    DV_HYP_SAVE_CSR fcsr, 192
    andi t0, s1, (1 << 3)
    beqz t0, .Ldv_hyp_save_f32
.macro DV_HYP_SAVE_F64 n
    fsd f\n, (256 + 8 * \n)(s0)
.endm
.irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    DV_HYP_SAVE_F64 \n
.endr
    j .Ldv_hyp_save_vector
.Ldv_hyp_save_f32:
.macro DV_HYP_SAVE_F32 n
    fsw f\n, (256 + 8 * \n)(s0)
.endm
.irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    DV_HYP_SAVE_F32 \n
.endr
.Ldv_hyp_save_vector:
    li t0, (1 << 21)
    and t0, s1, t0
    beqz t0, .Ldv_hyp_enter_case
    DV_HYP_SAVE_CSR vtype, 200
    DV_HYP_SAVE_CSR vl, 208
    DV_HYP_SAVE_CSR vstart, 216
    DV_HYP_SAVE_CSR vcsr, 224
    csrr t1, vlenb
    sd t1, 232(s0)
    beqz t1, dv_hypertest_fail
    li t0, 8192
    bgtu t1, t0, dv_hypertest_fail
    slli t0, t1, 5
    sub sp, sp, t0
    mv s2, sp
    csrw vstart, zero
    mv t0, s2
.macro DV_HYP_SAVE_V n
    vs1r.v v\n, (t0)
    add t0, t0, t1
.endm
.irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    DV_HYP_SAVE_V \n
.endr
.Ldv_hyp_enter_case:
    ld a0, 32(s0)
.globl dv_hypertest_begin
dv_hypertest_begin:
    nop
    call dv_hypertest_run_case
    beqz a0, dv_hypertest_fail
.globl dv_hypertest_pass
dv_hypertest_pass:
    nop
    /* TEST_END leaves M-mode, translation bare and interrupts disabled. */
    csrci mstatus, 8
    li t0, (1 << 17)
    csrc mstatus, t0
    li t0, 0x6600
    csrs mstatus, t0
    ld s1, 240(s0)
    li t0, (1 << 21)
    and t0, s1, t0
    beqz t0, .Ldv_hyp_restore_float
    csrw vstart, zero
    ld t1, 232(s0)
    mv t0, s2
.macro DV_HYP_RESTORE_V n
    vl1re8.v v\n, (t0)
    add t0, t0, t1
.endm
.irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    DV_HYP_RESTORE_V \n
.endr
    ld t0, 208(s0)
    ld t1, 200(s0)
    vsetvl zero, t0, t1
    DV_HYP_RESTORE_CSR vcsr, 224
    DV_HYP_RESTORE_CSR vstart, 216
.Ldv_hyp_restore_float:
    andi t0, s1, (1 << 5)
    beqz t0, .Ldv_hyp_restore_csrs
    andi t0, s1, (1 << 3)
    beqz t0, .Ldv_hyp_restore_f32
.macro DV_HYP_RESTORE_F64 n
    fld f\n, (256 + 8 * \n)(s0)
.endm
.irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    DV_HYP_RESTORE_F64 \n
.endr
    j .Ldv_hyp_restore_fcsr
.Ldv_hyp_restore_f32:
.macro DV_HYP_RESTORE_F32 n
    flw f\n, (256 + 8 * \n)(s0)
.endm
.irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    DV_HYP_RESTORE_F32 \n
.endr
.Ldv_hyp_restore_fcsr:
    DV_HYP_RESTORE_CSR fcsr, 192
.Ldv_hyp_restore_csrs:
    DV_HYP_RESTORE_CSR menvcfg, 120
    DV_HYP_RESTORE_CSR mcounteren, 112
    DV_HYP_RESTORE_CSR scounteren, 176
    DV_HYP_RESTORE_CSR senvcfg, 184
    DV_HYP_RESTORE_CSR satp, 168
    sfence.vma
    fence.i
    DV_HYP_RESTORE_CSR stvec, 128
    DV_HYP_RESTORE_CSR sepc, 136
    DV_HYP_RESTORE_CSR scause, 144
    DV_HYP_RESTORE_CSR stval, 152
    DV_HYP_RESTORE_CSR sscratch, 160
    DV_HYP_RESTORE_CSR medeleg, 88
    DV_HYP_RESTORE_CSR mideleg, 96
    DV_HYP_RESTORE_CSR mepc, 56
    DV_HYP_RESTORE_CSR mcause, 64
    DV_HYP_RESTORE_CSR mtval, 72
    DV_HYP_RESTORE_CSR mscratch, 80
    DV_HYP_RESTORE_CSR mtvec, 48
    DV_HYP_RESTORE_CSR mip, 248
    DV_HYP_RESTORE_CSR mie, 104
    ld t0, 40(s0)
    ld ra, 0(s0)
    ld s1, 16(s0)
    ld s2, 24(s0)
    mv sp, s0
    ld s0, 8(sp)
    addi sp, sp, 512
    csrw mstatus, t0
    ret
.size dv_hypertest_call, .-dv_hypertest_call

.globl dv_hypertest_fail
.type dv_hypertest_fail,@function
dv_hypertest_fail:
    li t0, 3
    lla t1, tohost
    sd t0, 0(t1)
    fence rw, rw
    .4byte 0x00000000
    j dv_hypertest_fail
.size dv_hypertest_fail, .-dv_hypertest_fail
.option pop

.section .dv_hypertest_context,"aw",@progbits
.balign 8
.globl dv_hypertest_outer_frame
dv_hypertest_outer_frame:
    .dword 0

.section .dv_hypertest_stack,"aw",@nobits
.balign 4096
.globl dv_hypertest_stack_bottom
dv_hypertest_stack_bottom:
    .skip 1048576
.globl dv_hypertest_stack_top
dv_hypertest_stack_top:

.section .note.GNU-stack,"",@progbits
