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

//-----------------------------------------------------------------------------
// RISC-V assembly program generator configuration class
//-----------------------------------------------------------------------------

class riscv_instr_gen_config extends uvm_object;

  //-----------------------------------------------------------------------------
  // Random instruction generation settings
  //-----------------------------------------------------------------------------

  // Instruction count of the main program
  rand int               main_program_instr_cnt;

  // Instruction count of each sub-program
  rand int               sub_program_instr_cnt[];

  // Instruction count of the debug rom
  rand int               debug_program_instr_cnt;

  // Instruction count of debug sub-programs
  rand int               debug_sub_program_instr_cnt[];

  // Pattern of data section: RAND_DATA, ALL_ZERO, INCR_VAL
  rand data_pattern_t    data_page_pattern;

  // Initialization of the vregs
  // SAME_VALUES_ALL_ELEMS - Using vmv.v.x to fill all the elements of the vreg with the same value as the one in the GPR selected
  // RANDOM_VALUES_VMV     - Using vmv.v.x + vslide1up.vx to randomize the contents of each vector element
  // RANDOM_VALUES_LOAD    - Using vle.v, same approach as RANDOM_VALUES_VMV but more efficient for big VLEN
  vreg_init_method_t     vreg_init_method = RANDOM_VALUES_VMV;

  // Associate array for delegation configuration for each exception and interrupt
  // When the bit is 1, the corresponding delegation is enabled.
  rand bit               m_mode_exception_delegation[exception_cause_t];
  rand bit               s_mode_exception_delegation[exception_cause_t];
  rand bit               m_mode_interrupt_delegation[interrupt_cause_t];
  rand bit               s_mode_interrupt_delegation[interrupt_cause_t];

  // Priviledged mode after boot
  rand privileged_mode_t init_privileged_mode;

  rand bit[XLEN-1:0]     mstatus, mie,
                         sstatus, sie,
                         ustatus, uie;

  // Key fields in xSTATUS
  // Memory protection bits
  rand bit               mstatus_mprv;
  rand bit               mstatus_mxr;
  rand bit               mstatus_sum;
  rand bit               mstatus_tvm;
  rand bit [1:0]         mstatus_fs;
  rand bit [1:0]         mstatus_vs;
  rand mtvec_mode_t      mtvec_mode;

  // TVEC alignment
  // This value is the log_2 of the byte-alignment of TVEC.BASE field
  // As per RISC-V privileged spec, default will be set to 2 (4-byte aligned)
  rand int tvec_alignment = 2;

  // Floating point rounding mode
  rand f_rounding_mode_t fcsr_rm;

  // Enable sfence.vma instruction
  rand bit               enable_sfence;

  // Reserved register
  // Reserved for various hardcoded routines
  rand riscv_reg_t       gpr[4];
  // Used by any DCSR operations inside of the debug rom.
  // Also used by the PMP generation.
  rand riscv_reg_t       scratch_reg;
  // Reg used exclusively by the PMP exception handling routine.
  // Can overlap with the other GPRs used in the random generation,
  // as PMP exception handler is hardcoded and does not include any
  // random instructions.
  rand riscv_reg_t       pmp_reg[2];
  // Use a random register for stack pointer/thread pointer
  rand riscv_reg_t       sp;
  rand riscv_reg_t       tp;
  rand riscv_reg_t       ra;

  // Options for privileged mode CSR checking
  // Below checking can be made optional as the ISS implementation could be different with the
  // processor.
  bit                    check_misa_init_val = 1'b0;
  bit                    check_xstatus = 1'b1;

  // Virtual address translation is on for this test
  rand bit               virtual_addr_translation_on;

  // Vector extension setting
  rand riscv_vector_cfg  vector_cfg;

  // PMP configuration settings
  rand riscv_pmp_cfg pmp_cfg;

  //-----------------------------------------------------------------------------
  //  User space memory region and stack setting
  //-----------------------------------------------------------------------------

  mem_region_t mem_region[$] = '{
    '{name:"region_0", size_in_bytes: 4096,      xwr: 3'b111},
    '{name:"region_1", size_in_bytes: 4096 * 16, xwr: 3'b111}
  };

  // Dedicated shared memory region for multi-harts atomic operations
  mem_region_t amo_region[$] = '{
    '{name:"amo_0",    size_in_bytes: 64,        xwr: 3'b111}
  };

  // Stack section word length
  int stack_len = 5000;

  //-----------------------------------------------------------------------------
  // Kernel section setting, used by supervisor mode programs
  //-----------------------------------------------------------------------------

  mem_region_t s_mem_region[$] = '{
    '{name:"s_region_0", size_in_bytes: 4096, xwr: 3'b111},
    '{name:"s_region_1", size_in_bytes: 4096, xwr: 3'b111}};

  // Kernel Stack section word length
  int kernel_stack_len = 4000;

  // Number of instructions for each kernel program
  int kernel_program_instr_cnt = 400;

  // Queue of all the main implemented CSRs that the boot privilege mode cannot access
  // e.g. these CSRs are in higher privilege modes - access should raise an exception
  privileged_reg_t       invalid_priv_mode_csrs[$];

  //-----------------------------------------------------------------------------
  // Command line options or control knobs
  //-----------------------------------------------------------------------------
  // Main options for RISC-V assembly program generation
  // Number of sub-programs per test
  int                    num_of_sub_program = 5;
  int                    instr_cnt = 200;
  int                    num_of_tests = 1;
  // For tests doesn't involve load/store, the data section generation could be skipped
  bit                    no_data_page;
  // Options to turn off some specific types of instructions
  bit                    no_branch_jump;     // No branch/jump instruction
  bit                    no_load_store;      // No load/store instruction
  bit                    no_csr_instr;       // No csr instruction
  bit                    no_ebreak = 1;      // No ebreak instruction
  // Only enable ecall if you have overriden the test_done mechanism.
  bit                    no_ecall = 1;       // No ecall instruction
  bit                    no_dret = 1;        // No dret instruction
  bit                    no_fence;           // No fence instruction
  bit                    no_wfi = 1;         // No WFI instruction
  bit                    enable_unaligned_load_store;
  int                    illegal_instr_ratio;
  int                    hint_instr_ratio;
  // CSR instruction control
  bit                    gen_all_csrs_by_default = 0; // Generate CSR instructions that use all supported CSRs. Other options below only take effect if this is enabled.
  bit                    gen_csr_ro_write = 0;        // Generate CSR writes to read-only CSRs
  privileged_reg_t       add_csr_write[] = {};        // CSRs to add to the set of writeable CSRs
  privileged_reg_t       remove_csr_write[] = {};     // CSRs to remove from the set of writeable CSRs
  // Number of harts to be simulated, must be <= NUM_HARTS
  int                    num_of_harts = NUM_HARTS;
  // Use SP as stack pointer
  bit                    fix_sp;
  // Use push/pop section for data pages
  bit                    use_push_data_section = 0;
  // Directed boot privileged mode, u, m, s
  string                 boot_mode_opts;
  // Select the initial privilege mode randomly from the target-supported modes.
  // This is opt-in so existing testlists retain their historical M-mode default.
  bit                    enable_random_boot_mode = 1'b0;
  int                    enable_page_table_exception;
  // 0: legacy mixed page-table faults, 1: Svade A=0 only, 2: Svade D=0 only.
  int                    svade_fault_mode = 0;
  bit                    enable_svpbmt = 1'b0;
  bit                    no_directed_instr;
  // Randomly inject directed instruction streams into each generated sequence.
  // The existing +directed_instr_N=name,ratio options provide the weighted
  // candidate list; when no list is supplied, riscv_loop_instr is used.
  bit                    enable_random_directed_instr = 1'b0;
  int                    random_directed_instr_ratio = 4;
  // A name suffix for the generated assembly program
  string                 asm_test_suffix;
  // Enable interrupt bit in MSTATUS (MIE, SIE, UIE)
  bit                    enable_interrupt;
  bit                    enable_nested_interrupt;
  // We need a separate control knob for enabling timer interrupts, as Spike
  // throws an exception if xIE.xTIE is enabled
  bit                    enable_timer_irq;
  // Generate a bare program without any init/exit/error handling/page table routines
  // The generated program can be integrated with a larger program.
  // Note that the bare mode program is not expected to run in standalone mode
  bit                    bare_program_mode;
  // Enable accessing illegal CSR instruction
  // - Accessing non-existence CSR
  // - Accessing CSR with wrong privileged mode
  bit                    enable_illegal_csr_instruction;
  // Enable accessing CSRs at an invalid privilege level
  bit                    enable_access_invalid_csr_level;
  // Enable misaligned instruction (caused by JALR instruction)
  bit                    enable_misaligned_instr;
  // Enable some dummy writes to main system CSRs (xSTATUS/xIE) at beginning of test
  // to check repeated writes
  bit                    enable_dummy_csr_write;
  bit                    randomize_csr = 0;
  // sfence support
  bit                    allow_sfence_exception = 0;
  // Report xTVAL through the signature handshake for architectural checking
  // (Sstvala). Disabled by default to preserve existing testbench traffic.
  bit                    enable_tval_check = 1'b0;
  // Enable the Sscofpmf local-counter-overflow interrupt path.
  bit                    enable_sscofpmf = 1'b0;
  // Interrupt/Exception Delegation
  bit                    no_delegation = 1;
  bit                    force_m_delegation = 0;
  bit                    force_s_delegation = 0;
  bit                    support_supervisor_mode;
  bit                    disable_compressed_instr;
  // "Memory mapped" address that when written to will indicate some event to
  // the testbench - testbench will take action based on the value written
  bit [XLEN - 1 : 0]     signature_addr = 32'hdead_beef;
  bit                    require_signature_addr = 1'b0;
  // Enable a full or empty debug_rom section.
  // Full debug_rom will contain random instruction streams.
  // Empty debug_rom will contain just dret instruction and will return immediately.
  // Will be empty by default.
  bit                    gen_debug_section = 1'b0;
  // Enable generation of a directed sequence of instructions containing
  // ebreak inside the debug_rom.
  // Disabled by default.
  bit                    enable_ebreak_in_debug_rom = 1'b0;
  // Enable setting dcsr.ebreak(m/s/u)
  bit                    set_dcsr_ebreak = 1'b0;
  // Number of sub programs in the debug rom
  int                    num_debug_sub_program = 0;
  // Enable debug single stepping
  bit                    enable_debug_single_step = 0;
  // Number of single stepping iterations
  rand int               single_step_iterations;
  // Enable mstatus.tw bit - causes u-mode WFI to raise illegal instruction exceptions
  bit                    set_mstatus_tw;
  // Enable users to set mstatus.mprv to enable privilege checks on memory accesses.
  bit                    set_mstatus_mprv;
  // Keep SUM set when an S-mode trap handler uses U-accessible shared pages.
  bit                    set_mstatus_sum;
  // Stack space allocated to each program, need to be enough to store necessary context
  // Example: RA, SP, T0
  int                    min_stack_len_per_program = 10 * (XLEN/8);
  int                    max_stack_len_per_program = 16 * (XLEN/8);
  // Maximum branch distance, avoid skipping large portion of the code
  int                    max_branch_step = 20;
  // Maximum directed instruction stream sequence count
  int                    max_directed_instr_stream_seq = 20;
  // Reserved registers
  riscv_reg_t            reserved_regs[];
  // Floating point support
  bit                    enable_floating_point;
  // Vector extension support
  bit                    enable_vector_extension;
  // Select the ratified V 1.0 assembly syntax and instruction set.
  bit                    use_vector_1_0 = 1'b0;
  bit                    use_vsetivli = 1'b1;
  // Random VSET instructions can desynchronize the DUT vector state from
  // vector_cfg. Keep them out of normal streams; state-aware setup still
  // emits all supported VSET forms.
  bit                    enable_random_vset_instr = 1'b0;
  bit                    enable_zvbb_extension = 1'b0;
  // Only generate vector instructions
  bit                    vector_instr_only;
  // Bit manipulation extension support
  bit                    enable_b_extension;

  bit                    enable_zba_extension;
  bit                    enable_zbb_extension;
  bit                    enable_zbc_extension;
  bit                    enable_zbkc_extension;
  bit                    enable_zbs_extension;
  bit                    enable_zcb_extension;
  bit                    enable_svinval_extension = 1'b0;

  // Platform interrupt-controller integration. These are disabled by default
  // so existing targets retain their testbench handshake behavior.
  bit                    enable_aclint_init = 1'b0;
  bit [XLEN-1:0]         aclint_msip_addr = '0;
  int                    aclint_msip_stride = 4;
  bit [XLEN-1:0]         aclint_mtimecmp_addr = '0;
  int                    aclint_mtimecmp_stride = 8;
  bit                    enable_plic_claim_complete = 1'b0;
  // Per-privilege PLIC contexts. plic_claim_complete_addr is retained as a
  // backwards-compatible fallback for targets with a single context.
  bit [XLEN-1:0]         plic_claim_complete_addr = '0;
  bit [XLEN-1:0]         plic_m_claim_complete_addr = '0;
  bit [XLEN-1:0]         plic_s_claim_complete_addr = '0;
  int                    plic_hart_stride = 0;

  // Ratified scalar extensions without a MISA bit.
  bit                    enable_zicond_extension;
  bit                    enable_zimop_extension;
  bit                    enable_zcmop_extension;
  bit                    enable_zicbom_extension;
  bit                    enable_zicbop_extension;
  bit                    enable_zicboz_extension;
  bit                    enable_zbkb_extension;
  bit                    enable_zbkx_extension;
  bit                    enable_zknd_extension;
  bit                    enable_zkne_extension;
  bit                    enable_zknh_extension;
  bit                    enable_zksed_extension;
  bit                    enable_zksh_extension;
  bit                    enable_zkn_extension;
  bit                    enable_zks_extension;

  b_ext_group_t          enable_bitmanip_groups[] = {ZBB, ZBS, ZBP, ZBE, ZBF, ZBC, ZBR, ZBM, ZBT,
                                                     ZB_TMP};

  //-----------------------------------------------------------------------------
  // Command line options for instruction distribution control
  //-----------------------------------------------------------------------------
  int                    dist_control_mode;
  int unsigned           category_dist[riscv_instr_category_t];


  constraint default_c {
    sub_program_instr_cnt.size() == num_of_sub_program;
    debug_sub_program_instr_cnt.size() == num_debug_sub_program;
    main_program_instr_cnt inside {[10 : instr_cnt]};
    foreach(sub_program_instr_cnt[i]) {
      sub_program_instr_cnt[i] inside {[10 : instr_cnt]};
    }
    // Svinval shares the SFENCE enable path.  Keep the extension reachable
    // and legal when explicitly requested instead of leaving enable_sfence
    // and mstatus.TVM to an unlucky random choice.
    if (enable_svinval_extension) {
      if (no_fence) {
        enable_sfence == 1'b0;
      } else {
        enable_sfence == 1'b1;
      }
      if (!allow_sfence_exception) {
        init_privileged_mode != USER_MODE;
      }
      if (init_privileged_mode == SUPERVISOR_MODE) {
        mstatus_tvm == allow_sfence_exception;
      }
    } else if(allow_sfence_exception) {
      // If sfence exceptions are requested, enable the instruction in any
      // privilege mode and force TVM when executing from S-mode.
      enable_sfence == 1'b1;
      (init_privileged_mode != SUPERVISOR_MODE) || (mstatus_tvm == 1'b1);
    } else {
      // Otherwise preserve the legacy rule that random SFENCE.VMA is only
      // enabled for a legal S-mode program.
      (init_privileged_mode != SUPERVISOR_MODE || !riscv_instr_pkg::support_sfence || mstatus_tvm
          || no_fence) -> (enable_sfence == 1'b0);
    }
  }

  constraint debug_mode_c {
      if (riscv_instr_pkg::support_debug_mode) {
        debug_program_instr_cnt inside {[100 : 300]};
        foreach(debug_sub_program_instr_cnt[i]) {
          debug_sub_program_instr_cnt[i] inside {[100 : 300]};
        }
      }
    `ifndef DSIM
       main_program_instr_cnt + sub_program_instr_cnt.sum() == instr_cnt;
    `else
       // dsim has some issue supporting sum(), use some approximate constraint to generate
       // instruction cnt
       if (num_of_sub_program > 0) {
         main_program_instr_cnt inside {[10:instr_cnt/2]};
         foreach (sub_program_instr_cnt[i]) {
           sub_program_instr_cnt[i] inside {[10:instr_cnt/num_of_sub_program]};
         }
       } else {
         main_program_instr_cnt == instr_cnt;
       }
    `endif
  }

  // Keep the number of single step iterations relatively small
  constraint debug_single_step_c {
    if (enable_debug_single_step) {
      single_step_iterations inside {[10 : 50]};
    }
  }

  // Boot privileged mode distribution
  constraint boot_privileged_mode_dist_c {
    // Boot to higher privileged mode more often
    if(riscv_instr_pkg::supported_privileged_mode.size() == 2) {
      init_privileged_mode dist {riscv_instr_pkg::supported_privileged_mode[0] := 6,
                                 riscv_instr_pkg::supported_privileged_mode[1] := 4};
    } else if (riscv_instr_pkg::supported_privileged_mode.size() == 3) {
      init_privileged_mode dist {riscv_instr_pkg::supported_privileged_mode[0] := 4,
                                 riscv_instr_pkg::supported_privileged_mode[1] := 3,
                                 riscv_instr_pkg::supported_privileged_mode[2] := 3};
    } else {
      init_privileged_mode == riscv_instr_pkg::supported_privileged_mode[0];
    }
  }

  constraint mtvec_c {
    mtvec_mode inside {supported_interrupt_mode};
    if (mtvec_mode == DIRECT) {
     soft tvec_alignment == 2;
    } else {
     // Setting MODE = Vectored may impose an additional alignmentconstraint on BASE,
     // requiring up to 4×XLEN-byte alignment
     soft tvec_alignment == $clog2((XLEN * 4) / 8);
    }
  }

  constraint mstatus_c {
    if (set_mstatus_mprv) {
      mstatus_mprv == 1'b1;
    } else {
      mstatus_mprv == 1'b0;
    }
    if (SATP_MODE == BARE) {
      mstatus_mxr == 0;
      mstatus_sum == 0;
      mstatus_tvm == 0;
    } else if (set_mstatus_sum) {
      mstatus_sum == 1;
    }
  }

  // Exception delegation setting
  constraint exception_delegation_c {
    // Do not delegate instructino page fault to supervisor/user mode because this may introduce
    // dead loop. All the subsequent instruction fetches may fail and program cannot recover.
    m_mode_exception_delegation[INSTRUCTION_PAGE_FAULT] == 1'b0;
    if(force_m_delegation) {
      foreach(m_mode_exception_delegation[i]) {
        soft m_mode_exception_delegation[i] == 1'b1;
      }
      foreach(m_mode_interrupt_delegation[i]) {
        soft m_mode_interrupt_delegation[i] == 1'b1;
      }
    }
    if(force_s_delegation) {
      foreach(s_mode_exception_delegation[i]) {
        soft s_mode_exception_delegation[i] == 1'b1;
      }
      foreach(s_mode_interrupt_delegation[i]) {
        soft s_mode_interrupt_delegation[i] == 1'b1;
      }
    }
  }

  // Spike only supports a subset of exception and interrupt delegation
  // You can modify this constraint if your ISS support different set of delegations
  constraint delegation_c {
    foreach(m_mode_exception_delegation[i]) {
      if(!support_supervisor_mode || no_delegation) {
        m_mode_exception_delegation[i] == 0;
      }
      if(!(i inside {INSTRUCTION_ADDRESS_MISALIGNED, BREAKPOINT, ECALL_UMODE,
                     INSTRUCTION_PAGE_FAULT, LOAD_PAGE_FAULT, STORE_AMO_PAGE_FAULT})) {
        m_mode_exception_delegation[i] == 0;
      }
    }
    foreach(m_mode_interrupt_delegation[i]) {
      if(!support_supervisor_mode || no_delegation) {
        m_mode_interrupt_delegation[i] == 0;
      }
      if(!(i inside {S_SOFTWARE_INTR, S_TIMER_INTR, S_EXTERNAL_INTR,
                     LOCAL_COUNTER_OVERFLOW_INTR})) {
        m_mode_interrupt_delegation[i] == 0;
      }
      if ((i == LOCAL_COUNTER_OVERFLOW_INTR) && !enable_sscofpmf) {
        m_mode_interrupt_delegation[i] == 0;
      }
      if ((i == LOCAL_COUNTER_OVERFLOW_INTR) && enable_sscofpmf &&
          !no_delegation && (init_privileged_mode != MACHINE_MODE)) {
        m_mode_interrupt_delegation[i] == 1;
      }
    }
  }

  constraint ra_c {
    ra dist {RA := 3, T1 := 2, [SP:T0] :/ 1, [T2:T6] :/ 4};
    ra != sp;
    ra != tp;
    ra != ZERO;
  }

  constraint sp_tp_c {
    if (fix_sp) {
      sp == SP;
    }
    sp != tp;
    !(sp inside {GP, RA, ZERO});
    // test_done uses the architectural A0 register as the result value
    // immediately before raising the terminating trap.  The trap prologue
    // uses TP as the kernel-stack pointer, so selecting A0 for TP would turn
    // that result write into a zero stack pointer and cause an endless chain
    // of store-access faults in the handler.
    !(tp inside {GP, RA, ZERO, A0});
  }

  // This reg is used in various places throughout the generator,
  // so need more conservative constraints on it.
  constraint reserve_scratch_reg_c {
    !(scratch_reg inside {ZERO, sp, tp, ra, GP});
  }

  // These registers is only used inside PMP exception routine,
  // so we can be a bit looser with constraints.
  constraint reserve_pmp_reg_c {
    foreach (pmp_reg[i]) {
      !(pmp_reg[i] inside {ZERO, sp, tp, scratch_reg});
    }
    unique {pmp_reg};
  }

  constraint gpr_c {
    foreach (gpr[i]) {
      !(gpr[i] inside {sp, tp, scratch_reg, pmp_reg, ZERO, RA, GP});
    }
    unique {gpr};
  }

  constraint addr_translaction_rnd_order_c {
    solve init_privileged_mode before virtual_addr_translation_on;
  }

  constraint addr_translaction_c {
    if ((init_privileged_mode != MACHINE_MODE) && (SATP_MODE != BARE)) {
      virtual_addr_translation_on == 1'b1;
    } else {
      virtual_addr_translation_on == 1'b0;
    }
  }

  constraint floating_point_c {
    if (enable_floating_point) {
      mstatus_fs == 2'b01;
    } else {
      mstatus_fs == 2'b00;
      vector_cfg.vec_fp == 1'b0;
    }
  }

  constraint mstatus_vs_c {
    if (enable_vector_extension) {
      mstatus_vs == 2'b01;
    } else {
      mstatus_vs == 2'b00;
    }
  }

  `uvm_object_utils_begin(riscv_instr_gen_config)
    `uvm_field_int(main_program_instr_cnt, UVM_DEFAULT)
    `uvm_field_sarray_int(sub_program_instr_cnt, UVM_DEFAULT)
    `uvm_field_int(debug_program_instr_cnt, UVM_DEFAULT)
    `uvm_field_enum(data_pattern_t, data_page_pattern, UVM_DEFAULT)
    `uvm_field_enum(privileged_mode_t, init_privileged_mode, UVM_DEFAULT)
    `uvm_field_array_enum(riscv_reg_t, reserved_regs, UVM_DEFAULT)
    `uvm_field_enum(riscv_reg_t, ra, UVM_DEFAULT)
    `uvm_field_enum(riscv_reg_t, sp, UVM_DEFAULT)
    `uvm_field_enum(riscv_reg_t, tp, UVM_DEFAULT)
    `uvm_field_int(tvec_alignment, UVM_DEFAULT)
    `uvm_field_int(no_data_page, UVM_DEFAULT)
    `uvm_field_int(no_branch_jump, UVM_DEFAULT)
    `uvm_field_int(no_load_store, UVM_DEFAULT)
    `uvm_field_int(no_csr_instr, UVM_DEFAULT)
    `uvm_field_int(no_ebreak, UVM_DEFAULT)
    `uvm_field_int(no_ecall, UVM_DEFAULT)
    `uvm_field_int(no_dret, UVM_DEFAULT)
    `uvm_field_int(no_fence, UVM_DEFAULT)
    `uvm_field_int(no_wfi, UVM_DEFAULT)
    `uvm_field_int(fix_sp, UVM_DEFAULT)
    `uvm_field_int(enable_unaligned_load_store, UVM_DEFAULT)
    `uvm_field_int(illegal_instr_ratio, UVM_DEFAULT)
    `uvm_field_int(hint_instr_ratio, UVM_DEFAULT)
    `uvm_field_int(gen_all_csrs_by_default, UVM_DEFAULT)
    `uvm_field_int(gen_csr_ro_write, UVM_DEFAULT)
    `uvm_field_array_enum(privileged_reg_t, add_csr_write, UVM_DEFAULT)
    `uvm_field_array_enum(privileged_reg_t, remove_csr_write, UVM_DEFAULT)
    `uvm_field_string(boot_mode_opts, UVM_DEFAULT)
    `uvm_field_int(enable_random_boot_mode, UVM_DEFAULT)
    `uvm_field_int(enable_page_table_exception, UVM_DEFAULT)
    `uvm_field_int(svade_fault_mode, UVM_DEFAULT)
    `uvm_field_int(enable_svpbmt, UVM_DEFAULT)
    `uvm_field_int(no_directed_instr, UVM_DEFAULT)
    `uvm_field_int(enable_random_directed_instr, UVM_DEFAULT)
    `uvm_field_int(random_directed_instr_ratio, UVM_DEFAULT)
    `uvm_field_int(enable_interrupt, UVM_DEFAULT)
    `uvm_field_int(enable_timer_irq, UVM_DEFAULT)
    `uvm_field_int(bare_program_mode, UVM_DEFAULT)
    `uvm_field_int(enable_illegal_csr_instruction, UVM_DEFAULT)
    `uvm_field_int(enable_access_invalid_csr_level, UVM_DEFAULT)
    `uvm_field_int(enable_misaligned_instr, UVM_DEFAULT)
    `uvm_field_int(enable_dummy_csr_write, UVM_DEFAULT)
    `uvm_field_int(randomize_csr, UVM_DEFAULT)
    `uvm_field_int(allow_sfence_exception, UVM_DEFAULT)
    `uvm_field_int(enable_tval_check, UVM_DEFAULT)
    `uvm_field_int(enable_sscofpmf, UVM_DEFAULT)
    `uvm_field_int(no_delegation, UVM_DEFAULT)
    `uvm_field_int(force_m_delegation, UVM_DEFAULT)
    `uvm_field_int(force_s_delegation, UVM_DEFAULT)
    `uvm_field_int(support_supervisor_mode, UVM_DEFAULT)
    `uvm_field_int(disable_compressed_instr, UVM_DEFAULT)
    `uvm_field_int(signature_addr, UVM_DEFAULT)
    `uvm_field_int(num_of_harts, UVM_DEFAULT)
    `uvm_field_int(require_signature_addr, UVM_DEFAULT)
    `uvm_field_int(gen_debug_section, UVM_DEFAULT)
    `uvm_field_int(enable_ebreak_in_debug_rom, UVM_DEFAULT)
    `uvm_field_int(set_dcsr_ebreak, UVM_DEFAULT)
    `uvm_field_int(num_debug_sub_program, UVM_DEFAULT)
    `uvm_field_int(enable_debug_single_step, UVM_DEFAULT)
    `uvm_field_int(single_step_iterations, UVM_DEFAULT)
    `uvm_field_int(set_mstatus_tw, UVM_DEFAULT)
    `uvm_field_int(set_mstatus_mprv, UVM_DEFAULT)
    `uvm_field_int(set_mstatus_sum, UVM_DEFAULT)
    `uvm_field_int(max_branch_step, UVM_DEFAULT)
    `uvm_field_int(max_directed_instr_stream_seq, UVM_DEFAULT)
    `uvm_field_int(enable_floating_point, UVM_DEFAULT)
    `uvm_field_int(enable_vector_extension, UVM_DEFAULT)
    `uvm_field_int(use_vector_1_0, UVM_DEFAULT)
    `uvm_field_int(use_vsetivli, UVM_DEFAULT)
    `uvm_field_int(enable_random_vset_instr, UVM_DEFAULT)
    `uvm_field_int(enable_zvbb_extension, UVM_DEFAULT)
    `uvm_field_int(vector_instr_only, UVM_DEFAULT)
    `uvm_field_int(enable_b_extension, UVM_DEFAULT)
    `uvm_field_array_enum(b_ext_group_t, enable_bitmanip_groups, UVM_DEFAULT)
    `uvm_field_int(enable_zba_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zbb_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zbc_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zbkc_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zbs_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zcb_extension, UVM_DEFAULT)
    `uvm_field_int(enable_svinval_extension, UVM_DEFAULT)
    `uvm_field_int(enable_aclint_init, UVM_DEFAULT)
    `uvm_field_int(aclint_msip_addr, UVM_DEFAULT)
    `uvm_field_int(aclint_msip_stride, UVM_DEFAULT)
    `uvm_field_int(aclint_mtimecmp_addr, UVM_DEFAULT)
    `uvm_field_int(aclint_mtimecmp_stride, UVM_DEFAULT)
    `uvm_field_int(enable_plic_claim_complete, UVM_DEFAULT)
    `uvm_field_int(plic_claim_complete_addr, UVM_DEFAULT)
    `uvm_field_int(plic_m_claim_complete_addr, UVM_DEFAULT)
    `uvm_field_int(plic_s_claim_complete_addr, UVM_DEFAULT)
    `uvm_field_int(plic_hart_stride, UVM_DEFAULT)
    `uvm_field_int(enable_zicond_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zimop_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zcmop_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zicbom_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zicbop_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zicboz_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zbkb_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zbkx_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zknd_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zkne_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zknh_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zksed_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zksh_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zkn_extension, UVM_DEFAULT)
    `uvm_field_int(enable_zks_extension, UVM_DEFAULT)
    `uvm_field_int(use_push_data_section, UVM_DEFAULT)
  `uvm_object_utils_end

  protected function void add_supported_isa(riscv_instr_group_t isa_group);
    if (!(isa_group inside {riscv_instr_pkg::supported_isa})) begin
      riscv_instr_pkg::supported_isa.push_back(isa_group);
    end
  endfunction

  function new (string name = "");
    string s;
    string random_directed_instr_arg;
    string random_boot_mode_arg;
    riscv_instr_group_t march_isa[];
    super.new(name);
    init_delegation();
    inst = uvm_cmdline_processor::get_inst();
    get_int_arg_value("+num_of_tests=", num_of_tests);
    get_int_arg_value("+enable_page_table_exception=", enable_page_table_exception);
    get_int_arg_value("+svade_fault_mode=", svade_fault_mode);
    get_bool_arg_value("+enable_svpbmt=", enable_svpbmt);
    get_bool_arg_value("+enable_interrupt=", enable_interrupt);
    get_bool_arg_value("+enable_nested_interrupt=", enable_nested_interrupt);
    get_bool_arg_value("+enable_timer_irq=", enable_timer_irq);
    get_int_arg_value("+num_of_sub_program=", num_of_sub_program);
    get_int_arg_value("+instr_cnt=", instr_cnt);
    get_bool_arg_value("+no_ebreak=", no_ebreak);
    get_bool_arg_value("+no_ecall=", no_ecall);
    get_bool_arg_value("+no_dret=", no_dret);
    get_bool_arg_value("+no_wfi=", no_wfi);
    get_bool_arg_value("+no_branch_jump=", no_branch_jump);
    get_bool_arg_value("+no_load_store=", no_load_store);
    get_bool_arg_value("+no_csr_instr=", no_csr_instr);
    get_bool_arg_value("+fix_sp=", fix_sp);
    get_bool_arg_value("+use_push_data_section=", use_push_data_section);
    get_bool_arg_value("+enable_illegal_csr_instruction=", enable_illegal_csr_instruction);
    get_bool_arg_value("+enable_access_invalid_csr_level=", enable_access_invalid_csr_level);
    get_bool_arg_value("+enable_misaligned_instr=", enable_misaligned_instr);
    get_bool_arg_value("+enable_dummy_csr_write=", enable_dummy_csr_write);
    get_bool_arg_value("+allow_sfence_exception=", allow_sfence_exception);
    get_bool_arg_value("+enable_tval_check=", enable_tval_check);
    get_bool_arg_value("+enable_sscofpmf=", enable_sscofpmf);
    get_bool_arg_value("+no_data_page=", no_data_page);
    get_bool_arg_value("+no_directed_instr=", no_directed_instr);
    get_bool_arg_value("+enable_random_directed_instr=", enable_random_directed_instr);
    // Short alias retained for testlists that prefer a compact option name.
    if (inst.get_arg_value("+random_directed_instr=", random_directed_instr_arg)) begin
      enable_random_directed_instr = random_directed_instr_arg.atobin();
    end
    get_int_arg_value("+random_directed_instr_ratio=", random_directed_instr_ratio);
    if ((random_directed_instr_ratio < 0) || (random_directed_instr_ratio > 1000)) begin
      `uvm_fatal(get_full_name(), $sformatf(
                "random_directed_instr_ratio must be in [0:1000], got %0d",
                random_directed_instr_ratio))
    end
    get_bool_arg_value("+enable_random_boot_mode=", enable_random_boot_mode);
    // Short alias retained for existing/local testlists.
    if (inst.get_arg_value("+random_boot_mode=", random_boot_mode_arg)) begin
      enable_random_boot_mode = random_boot_mode_arg.atobin();
    end
    get_bool_arg_value("+no_fence=", no_fence);
    get_bool_arg_value("+no_delegation=", no_delegation);
    get_int_arg_value("+illegal_instr_ratio=", illegal_instr_ratio);
    get_int_arg_value("+hint_instr_ratio=", hint_instr_ratio);
    get_bool_arg_value("+gen_all_csrs_by_default=", gen_all_csrs_by_default);
    get_bool_arg_value("+gen_csr_ro_write=", gen_csr_ro_write);
    cmdline_enum_processor #(privileged_reg_t)::get_array_values("+add_csr_write=",
                                                              1'b1, add_csr_write);
    cmdline_enum_processor #(privileged_reg_t)::get_array_values("+remove_csr_write=",
                                                              1'b1, remove_csr_write);
    get_int_arg_value("+num_of_harts=", num_of_harts);
    get_bool_arg_value("+enable_unaligned_load_store=", enable_unaligned_load_store);
    get_bool_arg_value("+force_m_delegation=", force_m_delegation);
    get_bool_arg_value("+force_s_delegation=", force_s_delegation);
    get_bool_arg_value("+require_signature_addr=", require_signature_addr);
    get_bool_arg_value("+disable_compressed_instr=", disable_compressed_instr);
    get_bool_arg_value("+randomize_csr=", randomize_csr);
    if (this.require_signature_addr) begin
      get_hex_arg_value("+signature_addr=", signature_addr);
    end
    if ($value$plusargs("tvec_alignment=%0d", tvec_alignment)) begin
      tvec_alignment.rand_mode(0);
    end
    get_bool_arg_value("+gen_debug_section=", gen_debug_section);
    get_bool_arg_value("+bare_program_mode=", bare_program_mode);
    get_int_arg_value("+num_debug_sub_program=", num_debug_sub_program);
    get_bool_arg_value("+enable_ebreak_in_debug_rom=", enable_ebreak_in_debug_rom);
    get_bool_arg_value("+set_dcsr_ebreak=", set_dcsr_ebreak);
    get_bool_arg_value("+enable_debug_single_step=", enable_debug_single_step);
    get_bool_arg_value("+set_mstatus_tw=", set_mstatus_tw);
    get_bool_arg_value("+set_mstatus_mprv=", set_mstatus_mprv);
    get_bool_arg_value("+set_mstatus_sum=", set_mstatus_sum);
    get_bool_arg_value("+enable_floating_point=", enable_floating_point);
    get_bool_arg_value("+enable_vector_extension=", enable_vector_extension);
    get_bool_arg_value("+vector_instr_only=", vector_instr_only);
    get_bool_arg_value("+enable_b_extension=", enable_b_extension);
    get_bool_arg_value("+enable_zba_extension=", enable_zba_extension);
    get_bool_arg_value("+enable_zbb_extension=", enable_zbb_extension);
    get_bool_arg_value("+enable_zbc_extension=", enable_zbc_extension);
    get_bool_arg_value("+enable_zbkc_extension=", enable_zbkc_extension);
    get_bool_arg_value("+enable_zbs_extension=", enable_zbs_extension);
    get_bool_arg_value("+enable_zcb_extension=", enable_zcb_extension);
    get_bool_arg_value("+enable_svinval_extension=", enable_svinval_extension);
    get_bool_arg_value("+use_vector_1_0=", use_vector_1_0);
    get_bool_arg_value("+use_vsetivli=", use_vsetivli);
    get_bool_arg_value("+enable_random_vset_instr=", enable_random_vset_instr);
    get_bool_arg_value("+enable_zvbb_extension=", enable_zvbb_extension);
    get_bool_arg_value("+enable_aclint_init=", enable_aclint_init);
    get_hex_arg_value("+aclint_msip_addr=", aclint_msip_addr);
    get_int_arg_value("+aclint_msip_stride=", aclint_msip_stride);
    get_hex_arg_value("+aclint_mtimecmp_addr=", aclint_mtimecmp_addr);
    get_int_arg_value("+aclint_mtimecmp_stride=", aclint_mtimecmp_stride);
    get_bool_arg_value("+enable_plic_claim_complete=", enable_plic_claim_complete);
    get_hex_arg_value("+plic_claim_complete_addr=", plic_claim_complete_addr);
    get_hex_arg_value("+plic_m_claim_complete_addr=", plic_m_claim_complete_addr);
    get_hex_arg_value("+plic_s_claim_complete_addr=", plic_s_claim_complete_addr);
    get_int_arg_value("+plic_hart_stride=", plic_hart_stride);
    get_bool_arg_value("+enable_zicond_extension=", enable_zicond_extension);
    get_bool_arg_value("+enable_zimop_extension=", enable_zimop_extension);
    get_bool_arg_value("+enable_zcmop_extension=", enable_zcmop_extension);
    get_bool_arg_value("+enable_zicbom_extension=", enable_zicbom_extension);
    get_bool_arg_value("+enable_zicbop_extension=", enable_zicbop_extension);
    get_bool_arg_value("+enable_zicboz_extension=", enable_zicboz_extension);
    get_bool_arg_value("+enable_zbkb_extension=", enable_zbkb_extension);
    get_bool_arg_value("+enable_zbkx_extension=", enable_zbkx_extension);
    get_bool_arg_value("+enable_zknd_extension=", enable_zknd_extension);
    get_bool_arg_value("+enable_zkne_extension=", enable_zkne_extension);
    get_bool_arg_value("+enable_zknh_extension=", enable_zknh_extension);
    get_bool_arg_value("+enable_zksed_extension=", enable_zksed_extension);
    get_bool_arg_value("+enable_zksh_extension=", enable_zksh_extension);
    get_bool_arg_value("+enable_zkn_extension=", enable_zkn_extension);
    get_bool_arg_value("+enable_zks_extension=", enable_zks_extension);
    cmdline_enum_processor #(b_ext_group_t)::get_array_values("+enable_bitmanip_groups=",
                                                              1'b0, enable_bitmanip_groups);
    if(inst.get_arg_value("+boot_mode=", boot_mode_opts)) begin
      `uvm_info(get_full_name(), $sformatf(
                "Got boot mode option - %0s", boot_mode_opts), UVM_LOW)
      case(boot_mode_opts)
        "m" : init_privileged_mode = MACHINE_MODE;
        "s" : init_privileged_mode = SUPERVISOR_MODE;
        "u" : init_privileged_mode = USER_MODE;
        default: `uvm_fatal(get_full_name(),
                  $sformatf("Illegal boot mode option - %0s", boot_mode_opts))
      endcase
      if (!(init_privileged_mode inside {riscv_instr_pkg::supported_privileged_mode})) begin
        `uvm_fatal(get_full_name(), $sformatf(
                  "Requested boot mode %0s is not supported by this target",
                  init_privileged_mode.name()))
      end
      init_privileged_mode.rand_mode(0);
      addr_translaction_rnd_order_c.constraint_mode(0);
    end else if (!enable_random_boot_mode) begin
      // Keep the legacy default deterministic when random boot selection is not
      // requested.  Targets normally include M-mode, but fall back to their
      // first advertised mode for lower-privilege-only profiles.
      if (riscv_instr_pkg::supported_privileged_mode.size() == 0) begin
        `uvm_fatal(get_full_name(), "No supported privileged mode is configured")
      end
      if (MACHINE_MODE inside {riscv_instr_pkg::supported_privileged_mode}) begin
        init_privileged_mode = MACHINE_MODE;
      end else begin
        init_privileged_mode = riscv_instr_pkg::supported_privileged_mode[0];
      end
      init_privileged_mode.rand_mode(0);
      addr_translaction_rnd_order_c.constraint_mode(0);
      `uvm_info(get_full_name(), $sformatf(
                "Random boot mode disabled; using %0s",
                init_privileged_mode.name()), UVM_LOW)
    end
    `uvm_info(`gfn, $sformatf("riscv_instr_pkg::supported_privileged_mode = %0d",
                   riscv_instr_pkg::supported_privileged_mode.size()), UVM_LOW)
    void'(inst.get_arg_value("+asm_test_suffix=", asm_test_suffix));
    // Directed march list from the runtime options, ex. RV32I, RV32M etc.
    cmdline_enum_processor #(riscv_instr_group_t)::get_array_values("+march=", 1'b0, march_isa);
    if (march_isa.size != 0) riscv_instr_pkg::supported_isa = march_isa;

    if (!(ZVBB inside {supported_isa})) enable_zvbb_extension = 1'b0;

    // Common instructions follow the existing riscv-dv convention of living
    // in their RV32 group.  Expand RV64 and aggregate K groups so a directed
    // +march selection still reaches all instructions implied by the profile.
    if (RV64ZICOND inside {supported_isa}) add_supported_isa(RV32ZICOND);
    if (RV64ZIMOP  inside {supported_isa}) add_supported_isa(RV32ZIMOP);
    if (RV64ZCMOP  inside {supported_isa}) add_supported_isa(RV32ZCMOP);
    if (RV64ZICBOM inside {supported_isa}) add_supported_isa(RV32ZICBOM);
    if (RV64ZICBOP inside {supported_isa}) add_supported_isa(RV32ZICBOP);
    if (RV64ZICBOZ inside {supported_isa}) add_supported_isa(RV32ZICBOZ);
    if (RV64ZBKC   inside {supported_isa}) add_supported_isa(RV32ZBKC);
    if (RV64ZBKB   inside {supported_isa}) add_supported_isa(RV32ZBKB);
    if (RV64ZBKX   inside {supported_isa}) add_supported_isa(RV32ZBKX);
    if (RV64ZKND   inside {supported_isa}) add_supported_isa(RV32ZKND);
    if (RV64ZKNE   inside {supported_isa}) add_supported_isa(RV32ZKNE);
    if (RV64ZKNH   inside {supported_isa}) add_supported_isa(RV32ZKNH);
    if (RV64ZKSED  inside {supported_isa}) add_supported_isa(RV32ZKSED);
    if (RV64ZKSH   inside {supported_isa}) add_supported_isa(RV32ZKSH);

    if ((RV32ZKN inside {supported_isa}) || (RV64ZKN inside {supported_isa})) begin
      add_supported_isa(RV32ZBKB);
      add_supported_isa(RV32ZBKX);
      add_supported_isa(RV32ZBKC);
      add_supported_isa(RV32ZKND);
      add_supported_isa(RV32ZKNE);
      add_supported_isa(RV32ZKNH);
      if (RV64ZKN inside {supported_isa}) begin
        add_supported_isa(RV64ZBKB);
        add_supported_isa(RV64ZBKX);
        add_supported_isa(RV64ZBKC);
        add_supported_isa(RV64ZKND);
        add_supported_isa(RV64ZKNE);
        add_supported_isa(RV64ZKNH);
      end
    end
    if ((RV32ZKS inside {supported_isa}) || (RV64ZKS inside {supported_isa})) begin
      add_supported_isa(RV32ZBKB);
      add_supported_isa(RV32ZBKX);
      add_supported_isa(RV32ZBKC);
      add_supported_isa(RV32ZKSED);
      add_supported_isa(RV32ZKSH);
      if (RV64ZKS inside {supported_isa}) begin
        add_supported_isa(RV64ZBKB);
        add_supported_isa(RV64ZBKX);
        add_supported_isa(RV64ZBKC);
        add_supported_isa(RV64ZKSED);
        add_supported_isa(RV64ZKSH);
      end
    end

    if (!(RV32C inside {supported_isa})) begin
      disable_compressed_instr = 1;
    end

    if (!((RV32ZBA inside {supported_isa}) ||
          (RV64ZBA inside {supported_isa}))) begin
      enable_zba_extension = 0;
    end

    if (!((RV32ZBB inside {supported_isa}) ||
          (RV64ZBB inside {supported_isa}))) begin
      enable_zbb_extension = 0;
    end

    if (!((RV32ZBC inside {supported_isa}) ||
          (RV64ZBC inside {supported_isa}))) begin
      enable_zbc_extension = 0;
    end

    if (!((RV32ZBKC inside {supported_isa}) ||
          (RV64ZBKC inside {supported_isa}))) begin
      enable_zbkc_extension = 0;
    end

    if (!((RV32ZBS inside {supported_isa}) ||
          (RV64ZBS inside {supported_isa}))) begin
      enable_zbs_extension = 0;
    end


    if (!((RV32ZCB inside {supported_isa}) ||
          (RV64ZCB inside {supported_isa}))) begin
      enable_zcb_extension = 0;
    end

    if (!((RV32ZICOND inside {supported_isa}) || (RV64ZICOND inside {supported_isa})))
      enable_zicond_extension = 0;
    if (!((RV32ZIMOP inside {supported_isa}) || (RV64ZIMOP inside {supported_isa})))
      enable_zimop_extension = 0;
    if (!((RV32ZCMOP inside {supported_isa}) || (RV64ZCMOP inside {supported_isa})))
      enable_zcmop_extension = 0;
    if (!((RV32ZICBOM inside {supported_isa}) || (RV64ZICBOM inside {supported_isa})))
      enable_zicbom_extension = 0;
    if (!((RV32ZICBOP inside {supported_isa}) || (RV64ZICBOP inside {supported_isa})))
      enable_zicbop_extension = 0;
    if (!((RV32ZICBOZ inside {supported_isa}) || (RV64ZICBOZ inside {supported_isa})))
      enable_zicboz_extension = 0;
    if (!((RV32ZBKB inside {supported_isa}) || (RV64ZBKB inside {supported_isa})))
      enable_zbkb_extension = 0;
    if (!((RV32ZBKX inside {supported_isa}) || (RV64ZBKX inside {supported_isa})))
      enable_zbkx_extension = 0;
    if (!((RV32ZKND inside {supported_isa}) || (RV64ZKND inside {supported_isa})))
      enable_zknd_extension = 0;
    if (!((RV32ZKNE inside {supported_isa}) || (RV64ZKNE inside {supported_isa})))
      enable_zkne_extension = 0;
    if (!((RV32ZKNH inside {supported_isa}) || (RV64ZKNH inside {supported_isa})))
      enable_zknh_extension = 0;
    if (!((RV32ZKSED inside {supported_isa}) || (RV64ZKSED inside {supported_isa})))
      enable_zksed_extension = 0;
    if (!((RV32ZKSH inside {supported_isa}) || (RV64ZKSH inside {supported_isa})))
      enable_zksh_extension = 0;
    if (!((RV32ZKN inside {supported_isa}) || (RV64ZKN inside {supported_isa})))
      enable_zkn_extension = 0;
    if (!((RV32ZKS inside {supported_isa}) || (RV64ZKS inside {supported_isa})))
      enable_zks_extension = 0;
    if (enable_zkn_extension || enable_zks_extension) begin
      enable_zbkc_extension = 1'b1;
    end
    vector_cfg = riscv_vector_cfg::type_id::create("vector_cfg");
    pmp_cfg = riscv_pmp_cfg::type_id::create("pmp_cfg");
    pmp_cfg.rand_mode(pmp_cfg.pmp_randomize);
    pmp_cfg.initialize(signature_addr);
    setup_instr_distribution();
    get_invalid_priv_lvl_csr();
  endfunction

  virtual function void setup_instr_distribution();
    string opts;
    int val;
    get_int_arg_value("+dist_control_mode=", dist_control_mode);
    if (dist_control_mode == 1) begin
      riscv_instr_category_t category;
      category = category.first;
      do begin
        opts = {$sformatf("dist_%0s=", category.name()), "%d"};
        opts = opts.tolower();
        if ($value$plusargs(opts, val)) begin
          category_dist[category] = val;
        end else begin
          category_dist[category] = 10; // Default ratio
        end
        `uvm_info(`gfn, $sformatf("Set dist[%0s] = %0d",
                        category.name(), category_dist[category]), UVM_LOW)
        category = category.next;
      end
      while(category != category.first);
    end
  endfunction

  // Initialize the exception/interrupt delegation associate array, set all delegation default to 0
  virtual function void init_delegation();
    exception_cause_t cause;
    interrupt_cause_t intr_cause;
    cause = cause.first;
    // Init exception delegation array
    do begin
      m_mode_exception_delegation[cause] = 1'b0;
      s_mode_exception_delegation[cause] = 1'b0;
      cause = cause.next;
    end
    while(cause != cause.first);
    // Init interrupt delegation array
    intr_cause = intr_cause.first;
    do begin
      m_mode_interrupt_delegation[intr_cause] = 1'b0;
      s_mode_interrupt_delegation[intr_cause] = 1'b0;
      intr_cause = intr_cause.next;
    end
    while(intr_cause != intr_cause.first);
  endfunction

  function void pre_randomize();
    foreach (riscv_instr_pkg::supported_privileged_mode[i]) begin
      if(riscv_instr_pkg::supported_privileged_mode[i] == SUPERVISOR_MODE)
        support_supervisor_mode = 1;
    end
  endfunction

  virtual function void get_non_reserved_gpr();
  endfunction

  function void post_randomize();
    // Setup the list all reserved registers
    reserved_regs = {tp, sp, scratch_reg};
    // Need to save all loop registers, and RA/T0
    min_stack_len_per_program = 2 * (XLEN/8);
    // The selected boot mode can change on every randomization.  Rebuild the
    // invalid-privilege CSR list after that selection so CSR generation uses
    // the mode of this iteration rather than a constructor-time value.
    get_invalid_priv_lvl_csr();
    // Check if the setting is legal
    check_setting();
  endfunction

  virtual function void check_setting();
    bit support_64b;
    bit support_128b;
    foreach (riscv_instr_pkg::supported_isa[i]) begin
      if (riscv_instr_pkg::supported_isa[i] inside {RV64I, RV64M, RV64A, RV64F, RV64D, RV64C,
                                                    RV64B, RV64ZBA, RV64ZBB, RV64ZBC,
                                                    RV64ZBKC, RV64ZBS, RV64ZCB, RV64ZMMUL,
                                                    RV64ZICOND, RV64ZIMOP, RV64ZCMOP,
                                                    RV64ZICBOM, RV64ZICBOP, RV64ZICBOZ,
                                                    RV64ZBKB, RV64ZBKX,
                                                    RV64ZKND, RV64ZKNE, RV64ZKNH,
                                                    RV64ZKSED, RV64ZKSH, RV64ZKN, RV64ZKS,
                                                    RV64X}) begin
        support_64b = 1'b1;
      end else if (riscv_instr_pkg::supported_isa[i] inside {RV128I, RV128C}) begin
        support_128b = 1'b1;
      end
    end
    if (support_128b && XLEN != 128) begin
      `uvm_fatal(`gfn, "XLEN should be set to 128 based on riscv_instr_pkg::supported_isa setting")
    end
    if (!support_128b && support_64b && XLEN != 64) begin
      `uvm_fatal(`gfn, "XLEN should be set to 64 based on riscv_instr_pkg::supported_isa setting")
    end
    if (!(support_128b || support_64b) && XLEN != 32) begin
      `uvm_fatal(`gfn, "XLEN should be set to 32 based on riscv_instr_pkg::supported_isa setting")
    end
    if (!(support_128b || support_64b) && !(SATP_MODE inside {SV32, BARE})) begin
      `uvm_fatal(`gfn, $sformatf("SATP mode %0s is not supported for RV32G ISA", SATP_MODE.name()))
    end
    if (enable_tval_check && !require_signature_addr) begin
      `uvm_fatal(`gfn, "+enable_tval_check requires +require_signature_addr=1")
    end
    if (!(svade_fault_mode inside {[0:2]})) begin
      `uvm_fatal(`gfn, $sformatf("Unsupported Svade fault mode %0d", svade_fault_mode))
    end
    if ((svade_fault_mode != 0) && !enable_page_table_exception) begin
      `uvm_fatal(`gfn, "+svade_fault_mode requires +enable_page_table_exception=1")
    end
    if (enable_plic_claim_complete &&
        (plic_claim_complete_addr == '0) &&
        (plic_m_claim_complete_addr == '0) &&
        (plic_s_claim_complete_addr == '0)) begin
      `uvm_fatal(`gfn, "PLIC claim/complete requires at least one context address")
    end
  endfunction

  // Populate invalid_priv_mode_csrs from the architectural privilege encoding
  // in csr[9:8]. This also classifies trigger CSRs such as tselect correctly;
  // their names do not carry an M-mode prefix.
  virtual function void get_invalid_priv_lvl_csr();
    privileged_reg_t csr;
    bit invalid;
    invalid_priv_mode_csrs.delete();
    foreach (implemented_csr[i]) begin
      csr = implemented_csr[i];
      // Debug-mode CSRs are inaccessible from every normal privilege mode.
      invalid = csr inside {[DCSR:DSCRATCH1]};
      case (init_privileged_mode)
        MACHINE_MODE: ;
        SUPERVISOR_MODE: invalid |= (csr[9:8] inside {2'b10, 2'b11});
        USER_MODE:       invalid |= (csr[9:8] != 2'b00);
        default: `uvm_fatal(`gfn, "Unsupported initialization privilege mode")
      endcase
      if (invalid) invalid_priv_mode_csrs.push_back(csr);
    end
  endfunction

endclass
