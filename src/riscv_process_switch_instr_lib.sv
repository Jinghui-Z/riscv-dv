// Native process-switch checks. All executable bytes are static; runtime stores
// touch only private page tables, task context/data and the reserved DV stack.
class riscv_process_switch_asm_instr extends riscv_instr;
  string assembly;
  `uvm_object_utils(riscv_process_switch_asm_instr)
  `uvm_object_new

  virtual function string convert2asm(string prefix = "");
    return {prefix, assembly};
  endfunction
endclass

class riscv_process_switch_stream extends riscv_directed_instr_stream;
  string lines[$];
  string tag;
  // The existing +directed_instr_N=name,ratio selects insertion frequency.
  // One stream exercises two A/B/A context-switch rounds by default.
  localparam int PROCESS_SWITCH_ROUNDS = 2;
  // All three aliases use one 4 KiB L0 table. Both roots map the loaded
  // 0x80000000 RAM window identically with a supervisor 1 GiB leaf.
  localparam longint CODE_VA = 'h400000;
  localparam longint DATA_VA = 'h401000;
  localparam longint GLOBAL_VA = 'h402000;
  localparam int FRAME_SIZE = 512;
  localparam int RESUME = 352;
  localparam int EXPECT_CAUSE = 360;
  localparam int EXPECT_EPC = 368;
  localparam int EXPECT_TVAL = 376;
  localparam int ASID_MASK = 384;
  localparam int ASID_A = 392;
  localparam int ASID_B = 400;
  localparam int ASID_ZERO = 408;
  localparam int EXPECT_CONTINUATION = 416;
  localparam int STORE_OFFSET = 20;
  localparam int ECALL_OFFSET = 84;

  `uvm_object_utils(riscv_process_switch_stream)
  `uvm_object_new

  function void emit(string line);
    lines.push_back(line);
  endfunction

  function string sym(string suffix);
    return {tag, "_", suffix};
  endfunction

  function void scenario(string name);
    emit({"# PROCESS_SWITCH_SCENARIO: ", name, " ", sym(name)});
    emit({sym(name), ": nop"});
  endfunction

  // x5=table, x6=PTE; all selected entries have imm12-compatible offsets.
  function void pte(string table_name, int index, string target, int flags);
    emit({"lla x5, ", sym(table_name)});
    emit({"lla x6, ", sym(target)});
    emit("srli x6, x6, 2");
    emit($sformatf("ori x6, x6, %0d", flags));
    emit($sformatf("sd x6, %0d(x5)", index * 8));
  endfunction

  function void scoped_fence(longint va, int asid_slot);
    emit($sformatf("li x5, 0x%0x", va));
    emit($sformatf("ld x6, %0d(x31)", asid_slot));
    emit("sfence.vma x5, x6");
  endfunction

  function void build_root(string task_name);
    if (SATP_MODE == SV48) begin
      pte({task_name, "_root"}, 0, {task_name, "_l2"}, 1);
      pte({task_name, "_l2"}, 0, {task_name, "_l1"}, 1);
      emit({"lla x5, ", sym({task_name, "_l2"})});
    end else begin
      pte({task_name, "_root"}, 0, {task_name, "_l1"}, 1);
      emit({"lla x5, ", sym({task_name, "_root"})});
    end
    emit("li x6, 0x200000cf # PA=0x80000000, 1 GiB RWX leaf, A/D=1");
    emit("sd x6, 16(x5)");
    pte({task_name, "_l1"}, 2, {task_name, "_l0"}, 1);
    pte({task_name, "_l0"}, 0, {"code_", task_name}, 'hcb);
    pte({task_name, "_l0"}, 1, {"data_", task_name}, 'hc7);
    pte({task_name, "_l0"}, 2, "global_old", 'he7);
  endfunction

  function void install_root(string task_name, int asid_slot, int fence_kind);
    string nofence_label;
    // The scoped-remap/permission checks already run with this root and tag.
    // Do not rewrite SATP: an implementation may flush on every SATP write,
    // which would hide a broken preceding targeted SFENCE.VMA.
    if (fence_kind == 3) return;
    emit({"lla x5, ", sym({task_name, "_root"})});
    emit("srli x5, x5, 12");
    emit($sformatf("ld x6, %0d(x31)", asid_slot));
    emit("slli x6, x6, 44");
    emit("or x5, x5, x6");
    emit($sformatf("li x6, 0x%0x", SATP_MODE == SV48 ?
                   64'h9000000000000000 : 64'h8000000000000000));
    emit("or x5, x5, x6");
    emit("csrw satp, x5");
    if (fence_kind == 2) begin
      emit("sfence.vma x0, x0");
    end else if (fence_kind == 1) begin
      emit($sformatf("ld x6, %0d(x31)", asid_slot));
      emit("sfence.vma x0, x6");
    end else if (fence_kind == 0) begin
      // An implementation with ASIDLEN=0 needs a fence whenever roots change.
      emit($sformatf("ld x6, %0d(x31)", ASID_MASK));
      nofence_label = sym({task_name, "_nofence_", $sformatf("%0d", lines.size())});
      emit({"bnez x6, ", nofence_label});
      emit("sfence.vma x0, x0");
      emit({nofence_label, ": nop"});
    end
  endfunction

  function void init_context(string task_name, int base_value);
    emit({"lla x30, ", sym({"ctx_", task_name})});
    // Context: ra/sp/tp/s0/s1, sscratch, sepc, stvec, SUM/MXR, successful
    // yields, and the task's saved resume PC.
    for (int i = 0; i < 7; i++) begin
      emit($sformatf("li x5, 0x%0x", base_value + i * 16));
      emit($sformatf("sd x5, %0d(x30)", i * 8));
    end
    emit({"lla x5, ", sym({"code_", task_name})});
    emit("sd x5, 56(x30)");
    emit($sformatf("li x5, 0x%0x", task_name == "a" ? 'h80000 : 'h40000));
    emit("sd x5, 64(x30)");
    emit("sd zero, 72(x30)");
    emit($sformatf("li x5, 0x%0x", CODE_VA));
    emit("sd x5, 80(x30)");
    emit({"lla x5, ", sym({"data_", task_name})});
    emit("sd zero, 0(x5)");
  endfunction

  // A private M-mode ECALL scheduler saves the selected task context at each
  // yield, then unrolled code restores another task. An unexpected/missing
  // fault or an incorrect retired task marker reaches the common failure path.
  function void visit(string task_name, string suffix, int asid_slot,
                      int fence_kind, int expected_marker, string data_page,
                      int expected_data, int expected_global, int fault_cause = 0);
    string regs[$] = '{"ra", "sp", "tp", "s0", "s1"};
    install_root(task_name, asid_slot, fence_kind);
    emit({"lla x30, ", sym({"ctx_", task_name})});
    emit({"lla x5, ", sym({suffix, "_resume"})});
    emit($sformatf("sd x5, %0d(x31)", RESUME));
    emit($sformatf("li x5, %0d", fault_cause == 0 ? 9 : fault_cause));
    emit($sformatf("sd x5, %0d(x31)", EXPECT_CAUSE));
    if (fault_cause == 12) emit("ld x5, 80(x30)");
    else emit($sformatf("li x5, 0x%0x", CODE_VA +
                        (fault_cause == 15 ? STORE_OFFSET : ECALL_OFFSET)));
    emit($sformatf("sd x5, %0d(x31)", EXPECT_EPC));
    if (fault_cause == 12) emit("ld x5, 80(x30)");
    else emit($sformatf("li x5, 0x%0x", DATA_VA));
    emit($sformatf("sd x5, %0d(x31)", EXPECT_TVAL));
    emit("li x5, 0xc198a # MPP, MPIE, SIE, MIE, SUM, MXR");
    emit("csrc mstatus, x5");
    emit("ld x5, 64(x30)");
    emit("li x6, 0x800 # MPP=S");
    emit("or x5, x5, x6");
    emit("csrs mstatus, x5");
    emit("ld x5, 40(x30)");
    emit("csrw sscratch, x5");
    emit("ld x5, 48(x30)");
    emit("csrw sepc, x5");
    emit("ld x5, 56(x30)");
    emit("csrw stvec, x5");
    foreach (regs[i]) emit($sformatf("ld %0s, %0d(x30)", regs[i], i * 8));
    // First dispatch starts at the body. A yielded task resumes after its
    // ECALL, where a continuation marker runs before branching to the body.
    // Check the saved PC against an independent yield count, so corruption
    // back to the body entry cannot silently pass the data/register checks.
    emit("ld x6, 72(x30)");
    emit("snez x6, x6");
    emit($sformatf("sd x6, %0d(x31)", EXPECT_CONTINUATION));
    emit($sformatf("li x5, 0x%0x", CODE_VA));
    emit({"beqz x6, ", sym({suffix, "_first_dispatch"})});
    emit($sformatf("addi x5, x5, %0d", ECALL_OFFSET + 4));
    emit({sym({suffix, "_first_dispatch"}), ": nop"});
    emit("ld x7, 80(x30)");
    emit({"bne x5, x7, ", sym("fail")});
    emit("csrw mepc, x7");
    emit("li a2, 0");
    emit("mret");
    emit({sym({suffix, "_resume"}), ": nop"});
    if (fault_cause == 12) emit("li x5, 0");
    else emit($sformatf("ld x5, %0d(x31)", EXPECT_CONTINUATION));
    emit({"bne a2, x5, ", sym("fail")});
    if (fault_cause == 0) begin
      emit($sformatf("li x5, %0d", expected_marker));
      emit({"bne a0, x5, ", sym("fail")});
      emit($sformatf("li x5, %0d", expected_global));
      emit({"bne a1, x5, ", sym("fail")});
    end
    foreach (regs[i]) begin
      emit($sformatf("ld x5, %0d(x30)", i * 8));
      if (fault_cause == 0) emit($sformatf("addi x5, x5, %0d", i == 1 ? 16 : 1));
      emit({"bne ", regs[i], ", x5, ", sym("fail")});
      emit($sformatf("sd %0s, %0d(x30)", regs[i], i * 8));
    end
    emit("csrr x6, sscratch");
    emit("ld x5, 40(x30)");
    if (fault_cause == 0) emit("addi x5, x5, 1");
    emit({"bne x6, x5, ", sym("fail")});
    emit("sd x6, 40(x30)");
    emit("csrr x6, sepc");
    emit("ld x5, 48(x30)");
    if (fault_cause == 0) emit("addi x5, x5, 4");
    emit({"bne x6, x5, ", sym("fail")});
    emit("sd x6, 48(x30)");
    emit("csrr x6, stvec");
    emit("ld x5, 56(x30)");
    emit({"bne x6, x5, ", sym("fail")});
    emit("csrr x6, sstatus");
    emit("li x5, 0xc0000 # SUM/MXR");
    emit("and x6, x6, x5");
    emit("ld x5, 64(x30)");
    emit({"bne x6, x5, ", sym("fail")});
    emit({"lla x5, ", sym(data_page)});
    emit("ld x6, 0(x5)");
    emit($sformatf("li x5, %0d", expected_data));
    emit({"bne x6, x5, ", sym("fail")});
    if (fault_cause == 0) begin
      emit("ld x5, 72(x30)");
      emit("addi x5, x5, 1");
      emit("sd x5, 72(x30)");
    end
  endfunction

  function void code_page(string task_name, int marker);
    emit(".balign 4096");
    emit({sym({"code_", task_name}), ":"});
    emit($sformatf("li t0, 0x%0x", GLOBAL_VA));
    emit("ld a1, 0(t0)");
    emit($sformatf("li t0, 0x%0x", DATA_VA));
    emit("ld t1, 0(t0)");
    emit("addi t1, t1, 1");
    emit("sd t1, 0(t0)");
    emit($sformatf("li a0, %0d", marker));
    emit("addi ra, ra, 1");
    emit("addi sp, sp, 16");
    emit("addi tp, tp, 1");
    emit("addi s0, s0, 1");
    emit("addi s1, s1, 1");
    emit("csrr t2, sscratch");
    emit("addi t2, t2, 1");
    emit("csrw sscratch, t2");
    emit("csrr t2, sepc");
    emit("addi t2, t2, 4");
    emit("csrw sepc, t2");
    emit("nop");
    emit("nop");
    emit("nop");
    emit("ecall");
    emit("addi a2, a2, 1 # saved-PC continuation executes once after dispatch");
    emit({"j ", sym({"code_", task_name})});
  endfunction

  function void post_randomize();
    riscv_process_switch_asm_instr block_instr;
    riscv_instr_stream base_stream;
    string csrs[$] = '{"mstatus", "medeleg", "mideleg", "satp", "mtvec", "mepc",
                      "mcause", "mtval", "sscratch", "stvec", "sepc"};
    string tasks[$] = '{"a", "b"};
    string tables[$] = '{"root", "l2", "l1", "l0"};
    string pages[$] = '{"data_a", "data_b", "data_recycled", "global_old", "global_new"};
    int a_count = 0;
    int b_count = 0;
    if ((XLEN != 64) || !(SATP_MODE inside {SV39, SV48}) ||
        !(SUPERVISOR_MODE inside {supported_privileged_mode}) ||
        (cfg.init_privileged_mode != MACHINE_MODE) || cfg.enable_random_boot_mode ||
        kernel_mode ||
        cfg.bare_program_mode || (cfg.num_of_harts != 1) || cfg.mstatus_mprv ||
        cfg.set_mstatus_mprv || cfg.pmp_cfg.enable_random_pmp_exception ||
        cfg.enable_interrupt || cfg.enable_nanhu_nmi_handler) begin
      `uvm_fatal(`gfn, "Process switch stream requires RV64 Sv39/Sv48 single-hart fixed M boot with S-mode, MPRV=0 and no async/PMP exception injection")
    end
    base_stream = this;
    tag = $sformatf("h%0d_%0s_process", hart, base_stream.label);
    lines.delete();
    emit({"# PROCESS_SWITCH_INJECTION: ", sym("injection_begin"), " ", sym("injection_pass")});
    emit({"# PROCESS_SWITCH_FAIL: ", sym("fail")});
    emit(".option push");
    emit(".option norvc");
    emit(".option norelax");
    emit({sym("injection_begin"), ": nop"});
    emit($sformatf("addi x%0d, x%0d, -%0d", cfg.sp, cfg.sp, FRAME_SIZE));
    for (int i = 1; i < 32; i++)
      emit($sformatf("sd x%0d, %0d(x%0d)", i, i * 8, cfg.sp));
    emit($sformatf("mv x31, x%0d", cfg.sp));
    foreach (csrs[i]) begin
      emit({"csrr x5, ", csrs[i]});
      emit($sformatf("sd x5, %0d(x31)", 256 + i * 8));
    end
    emit("li x5, 0x7e198a # TVM/TW/TSR/MPRV, MPP, MPIE, MIE, SIE, SUM/MXR");
    emit("csrc mstatus, x5");
    emit("csrw medeleg, zero");
    emit("csrw mideleg, zero");
    emit({"lla x5, ", sym("handler")});
    emit("csrw mtvec, x5");
    build_root("a");
    build_root("b");
    init_context("a", 'h1000);
    init_context("b", 'h2000);
    emit({"lla x5, ", sym("data_recycled")});
    emit("sd zero, 0(x5)");
    emit({"lla x5, ", sym("global_old")});
    emit("li x6, 49");
    emit("sd x6, 0(x5)");
    emit({"lla x5, ", sym("global_new")});
    emit("li x6, 98");
    emit("sd x6, 0(x5)");
    emit("fence rw, rw");
    // Probe the implemented low ASID bits instead of assuming a fixed ASIDLEN.
    emit({"lla x5, ", sym("a_root")});
    emit("srli x5, x5, 12");
    emit($sformatf("li x6, 0x%0x", SATP_MODE == SV48 ?
                   64'h9ffff00000000000 : 64'h8ffff00000000000));
    emit("or x5, x5, x6");
    emit("csrw satp, x5");
    emit("csrr x5, satp");
    emit("srli x5, x5, 44");
    emit("li x6, 0xffff");
    emit("and x5, x5, x6");
    emit($sformatf("sd x5, %0d(x31)", ASID_MASK));
    emit("andi x6, x5, 1");
    emit($sformatf("sd x6, %0d(x31)", ASID_A));
    emit("andi x6, x5, 2");
    emit($sformatf("sd x6, %0d(x31)", ASID_B));
    emit($sformatf("sd zero, %0d(x31)", ASID_ZERO));
    emit("sfence.vma x0, x0");
    emit($sformatf("ld x5, %0d(x31)", ASID_MASK));
    emit({"bnez x5, ", sym("asid_supported")});
    emit({sym("asidlen_zero_fallback"), ": nop"});
    emit({"j ", sym("asid_path_ready")});
    emit({sym("asid_supported"), ": nop"});
    emit({sym("asid_path_ready"), ": nop"});
    for (int r = 0; r < PROCESS_SWITCH_ROUNDS; r++) begin
      scenario($sformatf("asid_context_round_%0d", r));
      a_count++;
      visit("a", $sformatf("a_first_%0d", r), ASID_A, 0, 17, "data_a", a_count, 49);
      b_count++;
      visit("b", $sformatf("b_middle_%0d", r), ASID_B, 0, 34, "data_b", b_count, 49);
      a_count++;
      visit("a", $sformatf("a_return_%0d", r), ASID_A, 0, 17, "data_a", a_count, 49);
    end
    scenario("same_asid_root_recycle");
    b_count++;
    visit("b", "recycle_to_b", ASID_A, 1, 34, "data_b", b_count, 49);
    a_count++;
    visit("a", "recycle_to_a", ASID_A, 1, 17, "data_a", a_count, 49);
    scenario("asid_zero_root_recycle");
    b_count++;
    visit("b", "zero_recycle_to_b", ASID_ZERO, 1, 34, "data_b", b_count, 49);
    a_count++;
    visit("a", "zero_recycle_to_a", ASID_ZERO, 1, 17, "data_a", a_count, 49);
    a_count++;
    visit("a", "targeted_warmup", ASID_A, 0, 17, "data_a", a_count, 49);
    scenario("targeted_code_data_remap");
    pte("a_l0", 0, "code_b", 'hcb);
    pte("a_l0", 1, "data_recycled", 'hc7);
    scoped_fence(CODE_VA, ASID_A);
    scoped_fence(DATA_VA, ASID_A);
    visit("a", "remap_to_b_code", ASID_A, 3, 34, "data_recycled", 1, 49);
    pte("a_l0", 0, "code_a", 'hcb);
    pte("a_l0", 1, "data_a", 'hc7);
    scoped_fence(CODE_VA, ASID_A);
    scoped_fence(DATA_VA, ASID_A);
    a_count++;
    visit("a", "remap_back", ASID_A, 3, 17, "data_a", a_count, 49);
    scenario("global_mapping_remap");
    pte("a_l0", 2, "global_new", 'he7);
    pte("b_l0", 2, "global_new", 'he7);
    emit($sformatf("li x5, 0x%0x", GLOBAL_VA));
    emit("sfence.vma x5, x0 # a global translation needs rs2=x0");
    a_count++;
    visit("a", "global_a", ASID_A, 3, 17, "data_a", a_count, 98);
    b_count++;
    // With ASIDLEN=1, B's ASID is zero and the zero-ASID test last associated
    // that ID with root A. Recycle it explicitly; this excludes G entries.
    visit("b", "global_b", ASID_B, 1, 34, "data_b", b_count, 98);
    // Warm A before permission revocation. Later visits retain the same root
    // even with ASIDLEN=0, so an extra full fence cannot mask a broken scoped
    // permission invalidation.
    a_count++;
    visit("a", "permission_warmup", ASID_A, 0, 17, "data_a", a_count, 98);
    scenario("write_permission_revoke");
    pte("a_l0", 1, "data_a", 'hc3);
    scoped_fence(DATA_VA, ASID_A);
    visit("a", "write_fault", ASID_A, 3, 17, "data_a", a_count, 98, 15);
    pte("a_l0", 1, "data_a", 'hc7);
    scoped_fence(DATA_VA, ASID_A);
    a_count++;
    visit("a", "write_restored", ASID_A, 3, 17, "data_a", a_count, 98);
    scenario("execute_permission_revoke");
    pte("a_l0", 0, "code_a", 'hc3);
    scoped_fence(CODE_VA, ASID_A);
    visit("a", "execute_fault", ASID_A, 3, 17, "data_a", a_count, 98, 12);
    pte("a_l0", 0, "code_a", 'hcb);
    scoped_fence(CODE_VA, ASID_A);
    a_count++;
    visit("a", "execute_restored", ASID_A, 3, 17, "data_a", a_count, 98);
    emit({"j ", sym("restore")});

    emit(".balign 4");
    emit({sym("handler"), ":"});
    emit("csrr x5, mcause");
    emit($sformatf("ld x6, %0d(x31)", EXPECT_CAUSE));
    emit({"bne x5, x6, ", sym("fail")});
    emit("csrr x5, mepc");
    emit($sformatf("ld x6, %0d(x31)", EXPECT_EPC));
    emit({"bne x5, x6, ", sym("fail")});
    emit($sformatf("ld x5, %0d(x31)", EXPECT_CAUSE));
    emit("li x6, 9");
    emit({"bne x5, x6, ", sym("check_fault_tval")});
    emit("csrr x5, mepc");
    emit("addi x5, x5, 4");
    emit("sd x5, 80(x30) # save task resume PC after the yielded ECALL");
    emit({"j ", sym("trap_resume")});
    emit({sym("check_fault_tval"), ":"});
    emit("csrr x5, mtval");
    emit($sformatf("ld x6, %0d(x31)", EXPECT_TVAL));
    emit({"bne x5, x6, ", sym("fail")});
    emit({sym("trap_resume"), ":"});
    emit($sformatf("ld x5, %0d(x31)", RESUME));
    emit("jr x5 # remain in M-mode scheduler after the S-mode yield/fault");
    emit({sym("fail"), ": nop"});
    emit("li gp, 3");
    emit("lla x5, tohost");
    emit("sd gp, 0(x5)");
    emit(".word 0 # strict Spike checking rejects a self-check failure");
    emit({"j ", sym("fail")});
    emit({sym("restore"), ":"});
    foreach (csrs[i]) begin
      if (i == 0) continue;
      emit($sformatf("ld x5, %0d(x31)", 256 + i * 8));
      emit({"csrw ", csrs[i], ", x5"});
      if (csrs[i] == "satp") emit("sfence.vma x0, x0");
    end
    emit("ld x5, 256(x31)");
    emit("csrw mstatus, x5");
    for (int i = 1; i < 31; i++) emit($sformatf("ld x%0d, %0d(x31)", i, i * 8));
    emit("ld x31, 248(x31)");
    emit($sformatf("addi x%0d, x%0d, %0d", cfg.sp, cfg.sp, FRAME_SIZE));
    emit({"j ", sym("end")});

    emit(".pushsection .text.process_switch,\"ax\",@progbits");
    code_page("a", 17);
    code_page("b", 34);
    emit(".balign 4096");
    emit(".popsection");
    emit(".pushsection .data,\"aw\",@progbits");
    foreach (tasks[i]) begin
      foreach (tables[j]) begin
        if ((SATP_MODE == SV39) && (tables[j] == "l2")) continue;
        emit(".balign 4096");
        emit({sym({tasks[i], "_", tables[j]}), ": .zero 4096"});
      end
      emit(".balign 8");
      emit({sym({"ctx_", tasks[i]}), ": .zero 88"});
    end
    foreach (pages[i]) begin
      emit(".balign 4096");
      emit({sym(pages[i]), ": .zero 4096"});
    end
    emit(".popsection");
    emit({sym("end"), ":"});
    emit({sym("injection_pass"), ": nop"});
    emit(".option pop");
    block_instr = riscv_process_switch_asm_instr::type_id::create("block_instr");
    foreach (lines[i]) block_instr.assembly = {block_instr.assembly, lines[i], "\n"};
    instr_list = {block_instr};
    super.post_randomize();
  endfunction
endclass
