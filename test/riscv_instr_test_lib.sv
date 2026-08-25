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


class riscv_rand_instr_test extends riscv_instr_base_test;

  `uvm_component_utils(riscv_rand_instr_test)
  `uvm_component_new

  virtual function void randomize_cfg();
    cfg.instr_cnt = 10000;
    cfg.num_of_sub_program = 5;
    `DV_CHECK_RANDOMIZE_FATAL(cfg)
    `uvm_info(`gfn, $sformatf("riscv_instr_gen_config is randomized:\n%0s",
                    cfg.sprint()), UVM_LOW)
  endfunction

  virtual function void apply_directed_instr();
    // Mix below directed instruction streams with the random instructions
    asm_gen.add_directed_instr_stream("riscv_load_store_rand_instr_stream", 4);
    asm_gen.add_directed_instr_stream("riscv_loop_instr", 3);
    asm_gen.add_directed_instr_stream("riscv_jal_instr", 4);
    asm_gen.add_directed_instr_stream("riscv_hazard_instr_stream", 4);
    asm_gen.add_directed_instr_stream("riscv_load_store_hazard_instr_stream", 4);
    asm_gen.add_directed_instr_stream("riscv_multi_page_load_store_instr_stream", 4);
    asm_gen.add_directed_instr_stream("riscv_mem_region_stress_test", 4);
  endfunction

endclass

// Unit-level guard for CSR privilege filtering. Trigger CSR names begin with
// "T", so this catches regressions back to name-prefix based classification.
class riscv_invalid_priv_csr_filter_test extends riscv_instr_base_test;

  `uvm_component_utils(riscv_invalid_priv_csr_filter_test)
  `uvm_component_new

  virtual function void randomize_cfg();
    riscv_privil_reg mconfigptr;
    super.randomize_cfg();
    `DV_CHECK_FATAL(cfg.init_privileged_mode == SUPERVISOR_MODE,
                    "invalid trigger CSR filter test must boot in S-mode")
    `DV_CHECK_FATAL(TSELECT inside {cfg.invalid_priv_mode_csrs},
                    "S-mode invalid CSR list is missing TSELECT")
    `DV_CHECK_FATAL(TDATA1 inside {cfg.invalid_priv_mode_csrs},
                    "S-mode invalid CSR list is missing TDATA1")
    `DV_CHECK_FATAL(TCONTROL inside {cfg.invalid_priv_mode_csrs},
                    "S-mode invalid CSR list is missing TCONTROL")
    `DV_CHECK_FATAL(MCONFIGPTR inside {cfg.invalid_priv_mode_csrs},
                    "S-mode invalid CSR list is missing MCONFIGPTR")
    mconfigptr = riscv_privil_reg::type_id::create("mconfigptr_model");
    mconfigptr.init_reg(MCONFIGPTR);
    `DV_CHECK_FATAL(mconfigptr.get_val() == '0, "MCONFIGPTR model must read as zero")
  endfunction

  virtual function void apply_directed_instr();
    riscv_csr_instr::include_reg = {TSELECT, TDATA1, TDATA2, TDATA3, TINFO, TCONTROL};
  endfunction

endclass

class riscv_ml_test extends riscv_instr_base_test;

  `uvm_component_utils(riscv_ml_test)
  `uvm_component_new

  virtual function void randomize_cfg();
    cfg.addr_translaction_rnd_order_c.constraint_mode(0);
    `DV_CHECK_RANDOMIZE_FATAL(cfg)
    cfg.addr_translaction_rnd_order_c.constraint_mode(1);
    `uvm_info(`gfn, $sformatf("riscv_instr_gen_config is randomized:\n%0s",
                    cfg.sprint()), UVM_LOW)
  endfunction

endclass

// Directed regression for scalar-crypto encodings and instructions shared by
// multiple named extensions. It checks the architectural XLEN-specific alias
// and aggregate group buckets.
class riscv_scalar_ext_encoding_test extends riscv_instr_base_test;

  `uvm_component_utils(riscv_scalar_ext_encoding_test)
  `uvm_component_new

  function bit group_has_instr(riscv_instr_group_t query_group,
                               riscv_instr_name_t query_instr);
    if (!riscv_instr::instr_group.exists(query_group)) return 1'b0;
    foreach (riscv_instr::instr_group[query_group][i]) begin
      if (riscv_instr::instr_group[query_group][i] == query_instr) return 1'b1;
    end
    return 1'b0;
  endfunction

  function bit list_has_instr(riscv_instr_name_t query_instr);
    foreach (riscv_instr::instr_names[i]) begin
      if (riscv_instr::instr_names[i] == query_instr) return 1'b1;
    end
    return 1'b0;
  endfunction

  function void check_group_member(riscv_instr_group_t query_group,
                                   riscv_instr_name_t query_instr,
                                   bit expected);
    bit actual;
    actual = group_has_instr(query_group, query_instr);
    `DV_CHECK_FATAL(actual == expected,
                    $sformatf("group %0s instruction %0s expected %0b got %0b",
                              query_group.name(), query_instr.name(), expected, actual))
  endfunction

  function void check_encoding(riscv_instr_name_t query_instr,
                               bit [31:0] query_imm,
                               string expected);
    riscv_instr instr;
    string actual;
    string tagged_expected;
    instr = riscv_instr::create_instr(query_instr);
    instr.rd = T0;
    instr.rs1 = T1;
    instr.rs2 = T2;
    instr.imm = query_imm;
    actual = instr.convert2bin("tag:");
    tagged_expected = {"tag:", expected};
    `DV_CHECK_FATAL(actual == tagged_expected,
                    $sformatf("%0s expected tag:%0s got %0s",
                              query_instr.name(), expected, actual))
  endfunction

  task run_phase(uvm_phase phase);
    riscv_instr instr;

    cfg.enable_b_extension = 1'b0;
    cfg.enable_zbb_extension = 1'b0;
    cfg.enable_zbc_extension = 1'b0;
    cfg.enable_zbkc_extension = 1'b1;
    cfg.enable_zicond_extension = 1'b1;
    cfg.enable_zimop_extension = 1'b1;
    cfg.enable_zcmop_extension = 1'b1;
    cfg.enable_zicbom_extension = 1'b1;
    cfg.enable_zicbop_extension = 1'b1;
    cfg.enable_zicboz_extension = 1'b1;
    cfg.enable_zbkb_extension = 1'b1;
    cfg.enable_zbkx_extension = 1'b1;
    cfg.enable_zknd_extension = 1'b1;
    cfg.enable_zkne_extension = 1'b1;
    cfg.enable_zknh_extension = 1'b1;
    cfg.enable_zksed_extension = 1'b1;
    cfg.enable_zksh_extension = 1'b1;
    cfg.enable_zkn_extension = 1'b1;
    cfg.enable_zks_extension = 1'b1;
    riscv_instr::create_instr_list(cfg);

    check_encoding(CZERO_EQZ, 32'd0, "0e7352b3");
    check_encoding(CZERO_NEZ, 32'd0, "0e7372b3");
    check_encoding(MOP_R, 32'd0, "81c342f3");
    check_encoding(MOP_R, 32'd31, "cdf342f3");
    check_encoding(MOP_RR, 32'd0, "827342f3");
    check_encoding(MOP_RR, 32'd7, "ce7342f3");
    check_encoding(C_MOP, 32'd1, "6081");
    check_encoding(C_MOP, 32'd15, "6781");
    check_encoding(CBO_INVAL, 32'd0, "0003200f");
    check_encoding(CBO_CLEAN, 32'd0, "0013200f");
    check_encoding(CBO_FLUSH, 32'd0, "0023200f");
    check_encoding(CBO_ZERO, 32'd0, "0043200f");
    check_encoding(PREFETCH_I, 32'd0, "00036013");
    check_encoding(PREFETCH_R, 32'd32, "02136013");
    check_encoding(PREFETCH_W, 32'hffff_ffe0, "fe336013");
    check_encoding(BREV8, 32'd0, "68735293");
    check_encoding(XPERM4, 32'd0, "287322b3");
    check_encoding(XPERM8, 32'd0, "287342b3");
    check_encoding(ANDN, 32'd0, "407372b3");
    check_encoding(ORN, 32'd0, "407362b3");
    check_encoding(XNOR, 32'd0, "407342b3");
    check_encoding(ROL, 32'd0, "607312b3");
    check_encoding(ROR, 32'd0, "607352b3");
    check_encoding(CLMUL, 32'd0, "0a7312b3");
    check_encoding(CLMULH, 32'd0, "0a7332b3");
    check_encoding(PACK, 32'd0, "087342b3");
    check_encoding(PACKH, 32'd0, "087372b3");
    check_encoding(SHA256SUM0, 32'd0, "10031293");
    check_encoding(SHA256SUM1, 32'd0, "10131293");
    check_encoding(SHA256SIG0, 32'd0, "10231293");
    check_encoding(SHA256SIG1, 32'd0, "10331293");
    check_encoding(SM4ED, 32'd0, "307302b3");
    check_encoding(SM4ED, 32'd3, "f07302b3");
    check_encoding(SM4KS, 32'd0, "347302b3");
    check_encoding(SM4KS, 32'd3, "f47302b3");
    check_encoding(SM3P0, 32'd0, "10831293");
    check_encoding(SM3P1, 32'd0, "10931293");
    check_encoding(SINVAL_VMA, 32'd0, "16730073");
    check_encoding(SFENCE_W_INVAL, 32'd0, "18000073");
    check_encoding(SFENCE_INVAL_IR, 32'd0, "18100073");
    if (XLEN == 64) begin
      check_encoding(RORIW, 32'd31, "61f3529b");
      check_encoding(RORI, 32'd63, "63f35293");
      check_encoding(REV8, 32'd0, "6b835293");
      check_encoding(ZEXT_H, 32'd0, "080342bb");
      check_encoding(ROLW, 32'd0, "607312bb");
      check_encoding(RORW, 32'd0, "607352bb");
      check_encoding(PACKW, 32'd0, "087342bb");
      check_encoding(AES64DS, 32'd0, "3a7302b3");
      check_encoding(AES64DSM, 32'd0, "3e7302b3");
      check_encoding(AES64ES, 32'd0, "327302b3");
      check_encoding(AES64ESM, 32'd0, "367302b3");
      check_encoding(AES64IM, 32'd0, "30031293");
      check_encoding(AES64KS1I, 32'd0, "31031293");
      check_encoding(AES64KS1I, 32'd10, "31a31293");
      check_encoding(AES64KS2, 32'd0, "7e7302b3");
      check_encoding(SHA512SUM0, 32'd0, "10431293");
      check_encoding(SHA512SUM1, 32'd0, "10531293");
      check_encoding(SHA512SIG0, 32'd0, "10631293");
      check_encoding(SHA512SIG1, 32'd0, "10731293");

      check_group_member(RV64ZBKC, CLMUL, 1'b1);
      check_group_member(RV64ZBKC, CLMULH, 1'b1);
      check_group_member(RV64ZBKC, CLMULR, 1'b0);
      check_group_member(RV64ZBKB, BREV8, 1'b1);
      check_group_member(RV64ZBKB, PACKW, 1'b1);
      check_group_member(RV64ZBKB, ZIP, 1'b0);
      check_group_member(RV64ZBKX, XPERM4, 1'b1);
      check_group_member(RV64ZKND, AES64DS, 1'b1);
      check_group_member(RV64ZKNE, AES64KS1I, 1'b1);
      check_group_member(RV64ZKNH, SHA256SIG0, 1'b1);
      check_group_member(RV64ZKSED, SM4ED, 1'b1);
      check_group_member(RV64ZKSH, SM3P0, 1'b1);
      check_group_member(RV64ZKN, AES64DS, 1'b1);
      check_group_member(RV64ZKN, AES64ES, 1'b1);
      check_group_member(RV64ZKN, SHA512SIG0, 1'b1);
      check_group_member(RV64ZKN, SM4ED, 1'b0);
      check_group_member(RV64ZKS, SM4ED, 1'b1);
      check_group_member(RV64ZKS, SM3P0, 1'b1);
      check_group_member(RV64ZKS, AES64ES, 1'b0);
      check_group_member(RV64ZICOND, CZERO_EQZ, 1'b1);
      check_group_member(RV64ZIMOP, MOP_R, 1'b1);
      check_group_member(RV64ZCMOP, C_MOP, 1'b1);
      check_group_member(RV64ZICBOM, CBO_CLEAN, 1'b1);
      check_group_member(RV64ZICBOP, PREFETCH_R, 1'b1);
      check_group_member(RV64ZICBOZ, CBO_ZERO, 1'b1);
      `DV_CHECK_FATAL(!list_has_instr(ZIP), "ZIP must be RV32-only")
      `DV_CHECK_FATAL(!list_has_instr(AES32DSI), "AES32DSI must be RV32-only")
      `DV_CHECK_FATAL(!list_has_instr(SHA512SIG0H), "SHA512SIG0H must be RV32-only")

      repeat (100) begin
        instr = riscv_instr::get_rand_instr(.include_group({RV64ZBKC}));
        `DV_CHECK_FATAL(instr.instr_name inside {CLMUL, CLMULH},
                        $sformatf("strict RV64 Zbkc bucket selected %0s",
                                  instr.instr_name.name()))
      end
    end else begin
      check_encoding(RORI, 32'd31, "61f35293");
      check_encoding(REV8, 32'd0, "69835293");
      check_encoding(ZEXT_H, 32'd0, "080342b3");
      check_encoding(ZIP, 32'd0, "08f31293");
      check_encoding(UNZIP, 32'd0, "08f35293");
      check_encoding(AES32DSI, 32'd0, "2a7302b3");
      check_encoding(AES32DSI, 32'd3, "ea7302b3");
      check_encoding(AES32DSMI, 32'd0, "2e7302b3");
      check_encoding(AES32ESI, 32'd0, "227302b3");
      check_encoding(AES32ESMI, 32'd0, "267302b3");
      check_encoding(SHA512SUM0R, 32'd0, "507302b3");
      check_encoding(SHA512SUM1R, 32'd0, "527302b3");
      check_encoding(SHA512SIG0H, 32'd0, "5c7302b3");
      check_encoding(SHA512SIG0L, 32'd0, "547302b3");
      check_encoding(SHA512SIG1H, 32'd0, "5e7302b3");
      check_encoding(SHA512SIG1L, 32'd0, "567302b3");

      check_group_member(RV32ZBKC, CLMUL, 1'b1);
      check_group_member(RV32ZBKC, CLMULH, 1'b1);
      check_group_member(RV32ZBKC, CLMULR, 1'b0);
      check_group_member(RV32ZBKB, BREV8, 1'b1);
      check_group_member(RV32ZBKB, PACK, 1'b1);
      check_group_member(RV32ZBKB, ZIP, 1'b1);
      check_group_member(RV32ZBKX, XPERM4, 1'b1);
      check_group_member(RV32ZKND, AES32DSI, 1'b1);
      check_group_member(RV32ZKNE, AES32ESI, 1'b1);
      check_group_member(RV32ZKNH, SHA512SIG0H, 1'b1);
      check_group_member(RV32ZKSED, SM4ED, 1'b1);
      check_group_member(RV32ZKSH, SM3P0, 1'b1);
      check_group_member(RV32ZKN, AES32DSI, 1'b1);
      check_group_member(RV32ZKN, AES32ESI, 1'b1);
      check_group_member(RV32ZKN, SHA512SIG0H, 1'b1);
      check_group_member(RV32ZKN, SM4ED, 1'b0);
      check_group_member(RV32ZKS, SM4ED, 1'b1);
      check_group_member(RV32ZKS, SM3P0, 1'b1);
      check_group_member(RV32ZKS, AES32ESI, 1'b0);
      check_group_member(RV32ZICOND, CZERO_EQZ, 1'b1);
      check_group_member(RV32ZIMOP, MOP_R, 1'b1);
      check_group_member(RV32ZCMOP, C_MOP, 1'b1);
      check_group_member(RV32ZICBOM, CBO_CLEAN, 1'b1);
      check_group_member(RV32ZICBOP, PREFETCH_R, 1'b1);
      check_group_member(RV32ZICBOZ, CBO_ZERO, 1'b1);
      `DV_CHECK_FATAL(!list_has_instr(PACKW), "PACKW must be RV64-only")
      `DV_CHECK_FATAL(!list_has_instr(RORIW), "RORIW must be RV64-only")
      `DV_CHECK_FATAL(!list_has_instr(AES64DS), "AES64DS must be RV64-only")
      `DV_CHECK_FATAL(!list_has_instr(SHA512SIG0), "SHA512SIG0 must be RV64-only")

      repeat (100) begin
        instr = riscv_instr::get_rand_instr(.include_group({RV32ZBKC}));
        `DV_CHECK_FATAL(instr.instr_name inside {CLMUL, CLMULH},
                        $sformatf("strict RV32 Zbkc bucket selected %0s",
                                  instr.instr_name.name()))
      end
    end
  endtask

endclass
