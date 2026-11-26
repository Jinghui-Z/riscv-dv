/* SPDX-License-Identifier: Apache-2.0
 * Synthetic bridge validation only; this is not an imported hypertest case.
 * The driver calls the same clobber case twice and checks state restoration.
 */
# HYPERTEST_CASE: dv_hypertest_selftest_clobber_case
# HYPERTEST_CASE: dv_hypertest_selftest_clobber_case

.section .text,"ax",@progbits
.option push
.option norvc
.option norelax
.balign 4
.globl _start
.type _start,@function
_start:
    csrw mie, zero
    li t0, 0x7e00
    csrw mstatus, t0
    lla t0, dv_hypertest_fail
    csrw mtvec, t0
    lla sp, dv_hypertest_selftest_stack_top
    call dv_hypertest_bridge_selftest
    beqz a0, dv_hypertest_fail
.globl dv_hypertest_program_pass
dv_hypertest_program_pass:
    li t0, 1
    lla t1, tohost
    sd t0, 0(t1)
    fence rw, rw
.Ldv_hypertest_selftest_done:
    j .Ldv_hypertest_selftest_done
.size _start, .-_start
.option pop

.section .tohost,"aw",@progbits
.balign 64
.globl tohost
tohost:
    .dword 0
.balign 64
.globl fromhost
fromhost:
    .dword 0
