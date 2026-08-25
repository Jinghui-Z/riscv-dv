/*
 * Copyright 2026
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

// Common implementation for the ratified scalar extensions which use the
// integer register file.  Keeping these encodings in one class avoids
// duplicating operand/trace handling while each instruction remains a normal
// riscv_instr factory object.
class riscv_scalar_ext_instr extends riscv_instr;
  `uvm_object_utils(riscv_scalar_ext_instr)

  constraint scalar_ext_imm_c {
    if (instr_name inside {PREFETCH_I, PREFETCH_R, PREFETCH_W}) {
      imm[4:0] == 0;
    }
    if (instr_name == AES64KS1I) {
      imm[3:0] inside {[0:10]};
    }
    if (instr_name == C_MOP) {
      imm[3:0] inside {1, 3, 5, 7, 9, 11, 13, 15};
    }
  }

  function new(string name = "");
    super.new(name);
  endfunction

  virtual function void set_rand_mode();
    super.set_rand_mode();
    case (instr_name)
      MOP_R: begin
        has_rs2 = 1'b0;
        has_imm = 1'b1;
      end
      MOP_RR,
      AES32DSI, AES32DSMI, AES32ESI, AES32ESMI,
      SM4ED, SM4KS: has_imm = 1'b1;
      C_MOP: begin
        is_compressed = 1'b1;
        has_rs1 = 1'b0;
        has_rs2 = 1'b0;
        has_rd = 1'b0;
        has_imm = 1'b1;
      end
      CBO_CLEAN, CBO_FLUSH, CBO_INVAL, CBO_ZERO: begin
        has_rs2 = 1'b0;
        has_rd = 1'b0;
        has_imm = 1'b0;
      end
      PREFETCH_I, PREFETCH_R, PREFETCH_W: begin
        has_rs2 = 1'b0;
        has_rd = 1'b0;
        has_imm = 1'b1;
      end
      BREV8, ZIP, UNZIP,
      AES64IM,
      SHA256SIG0, SHA256SIG1, SHA256SUM0, SHA256SUM1,
      SHA512SIG0, SHA512SIG1, SHA512SUM0, SHA512SUM1,
      SM3P0, SM3P1: has_imm = 1'b0;
      default: ;
    endcase
  endfunction

  virtual function void set_imm_len();
    case (instr_name)
      MOP_R: imm_len = 5;
      MOP_RR: imm_len = 3;
      C_MOP, AES64KS1I: imm_len = 4;
      AES32DSI, AES32DSMI, AES32ESI, AES32ESMI,
      SM4ED, SM4KS: imm_len = 2;
      PREFETCH_I, PREFETCH_R, PREFETCH_W: imm_len = 12;
      default: imm_len = 12;
    endcase
    imm_mask = imm_mask << imm_len;
  endfunction

  virtual function string convert2asm(string prefix = "");
    string mnemonic;
    string asm_str;
    mnemonic = format_string(get_instr_name(), MAX_INSTR_STR_LEN);
    case (instr_name)
      MOP_R:
        asm_str = $sformatf("mop.r.%0d %0s, %0s", imm[4:0], rd.name(), rs1.name());
      MOP_RR:
        asm_str = $sformatf("mop.rr.%0d %0s, %0s, %0s", imm[2:0], rd.name(), rs1.name(),
                            rs2.name());
      C_MOP:
        asm_str = $sformatf("c.mop.%0d", imm[3:0]);
      CBO_CLEAN, CBO_FLUSH, CBO_INVAL, CBO_ZERO:
        asm_str = $sformatf("%0s(%0s)", mnemonic, rs1.name());
      PREFETCH_I, PREFETCH_R, PREFETCH_W:
        asm_str = $sformatf("%0s%0d(%0s)", mnemonic, $signed(imm), rs1.name());
      AES32DSI, AES32DSMI, AES32ESI, AES32ESMI, SM4ED, SM4KS:
        asm_str = $sformatf("%0s%0s, %0s, %0s, %0d", mnemonic, rd.name(), rs1.name(),
                            rs2.name(), imm[1:0]);
      AES64KS1I:
        asm_str = $sformatf("%0s%0s, %0s, %0d", mnemonic, rd.name(), rs1.name(), imm[3:0]);
      BREV8, ZIP, UNZIP,
      AES64IM,
      SHA256SIG0, SHA256SIG1, SHA256SUM0, SHA256SUM1,
      SHA512SIG0, SHA512SIG1, SHA512SUM0, SHA512SUM1,
      SM3P0, SM3P1:
        asm_str = $sformatf("%0s%0s, %0s", mnemonic, rd.name(), rs1.name());
      default: return super.convert2asm(prefix);
    endcase
    if (comment != "") asm_str = {asm_str, " #", comment};
    return {prefix, asm_str.tolower()};
  endfunction

  virtual function string convert2bin(string prefix = "");
    bit [4:0] hint;
    string binary;
    case (instr_name)
      MOP_R:
        binary = $sformatf("%8h", {1'b1, imm[4], 2'b00, imm[3:2], 4'b0111, imm[1:0],
                                    rs1, 3'b100, rd, 7'b1110011});
      MOP_RR:
        binary = $sformatf("%8h", {1'b1, imm[2], 2'b00, imm[1:0], 1'b1, rs2, rs1,
                                    3'b100, rd, 7'b1110011});
      C_MOP:
        binary = $sformatf("%4h", {3'b011, 1'b0, 1'b0, imm[3:0], 5'b0, 2'b01});
      CBO_CLEAN:
        binary = $sformatf("%8h", {12'd1, rs1, 3'b010, 5'b0, 7'b0001111});
      CBO_FLUSH:
        binary = $sformatf("%8h", {12'd2, rs1, 3'b010, 5'b0, 7'b0001111});
      CBO_INVAL:
        binary = $sformatf("%8h", {12'd0, rs1, 3'b010, 5'b0, 7'b0001111});
      CBO_ZERO:
        binary = $sformatf("%8h", {12'd4, rs1, 3'b010, 5'b0, 7'b0001111});
      PREFETCH_I, PREFETCH_R, PREFETCH_W: begin
        case (instr_name)
          PREFETCH_I: hint = 5'd0;
          PREFETCH_R: hint = 5'd1;
          default:    hint = 5'd3;
        endcase
        binary = $sformatf("%8h", {imm[11:5], hint, rs1, 3'b110, 5'b0, 7'b0010011});
      end
      BREV8:
        binary = $sformatf("%8h", {12'h687, rs1, 3'b101, rd, 7'b0010011});
      ZIP:
        binary = $sformatf("%8h", {12'h08f, rs1, 3'b001, rd, 7'b0010011});
      UNZIP:
        binary = $sformatf("%8h", {12'h08f, rs1, 3'b101, rd, 7'b0010011});
      AES32DSI:
        binary = $sformatf("%8h", {imm[1:0], 5'b10101, rs2, rs1, 3'b000, rd, 7'b0110011});
      AES32DSMI:
        binary = $sformatf("%8h", {imm[1:0], 5'b10111, rs2, rs1, 3'b000, rd, 7'b0110011});
      AES32ESI:
        binary = $sformatf("%8h", {imm[1:0], 5'b10001, rs2, rs1, 3'b000, rd, 7'b0110011});
      AES32ESMI:
        binary = $sformatf("%8h", {imm[1:0], 5'b10011, rs2, rs1, 3'b000, rd, 7'b0110011});
      AES64IM:
        binary = $sformatf("%8h", {12'h300, rs1, 3'b001, rd, 7'b0010011});
      AES64KS1I:
        binary = $sformatf("%8h", {8'h31, imm[3:0], rs1, 3'b001, rd, 7'b0010011});
      SHA256SUM0:
        binary = $sformatf("%8h", {12'h100, rs1, 3'b001, rd, 7'b0010011});
      SHA256SUM1:
        binary = $sformatf("%8h", {12'h101, rs1, 3'b001, rd, 7'b0010011});
      SHA256SIG0:
        binary = $sformatf("%8h", {12'h102, rs1, 3'b001, rd, 7'b0010011});
      SHA256SIG1:
        binary = $sformatf("%8h", {12'h103, rs1, 3'b001, rd, 7'b0010011});
      SHA512SUM0:
        binary = $sformatf("%8h", {12'h104, rs1, 3'b001, rd, 7'b0010011});
      SHA512SUM1:
        binary = $sformatf("%8h", {12'h105, rs1, 3'b001, rd, 7'b0010011});
      SHA512SIG0:
        binary = $sformatf("%8h", {12'h106, rs1, 3'b001, rd, 7'b0010011});
      SHA512SIG1:
        binary = $sformatf("%8h", {12'h107, rs1, 3'b001, rd, 7'b0010011});
      SM3P0:
        binary = $sformatf("%8h", {12'h108, rs1, 3'b001, rd, 7'b0010011});
      SM3P1:
        binary = $sformatf("%8h", {12'h109, rs1, 3'b001, rd, 7'b0010011});
      SM4ED:
        binary = $sformatf("%8h", {imm[1:0], 5'b11000, rs2, rs1, 3'b000, rd, 7'b0110011});
      SM4KS:
        binary = $sformatf("%8h", {imm[1:0], 5'b11010, rs2, rs1, 3'b000, rd, 7'b0110011});
      default: begin
        if (format == R_FORMAT) begin
          binary = $sformatf("%8h", {get_func7(), rs2, rs1, get_func3(), rd, get_opcode()});
        end else begin
          binary = super.convert2bin();
        end
      end
    endcase
    return {prefix, binary};
  endfunction

  virtual function bit [6:0] get_opcode();
    case (instr_name)
      CZERO_EQZ, CZERO_NEZ, XPERM4, XPERM8,
      AES64DS, AES64DSM, AES64ES, AES64ESM, AES64KS2,
      SHA512SIG0H, SHA512SIG0L, SHA512SIG1H, SHA512SIG1L,
      SHA512SUM0R, SHA512SUM1R: get_opcode = 7'b0110011;
      default: get_opcode = super.get_opcode();
    endcase
  endfunction

  virtual function bit [2:0] get_func3();
    case (instr_name)
      CZERO_EQZ: get_func3 = 3'b101;
      CZERO_NEZ: get_func3 = 3'b111;
      XPERM4:    get_func3 = 3'b010;
      XPERM8:    get_func3 = 3'b100;
      AES64DS, AES64DSM, AES64ES, AES64ESM, AES64KS2,
      SHA512SIG0H, SHA512SIG0L, SHA512SIG1H, SHA512SIG1L,
      SHA512SUM0R, SHA512SUM1R: get_func3 = 3'b000;
      default: get_func3 = super.get_func3();
    endcase
  endfunction

  virtual function bit [6:0] get_func7();
    case (instr_name)
      CZERO_EQZ, CZERO_NEZ: get_func7 = 7'b0000111;
      XPERM4, XPERM8:       get_func7 = 7'b0010100;
      AES64ES:              get_func7 = 7'b0011001;
      AES64ESM:             get_func7 = 7'b0011011;
      AES64DS:              get_func7 = 7'b0011101;
      AES64DSM:             get_func7 = 7'b0011111;
      AES64KS2:             get_func7 = 7'b0111111;
      SHA512SUM0R:          get_func7 = 7'b0101000;
      SHA512SUM1R:          get_func7 = 7'b0101001;
      SHA512SIG0L:          get_func7 = 7'b0101010;
      SHA512SIG1L:          get_func7 = 7'b0101011;
      SHA512SIG0H:          get_func7 = 7'b0101110;
      SHA512SIG1H:          get_func7 = 7'b0101111;
      default: get_func7 = super.get_func7();
    endcase
  endfunction

  virtual function bit is_supported(riscv_instr_gen_config cfg);
    if ((XLEN != 32) && instr_name inside {
          ZIP, UNZIP,
          AES32DSI, AES32DSMI, AES32ESI, AES32ESMI,
          SHA512SIG0H, SHA512SIG0L, SHA512SIG1H, SHA512SIG1L,
          SHA512SUM0R, SHA512SUM1R}) begin
      return 1'b0;
    end
    if ((XLEN != 64) && instr_name inside {
          AES64DS, AES64DSM, AES64ES, AES64ESM, AES64IM, AES64KS1I, AES64KS2,
          SHA512SIG0, SHA512SIG1, SHA512SUM0, SHA512SUM1}) begin
      return 1'b0;
    end
    case (instr_name)
      CZERO_EQZ, CZERO_NEZ:
        return cfg.enable_zicond_extension;
      MOP_R, MOP_RR:
        return cfg.enable_zimop_extension;
      C_MOP:
        return cfg.enable_zcmop_extension;
      CBO_CLEAN, CBO_FLUSH, CBO_INVAL:
        return cfg.enable_zicbom_extension;
      CBO_ZERO:
        return cfg.enable_zicboz_extension;
      PREFETCH_I, PREFETCH_R, PREFETCH_W:
        return cfg.enable_zicbop_extension;
      BREV8, ZIP, UNZIP:
        return cfg.enable_zbkb_extension || cfg.enable_zkn_extension || cfg.enable_zks_extension;
      XPERM4, XPERM8:
        return cfg.enable_zbkx_extension || cfg.enable_zkn_extension || cfg.enable_zks_extension;
      AES32DSI, AES32DSMI, AES64DS, AES64DSM, AES64IM:
        return cfg.enable_zknd_extension || cfg.enable_zkn_extension;
      AES32ESI, AES32ESMI, AES64ES, AES64ESM:
        return cfg.enable_zkne_extension || cfg.enable_zkn_extension;
      AES64KS1I, AES64KS2:
        // The AES key-schedule instructions are shared by the decrypt and
        // encrypt subsets.  Crypto v1.0 (and the GNU toolchain) exposes them
        // when either Zknd or Zkne is present.
        return cfg.enable_zknd_extension || cfg.enable_zkne_extension ||
               cfg.enable_zkn_extension;
      SHA256SIG0, SHA256SIG1, SHA256SUM0, SHA256SUM1,
      SHA512SIG0, SHA512SIG1, SHA512SUM0, SHA512SUM1,
      SHA512SIG0H, SHA512SIG0L, SHA512SIG1H, SHA512SIG1L,
      SHA512SUM0R, SHA512SUM1R:
        return cfg.enable_zknh_extension || cfg.enable_zkn_extension;
      SM4ED, SM4KS:
        return cfg.enable_zksed_extension || cfg.enable_zks_extension;
      SM3P0, SM3P1:
        return cfg.enable_zksh_extension || cfg.enable_zks_extension;
      default: return 1'b0;
    endcase
  endfunction

  virtual function bit is_group_member(riscv_instr_group_t query_group);
    if (super.is_group_member(query_group)) return 1'b1;
    case (instr_name)
      CZERO_EQZ, CZERO_NEZ:
        return query_group == ((XLEN == 32) ? RV32ZICOND : RV64ZICOND);
      MOP_R, MOP_RR:
        return query_group == ((XLEN == 32) ? RV32ZIMOP : RV64ZIMOP);
      C_MOP:
        return query_group == ((XLEN == 32) ? RV32ZCMOP : RV64ZCMOP);
      CBO_CLEAN, CBO_FLUSH, CBO_INVAL:
        return query_group == ((XLEN == 32) ? RV32ZICBOM : RV64ZICBOM);
      CBO_ZERO:
        return query_group == ((XLEN == 32) ? RV32ZICBOZ : RV64ZICBOZ);
      PREFETCH_I, PREFETCH_R, PREFETCH_W:
        return query_group == ((XLEN == 32) ? RV32ZICBOP : RV64ZICBOP);
      BREV8, ZIP, UNZIP: begin
        if (XLEN == 32) begin
          return query_group inside {RV32ZBKB, RV32ZKN, RV32ZKS};
        end else begin
          return query_group inside {RV64ZBKB, RV64ZKN, RV64ZKS};
        end
      end
      XPERM4, XPERM8: begin
        if (XLEN == 32) begin
          return query_group inside {RV32ZBKX, RV32ZKN, RV32ZKS};
        end else begin
          return query_group inside {RV64ZBKX, RV64ZKN, RV64ZKS};
        end
      end
      AES32DSI, AES32DSMI:
        return query_group inside {RV32ZKND, RV32ZKN};
      AES32ESI, AES32ESMI:
        return query_group inside {RV32ZKNE, RV32ZKN};
      AES64DS, AES64DSM, AES64IM:
        return query_group inside {RV64ZKND, RV64ZKN};
      AES64ES, AES64ESM:
        return query_group inside {RV64ZKNE, RV64ZKN};
      AES64KS1I, AES64KS2:
        return query_group inside {RV64ZKND, RV64ZKNE, RV64ZKN};
      SHA256SIG0, SHA256SIG1, SHA256SUM0, SHA256SUM1: begin
        if (XLEN == 32) begin
          return query_group inside {RV32ZKNH, RV32ZKN};
        end else begin
          return query_group inside {RV64ZKNH, RV64ZKN};
        end
      end
      SHA512SIG0, SHA512SIG1, SHA512SUM0, SHA512SUM1:
        return query_group inside {RV64ZKNH, RV64ZKN};
      SHA512SIG0H, SHA512SIG0L, SHA512SIG1H, SHA512SIG1L,
      SHA512SUM0R, SHA512SUM1R:
        return query_group inside {RV32ZKNH, RV32ZKN};
      SM4ED, SM4KS: begin
        if (XLEN == 32) begin
          return query_group inside {RV32ZKSED, RV32ZKS};
        end else begin
          return query_group inside {RV64ZKSED, RV64ZKS};
        end
      end
      SM3P0, SM3P1: begin
        if (XLEN == 32) begin
          return query_group inside {RV32ZKSH, RV32ZKS};
        end else begin
          return query_group inside {RV64ZKSH, RV64ZKS};
        end
      end
      default: return 1'b0;
    endcase
  endfunction

  // The trace CSV keeps the architectural assembly syntax.  Several of these
  // extensions do not use the base R/I operand layouts (for example CBO has no
  // rd and AES32 has a byte-select operand), so decode their source operands
  // explicitly for functional coverage.
  virtual function void update_src_regs(string operands[$]);
    string reg_name;
    case (instr_name)
      MOP_R: begin
        `DV_CHECK_FATAL(operands.size() == 2, instr_name)
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
        imm = {27'b0, binary[30], binary[27:26], binary[21:20]};
      end
      MOP_RR: begin
        `DV_CHECK_FATAL(operands.size() == 3, instr_name)
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
        rs2 = get_gpr(operands[2]);
        rs2_value = get_gpr_state(operands[2]);
        imm = {29'b0, binary[30], binary[27:26]};
      end
      C_MOP: begin
        imm = {28'b0, binary[10:7]};
      end
      CBO_CLEAN, CBO_FLUSH, CBO_INVAL, CBO_ZERO: begin
        rs1 = riscv_reg_t'(binary[19:15]);
        reg_name = rs1.name();
        rs1_value = get_gpr_state(reg_name.tolower());
      end
      PREFETCH_I, PREFETCH_R, PREFETCH_W: begin
        rs1 = riscv_reg_t'(binary[19:15]);
        reg_name = rs1.name();
        rs1_value = get_gpr_state(reg_name.tolower());
        imm = {{20{binary[31]}}, binary[31:25], 5'b0};
      end
      AES32DSI, AES32DSMI, AES32ESI, AES32ESMI, SM4ED, SM4KS: begin
        `DV_CHECK_FATAL(operands.size() == 4, instr_name)
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
        rs2 = get_gpr(operands[2]);
        rs2_value = get_gpr_state(operands[2]);
        get_val(operands[3], imm);
      end
      AES64KS1I: begin
        `DV_CHECK_FATAL(operands.size() == 3, instr_name)
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
        get_val(operands[2], imm);
      end
      BREV8, ZIP, UNZIP,
      AES64IM,
      SHA256SIG0, SHA256SIG1, SHA256SUM0, SHA256SUM1,
      SHA512SIG0, SHA512SIG1, SHA512SUM0, SHA512SUM1,
      SM3P0, SM3P1: begin
        `DV_CHECK_FATAL(operands.size() == 2, instr_name)
        rs1 = get_gpr(operands[1]);
        rs1_value = get_gpr_state(operands[1]);
      end
      default: super.update_src_regs(operands);
    endcase
  endfunction : update_src_regs

endclass
