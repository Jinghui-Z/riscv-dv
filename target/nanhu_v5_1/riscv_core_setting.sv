/*
 * Copyright 2026 Institute of Computing Technology, Chinese Academy of Sciences.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// NanHu V5.1 architectural profile derived from the V5.1 workbook in docs/.

// NanHu implements Svade rather than Svadu. menvcfg.ADUE is therefore a
// read-only zero bit for this target.
`define RISCV_DV_ADUE_READ_ONLY_ZERO

parameter int XLEN = 64;

// Sv39 is the normal target. Select another supported translation mode at
// generator compile time with +define+NANHU_V5_1_SV48 or
// +define+NANHU_V5_1_BARE.
`ifdef NANHU_V5_1_BARE
parameter satp_mode_t SATP_MODE = BARE;
`elsif NANHU_V5_1_SV48
parameter satp_mode_t SATP_MODE = SV48;
`else
parameter satp_mode_t SATP_MODE = SV39;
`endif

privileged_mode_t supported_privileged_mode[] = {
    USER_MODE, SUPERVISOR_MODE, MACHINE_MODE
};

riscv_instr_name_t unsupported_instr[] = {};

// Only ratified sub-extension groups are enabled. RV32 groups contain the
// encodings shared by RV32 and RV64 in the riscv-dv class hierarchy.
riscv_instr_group_t supported_isa[$] = {
    RV32I, RV64I,
    RV32M, RV64M,
    RV32A, RV64A,
    RV32F, RV64F,
    RV32D, RV64D,
    RV32C, RV64C, RV32DC,
    RVV,
    RV32ZBA, RV64ZBA,
    RV32ZBB, RV64ZBB,
    RV32ZBC, RV64ZBC,
    RV32ZBKC, RV64ZBKC,
    RV32ZBS, RV64ZBS,
    RV32ZCB, RV64ZCB,
    RV32ZICOND, RV64ZICOND,
    RV32ZIMOP, RV64ZIMOP,
    RV32ZCMOP, RV64ZCMOP,
    RV32ZICBOM, RV64ZICBOM,
    RV32ZICBOP, RV64ZICBOP,
    RV32ZICBOZ, RV64ZICBOZ,
    RV32ZBKB, RV64ZBKB,
    RV32ZBKX, RV64ZBKX,
    RV32ZKND, RV64ZKND,
    RV32ZKNE, RV64ZKNE,
    RV32ZKNH, RV64ZKNH,
    RV32ZKSED, RV64ZKSED,
    RV32ZKSH, RV64ZKSH,
    RV32ZKN, RV64ZKN,
    RV32ZKS, RV64ZKS,
    SVINVAL,
    ZVBB
};

mtvec_mode_t supported_interrupt_mode[$] = {DIRECT, VECTORED};
int max_interrupt_vector_num = 16;

// PMP/ePMP are not marked as supported in the V5.1 workbook.
bit support_pmp = 0;
bit support_epmp = 0;

bit support_debug_mode = 1;
bit support_umode_trap = 0;
bit support_sfence = 1;

// Zicclsm is marked supported for V5.1.
bit support_unaligned_load_store = 1'b1;

parameter int NUM_FLOAT_GPR = 32;
parameter int NUM_GPR = 32;
parameter int NUM_VEC_GPR = 32;

parameter int VECTOR_EXTENSION_ENABLE = 1;
parameter int VLEN = 128;
parameter int ELEN = 64;
parameter int SELEN = 8;
parameter int VELEN = int'($ln(ELEN)/$ln(2)) - 3;
parameter int MAX_LMUL = 8;

// This target describes one NanHu core. A multi-hart integration must raise
// NUM_HARTS in its target at compile time, then use the ACLINT/PLIC strides
// from the testlist template; the generated program arrays are compile-sized.
parameter int NUM_HARTS = 1;

`ifdef DSIM
privileged_reg_t implemented_csr[] = {
`else
const privileged_reg_t implemented_csr[] = {
`endif
    // Unprivileged floating-point/vector state. User-trap/N CSRs are omitted:
    // U-mode support does not imply the deprecated user interrupt extension.
    FFLAGS, FRM, FCSR,
    VSTART, VXSAT, VXRM, VCSR, VL, VTYPE, VLENB,

    // Zicntr/Zihpm user-visible counters.
    CYCLE, TIME, INSTRET,
    HPMCOUNTER3, HPMCOUNTER4, HPMCOUNTER5, HPMCOUNTER6,
    HPMCOUNTER7, HPMCOUNTER8, HPMCOUNTER9, HPMCOUNTER10,
    HPMCOUNTER11, HPMCOUNTER12, HPMCOUNTER13, HPMCOUNTER14,
    HPMCOUNTER15, HPMCOUNTER16, HPMCOUNTER17, HPMCOUNTER18,
    HPMCOUNTER19, HPMCOUNTER20, HPMCOUNTER21, HPMCOUNTER22,
    HPMCOUNTER23, HPMCOUNTER24, HPMCOUNTER25, HPMCOUNTER26,
    HPMCOUNTER27, HPMCOUNTER28, HPMCOUNTER29, HPMCOUNTER30,
    HPMCOUNTER31,

    // Supervisor architecture 1.13, Sstc, Sscofpmf, Ssstateen and Svpbmt.
    SSTATUS, SIE, STVEC, SCOUNTEREN, SENVCFG,
    SSTATEEN0, SSTATEEN1, SSTATEEN2, SSTATEEN3,
    SSCRATCH, SEPC, SCAUSE, STVAL, SIP, STIMECMP,
    SATP, SCOUNTOVF, SCONTEXT,

    // Machine architecture and Smstateen.
    MVENDORID, MARCHID, MIMPID, MHARTID, MCONFIGPTR,
    MSTATUS, MISA, MEDELEG, MIDELEG, MIE, MTVEC, MCOUNTEREN,
    MSTATEEN0, MSTATEEN1, MSTATEEN2, MSTATEEN3,
    MENVCFG, MSCRATCH, MEPC, MCAUSE, MTVAL, MIP,

    // Machine counters and Sscofpmf event selectors.
    MCYCLE, MINSTRET,
    MHPMCOUNTER3, MHPMCOUNTER4, MHPMCOUNTER5, MHPMCOUNTER6,
    MHPMCOUNTER7, MHPMCOUNTER8, MHPMCOUNTER9, MHPMCOUNTER10,
    MHPMCOUNTER11, MHPMCOUNTER12, MHPMCOUNTER13, MHPMCOUNTER14,
    MHPMCOUNTER15, MHPMCOUNTER16, MHPMCOUNTER17, MHPMCOUNTER18,
    MHPMCOUNTER19, MHPMCOUNTER20, MHPMCOUNTER21, MHPMCOUNTER22,
    MHPMCOUNTER23, MHPMCOUNTER24, MHPMCOUNTER25, MHPMCOUNTER26,
    MHPMCOUNTER27, MHPMCOUNTER28, MHPMCOUNTER29, MHPMCOUNTER30,
    MHPMCOUNTER31,
    MCOUNTINHIBIT,
    MHPMEVENT3, MHPMEVENT4, MHPMEVENT5, MHPMEVENT6,
    MHPMEVENT7, MHPMEVENT8, MHPMEVENT9, MHPMEVENT10,
    MHPMEVENT11, MHPMEVENT12, MHPMEVENT13, MHPMEVENT14,
    MHPMEVENT15, MHPMEVENT16, MHPMEVENT17, MHPMEVENT18,
    MHPMEVENT19, MHPMEVENT20, MHPMEVENT21, MHPMEVENT22,
    MHPMEVENT23, MHPMEVENT24, MHPMEVENT25, MHPMEVENT26,
    MHPMEVENT27, MHPMEVENT28, MHPMEVENT29, MHPMEVENT30,
    MHPMEVENT31,

    // Sdtrig and external debug CSRs.
    TSELECT, TDATA1, TDATA2, TDATA3, TINFO, TCONTROL,
    MCONTEXT, MSCONTEXT,
    DCSR, DPC, DSCRATCH0, DSCRATCH1
};

bit [11:0] custom_csr[] = {};

`ifdef DSIM
interrupt_cause_t implemented_interrupt[] = {
`else
const interrupt_cause_t implemented_interrupt[] = {
`endif
    S_SOFTWARE_INTR,
    M_SOFTWARE_INTR,
    S_TIMER_INTR,
    M_TIMER_INTR,
    S_EXTERNAL_INTR,
    M_EXTERNAL_INTR,
    LOCAL_COUNTER_OVERFLOW_INTR
};

`ifdef DSIM
exception_cause_t implemented_exception[] = {
`else
const exception_cause_t implemented_exception[] = {
`endif
    INSTRUCTION_ADDRESS_MISALIGNED,
    INSTRUCTION_ACCESS_FAULT,
    ILLEGAL_INSTRUCTION,
    BREAKPOINT,
    LOAD_ADDRESS_MISALIGNED,
    LOAD_ACCESS_FAULT,
    STORE_AMO_ADDRESS_MISALIGNED,
    STORE_AMO_ACCESS_FAULT,
    ECALL_UMODE,
    ECALL_SMODE,
    ECALL_MMODE,
    INSTRUCTION_PAGE_FAULT,
    LOAD_PAGE_FAULT,
    STORE_AMO_PAGE_FAULT
};
