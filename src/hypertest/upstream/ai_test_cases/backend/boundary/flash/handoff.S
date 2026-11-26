.section .text,"ax",@progbits
.option norvc
.global boot_boundary_entry
boot_boundary_entry:
    /* The normal harness expects a0 to contain the hart ID, not reset GPR data. */
    csrr a0,mhartid
    li t0,0x80000000
    jr t0
