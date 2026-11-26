OUTPUT_ARCH(riscv)
ENTRY(boot_seed)
PHDRS { seed PT_LOAD FLAGS(6); }
SECTIONS {
    /* The IT ELF loader normalizes its first load address to RAM base. */
    . = 0x80000000;
    .ram_base : { BYTE(0) } :seed
    . = 0x80010000;
    .seed : { *(.data.boot_seed) } :seed
    . = 0x80011000;
    .tohost : { *(.tohost) } :seed
    /DISCARD/ : { *(.text*) *(.comment) *(.riscv.attributes) }
}
