OUTPUT_ARCH(riscv)
ENTRY(boot_boundary_entry)
PHDRS { flash PT_LOAD FLAGS(5); seed PT_LOAD FLAGS(6); }
SECTIONS {
    . = DEFINED(BOUNDARY_FLASH_BASE) ? BOUNDARY_FLASH_BASE : 0x10000000;
    .text : { *(.text.boot) } :flash
    ASSERT(SIZEOF(.text) <= 0x8000, "flash exceeds 32KiB responder")
    . = 0x80010000;
    .seed : { *(.data.boot_seed) } :seed
    . = 0x80011000;
    .tohost : { *(.tohost) } :seed
    /DISCARD/ : { *(.comment) *(.riscv.attributes) }
}
