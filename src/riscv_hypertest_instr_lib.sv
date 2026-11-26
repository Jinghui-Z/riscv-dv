// Sv48 fetch scenarios ported from hypertest:
// ai_test_cases/frontend/fetch/ai_micro_sv48_upper_non_cfi_jump_cases.c.
// Keep the executable pages static: no runtime store writes instruction bytes.

// An indivisible assembly template. Normal branch/illegal-instruction
// postprocessing must never split the privilege transition or local handler.
class riscv_hypertest_asm_instr extends riscv_instr;
  string assembly;
  `uvm_object_utils(riscv_hypertest_asm_instr)
  `uvm_object_new

  virtual function string convert2asm(string prefix = "");
    return {prefix, assembly};
  endfunction
endclass

class riscv_hypertest_sv48_canonical_stream extends riscv_directed_instr_stream;
  bit boundary_x = 1;
  string lines[$];
  string tag;

  `uvm_object_utils(riscv_hypertest_sv48_canonical_stream)
  `uvm_object_new

  function void emit(string line);
    lines.push_back(line);
  endfunction

  function string sym(string suffix);
    return {tag, "_", suffix};
  endfunction

  // x5 = table base, x6 = encoded PTE. Large L0 indices exceed sd's imm12.
  function void write_pte(string table_name, int index, string target, int flags);
    emit({"lla x5, ", sym(table_name)});
    emit({"lla x6, ", sym(target)});
    emit("srli x6, x6, 2");
    emit($sformatf("ori x6, x6, %0d", flags));
    if (index * 8 > 2047) begin
      emit($sformatf("li x7, %0d", index * 8));
      emit("add x5, x5, x7");
      emit("sd x6, 0(x5)");
    end else begin
      emit($sformatf("sd x6, %0d(x5)", index * 8));
    end
  endfunction

  // Each entry starts in M, visits S, then returns to M through the private
  // handler. An absent PF reaches an ECALL with the wrong expected cause.
  function void probe(string suffix, string target, int seed,
                      int expected_value, int expected_marker, bit expect_pf);
    emit("li x5, 0x5a7f39f400000004");
    emit("sd x5, 320(x31) # marker");
    emit($sformatf("li x5, %0d", expect_pf ? 12 : 9));
    emit("sd x5, 344(x31) # expected trap cause");
    emit({"lla x5, ", sym({suffix, "_resume"})});
    emit("sd x5, 336(x31)");
    emit($sformatf("li a0, %0d", seed));
    emit("addi a1, x31, 320");
    emit({"li x7, ", target});
    emit({"lla x5, ", sym("s_entry")});
    emit("csrw mepc, x5");
    emit("li x5, 0x1800");
    emit("csrc mstatus, x5");
    emit("li x5, 0x800 # MPP=S");
    emit("csrs mstatus, x5");
    emit("mret");
    emit({sym({suffix, "_resume"}), ":"});
    emit($sformatf("li x5, %0d", expected_value));
    emit({"bne a0, x5, ", sym("fail")});
    emit("ld x6, 320(x31)");
    emit($sformatf("li x5, %0d", expected_marker));
    emit({"bne x6, x5, ", sym("fail")});
  endfunction

  function void post_randomize();
    riscv_hypertest_asm_instr block_instr;
    riscv_instr_stream base_stream;
    string csrs[$] = '{"mstatus", "medeleg", "satp", "mtvec", "mepc", "mcause", "mtval"};
    string tables[$] = '{"root", "low_l2", "high_l2", "high_l1", "high_l0",
                         "boundary_l2", "boundary_l1", "boundary_l0"};
    // Explicit +directed_instr_N use must obey the same contract as the pool.
    if (!cfg.enable_random_hypertest || (XLEN != 64) || (SATP_MODE != SV48) ||
        (cfg.init_privileged_mode != MACHINE_MODE) || kernel_mode ||
        cfg.bare_program_mode || (cfg.num_of_harts != 1) || cfg.set_mstatus_mprv ||
        cfg.pmp_cfg.enable_random_pmp_exception) begin
      `uvm_fatal(`gfn, "Hypertest Sv48 streams require enable_random_hypertest=1, RV64 Sv48 M boot, one hart, and no bare/MPRV/PMP-exception/kernel mode")
    end
    // The upstream directed base hides riscv_instr_stream::label. The
    // program generator assigns through the base handle, so read that field.
    base_stream = this;
    tag = $sformatf("h%0d_%0s_hypertest", hart, base_stream.label);
    lines.delete();
    emit({"# HYPERTEST_INJECTION: native ", boundary_x ?
          "ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_corner" :
          "ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_x_deny_corner",
          " ", sym("injection_begin"), " ", sym("injection_pass")});
    emit($sformatf("# hypertest Sv48 canonical boundary, boundary_X=%0d", boundary_x));
    emit(".option push");
    emit(".option norvc");
    emit(".option norelax");
    emit({sym("injection_begin"), ": nop"});
    // cfg.sp is reserved by the generator but is not necessarily architectural
    // x2. Save every GPR before using fixed registers in the source scenario.
    emit($sformatf("addi x%0d, x%0d, -384", cfg.sp, cfg.sp));
    for (int i = 1; i < 32; i++) begin
      emit($sformatf("sd x%0d, %0d(x%0d)", i, i * 8, cfg.sp));
    end
    emit($sformatf("mv x31, x%0d", cfg.sp));
    foreach (csrs[i]) begin
      emit({"csrr x5, ", csrs[i]});
      emit($sformatf("sd x5, %0d(x31)", 256 + 8 * i));
    end
    // Mask asynchronous interrupts and remove translation-affecting status
    // fields during the isolated S-mode visit. Restore the saved value later.
    emit("li x5, 0x2198a # MPRV, MPP, SPP, MPIE, MIE, SIE");
    emit("csrc mstatus, x5");
    emit("li x5, 0x100000 # TVM");
    emit("csrc mstatus, x5");
    emit("csrw medeleg, zero");
    emit("csrw satp, zero");
    emit("sfence.vma x0, x0");
    emit("sd zero, 328(x31) # precise IPF count");
    write_pte("root", 0, "low_l2", 1);
    emit({"lla x5, ", sym("low_l2")});
    for (int i = 0; i < 4; i++) begin
      emit($sformatf("li x6, 0x%0x", (64'(i) << 28) | 64'hcf));
      emit($sformatf("sd x6, %0d(x5)", i * 8));
    end
    write_pte("root", 255, "high_l2", 1);
    write_pte("high_l2", 511, "high_l1", 1);
    write_pte("high_l1", 511, "high_l0", 1);
    write_pte("high_l0", 511, "tail_page", 'hcf);
    write_pte("root", 256, "boundary_l2", 1);
    write_pte("boundary_l2", 0, "boundary_l1", 1);
    write_pte("boundary_l1", 0, "boundary_l0", 1);
    write_pte("boundary_l0", 0, "alias_page", boundary_x ? 'hcf : 'hc7);
    emit("fence rw, rw");
    emit("fence.i");
    emit({"lla x5, ", sym("handler")});
    emit("csrw mtvec, x5");
    emit({"lla x5, ", sym("root")});
    emit("srli x5, x5, 12");
    emit("li x6, 0x9000000000000000");
    emit("or x5, x5, x6");
    emit("csrw satp, x5");
    emit("sfence.vma x0, x0");
    probe("cold", "0x00007ffffffffff0", 1, 113, 49, 1);
    probe("warm", "0x00007ffffffffff0", 10, 122, 58, 1);
    // Do not pre-warm the alias before the cold-fetch boundary test.
    if (boundary_x) probe("alias", "0xffff800000000000", 'h55, 'h55, 'h55, 0);
    emit("ld x6, 328(x31)");
    emit("li x5, 2");
    emit({"bne x6, x5, ", sym("fail")});
    emit({"j ", sym("restore")});

    emit(".balign 4");
    emit({sym("s_entry"), ":"});
    emit("jalr ra, 0(x7)");
    emit({sym("s_return"), ":"});
    emit("ecall");
    emit({"j ", sym("fail")});
    emit(".balign 4");
    emit({sym("handler"), ":"});
    emit("csrr x5, mcause");
    emit("ld x6, 344(x31)");
    emit({"bne x5, x6, ", sym("panic")});
    emit("li x6, 12");
    emit({"bne x5, x6, ", sym("check_ecall")});
    emit("li x6, 0x0000800000000000");
    emit("csrr x5, mepc");
    emit({"bne x5, x6, ", sym("panic")});
    emit("csrr x5, mtval");
    emit({"bne x5, x6, ", sym("panic")});
    emit("ld x5, 328(x31)");
    emit("addi x5, x5, 1");
    emit("sd x5, 328(x31)");
    emit({"j ", sym("trap_resume")});
    emit({sym("check_ecall"), ":"});
    emit("csrr x5, mepc");
    emit({"lla x6, ", sym("s_return")});
    emit({"bne x5, x6, ", sym("panic")});
    emit({sym("trap_resume"), ":"});
    emit("li x5, 0x1800 # return directly to the M-mode checker");
    emit("csrs mstatus, x5");
    emit("ld x5, 336(x31)");
    emit("csrw mepc, x5");
    emit("mret");
    emit({sym("fail"), ":"});
    // The unexpected illegal trap makes strict Spike checking fail as well as
    // the HTIF status. A DUT that ignores the canonical check cannot pass.
    emit(".word 0");
    emit({sym("panic"), ":"});
    emit("li gp, 3");
    emit("lla x5, tohost");
    emit("sd gp, 0(x5)");
    emit({"j ", sym("panic")});

    emit({sym("restore"), ":"});
    // Restore translation first, then the original trap context and status.
    foreach (csrs[i]) begin
      if (i == 0) continue;
      emit($sformatf("ld x5, %0d(x31)", 256 + 8 * i));
      emit({"csrw ", csrs[i], ", x5"});
      if (csrs[i] == "satp") emit("sfence.vma x0, x0");
    end
    emit("ld x5, 256(x31)");
    emit("csrw mstatus, x5");
    for (int i = 1; i < 31; i++) emit($sformatf("ld x%0d, %0d(x31)", i, i * 8));
    emit("ld x31, 248(x31)");
    emit($sformatf("addi x%0d, x%0d, 384", cfg.sp, cfg.sp));
    emit({"j ", sym("end")});

    // Read-only executable payload; the 32B block contains no CFI instruction.
    emit(".pushsection .text.hypertest,\"ax\",@progbits");
    emit(".balign 4096");
    emit({sym("tail_page"), ":"});
    emit(".zero 4064");
    emit("addi a0, a0, 1");
    emit("addi a0, a0, 2");
    emit("addi a0, a0, 4");
    emit("addi a0, a0, 8");
    emit("addi a0, a0, 16");
    emit("addi a0, a0, 32");
    emit("sd a0, 0(a1)");
    emit("addi a0, a0, 64");
    emit(".balign 4096");
    emit({sym("alias_page"), ":"});
    emit("sd a0, 0(a1)");
    emit(".rept 7");
    emit("nop");
    emit(".endr");
    emit("ret");
    emit(".balign 4096");
    emit(".popsection");
    emit(".pushsection .data,\"aw\",@progbits");
    foreach (tables[i]) begin
      emit(".balign 4096");
      emit({sym(tables[i]), ":"});
      emit(".zero 4096");
    end
    emit(".popsection");
    emit({sym("end"), ":"});
    emit({sym("injection_pass"), ": nop"});
    emit(".option pop");
    block_instr = riscv_hypertest_asm_instr::type_id::create("block_instr");
    foreach (lines[i]) block_instr.assembly = {block_instr.assembly, lines[i], "\n"};
    instr_list = {block_instr};
    super.post_randomize();
  endfunction
endclass

// Source-preserving port for the rest of the catalog. The GNU stage resolves
// HYPERTEST_CASE markers and links the original C/assembly and the DV bridge.
// Arbitrary original cases may write executable fixtures or alter PMP; they
// cannot inherit the bounded-store proof of the two native templates above.
class riscv_hypertest_case_stream extends riscv_directed_instr_stream;
  string case_name;
  `uvm_object_utils(riscv_hypertest_case_stream)
  `uvm_object_new

  function void post_randomize();
    riscv_hypertest_asm_instr block_instr;
    riscv_instr_stream base_stream;
    string tag;
    string text;
    if (!cfg.enable_random_hypertest || case_name == "" ||
        (cfg.init_privileged_mode != MACHINE_MODE) || (XLEN != 64) || kernel_mode ||
        cfg.bare_program_mode || (cfg.num_of_harts != 1) || cfg.mstatus_mprv ||
        cfg.pmp_cfg.enable_random_pmp_exception || cfg.restrict_store_to_data) begin
      `uvm_fatal(`gfn, "Source hypertest streams require an enabled RV64 M-mode single-hart bridge, MPRV=0, and restrict_store_to_data=0")
    end
    base_stream = this;
    tag = $sformatf("h%0d_%0s_hypertest", hart, base_stream.label);
    text = {"# HYPERTEST_CASE: ", case_name, "\n",
            "# HYPERTEST_INJECTION: source ", case_name, " ", tag,
            "_injection_begin ", tag, "_injection_pass\n",
            ".option push\n.option norvc\n.option norelax\n",
            tag, "_injection_begin: nop\n"};
    text = {text, $sformatf("addi x%0d, x%0d, -384\n", cfg.sp, cfg.sp)};
    for (int i = 1; i < 32; i++) begin
      text = {text, $sformatf("sd x%0d, %0d(x%0d)\n", i, i * 8, cfg.sp)};
    end
    text = {text, $sformatf("mv x31, x%0d\n", cfg.sp),
            "lla x5, dv_hypertest_outer_frame\nsd x31, 0(x5)\n",
            "lla sp, dv_hypertest_stack_top\nlla a0, ", case_name,
            "\ncall dv_hypertest_call\n",
            "lla x31, dv_hypertest_outer_frame\nld x31, 0(x31)\n"};
    for (int i = 1; i < 31; i++) begin
      text = {text, $sformatf("ld x%0d, %0d(x31)\n", i, i * 8)};
    end
    text = {text, "ld x31, 248(x31)\n",
            $sformatf("addi x%0d, x%0d, 384\n", cfg.sp, cfg.sp),
            tag, "_injection_pass: nop\n.option pop\n"};
    block_instr = riscv_hypertest_asm_instr::type_id::create("block_instr");
    block_instr.assembly = text;
    instr_list = {block_instr};
    super.post_randomize();
  endfunction
endclass

class riscv_hypertest_sv48_canonical_x_deny_stream extends riscv_hypertest_sv48_canonical_stream;
  `uvm_object_utils(riscv_hypertest_sv48_canonical_x_deny_stream)
  function new(string name = "");
    super.new(name);
    boundary_x = 0;
  endfunction
endclass
