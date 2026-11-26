# Hypertest C source adapter

`bridge.c` and `bridge.S` run original `bool case(void)` functions between DV
random instruction blocks. Keep the upstream C assertions and exception
handlers: an imported function returning false fails the generated ELF.

Link the selected case dependency closure, `src/rvh_test.c`, `src/instruction.c`,
the appropriate upstream page-table implementation, `asm/handlers.S`, and these
two bridge files into the DV ELF. Do not link upstream `boot.S`, `main.c`,
`test_register.c`, platform syscalls, or the upstream linker script.

Compile **only upstream `src/rvh_test.c`** with:

```text
-Dreset_state=dv_hyp_legacy_reset_state
-Dgoto_priv=dv_hyp_legacy_goto_priv
```

Compile the bridge and cases without these renames. All C objects use the same
RV64 ISA/LP64 ABI, `-mcmodel=medany -msmall-data-limit=0`, `-ffreestanding
-fno-builtin -ffunction-sections -fdata-sections -fno-tree-vectorize
-fno-tree-slp-vectorize -DLOG_LEVEL=0`. The silent printing functions do not
implement formatted output; source cases requiring observable printf output
are ineligible for this adapter. Compile assembly with F, D, and Vector 1.0
assembler support; the bridge accesses each register file only when misa
advertises it.

The caller must enter in M-mode with MPRV=0, save all outer GPRs in its own
known data/stack frame, store that frame address in `dv_hypertest_outer_frame`,
set architectural `sp` to `dv_hypertest_stack_top`, put the selected function
address in `a0`, and call `dv_hypertest_call`. On return it restores the outer
frame and GPRs. The C adapter has a separate 1 MiB NOBITS stack. The linker must
collect `.bss.*`, `.data.*`, `.sdata.*`, `.rodata.*`, `.dv_hypertest_context`, and
COMMON without overlapping the original cases' fixed physical scratch ranges.
Its `__dv_hypertest_bss_start/end` symbols bound C zero-initialized storage,
cleared on every case entry. Keep the NOBITS `.dv_hypertest_stack` and the
outer-frame context outside those bounds so the active bridge frame survives.

The bridge preserves the M/S control CSRs it touches, all FP registers/FCSR,
and all vector registers plus VL, VTYPE, VSTART, and VCSR. Whole-register vector
save/restore uses runtime VLENB, supporting architectural VLEN through 65536
bits. It resets the harness state and checks the original return value.
`dv_hypertest_begin` marks actual entry; `dv_hypertest_pass` executes only after
a successful boolean return. The runner must pair their PCs and reject missing
returns, unexpected traps outside the case interval, or `dv_hypertest_fail`.
Any `exit`, `abort`, assertion runtime failure, or failed boolean result writes
the failure value 3 to DV's tohost and reaches an illegal instruction.

The replacement `reset_state` installs the original M/S handlers and resets
ordinary M/S state. It omits unconditional optional PMP/Smrnmi/H CSR accesses.
HS/HU mean ordinary S/U on the current NanHu target. Entering VS/VU without
misa.H fails. Cases requiring H, PMP, Smrnmi, custom device behavior, or external
injection must be admitted explicitly by the target requirements manifest;
successful compilation is insufficient evidence. The upstream private H-state
flag is not initialized by this MSU adapter, so H cases remain unsupported.

Self-locking PMP, reset, terminal, device/no-response, self-modifying-code, and
unknown state effects cannot enter the default random pool. The wrapper does
not restore locked PMP entries, arbitrary custom CSRs, modified case data, or
written instruction bytes. Cases need individual eligibility and store-policy
review, and repeat execution must initialize its own mutable fixtures. Do not
mark imported C cases as `restrict_store_to_data` safe without checking their
actual writes. Retain their catalog entries and explicit unsupported reasons.

The upstream assembly handlers allocate 248 bytes before calling C. In the
vendored integration, reserve 256 bytes instead to preserve the 16-byte RISC-V
ABI alignment; existing register slot offsets can remain unchanged.

`bridge_selftest.c` is a separate synthetic validation driver, not an upstream
case migration. In a dedicated validation ELF, set `sp` to
`dv_hypertest_selftest_stack_top`, call `dv_hypertest_bridge_selftest`, and fail
unless it returns true. It invokes a deliberate state-clobbering function
through the bridge twice and compares all saved M/S CSR readbacks, all 32 FP
register bit patterns, all 32 vector register byte patterns, and vector control
state. It requires F/D/V and no asynchronous external events. The driver sets
its own test state, so it should finish the validation ELF rather than resume
an unrelated random program. Place `.dv_hypertest_selftest_data` and the NOBITS
`.dv_hypertest_selftest_stack` outside the case BSS clearing bounds.
