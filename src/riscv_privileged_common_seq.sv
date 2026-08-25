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

// This class provides some common routines for privileged mode operations
class riscv_privileged_common_seq extends uvm_sequence;

  riscv_instr_gen_config  cfg;
  int                     hart;
  riscv_privil_reg        mstatus;
  rand bit                mstatus_mie;
  riscv_privil_reg        mie;
  riscv_privil_reg        sstatus;
  riscv_privil_reg        sie;
  riscv_privil_reg        ustatus;
  riscv_privil_reg        uie;

  `uvm_object_utils(riscv_privileged_common_seq)

  function new(string name = "");
    super.new(name);
  endfunction

  virtual function void enter_privileged_mode(input privileged_mode_t mode,
                                              output string instrs[$]);
    string label = format_string({$sformatf("%0sinit_%0s:",
                                 hart_prefix(hart), mode.name())}, LABEL_STR_LEN);
    string ret_instr[] = {"mret"};
    riscv_privil_reg regs[$];
    label = label.tolower();
    setup_mmode_reg(mode, regs);
    if(mode == SUPERVISOR_MODE) begin
      setup_smode_reg(mode, regs);
    end
    if(mode == USER_MODE) begin
      setup_umode_reg(mode, regs);
    end
    setup_stateen(mode, instrs);
    setup_sstc(mode, instrs);
    setup_cbo(mode, instrs);
    if(cfg.virtual_addr_translation_on) begin
      setup_svpbmt(instrs);
      setup_satp(instrs);
    end
    gen_csr_instr(regs, instrs);
    // Use mret/sret to switch to the target privileged mode
    instrs.push_back(ret_instr[0]);
    foreach(instrs[i]) begin
      instrs[i] = {indent, instrs[i]};
    end
    instrs.push_front(label);
  endfunction

  // When M-mode is implemented, Svpbmt PTE encodings are enabled through
  // menvcfg.PBMTE.  Set it before installing SATP so generated PBMT leaf PTEs
  // are architecturally active as soon as translation starts.
  virtual function void setup_svpbmt(ref string instrs[$]);
    riscv_privil_reg menvcfg;
    privileged_reg_t menvcfg_csr;
    if (!cfg.enable_svpbmt) return;
    // Without M-mode there is no menvcfg gate; PBMT interpretation is enabled
    // directly by the supervisor implementation.
    if (!(MACHINE_MODE inside {supported_privileged_mode})) return;
    menvcfg_csr = (XLEN == 32) ? MENVCFGH : MENVCFG;
    if (!(menvcfg_csr inside {implemented_csr})) begin
      `uvm_fatal(`gfn, $sformatf(
          "Svpbmt requires %0s.PBMTE when M-mode is implemented", menvcfg_csr.name()))
    end
    menvcfg = riscv_privil_reg::type_id::create("menvcfg_svpbmt");
    menvcfg.init_reg(menvcfg_csr);
    menvcfg.set_field("PBMTE", 1'b1);
    instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], menvcfg.get_val()));
    instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable Svpbmt",
                              menvcfg_csr, cfg.gpr[0]));
  endfunction : setup_svpbmt

  // Open the state-enable hierarchy needed by the selected lower privilege.
  // The NanHu profile implements C/ENVCFG/SE0 in mstateen0 and the SE bits in
  // mstateen1-3. RV32 places the upper controls in the corresponding *H CSR.
  virtual function void setup_stateen(privileged_mode_t mode, ref string instrs[$]);
    riscv_privil_reg stateen;
    privileged_reg_t mstateen_low[4] = '{MSTATEEN0, MSTATEEN1, MSTATEEN2, MSTATEEN3};
    privileged_reg_t mstateen_high[4] =
        '{MSTATEEN0H, MSTATEEN1H, MSTATEEN2H, MSTATEEN3H};
    if ((mode == MACHINE_MODE) || !(MACHINE_MODE inside {supported_privileged_mode})) return;

    if (MSTATEEN0 inside {implemented_csr}) begin
      stateen = riscv_privil_reg::type_id::create("mstateen0_setup");
      stateen.init_reg(MSTATEEN0);
      stateen.set_field("C", 1'b1);
      stateen.set_field("FCSR", cfg.enable_floating_point);
      if (XLEN == 64) begin
        stateen.set_field("ENVCFG", 1'b1);
        stateen.set_field("SE0", 1'b1);
      end
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], stateen.get_val()));
      instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable mstateen0",
                                MSTATEEN0, cfg.gpr[0]));
    end
    if ((XLEN == 32) && (MSTATEEN0H inside {implemented_csr})) begin
      stateen = riscv_privil_reg::type_id::create("mstateen0h_setup");
      stateen.init_reg(MSTATEEN0H);
      stateen.set_field("ENVCFG", 1'b1);
      stateen.set_field("SE0", 1'b1);
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], stateen.get_val()));
      instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable mstateen0h",
                                MSTATEEN0H, cfg.gpr[0]));
    end
    for (int i = 1; i < 4; i++) begin
      privileged_reg_t csr = (XLEN == 32) ? mstateen_high[i] : mstateen_low[i];
      if (!(csr inside {implemented_csr})) continue;
      stateen = riscv_privil_reg::type_id::create($sformatf("mstateen%0d_setup", i));
      stateen.init_reg(csr);
      stateen.set_field("SE", 1'b1);
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], stateen.get_val()));
      instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable mstateen%0d",
                                csr, cfg.gpr[0], i));
    end
    if ((mode == USER_MODE) && (SSTATEEN0 inside {implemented_csr})) begin
      stateen = riscv_privil_reg::type_id::create("sstateen0_setup");
      stateen.init_reg(SSTATEEN0);
      stateen.set_field("C", 1'b1);
      stateen.set_field("FCSR", cfg.enable_floating_point);
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], stateen.get_val()));
      instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable sstateen0",
                                SSTATEEN0, cfg.gpr[0]));
    end
  endfunction : setup_stateen

  // Enable direct S-mode access to stimecmp.  mcounteren.TM is initialized in
  // setup_mmode_reg; menvcfg.STCE is in menvcfgh on RV32.
  virtual function void setup_sstc(privileged_mode_t mode, ref string instrs[$]);
    riscv_privil_reg menvcfg;
    privileged_reg_t menvcfg_csr;
    if ((mode == MACHINE_MODE) || !(STIMECMP inside {implemented_csr})) return;
    if (!(MACHINE_MODE inside {supported_privileged_mode})) return;
    menvcfg_csr = (XLEN == 32) ? MENVCFGH : MENVCFG;
    if (!(menvcfg_csr inside {implemented_csr})) begin
      `uvm_fatal(`gfn, $sformatf(
          "Sstc requires %0s.STCE when M-mode is implemented", menvcfg_csr.name()))
    end
    menvcfg = riscv_privil_reg::type_id::create("menvcfg_sstc");
    menvcfg.init_reg(menvcfg_csr);
    menvcfg.set_field("STCE", 1'b1);
    instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], menvcfg.get_val()));
    instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable Sstc",
                              menvcfg_csr, cfg.gpr[0]));
  endfunction : setup_sstc

  // Enable CBO instructions below M-mode.  CBIE=3 selects invalidate behavior;
  // CBCFE enables clean/flush, and CBZE enables zero.  U-mode additionally
  // requires the corresponding senvcfg bits.
  virtual function void setup_cbo(privileged_mode_t mode, ref string instrs[$]);
    riscv_privil_reg envcfg;
    bit enable_zicbom = cfg.enable_zicbom_extension;
    bit enable_zicboz = cfg.enable_zicboz_extension;
    if ((mode == MACHINE_MODE) || !(enable_zicbom || enable_zicboz)) return;
    if (MACHINE_MODE inside {supported_privileged_mode}) begin
      if (!(MENVCFG inside {implemented_csr})) begin
        `uvm_fatal(`gfn, "CBO instructions below M-mode require MENVCFG")
      end
      envcfg = riscv_privil_reg::type_id::create("menvcfg_cbo");
      envcfg.init_reg(MENVCFG);
      if (enable_zicbom) begin
        envcfg.set_field("CBIE", 2'b11);
        envcfg.set_field("CBCFE", 1'b1);
      end
      if (enable_zicboz) envcfg.set_field("CBZE", 1'b1);
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], envcfg.get_val()));
      instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable CBO in S-mode",
                                MENVCFG, cfg.gpr[0]));
    end
    if (mode == USER_MODE) begin
      if (!(SENVCFG inside {implemented_csr})) begin
        `uvm_fatal(`gfn, "CBO instructions in U-mode require SENVCFG")
      end
      envcfg = riscv_privil_reg::type_id::create("senvcfg_cbo");
      envcfg.init_reg(SENVCFG);
      if (enable_zicbom) begin
        envcfg.set_field("CBIE", 2'b11);
        envcfg.set_field("CBCFE", 1'b1);
      end
      if (enable_zicboz) envcfg.set_field("CBZE", 1'b1);
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], envcfg.get_val()));
      instrs.push_back($sformatf("csrs 0x%0x, x%0d # enable CBO in U-mode",
                                SENVCFG, cfg.gpr[0]));
    end
  endfunction : setup_cbo

  virtual function void setup_mmode_reg(privileged_mode_t mode, ref riscv_privil_reg regs[$]);
    riscv_privil_reg mcounteren;
    mstatus = riscv_privil_reg::type_id::create("mstatus");
    mstatus.init_reg(MSTATUS);
    if (cfg.randomize_csr) begin
      mstatus.set_val(cfg.mstatus);
    end
    mstatus.set_field("MPRV", cfg.mstatus_mprv);
    mstatus.set_field("MXR", cfg.mstatus_mxr);
    mstatus.set_field("SUM", cfg.mstatus_sum);
    mstatus.set_field("TVM", cfg.mstatus_tvm);
    mstatus.set_field("TW", cfg.set_mstatus_tw);
    mstatus.set_field("FS", cfg.mstatus_fs);
    mstatus.set_field("VS", cfg.mstatus_vs);
    if (!(SUPERVISOR_MODE inside {supported_privileged_mode}) && (XLEN != 32)) begin
      mstatus.set_field("SXL", 2'b00);
    end else if (XLEN == 64) begin
      mstatus.set_field("SXL", 2'b10);
    end
    if (!(USER_MODE inside {supported_privileged_mode}) && (XLEN != 32)) begin
      mstatus.set_field("UXL", 2'b00);
    end else if (XLEN == 64) begin
      mstatus.set_field("UXL", 2'b10);
    end
    mstatus.set_field("XS", 0);
    mstatus.set_field("SD", 0);
    mstatus.set_field("UIE", 0);
    // Set the previous privileged mode as the target mode
    mstatus.set_field("MPP", mode);
    mstatus.set_field("SPP", 0);
    // Enable interrupt
    // Only machine mode requires mstatus.MIE to be 1 for enabling interrupt
    if (mode == MACHINE_MODE) begin
      mstatus.set_field("MPIE", cfg.enable_interrupt);
    end else begin
      mstatus.set_field("MPIE", cfg.enable_interrupt & mstatus_mie);
    end
    // MIE is set when returning with mret, avoids trapping before returning
    mstatus.set_field("MIE", 0);
    mstatus.set_field("SPIE", cfg.enable_interrupt);
    mstatus.set_field("SIE",  cfg.enable_interrupt);
    mstatus.set_field("UPIE", cfg.enable_interrupt);
    mstatus.set_field("UIE", riscv_instr_pkg::support_umode_trap);
    `uvm_info(`gfn, $sformatf("mstatus_val: 0x%0x", mstatus.get_val()), UVM_LOW)
    regs.push_back(mstatus);
    // Enable external and timer interrupt
    if (MIE inside {implemented_csr}) begin
      mie = riscv_privil_reg::type_id::create("mie");
      mie.init_reg(MIE);
      if (cfg.randomize_csr) begin
        mie.set_val(cfg.mie);
      end
      mie.set_field("UEIE", cfg.enable_interrupt);
      mie.set_field("SEIE", cfg.enable_interrupt);
      mie.set_field("MEIE", cfg.enable_interrupt);
      mie.set_field("USIE", cfg.enable_interrupt);
      mie.set_field("SSIE", cfg.enable_interrupt);
      mie.set_field("MSIE", cfg.enable_interrupt);
      mie.set_field("MTIE", cfg.enable_interrupt & cfg.enable_timer_irq);
      mie.set_field("STIE", cfg.enable_interrupt & cfg.enable_timer_irq);
      mie.set_field("UTIE", cfg.enable_interrupt & cfg.enable_timer_irq);
      mie.set_field("LCOFIE", cfg.enable_interrupt & cfg.enable_sscofpmf);
      regs.push_back(mie);
    end
    if ((mode != MACHINE_MODE) && (STIMECMP inside {implemented_csr}) &&
        (MCOUNTEREN inside {implemented_csr})) begin
      mcounteren = riscv_privil_reg::type_id::create("mcounteren_sstc");
      mcounteren.init_reg(MCOUNTEREN);
      mcounteren.set_field("TM", 1'b1);
      regs.push_back(mcounteren);
    end
  endfunction

  virtual function void setup_smode_reg(privileged_mode_t mode, ref riscv_privil_reg regs[$]);
    sstatus = riscv_privil_reg::type_id::create("sstatus");
    sstatus.init_reg(SSTATUS);
    `DV_CHECK_RANDOMIZE_FATAL(sstatus, "cannot randomize sstatus")
    if (cfg.randomize_csr) begin
      sstatus.set_val(cfg.sstatus);
    end
    sstatus.set_field("SPIE", cfg.enable_interrupt);
    sstatus.set_field("SIE",  cfg.enable_interrupt);
    sstatus.set_field("UPIE", cfg.enable_interrupt);
    sstatus.set_field("UIE", riscv_instr_pkg::support_umode_trap);
    if(XLEN==64) begin
      sstatus.set_field("UXL", 2'b10);
    end
    sstatus.set_field("FS", cfg.mstatus_fs);
    sstatus.set_field("VS", cfg.mstatus_vs);
    sstatus.set_field("XS", 0);
    sstatus.set_field("SD", 0);
    sstatus.set_field("UIE", 0);
    sstatus.set_field("SPP", 0);
    regs.push_back(sstatus);
    // Enable external and timer interrupt
    if (SIE inside {implemented_csr}) begin
      sie = riscv_privil_reg::type_id::create("sie");
      sie.init_reg(SIE);
      if (cfg.randomize_csr) begin
        sie.set_val(cfg.sie);
      end
      sie.set_field("UEIE", cfg.enable_interrupt);
      sie.set_field("SEIE", cfg.enable_interrupt);
      sie.set_field("USIE", cfg.enable_interrupt);
      sie.set_field("SSIE", cfg.enable_interrupt);
      sie.set_field("STIE", cfg.enable_interrupt & cfg.enable_timer_irq);
      sie.set_field("UTIE", cfg.enable_interrupt & cfg.enable_timer_irq);
      sie.set_field("LCOFIE", cfg.enable_interrupt & cfg.enable_sscofpmf);
      regs.push_back(sie);
    end
  endfunction

  virtual function void setup_umode_reg(privileged_mode_t mode, ref riscv_privil_reg regs[$]);
    // For implementations that do not provide any U-mode CSRs, return immediately
    if (!riscv_instr_pkg::support_umode_trap) begin
      return;
    end
    ustatus = riscv_privil_reg::type_id::create("ustatus");
    ustatus.init_reg(USTATUS);
    `DV_CHECK_RANDOMIZE_FATAL(ustatus, "cannot randomize ustatus")
    if (cfg.randomize_csr) begin
      ustatus.set_val(cfg.ustatus);
    end
    ustatus.set_field("UIE", cfg.enable_interrupt);
    ustatus.set_field("UPIE", cfg.enable_interrupt);
    regs.push_back(ustatus);
    if (UIE inside {implemented_csr}) begin
      uie = riscv_privil_reg::type_id::create("uie");
      uie.init_reg(UIE);
      if (cfg.randomize_csr) begin
        uie.set_val(cfg.uie);
      end
      uie.set_field("UEIE", cfg.enable_interrupt);
      uie.set_field("USIE", cfg.enable_interrupt);
      uie.set_field("UTIE", cfg.enable_interrupt & cfg.enable_timer_irq);
      regs.push_back(uie);
    end
  endfunction

  virtual function void gen_csr_instr(riscv_privil_reg regs[$], ref string instrs[$]);
    foreach(regs[i]) begin
      instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], regs[i].get_val()));
      instrs.push_back($sformatf("csrw 0x%0x, x%0d # %0s",
                       regs[i].reg_name, cfg.gpr[0], regs[i].reg_name.name()));
    end
  endfunction

  virtual function void setup_satp(ref string instrs[$]);
    riscv_privil_reg satp;
    bit [XLEN-1:0] satp_ppn_mask;
    if(SATP_MODE == BARE) return;
    satp = riscv_privil_reg::type_id::create("satp");
    satp.init_reg(SATP);
    satp.set_field("MODE", SATP_MODE);
    instrs.push_back($sformatf("li x%0d, 0x%0x", cfg.gpr[0], satp.get_val()));
    instrs.push_back($sformatf("csrw 0x%0x, x%0d # satp", SATP, cfg.gpr[0]));
    satp_ppn_mask = '1 >> (XLEN - satp.get_field_by_name("PPN").bit_width);
    // Load the root page table physical address
    instrs.push_back($sformatf("la x%0d, page_table_0", cfg.gpr[0]));
    // Right shift to get PPN at 4k granularity
    instrs.push_back($sformatf("srli x%0d, x%0d, 12", cfg.gpr[0], cfg.gpr[0]));
    instrs.push_back($sformatf("li   x%0d, 0x%0x", cfg.gpr[1], satp_ppn_mask));
    instrs.push_back($sformatf("and x%0d, x%0d, x%0d", cfg.gpr[0], cfg.gpr[0], cfg.gpr[1]));
    // Set the PPN field for SATP
    instrs.push_back($sformatf("csrs 0x%0x, x%0d # satp", SATP, cfg.gpr[0]));
  endfunction

endclass
