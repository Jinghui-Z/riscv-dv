/*
 * Copyright 2026 Institute of Computing Technology, Chinese Academy of Sciences.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

// Vector configuration instructions.  The original VSET_FORMAT declarations
// did not have an implementation, so selecting CSR-category instructions could
// reach the generic converter and fail during assembly generation.
class riscv_vector_set_instr extends riscv_instr;

  rand bit [4:0] avl;
  rand bit [10:0] vtype_imm;

  constraint vtype_c {
    // vlmul is a signed three-bit encoding.  3'b100 is reserved; 3'b101,
    // 3'b110 and 3'b111 select mf8, mf4 and mf2 respectively.
    vtype_imm[2:0] inside {[0:3], [5:7]};
    vtype_imm[5:3] inside {[0:3]};
    if ((m_cfg != null) && !m_cfg.use_vector_1_0) {
      vtype_imm[7:6] == 2'b00;
    }
    vtype_imm[10:8] == 3'b000;
  }

  `uvm_object_utils(riscv_vector_set_instr)
  `uvm_object_new

  virtual function void set_rand_mode();
    has_rd = 1'b1;
    has_rs1 = (instr_name != VSETIVLI);
    has_rs2 = (instr_name == VSETVL);
    has_imm = 1'b0;
    avl.rand_mode(instr_name == VSETIVLI);
    vtype_imm.rand_mode(instr_name != VSETVL);
  endfunction : set_rand_mode

  function void pre_randomize();
    super.pre_randomize();
    // VSET* is kept in the CSR category for stream selection, but it does not
    // carry a CSR address operand.
    csr.rand_mode(1'b0);
  endfunction : pre_randomize

  virtual function bit is_supported(riscv_instr_gen_config cfg);
    if (!cfg.enable_vector_extension || !(RVV inside {supported_isa})) return 1'b0;
    if (cfg.use_vector_1_0 && !cfg.enable_random_vset_instr) return 1'b0;
    if (instr_name == VSETIVLI) return cfg.use_vector_1_0 && cfg.use_vsetivli;
    return 1'b1;
  endfunction : is_supported

  virtual function string convert2asm(string prefix = "");
    string asm_str;
    case (instr_name)
      VSETVL:
        asm_str = $sformatf("vsetvl %0s, %0s, %0s", rd.name(), rs1.name(), rs2.name());
      VSETVLI:
        asm_str = $sformatf("vsetvli %0s, %0s, %0s", rd.name(), rs1.name(), vtype_str());
      VSETIVLI:
        asm_str = $sformatf("vsetivli %0s, %0d, %0s", rd.name(), avl, vtype_str());
      default:
        `uvm_fatal(`gfn, $sformatf("Unsupported vector configuration instruction %0s",
                                   instr_name.name()))
    endcase
    if (comment != "") asm_str = {asm_str, " #", comment};
    return {prefix, asm_str.tolower()};
  endfunction : convert2asm

  virtual function string convert2bin(string prefix = "");
    bit [31:0] binary;
    case (instr_name)
      VSETVL:   binary = {7'b1000000, rs2, rs1, 3'b111, rd, 7'b1010111};
      VSETVLI:  binary = {1'b0, vtype_imm, rs1, 3'b111, rd, 7'b1010111};
      VSETIVLI: binary = {2'b11, vtype_imm[9:0], avl, 3'b111, rd, 7'b1010111};
      default:
        `uvm_fatal(`gfn, $sformatf("Unsupported vector configuration instruction %0s",
                                   instr_name.name()))
    endcase
    return {prefix, $sformatf("%8h", binary)};
  endfunction : convert2bin

  virtual function void update_src_regs(string operands[$]);
    `DV_CHECK_FATAL(operands.size() >= 3, instr_name)
    case (instr_name)
      VSETVL: begin
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
        rs2 = get_gpr(operands[2]);
        rs2_value = get_gpr_state(operands[2]);
      end
      VSETVLI: begin
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
        vtype_imm = binary[30:20];
      end
      VSETIVLI: begin
        avl = binary[19:15];
        vtype_imm = {1'b0, binary[29:20]};
      end
      default: ;
    endcase
  endfunction : update_src_regs

  protected function string vtype_str();
    int unsigned sew;
    string lmul;
    sew = 8 << vtype_imm[5:3];
    if (vtype_imm[2]) begin
      lmul = $sformatf("mf%0d", 1 << (8 - vtype_imm[2:0]));
    end else begin
      lmul = $sformatf("m%0d", 1 << vtype_imm[1:0]);
    end
    if ((m_cfg != null) && m_cfg.use_vector_1_0) begin
      return $sformatf("e%0d, %0s, %0s, %0s", sew, lmul,
                       vtype_imm[6] ? "ta" : "tu",
                       vtype_imm[7] ? "ma" : "mu");
    end
    return $sformatf("e%0d, %0s, d1", sew, lmul);
  endfunction : vtype_str

endclass : riscv_vector_set_instr

`DEFINE_VSET_INSTR(VSETVLI)
`DEFINE_VSET_INSTR(VSETIVLI)
`DEFINE_VSET_INSTR(VSETVL)
