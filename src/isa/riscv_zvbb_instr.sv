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

// Vector basic bit-manipulation extension, version 1.0.
class riscv_zvbb_instr extends riscv_vector_instr;

  `uvm_object_utils(riscv_zvbb_instr)
  `uvm_object_new

  virtual function void set_imm_len();
    case (instr_name)
      // vror.vi uses uimm[5:0]; bit 5 selects the second funct6 encoding.
      VROR: begin
        imm_type = UIMM;
        imm_len = 6;
      end
      // vwsll.vi uses the normal five-bit vector immediate field.
      VWSLL: begin
        imm_type = UIMM;
        imm_len = 5;
      end
      default: super.set_imm_len();
    endcase
    imm_mask = 32'hffff_ffff << imm_len;
  endfunction : set_imm_len

  virtual function void set_rand_mode();
    super.set_rand_mode();
    has_vs3 = 1'b0;
    if (format == VS2_FORMAT) begin
      has_rs1 = 1'b0;
      has_vs1 = 1'b0;
      has_imm = 1'b0;
    end
  endfunction : set_rand_mode

  virtual function bit is_supported(riscv_instr_gen_config cfg);
    return cfg.enable_vector_extension && cfg.enable_zvbb_extension &&
           (RVV inside {supported_isa}) && (ZVBB inside {supported_isa}) &&
           super.is_supported(cfg);
  endfunction : is_supported

endclass : riscv_zvbb_instr

`define DEFINE_ZVBB_INSTR(instr_n, instr_format, instr_category, variants = {}) \
  class riscv_``instr_n``_instr extends riscv_zvbb_instr; \
    `VA_INSTR_BODY(instr_n, instr_format, instr_category, ZVBB, variants, "zvbb")

`DEFINE_ZVBB_INSTR(VANDN,    VA_FORMAT,  LOGICAL,    {VV, VX})
`DEFINE_ZVBB_INSTR(VBREV_V,  VS2_FORMAT, LOGICAL)
`DEFINE_ZVBB_INSTR(VBREV8_V, VS2_FORMAT, LOGICAL)
`DEFINE_ZVBB_INSTR(VREV8_V,  VS2_FORMAT, LOGICAL)
`DEFINE_ZVBB_INSTR(VCLZ_V,   VS2_FORMAT, ARITHMETIC)
`DEFINE_ZVBB_INSTR(VCTZ_V,   VS2_FORMAT, ARITHMETIC)
`DEFINE_ZVBB_INSTR(VCPOP_V,  VS2_FORMAT, ARITHMETIC)
`DEFINE_ZVBB_INSTR(VROL,     VA_FORMAT,  SHIFT,      {VV, VX})
`DEFINE_ZVBB_INSTR(VROR,     VA_FORMAT,  SHIFT,      {VV, VX, VI})
`DEFINE_ZVBB_INSTR(VWSLL,    VA_FORMAT,  SHIFT,      {VV, VX, VI})

`undef DEFINE_ZVBB_INSTR
