/*
 * Copyright 2018 Google LLC
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

// RISC-V privileged register class
class riscv_privil_reg extends riscv_reg#(privileged_reg_t);

  `uvm_object_utils(riscv_privil_reg)

  function new(string name = "");
    super.new(name);
  endfunction

  function void init_reg(REG_T reg_name);
    super.init_reg(reg_name);
    case(reg_name) inside
      /////////////// Machine mode reigster //////////////
      // Machine ISA Register
      MISA: begin
        privil_level = M_LEVEL;
        add_field("WARL0", 26, WARL);
        add_field("WLRL", XLEN-28, WLRL);
        add_field("MXL", 2, WARL);
      end
      // Machine Vendor ID Register
      MVENDORID: begin
        privil_level = M_LEVEL;
        add_field("OFFSET", 7, WPRI);
        add_field("BANK", XLEN-7, WPRI);
      end
      // Machine Architecture ID Register
      MARCHID: begin
        privil_level = M_LEVEL;
        add_field("ARCHITECTURE_ID", XLEN, WPRI);
      end
      // Machine Implementation ID Register
      MIMPID: begin
        privil_level = M_LEVEL;
        add_field("IMPLEMENTATION", XLEN, WPRI);
      end
      // Hart ID Register
      MHARTID: begin
        privil_level = M_LEVEL;
        add_field("HART_ID", XLEN, WPRI);
      end
      // Machine configuration pointer. A zero value indicates that no
      // configuration data structure is provided.
      MCONFIGPTR: begin
        privil_level = M_LEVEL;
        add_field("CONFIG_PTR", XLEN, WPRI);
      end
      // Machine Status Register
      MSTATUS: begin
        privil_level = M_LEVEL;
        add_field("UIE",   1,  WARL);
        add_field("SIE",   1,  WARL);
        add_field("WPRI0", 1,  WPRI);
        add_field("MIE",   1,  WARL);
        add_field("UPIE",  1,  WARL);
        add_field("SPIE",  1,  WARL);
        add_field("WPRI1", 1,  WPRI);
        add_field("MPIE",  1,  WARL);
        add_field("SPP",   1,  WLRL);
        add_field("VS",    2,  WARL);
        add_field("MPP",   2,  WLRL);
        add_field("FS",    2,  WARL);
        add_field("XS",    2,  WARL);
        add_field("MPRV",  1,  WARL);
        add_field("SUM",   1,  WARL);
        add_field("MXR",   1,  WARL);
        add_field("TVM",   1,  WARL);
        add_field("TW",    1,  WARL);
        add_field("TSR",   1,  WARL);
        if(XLEN == 32) begin
          add_field("WPRI3", 8,  WPRI);
        end else begin
          add_field("WPRI3", 9,  WPRI);
          add_field("UXL",   2,  WARL);
          add_field("SXL",   2,  WARL);
          add_field("WPRI4", XLEN - 37, WPRI);
        end
        add_field("SD",   1,  WARL);
      end
      // Machine Trap-Vector Base-Address Register
      MTVEC: begin
        privil_level = M_LEVEL;
        add_field("MODE",  2,  WARL);
        add_field("BASE",  XLEN - 2,  WARL);
      end
      // Machine Exception Delegation Register
      MEDELEG: begin
        privil_level = M_LEVEL;
        add_field("IAM", 1, WARL);
        add_field("IAF", 1, WARL);
        add_field("ILGL", 1, WARL);
        add_field("BREAK", 1, WARL);
        add_field("LAM", 1, WARL);
        add_field("LAF", 1, WARL);
        add_field("SAM", 1, WARL);
        add_field("SAF", 1, WARL);
        add_field("ECFU", 1, WARL);
        add_field("ECFS", 1, WARL);
        add_field("WARL0", 1, WARL);
        add_field("ECFM", 1, WARL);
        add_field("IPF", 1, WARL);
        add_field("LPF", 1, WARL);
        add_field("WARL1", 1, WARL);
        add_field("SPF", 1, WARL);
        add_field("WARL2", XLEN-16, WARL);
      end
      // Machine Interrupt Delegation Register
      MIDELEG: begin
        privil_level = M_LEVEL;
        add_field("USIP", 1, WARL);
        add_field("SSIP", 1, WARL);
        add_field("WARL0", 1, WARL);
        add_field("MSIP", 1, WARL);
        add_field("UTIP", 1, WARL);
        add_field("STIP", 1, WARL);
        add_field("WARL1", 1, WARL);
        add_field("MTIP", 1, WARL);
        add_field("UEIP", 1, WARL);
        add_field("SEIP", 1, WARL);
        add_field("WARL2", 1, WARL);
        add_field("MEIP", 1, WARL);
        add_field("WPRI3", 1, WPRI);
        add_field("LCOFI", 1, WARL);
        add_field("WARL3", XLEN-14, WARL);
      end
      // Machine trap-enable register
      MIP: begin
        privil_level = M_LEVEL;
        add_field("USIP",   1,  WARL);
        add_field("SSIP",   1,  WARL);
        add_field("WPRI0",  1,  WPRI);
        add_field("MSIP",   1,  WARL);
        add_field("UTIP",   1,  WARL);
        add_field("STIP",   1,  WARL);
        add_field("WPRI1",  1,  WPRI);
        add_field("MTIP",   1,  WARL);
        add_field("UEIP",   1,  WARL);
        add_field("SEIP",   1,  WARL);
        add_field("WPRI2",  1,  WPRI);
        add_field("MEIP",   1,  WARL);
        add_field("WPRI3",  1,  WPRI);
        add_field("LCOFIP", 1,  WARL);
        add_field("WPRI4",  XLEN - 14,  WPRI);
      end
      // Machine interrupt-enable register
      MIE: begin
        privil_level = M_LEVEL;
        add_field("USIE",   1,  WARL);
        add_field("SSIE",   1,  WARL);
        add_field("WPRI0",  1,  WPRI);
        add_field("MSIE",   1,  WARL);
        add_field("UTIE",   1,  WARL);
        add_field("STIE",   1,  WARL);
        add_field("WPRI1",  1,  WPRI);
        add_field("MTIE",   1,  WARL);
        add_field("UEIE",   1,  WARL);
        add_field("SEIE",   1,  WARL);
        add_field("WPRI2",  1,  WPRI);
        add_field("MEIE",   1,  WARL);
        add_field("WPRI3",  1,  WPRI);
        add_field("LCOFIE", 1,  WARL);
        add_field("WPRI4",  XLEN - 14,  WPRI);
      end
      // Cycle Count Register
      MCYCLE: begin
        privil_level = M_LEVEL;
        add_field("MCYCLE", XLEN, WARL);
      end
      // Instruction Count Register
      MINSTRET: begin
        privil_level = M_LEVEL;
        add_field("MINSTRET", XLEN, WARL);
      end
      // Cycle Count Register - RV32I only
      MCYCLEH: begin
        privil_level = M_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "CSR MCYCLEH only exists in RV32.")
        end
        add_field("MCYCLEH", XLEN, WARL);
      end
      // Instruction Count Register - RV32I only
      MINSTRETH: begin
        privil_level = M_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "CSR MINSTRETH only exists in RV32.")
        end
        add_field("MINSTRETH", XLEN, WARL);
      end
      // Hardware Performance Monitor Counters
      [MHPMCOUNTER3:MHPMCOUNTER31]: begin
        privil_level = M_LEVEL;
        add_field($sformatf("%s", reg_name.name()), XLEN, WARL);
      end
      // Hardware Performance Monitor Events. Sscofpmf adds mode filters and
      // the sticky overflow bit in the most-significant six bits.
      [MHPMEVENT3:MHPMEVENT31]: begin
        privil_level = M_LEVEL;
        if (XLEN == 64) begin
          add_field("EVENT", 56, WARL);
          add_field("WPRI",  2, WPRI);
          add_field("VUINH", 1, WARL);
          add_field("VSINH", 1, WARL);
          add_field("UINH",  1, WARL);
          add_field("SINH",  1, WARL);
          add_field("MINH",  1, WARL);
          add_field("OF",    1, WARL);
        end else begin
          add_field("EVENT", XLEN, WARL);
        end
      end
      // Upper half of the 64-bit event selector, provided by Sscofpmf on RV32.
      [MHPMEVENT3H:MHPMEVENT31H]: begin
        privil_level = M_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, $sformatf("CSR %0s only exists in RV32.", reg_name.name()))
        end
        add_field("EVENTH", 24, WARL);
        add_field("WPRI",    2, WPRI);
        add_field("VUINH",   1, WARL);
        add_field("VSINH",   1, WARL);
        add_field("UINH",    1, WARL);
        add_field("SINH",    1, WARL);
        add_field("MINH",    1, WARL);
        add_field("OF",      1, WARL);
      end
      MCOUNTINHIBIT: begin
        privil_level = M_LEVEL;
        add_field("CY", 1, WARL);
        add_field("WPRI0", 1, WPRI);
        add_field("IR", 1, WARL);
        add_field("HPM", 29, WARL);
        if (XLEN == 64) add_field("WPRI1", 32, WPRI);
      end
      // Hardware Performance Monitor Counters - RV32I only
      [MHPMCOUNTER3H:MHPMCOUNTER31H]: begin
        if(XLEN != 32) begin
          `uvm_fatal(get_full_name(), $sformatf("Register %s is only in RV32I", reg_name.name()))
        end
        privil_level = M_LEVEL;
        add_field($sformatf("%s", reg_name.name()), 32, WARL);
      end
      // Machine Counter Enable Register
      MCOUNTEREN: begin
        privil_level = M_LEVEL;
        add_field("CY", 1, WARL);
        add_field("TM", 1, WARL);
        add_field("IR", 1, WARL);
        add_field("HPM3", 1, WARL);
        add_field("HPM4", 1, WARL);
        add_field("HPM5", 1, WARL);
        add_field("HPM6", 1, WARL);
        add_field("HPM7", 1, WARL);
        add_field("HPM8", 1, WARL);
        add_field("HPM9", 1, WARL);
        add_field("HPM10", 1, WARL);
        add_field("HPM11", 1, WARL);
        add_field("HPM12", 1, WARL);
        add_field("HPM13", 1, WARL);
        add_field("HPM14", 1, WARL);
        add_field("HPM15", 1, WARL);
        add_field("HPM16", 1, WARL);
        add_field("HPM17", 1, WARL);
        add_field("HPM18", 1, WARL);
        add_field("HPM19", 1, WARL);
        add_field("HPM20", 1, WARL);
        add_field("HPM21", 1, WARL);
        add_field("HPM22", 1, WARL);
        add_field("HPM23", 1, WARL);
        add_field("HPM24", 1, WARL);
        add_field("HPM25", 1, WARL);
        add_field("HPM26", 1, WARL);
        add_field("HPM27", 1, WARL);
        add_field("HPM28", 1, WARL);
        add_field("HPM29", 1, WARL);
        add_field("HPM30", 1, WARL);
        add_field("HPM31", 1, WARL);
        if(XLEN == 64) begin
          add_field("WPRI",  32,  WPRI);
        end
      end
      // Machine Scratch Register
      MSCRATCH: begin
        privil_level = M_LEVEL;
        add_field("MSCRATCH", XLEN, WARL);
      end
      // Machine Exception Program Counter
      MEPC: begin
        privil_level = M_LEVEL;
        add_field("BASE",  XLEN,  WARL);
      end
      // Machine Cause Register
      MCAUSE: begin
        privil_level = M_LEVEL;
        add_field("CODE",  4,  WLRL);
        add_field("WLRL", XLEN-5, WLRL);
        add_field("INTERRUPT",  1,  WARL);
      end
      // Machine Trap Value
      MTVAL: begin
        privil_level = M_LEVEL;
        add_field("VALUE",  XLEN,  WARL);
      end
      // Machine security configuration
      MSECCFG: begin
        privil_level = M_LEVEL;
        add_field("MML", 1, WARL); // RW1S
        add_field("MMWP", 1, WARL); // RW1S
        add_field("RLB", 1, WARL); // RW0C
      end
      // Machine security configuration, RV32 only
      MSECCFGH: begin
        privil_level = M_LEVEL;
        if(XLEN!=32) begin
          `uvm_fatal(`gfn, "CSR MSECCFGH only exists in RV32.")
        end
      end
      // Machine environment configuration (Privileged Architecture 1.13).
      MENVCFG: begin
        privil_level = M_LEVEL;
        add_field("FIOM", 1, WARL);
        add_field("WPRI0", 3, WPRI);
        add_field("CBIE", 2, WARL);
        add_field("CBCFE", 1, WARL);
        add_field("CBZE", 1, WARL);
        if (XLEN == 64) begin
          add_field("WPRI1", 53, WPRI);
`ifdef RISCV_DV_ADUE_READ_ONLY_ZERO
          add_field("ADUE", 1, WPRI);
`else
          add_field("ADUE", 1, WARL);
`endif
          add_field("PBMTE", 1, WARL);
          add_field("STCE", 1, WARL);
        end else begin
          add_field("WPRI1", 24, WPRI);
        end
      end
      MENVCFGH: begin
        privil_level = M_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "CSR MENVCFGH only exists in RV32.")
        end
        add_field("WPRI0", 29, WPRI);
`ifdef RISCV_DV_ADUE_READ_ONLY_ZERO
        add_field("ADUE", 1, WPRI);
`else
        add_field("ADUE", 1, WARL);
`endif
        add_field("PBMTE", 1, WARL);
        add_field("STCE", 1, WARL);
      end
      // Smstateen/Ssstateen. Registers 1-3 reserve bits 62:0; their SE bit
      // controls access to the corresponding lower-privilege stateen CSR.
      MSTATEEN0: begin
        privil_level = M_LEVEL;
        add_field("C", 1, WARL);
        add_field("FCSR", 1, WARL);
        add_field("JVT", 1, WARL);
        if (XLEN == 64) begin
          add_field("WPRI0", 53, WPRI);
          add_field("P1P13", 1, WARL);
          add_field("CONTEXT", 1, WARL);
          add_field("IMSIC", 1, WARL);
          add_field("AIA", 1, WARL);
          add_field("CSRIND", 1, WARL);
          add_field("WPRI1", 1, WPRI);
          add_field("ENVCFG", 1, WARL);
          add_field("SE0", 1, WARL);
        end else begin
          add_field("WPRI0", 29, WPRI);
        end
      end
      MSTATEEN1, MSTATEEN2, MSTATEEN3: begin
        privil_level = M_LEVEL;
        if (XLEN == 64) begin
          add_field("WPRI", 63, WPRI);
          add_field("SE", 1, WARL);
        end else begin
          add_field("WPRI", XLEN, WPRI);
        end
      end
      MSTATEEN0H: begin
        privil_level = M_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "CSR MSTATEEN0H only exists in RV32.")
        end
        add_field("WPRI0", 24, WPRI);
        add_field("P1P13", 1, WARL);
        add_field("CONTEXT", 1, WARL);
        add_field("IMSIC", 1, WARL);
        add_field("AIA", 1, WARL);
        add_field("CSRIND", 1, WARL);
        add_field("WPRI1", 1, WPRI);
        add_field("ENVCFG", 1, WARL);
        add_field("SE0", 1, WARL);
      end
      MSTATEEN1H, MSTATEEN2H, MSTATEEN3H: begin
        privil_level = M_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "High-half state-enable CSRs only exist in RV32.")
        end
        add_field("WPRI", 31, WPRI);
        add_field("SE", 1, WARL);
      end
      // Physical Memory Protection Configuration Register
      PMPCFG0: begin
        privil_level = M_LEVEL;
        add_field("PMP0CFG", 8, WARL);
        add_field("PMP1CFG", 8, WARL);
        add_field("PMP2CFG", 8, WARL);
        add_field("PMP3CFG", 8, WARL);
        if(XLEN==64) begin
          add_field("PMP4CFG", 8, WARL);
          add_field("PMP5CFG", 8, WARL);
          add_field("PMP6CFG", 8, WARL);
          add_field("PMP7CFG", 8, WARL);
        end
      end
      // Physical Memory Protection Configuration Register
      PMPCFG1: begin
        privil_level = M_LEVEL;
        if(XLEN!=32) begin
          `uvm_fatal(`gfn, "CSR PMPCFG1 only exists in RV32.")
        end else begin
          add_field("PMP4CFG", 8, WARL);
          add_field("PMP5CFG", 8, WARL);
          add_field("PMP6CFG", 8, WARL);
          add_field("PMP7CFG", 8, WARL);
        end
      end
      // Physical Memory Protection Configuration Register
      PMPCFG2: begin
        privil_level = M_LEVEL;
        add_field("PMP8CFG", 8, WARL);
        add_field("PMP9CFG", 8, WARL);
        add_field("PMP10CFG", 8, WARL);
        add_field("PMP11CFG", 8, WARL);
        if(XLEN==64) begin
          add_field("PMP12CFG", 8, WARL);
          add_field("PMP13CFG", 8, WARL);
          add_field("PMP14CFG", 8, WARL);
          add_field("PMP15CFG", 8, WARL);
        end
      end
      // Physical Memory Protection Configuration Register
      PMPCFG3: begin
        if(XLEN!=32) begin
          `uvm_fatal(get_full_name(), "CSR PMPCFG3 only exists in RV32.")
        end
        privil_level = M_LEVEL;
        add_field("PMP12CFG", 8, WARL);
        add_field("PMP13CFG", 8, WARL);
        add_field("PMP14CFG", 8, WARL);
        add_field("PMP15CFG", 8, WARL);
      end
      // Physical Memory Protection Configuration Registers
      [PMPADDR0:PMPADDR15]: begin
        privil_level = M_LEVEL;
        if(XLEN==64) begin
          add_field("ADDRESS", 54, WARL);
          add_field("WARL", 10, WARL);
        end else begin
          add_field("ADDRESS", 32, WARL);
        end
      end
      /////////////// Supervisor mode reigster //////////////
      // Supervisor status register
      SSTATUS: begin
        privil_level = S_LEVEL;
        add_field("UIE",   1,  WARL);
        add_field("SIE",   1,  WARL);
        add_field("WPRI0", 2,  WPRI);
        add_field("UPIE",  1,  WARL);
        add_field("SPIE",  1,  WARL);
        add_field("WPRI1", 2,  WPRI);
        add_field("SPP",   1,  WLRL);
        add_field("VS",    2,  WARL);
        add_field("WPRI2", 2,  WPRI);
        add_field("FS",    2,  WARL);
        add_field("XS",    2,  WARL);
        add_field("WPRI3", 1,  WPRI);
        add_field("SUM",   1,  WARL);
        add_field("MXR",   1,  WARL);
        if(XLEN == 32) begin
          add_field("WPRI4", 11,  WPRI);
        end else begin
          add_field("WPRI4", 12,  WPRI);
          add_field("UXL",   2,  WARL);
          add_field("WPRI4", XLEN - 35, WPRI);
        end
        add_field("SD",   1,  WARL);
      end
      // Supervisor Trap Vector Base Address Register
      STVEC: begin
        privil_level = S_LEVEL;
        add_field("MODE", 2, WARL);
        add_field("BASE", XLEN-2, WLRL);
      end
      // Supervisor Exception Delegation Register
      SEDELEG: begin
        privil_level = S_LEVEL;
        add_field("IAM", 1, WARL);
        add_field("IAF", 1, WARL);
        add_field("II", 1, WARL);
        add_field("WPRI0", 1, WPRI);
        add_field("LAM", 1, WARL);
        add_field("LAF", 1, WARL);
        add_field("SAM", 1, WARL);
        add_field("SAF", 1, WARL);
        add_field("ECFU", 1, WARL);
        add_field("WPRI1", 1, WPRI);
        add_field("WARL0", 1, WARL);
        add_field("WPRI2", 1, WPRI);
        add_field("IPF", 1, WARL);
        add_field("LPF", 1, WARL);
        add_field("WARL1", 1, WARL);
        add_field("SPF", 1, WARL);
        add_field("WARL2", XLEN-16, WARL);
      end
      // Supervisor Interrupt Delegation Register
      SIDELEG: begin
        privil_level = S_LEVEL;
        add_field("USIP", 1, WARL);
        add_field("SSIP", 1, WARL);
        add_field("WARL0", 1, WARL);
        add_field("WPRI0", 1, WPRI);
        add_field("UTIP", 1, WARL);
        add_field("STIP", 1, WARL);
        add_field("WARL1", 1, WARL);
        add_field("WPRI1", 1, WPRI);
        add_field("UEIP", 1, WARL);
        add_field("SEIP", 1, WARL);
        add_field("WARL2", 1, WARL);
        add_field("WPRI2", 1, WPRI);
        add_field("WARL3", XLEN-12, WARL);
      end
      // Supervisor trap-enable register
      SIP: begin
        privil_level = S_LEVEL;
        add_field("USIP",   1,  WARL);
        add_field("SSIP",   1,  WARL);
        add_field("WPRI0",  2,  WPRI);
        add_field("UTIP",   1,  WARL);
        add_field("STIP",   1,  WARL);
        add_field("WPRI1",  2,  WPRI);
        add_field("UEIP",   1,  WARL);
        add_field("SEIP",   1,  WARL);
        add_field("WPRI2", 3, WPRI);
        add_field("LCOFIP", 1, WARL);
        add_field("WPRI3",  XLEN - 14,  WPRI);
      end
      // Supervisor interrupt-enable register
      SIE: begin
        privil_level = S_LEVEL;
        add_field("USIE",   1,  WARL);
        add_field("SSIE",   1,  WARL);
        add_field("WPRI0",  2,  WPRI);
        add_field("UTIE",   1,  WARL);
        add_field("STIE",   1,  WARL);
        add_field("WPRI1",  2,  WPRI);
        add_field("UEIE",   1,  WARL);
        add_field("SEIE",   1,  WARL);
        add_field("WPRI2", 3, WPRI);
        add_field("LCOFIE", 1, WARL);
        add_field("WPRI3", XLEN - 14, WPRI);
      end
      // Supervisor Counter Enable Register
      SCOUNTEREN: begin
        privil_level = S_LEVEL;
        add_field("CY", 1, WARL);
        add_field("TM", 1, WARL);
        add_field("IR", 1, WARL);
        add_field("HPM3", 1, WARL);
        add_field("HPM4", 1, WARL);
        add_field("HPM5", 1, WARL);
        add_field("HPM6", 1, WARL);
        add_field("HPM7", 1, WARL);
        add_field("HPM8", 1, WARL);
        add_field("HPM9", 1, WARL);
        add_field("HPM10", 1, WARL);
        add_field("HPM11", 1, WARL);
        add_field("HPM12", 1, WARL);
        add_field("HPM13", 1, WARL);
        add_field("HPM14", 1, WARL);
        add_field("HPM15", 1, WARL);
        add_field("HPM16", 1, WARL);
        add_field("HPM17", 1, WARL);
        add_field("HPM18", 1, WARL);
        add_field("HPM19", 1, WARL);
        add_field("HPM20", 1, WARL);
        add_field("HPM21", 1, WARL);
        add_field("HPM22", 1, WARL);
        add_field("HPM23", 1, WARL);
        add_field("HPM24", 1, WARL);
        add_field("HPM25", 1, WARL);
        add_field("HPM26", 1, WARL);
        add_field("HPM27", 1, WARL);
        add_field("HPM28", 1, WARL);
        add_field("HPM29", 1, WARL);
        add_field("HPM30", 1, WARL);
        add_field("HPM31", 1, WARL);
        if(XLEN == 64) begin
          add_field("WPRI",  32,  WPRI);
        end
      end
      SENVCFG: begin
        privil_level = S_LEVEL;
        add_field("FIOM", 1, WARL);
        add_field("WPRI0", 3, WPRI);
        add_field("CBIE", 2, WARL);
        add_field("CBCFE", 1, WARL);
        add_field("CBZE", 1, WARL);
        add_field("WPRI1", XLEN - 8, WPRI);
      end
      SSTATEEN0: begin
        privil_level = S_LEVEL;
        add_field("C", 1, WARL);
        add_field("FCSR", 1, WARL);
        add_field("JVT", 1, WARL);
        add_field("WPRI", XLEN - 3, WPRI);
      end
      SSTATEEN1, SSTATEEN2, SSTATEEN3: begin
        privil_level = S_LEVEL;
        add_field("WPRI", XLEN, WPRI);
      end
      STIMECMP: begin
        privil_level = S_LEVEL;
        add_field("TIMECMP", XLEN, WARL);
      end
      STIMECMPH: begin
        privil_level = S_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "CSR STIMECMPH only exists in RV32.")
        end
        add_field("TIMECMPH", XLEN, WARL);
      end
      SCOUNTOVF: begin
        privil_level = S_LEVEL;
        add_field("WPRI0", 3, WPRI);
        // scountovf is a read-only shadow of mhpmevent3-31.OF.  The lightweight
        // register model has no read-only field type, so do not randomize it as
        // writable architectural state.
        add_field("OFVEC", 29, WPRI);
        if (XLEN == 64) add_field("WPRI1", 32, WPRI);
      end
      // Supervisor Scratch Register
      SSCRATCH: begin
        privil_level = S_LEVEL;
        add_field("SSCRATCH", XLEN, WARL);
      end
      // Supervisor Exception Program Counter
      SEPC: begin
        privil_level = S_LEVEL;
        add_field("BASE",  XLEN,  WARL);
      end
      // Supervisor Cause Register
      SCAUSE: begin
        privil_level = S_LEVEL;
        add_field("CODE",  4,  WLRL);
        add_field("WLRL", XLEN-5, WLRL);
        add_field("INTERRUPT",  1,  WARL);
      end
      // Supervisor Trap Value
      STVAL: begin
        privil_level = S_LEVEL;
        add_field("VALUE",  XLEN,  WARL);
      end
      // Supervisor Address Translation and Protection
      SATP: begin
        privil_level = S_LEVEL;
        if(XLEN == 32) begin
          add_field("PPN",  22, WARL);
          add_field("ASID", 9,  WARL);
          add_field("MODE", 1,  WARL);
        end else begin
          add_field("PPN",  44, WARL);
          add_field("ASID", 16, WARL);
          add_field("MODE", 4,  WARL);
        end
      end
      // Debug trigger CSRs (Sdtrig) and external debug CSRs (Sdext).
      TSELECT: begin
        privil_level = M_LEVEL;
        add_field("INDEX", XLEN, WARL);
      end
      TDATA1, TDATA2, TDATA3: begin
        privil_level = M_LEVEL;
        add_field("DATA", XLEN, WARL);
      end
      TINFO: begin
        privil_level = M_LEVEL;
        add_field("INFO", XLEN, WPRI);
      end
      TCONTROL: begin
        privil_level = M_LEVEL;
        add_field("WPRI0", 3, WPRI);
        add_field("MTE", 1, WARL);
        add_field("WPRI1", 3, WPRI);
        add_field("MPTE", 1, WARL);
        add_field("WPRI2", XLEN - 8, WPRI);
      end
      MCONTEXT, MSCONTEXT: begin
        privil_level = M_LEVEL;
        add_field("CONTEXT", XLEN, WARL);
      end
      SCONTEXT: begin
        privil_level = S_LEVEL;
        add_field("CONTEXT", XLEN, WARL);
      end
      DCSR: begin
        privil_level = M_LEVEL;
        add_field("PRV", 2, WARL);
        add_field("STEP", 1, WARL);
        add_field("NMIP", 1, WPRI);
        add_field("MPRVEN", 1, WARL);
        add_field("V", 1, WPRI);
        add_field("CAUSE", 3, WPRI);
        add_field("STOPTIME", 1, WARL);
        add_field("STOPCOUNT", 1, WARL);
        add_field("STEPIE", 1, WARL);
        add_field("EBREAKU", 1, WARL);
        add_field("EBREAKS", 1, WARL);
        add_field("WPRI0", 1, WPRI);
        add_field("EBREAKM", 1, WARL);
        add_field("WPRI1", 12, WPRI);
        add_field("XDEBUGVER", 4, WPRI);
        if (XLEN == 64) add_field("WPRI2", 32, WPRI);
      end
      DPC, DSCRATCH0, DSCRATCH1: begin
        privil_level = M_LEVEL;
        add_field("DATA", XLEN, WARL);
      end
      /////////////// User mode reigster //////////////
      // User Status Register
      USTATUS: begin
        privil_level = U_LEVEL;
        add_field("UIE", 1, WARL);
        add_field("WPRI0", 3, WPRI);
        add_field("UPIE", 1, WARL);
        add_field("WPRI1", XLEN-5, WPRI);
      end
      // User Trap Vector Base Address Register
      UTVEC: begin
        privil_level = U_LEVEL;
        add_field("MODE", 2, WARL);
        add_field("BASE", XLEN-2, WLRL);
      end
      // User Interrupt-Enable register
      UIE: begin
        privil_level = U_LEVEL;
        add_field("USIE", 1, WARL);
        add_field("WPRI0", 3, WPRI);
        add_field("UTIE", 1, WARL);
        add_field("WPRI1", 3, WPRI);
        add_field("UEIE", 1, WARL);
        add_field("WPRI2", XLEN-9, WPRI);
      end
      // User Trap-Enable register
      UIP: begin
        privil_level = U_LEVEL;
        add_field("USIP", 1, WARL);
        add_field("WPRI0", 3, WPRI);
        add_field("UTIP", 1, WARL);
        add_field("WPRI1", 3, WPRI);
        add_field("UEIP", 1, WARL);
        add_field("WPRI2", XLEN-9, WPRI);
      end
      // User Scratch Register
      USCRATCH: begin
        privil_level = U_LEVEL;
        add_field("MSCRATCH", XLEN, WARL);
      end
      // User Exception Program Counter
      UEPC: begin
        privil_level = U_LEVEL;
        add_field("BASE",  XLEN,  WARL);
      end
      // User Cause Register
      UCAUSE: begin
        privil_level = U_LEVEL;
        add_field("CODE",  4,  WLRL);
        add_field("WLRL", XLEN-5, WLRL);
        add_field("INTERRUPT",  1,  WARL);
      end
      // User Trap Value
      UTVAL: begin
        privil_level = U_LEVEL;
        add_field("VALUE",  XLEN,  WARL);
      end
      // Unprivileged counters and architectural extension state.
      [CYCLE:HPMCOUNTER31]: begin
        privil_level = U_LEVEL;
        add_field($sformatf("%s", reg_name.name()), XLEN, WPRI);
      end
      [CYCLEH:HPMCOUNTER31H]: begin
        privil_level = U_LEVEL;
        if (XLEN != 32) begin
          `uvm_fatal(`gfn, "High-half counter CSRs only exist in RV32.")
        end
        add_field($sformatf("%s", reg_name.name()), XLEN, WPRI);
      end
      FFLAGS: begin
        privil_level = U_LEVEL;
        add_field("FFLAGS", 5, WARL);
        add_field("WPRI", XLEN - 5, WPRI);
      end
      FRM: begin
        privil_level = U_LEVEL;
        add_field("FRM", 3, WARL);
        add_field("WPRI", XLEN - 3, WPRI);
      end
      FCSR: begin
        privil_level = U_LEVEL;
        add_field("FFLAGS", 5, WARL);
        add_field("FRM", 3, WARL);
        add_field("WPRI", XLEN - 8, WPRI);
      end
      VSTART: begin
        privil_level = U_LEVEL;
        add_field("VSTART", XLEN, WARL);
      end
      VXSAT: begin
        privil_level = U_LEVEL;
        add_field("VXSAT", 1, WARL);
        add_field("WPRI", XLEN - 1, WPRI);
      end
      VXRM: begin
        privil_level = U_LEVEL;
        add_field("VXRM", 2, WARL);
        add_field("WPRI", XLEN - 2, WPRI);
      end
      VCSR: begin
        privil_level = U_LEVEL;
        add_field("VXSAT", 1, WARL);
        add_field("VXRM", 2, WARL);
        add_field("WPRI", XLEN - 3, WPRI);
      end
      VL, VTYPE: begin
        privil_level = U_LEVEL;
        add_field("VALUE", XLEN, WPRI);
      end
      VLENB: begin
        privil_level = U_LEVEL;
        add_field("VLENB", XLEN, WPRI);
      end
      default:
        `uvm_fatal(get_full_name(), $sformatf("reg %0s is not supported yet", reg_name.name()))
    endcase
  endfunction

endclass
