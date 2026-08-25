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

// Supervisor address-translation cache invalidation instructions (Svinval 1.0).
class riscv_svinval_instr extends riscv_instr;

  `uvm_object_utils(riscv_svinval_instr)
  `uvm_object_new

  virtual function void set_rand_mode();
    super.set_rand_mode();
    has_imm = 1'b0;
    has_rd = 1'b0;
    if (instr_name inside {SFENCE_W_INVAL, SFENCE_INVAL_IR}) begin
      has_rs1 = 1'b0;
      has_rs2 = 1'b0;
    end
  endfunction : set_rand_mode

  virtual function string convert2asm(string prefix = "");
    string asm_str;
    case (instr_name)
      SINVAL_VMA:
        asm_str = $sformatf("sinval.vma %0s, %0s", rs1.name(), rs2.name());
      SFENCE_W_INVAL:
        asm_str = "sfence.w.inval";
      SFENCE_INVAL_IR:
        asm_str = "sfence.inval.ir";
      default:
        `uvm_fatal(`gfn, $sformatf("Unsupported Svinval instruction %0s", instr_name.name()))
    endcase
    return asm_str.tolower();
  endfunction : convert2asm

  virtual function string convert2bin(string prefix = "");
    bit [31:0] binary;
    case (instr_name)
      SINVAL_VMA:      binary = {7'b0001011, rs2, rs1, 3'b000, 5'b0, 7'b1110011};
      SFENCE_W_INVAL:  binary = 32'h1800_0073;
      SFENCE_INVAL_IR: binary = 32'h1810_0073;
      default:
        `uvm_fatal(`gfn, $sformatf("Unsupported Svinval instruction %0s", instr_name.name()))
    endcase
    return {prefix, $sformatf("%8h", binary)};
  endfunction : convert2bin

  virtual function bit is_supported(riscv_instr_gen_config cfg);
    return cfg.enable_svinval_extension && cfg.enable_sfence && support_sfence &&
           (SVINVAL inside {supported_isa});
  endfunction : is_supported

  virtual function void update_src_regs(string operands[$]);
    case (instr_name)
      SINVAL_VMA: begin
        `DV_CHECK_FATAL(operands.size() == 2, instr_name)
        rs1 = get_gpr(operands[0]);
        rs1_value = get_gpr_state(operands[0]);
        rs2 = get_gpr(operands[1]);
        rs2_value = get_gpr_state(operands[1]);
      end
      SFENCE_W_INVAL, SFENCE_INVAL_IR: begin
        `DV_CHECK_FATAL(operands.size() == 0, instr_name)
      end
      default: super.update_src_regs(operands);
    endcase
  endfunction : update_src_regs

endclass : riscv_svinval_instr

`define DEFINE_SVINVAL_INSTR(instr_n) \
  class riscv_``instr_n``_instr extends riscv_svinval_instr; \
    `INSTR_BODY(instr_n, R_FORMAT, SYNCH, SVINVAL)

`DEFINE_SVINVAL_INSTR(SINVAL_VMA)
`DEFINE_SVINVAL_INSTR(SFENCE_W_INVAL)
`DEFINE_SVINVAL_INSTR(SFENCE_INVAL_IR)

`undef DEFINE_SVINVAL_INSTR
