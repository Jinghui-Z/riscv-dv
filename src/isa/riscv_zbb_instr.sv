/*
 * Copyright 2018 Google LLC
 * Copyright 2021 Silicon Labs, Inc.
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
class riscv_zbb_instr extends riscv_instr;
  `uvm_object_utils(riscv_zbb_instr)

  function new(string name = "");
    super.new(name);
  endfunction : new

  virtual function void set_rand_mode();
    super.set_rand_mode();
    case (format) inside
      R_FORMAT: begin
        if (instr_name inside { ZEXT_H }) begin
          has_rs2 = 1'b0;
        end
      end

      I_FORMAT: begin
        if (instr_name inside { CLZ, CLZW, CTZ, CTZW, CPOP, CPOPW, ORC_B, SEXT_B, SEXT_H, REV8 })
        begin
          has_imm = 1'b0;
        end
      end

    endcase
  endfunction : set_rand_mode

  function void pre_randomize();
    super.pre_randomize();
  endfunction

  function bit is_rv64();
    // Common Zbb/Zbkb instructions are declared in the RV32 group so the
    // same class can be selected for both XLENs.  The group therefore cannot
    // be used to select the RV64 encoding (for example REV8 and ZEXT.H).
    is_rv64 = (XLEN == 64);
  endfunction : is_rv64

  virtual function void set_imm_len();
    if (format inside {I_FORMAT}) begin
      if (instr_name inside {RORI}) begin
        imm_len = $clog2(XLEN);
      end else begin
        imm_len = 5;
      end
    end
    imm_mask = imm_mask << imm_len;
  endfunction : set_imm_len

  virtual function string convert2asm(string prefix = "");
    string asm_str_final;
    string asm_str;

    asm_str = format_string(get_instr_name(), MAX_INSTR_STR_LEN);

    case (format)
      I_FORMAT : begin // instr rd rs1
        if (!has_imm) begin
          asm_str_final = $sformatf("%0s%0s, %0s", asm_str, rd.name(), rs1.name());
        end
      end

      R_FORMAT : begin // instr rd rs1
        if (!has_rs2) begin
          asm_str_final = $sformatf("%0s%0s, %0s", asm_str, rd.name(), rs1.name());
        end
      end

      default: `uvm_info(`gfn, $sformatf("Unsupported format %0s", format.name()), UVM_LOW)
    endcase

    if (asm_str_final == "") begin
      return super.convert2asm(prefix);
    end

    if (comment != "") begin
      asm_str_final = { asm_str_final, " #", comment };
    end

    return asm_str_final.tolower();

  endfunction : convert2asm

  function bit[6:0] get_opcode();
    case (instr_name) inside
      ANDN, MAX, MAXU, MIN, MINU,
      ORN, ROL, ROR, XNOR          : get_opcode = 7'b011_0011;
      ZEXT_H                       : get_opcode = 7'b011_0011 | (is_rv64() << 3);
      ROLW, RORW                   : get_opcode = 7'b011_1011;
      CLZ, CPOP, CTZ, ORC_B,
      CLZW, CPOPW, CTZW, RORIW     : get_opcode = 7'b001_1011;
      REV8, RORI, SEXT_B, SEXT_H   : get_opcode = 7'b001_0011;
      default                      : get_opcode = super.get_opcode();
    endcase
  endfunction : get_opcode

  virtual function bit [2:0] get_func3();
    case (instr_name) inside
      ANDN    : get_func3 = 3'b111;
      CLZ     : get_func3 = 3'b001;
      CLZW    : get_func3 = 3'b001;
      CPOP    : get_func3 = 3'b001;
      CPOPW   : get_func3 = 3'b001;
      CTZ     : get_func3 = 3'b001;
      CTZW    : get_func3 = 3'b001;
      MAX     : get_func3 = 3'b110;
      MAXU    : get_func3 = 3'b111;
      MIN     : get_func3 = 3'b100;
      MINU    : get_func3 = 3'b101;
      ORC_B   : get_func3 = 3'b101;
      ORN     : get_func3 = 3'b110;
      REV8    : get_func3 = 3'b101;
      ROL     : get_func3 = 3'b001;
      ROLW    : get_func3 = 3'b001;
      ROR     : get_func3 = 3'b101;
      RORW    : get_func3 = 3'b101;
      RORI    : get_func3 = 3'b101;
      RORIW   : get_func3 = 3'b101;
      SEXT_B  : get_func3 = 3'b001;
      SEXT_H  : get_func3 = 3'b001;
      XNOR    : get_func3 = 3'b100;
      ZEXT_H  : get_func3 = 3'b100;
      default : get_func3 = super.get_func3();
    endcase
  endfunction : get_func3

  virtual function bit [4:0] get_func5();
    case (instr_name) inside
      CLZ     : get_func5 = 5'b0_0000;
      CLZW    : get_func5 = 5'b0_0000;
      CPOP    : get_func5 = 5'b0_0010;
      CPOPW   : get_func5 = 5'b0_0010;
      CTZ     : get_func5 = 5'b0_0001;
      CTZW    : get_func5 = 5'b0_0001;
      ORC_B   : get_func5 = 5'b0_0111;
      REV8    : get_func5 = 5'b1_1000;
      SEXT_B  : get_func5 = 5'b0_0100;
      SEXT_H  : get_func5 = 5'b0_0101;
    endcase
  endfunction : get_func5

  virtual function bit [6:0] get_func7();
    case (instr_name) inside
      ANDN    : get_func7 = 7'b010_0000;
      CLZ     : get_func7 = 7'b011_0000;
      CLZW    : get_func7 = 7'b011_0000;
      CPOP    : get_func7 = 7'b011_0000;
      CPOPW   : get_func7 = 7'b011_0000;
      CTZ     : get_func7 = 7'b011_0000;
      CTZW    : get_func7 = 7'b011_0000;
      MAX     : get_func7 = 7'b000_0101;
      MAXU    : get_func7 = 7'b000_0101;
      MIN     : get_func7 = 7'b000_0101;
      MINU    : get_func7 = 7'b000_0101;
      ORC_B   : get_func7 = 7'b001_0100;
      ORN     : get_func7 = 7'b010_0000;
      REV8    : get_func7 = 7'b011_0100 | is_rv64(); // 0110101 64 bit
      ROL     : get_func7 = 7'b011_0000;
      ROLW    : get_func7 = 7'b011_0000;
      ROR     : get_func7 = 7'b011_0000;
      RORW    : get_func7 = 7'b011_0000;
      RORI    : get_func7 = 7'b011_0000;
      RORIW   : get_func7 = 7'b011_0000;
      SEXT_B  : get_func7 = 7'b011_0000;
      SEXT_H  : get_func7 = 7'b011_0000;
      XNOR    : get_func7 = 7'b010_0000;
      ZEXT_H  : get_func7 = 7'b000_0100;
      default : get_func7 = super.get_func7();
    endcase
  endfunction : get_func7

  virtual function string convert2bin(string prefix = "");
    string binary = "";

    case (format)
      R_FORMAT: begin
        if (instr_name inside { ZEXT_H }) begin
          binary = $sformatf("%8h", {get_func7(), get_func5(), rs1, get_func3(), rd, get_opcode()});
        end else begin
          binary = $sformatf("%8h", {get_func7(), rs2, rs1, get_func3(), rd, get_opcode()});
        end
      end

      I_FORMAT: begin
        case (instr_name) inside
          CLZ, CLZW, CPOP, CPOPW, CTZ, CTZW, ORC_B, REV8, SEXT_B, SEXT_H: begin
            binary = $sformatf("%8h", {get_func7(), get_func5(), rs1, get_func3(), rd,
                                       get_opcode()});
          end
          RORIW: begin
            binary = $sformatf("%8h", {get_func7(), imm[4:0], rs1, get_func3(), rd, get_opcode()});
          end
          RORI: begin
            // set bit 0 of funct7 only if rv64 and shamt[MSB] is set
            binary = $sformatf("%8h", {(get_func7() | (is_rv64() && imm[5])), imm[4:0], rs1,
                                        get_func3(), rd, get_opcode()});
          end
        endcase
      end

      default: ;

    endcase // case (format)
    if (binary == "") binary = super.convert2bin();
    return {prefix, binary};
  endfunction : convert2bin

  virtual function bit is_supported(riscv_instr_gen_config cfg);
    // The *W rotate forms are RV64-only.  They share encodings with Zbkb on
    // RV64, but must not become visible in an RV32 aggregate Zkn/Zks profile.
    if ((XLEN == 32) && instr_name inside {CLZW, CPOPW, CTZW, ROLW, RORW, RORIW}) begin
      return 1'b0;
    end
    return ((cfg.enable_zbb_extension ||
             ((cfg.enable_zbkb_extension || cfg.enable_zkn_extension || cfg.enable_zks_extension) &&
              instr_name inside {ANDN, ORN, XNOR, ROL, ROR, RORI,
                                 ROLW, RORW, RORIW, REV8,
                                 ZEXT_H})) &&
           instr_name inside {
             ANDN,
             CLZ, CLZW,
             CPOP, CPOPW,
             CTZ, CTZW,
             MAX, MAXU,
             MIN, MINU,
             ORC_B, ORN,
             REV8,
             ROL, ROLW,
             ROR, RORW,
             RORI, RORIW,
             SEXT_B, SEXT_H,
             XNOR,
             ZEXT_H
           });
  endfunction : is_supported

  virtual function bit is_group_member(riscv_instr_group_t query_group);
    if ((XLEN == 32) && instr_name inside {CLZW, CPOPW, CTZW, ROLW, RORW, RORIW}) begin
      return 1'b0;
    end
    if (super.is_group_member(query_group)) return 1'b1;
    if (instr_name inside {ANDN, ORN, XNOR, ROL, ROR, RORI, ROLW, RORW, RORIW,
                           REV8, ZEXT_H}) begin
      if (XLEN == 32) begin
        return query_group inside {RV32ZBKB, RV32ZKN, RV32ZKS};
      end else begin
        return query_group inside {RV64ZBKB, RV64ZKN, RV64ZKS};
      end
    end
    return 1'b0;
  endfunction

  virtual function void update_src_regs(string operands[$]);
    // All ZBB I_FORMAT instructions other than RORI use the immediate to specify the operation,
    // rather than being an explicit operand. Handle this case here, otherwise use the normal
    // `update_src_regs`
    if ((format == I_FORMAT) && (instr_name != RORI)) begin
      `DV_CHECK_FATAL(operands.size() == 2, instr_name)
      rs1 = get_gpr(operands[1]);
      rs1_value = get_gpr_state(operands[1]);
    end else begin
      super.update_src_regs(operands);
    end
  endfunction : update_src_regs

endclass : riscv_zbb_instr
