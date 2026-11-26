/*
 * Copyright 2026
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

// Exercise the allocator at byte-granular region boundaries before producing
// an ordinary ELF test. These checks deliberately use tiny, odd-sized regions:
// naturally aligned 4 KiB regions hide a start-byte-only bounds check.
class riscv_restrict_store_test extends riscv_instr_base_test;

  `uvm_component_utils(riscv_restrict_store_test)
  `uvm_component_new

  function int access_bytes(riscv_instr_name_t name);
    case (name)
      SB, C_SB: return 1;
      SH, C_SH: return 2;
      SW, FSW, C_SW, C_SWSP, C_FSW, C_FSWSP: return 4;
      SD, FSD, C_SD, C_SDSP, C_FSD, C_FSDSP: return 8;
      default: begin
        `uvm_fatal(`gfn, $sformatf("Unexpected scalar store in bounds test: %0s", name.name()))
        return 0;
      end
    endcase
  endfunction

  function riscv_load_store_base_instr_stream boundary_stream(int size, int addr);
    riscv_load_store_base_instr_stream stream;
    bit found_reg;
    stream = riscv_load_store_base_instr_stream::type_id::create("boundary_stream");
    stream.cfg = cfg;
    // Prefer compressed-register operands so the same check covers compressed
    // choices when the target enables C, as well as ordinary scalar stores.
    for (int reg_idx = int'(S0); reg_idx <= int'(A5); reg_idx++) begin
      if (!found_reg && !(riscv_reg_t'(reg_idx) inside {cfg.reserved_regs})) begin
        stream.rs1_reg = riscv_reg_t'(reg_idx);
        found_reg = 1;
      end
    end
    `DV_CHECK_FATAL(found_reg, "No non-reserved register for scalar bounds test")
    stream.reserved_rd = {stream.rs1_reg};
    stream.max_load_store_offset = size;
    stream.base = 0;
    stream.num_load_store = 1;
    stream.addr = new[1];
    stream.offset = new[1];
    stream.addr[0] = addr;
    stream.offset[0] = addr;
    return stream;
  endfunction

  function void check_scalar_boundaries();
    int sizes[$] = {1, 3, 7, 8, 9, 16};
    int store_count;
    bit saw_legacy_overrun;
    bit saved_unaligned = cfg.enable_unaligned_load_store;
    bit saved_strict = cfg.restrict_store_to_data;
    bit saved_scoped = cfg.restrict_generated_store_to_data;
    riscv_load_store_base_instr_stream stream;

    // Establish that this test actually exercises the previous defect. With
    // the option disabled, an unaligned wide store may start at the last byte.
    cfg.enable_unaligned_load_store = 1;
    cfg.restrict_store_to_data = 0;
    cfg.restrict_generated_store_to_data = 0;
    stream = boundary_stream(8, 7);
    repeat (128) begin
      stream.instr_list.delete();
      stream.load_store_instr.delete();
      stream.gen_load_store_instr();
      foreach (stream.load_store_instr[i]) begin
        if (stream.load_store_instr[i].category == STORE) begin
          if (7 + access_bytes(stream.load_store_instr[i].instr_name) > 8)
            saw_legacy_overrun = 1;
        end
      end
    end
    `DV_CHECK_FATAL(saw_legacy_overrun, "Bounds regression did not exercise the legacy overrun")
    cfg.restrict_store_to_data = saved_strict;
    cfg.restrict_generated_store_to_data = saved_scoped;

    // Cover both unaligned selection and aligned/compressed selection. Region
    // end addresses themselves need not be aligned to the selected store width.
    for (int unaligned = 0; unaligned < 2; unaligned++) begin
      cfg.enable_unaligned_load_store = bit'(unaligned);
      foreach (sizes[s]) begin
        for (int addr = 0; addr < sizes[s]; addr++) begin
          stream = boundary_stream(sizes[s], addr);
          repeat (32) begin
            stream.instr_list.delete();
            stream.load_store_instr.delete();
            stream.gen_load_store_instr();
            foreach (stream.load_store_instr[i]) begin
              if (stream.load_store_instr[i].category == STORE) begin
                int bytes_written = access_bytes(stream.load_store_instr[i].instr_name);
                store_count++;
                `DV_CHECK_FATAL(addr + bytes_written <= sizes[s], $sformatf(
                    "%0s crosses region end: offset=%0d bytes=%0d size=%0d",
                    stream.load_store_instr[i].instr_name.name(), addr, bytes_written, sizes[s]))
                `DV_CHECK_FATAL(stream.load_store_instr[i].rs1 == stream.rs1_reg,
                                "Scalar store lost its data-region base register")
              end
            end
          end
        end
      end
    end
    cfg.enable_unaligned_load_store = saved_unaligned;
    `DV_CHECK_FATAL(store_count > 0, "Bounds regression did not generate scalar stores")
    `uvm_info(`gfn, $sformatf("Verified %0d scalar stores at region boundaries", store_count),
              UVM_LOW)
  endfunction

  function void check_atomic_boundaries();
    mem_region_t saved_regions[$];
    riscv_amo_instr_stream amo;
    riscv_lr_sc_instr_stream lr_sc;
    if (!(RV32A inside {supported_isa})) return;
    saved_regions = cfg.amo_region;
    // Eleven bytes admits a naturally aligned eight-byte start but cannot
    // contain an eight-byte write there. Also exercises the RV32 four-byte end.
    cfg.amo_region = '{'{name:"unit_amo", size_in_bytes:11, xwr:3'b011}};
    repeat (32) begin
      amo = riscv_amo_instr_stream::type_id::create("boundary_amo");
      amo.cfg = cfg;
      `DV_CHECK_RANDOMIZE_WITH_FATAL(amo, num_amo == 4; num_mixed_instr == 0;)
      foreach (amo.offset[i]) begin
        `DV_CHECK_FATAL(amo.offset[i] >= 0 && amo.offset[i] + XLEN/8 <= 11,
                        "AMO access crosses shared data-region end")
      end
      lr_sc = riscv_lr_sc_instr_stream::type_id::create("boundary_lr_sc");
      lr_sc.cfg = cfg;
      `DV_CHECK_RANDOMIZE_WITH_FATAL(lr_sc, num_mixed_instr == 0;)
      `DV_CHECK_FATAL(lr_sc.offset[0] >= 0 && lr_sc.offset[0] + XLEN/8 <= 11,
                      "LR/SC access crosses shared data-region end")
    end
    cfg.amo_region = saved_regions;
  endfunction

  virtual function void apply_directed_instr();
    `DV_CHECK_FATAL(cfg.restrict_store_to_data,
                    "riscv_restrict_store_test requires +restrict_store_to_data=1")
    check_scalar_boundaries();
    check_atomic_boundaries();
  endfunction

endclass

// Verify the scoped option retains generated-store bounds while admitting an
// imported source Hypertest stream. The existing strict-mode test still covers
// the original option independently.
class riscv_restrict_generated_store_test extends riscv_restrict_store_test;
  `uvm_component_utils(riscv_restrict_generated_store_test)
  `uvm_component_new

  virtual function void apply_directed_instr();
    riscv_hypertest_case_stream source_stream;
    `DV_CHECK_FATAL(cfg.restrict_generated_store_to_data && !cfg.restrict_store_to_data,
                    "riscv_restrict_generated_store_test requires scoped=1, strict=0")
    `DV_CHECK_FATAL(cfg.restrict_dv_stores_to_data(),
                    "Scoped mode must bound generated stores")
    source_stream = riscv_hypertest_case_stream::type_id::create("source_stream");
    source_stream.cfg = cfg;
    asm_gen.check_restricted_directed_stream(source_stream);
    `DV_CHECK_FATAL(source_stream.preserve_runtime_regs,
                    "Scoped Hypertest stream must preserve runtime registers")
    check_scalar_boundaries();
    check_atomic_boundaries();
  endfunction
endclass

// A registered stream with an unproven address intentionally remains outside
// the production allowlist. The negative regression must reject its actual
// factory type before post_randomize can emit a store.
class riscv_unproven_store_test_stream extends riscv_directed_instr_stream;
  `uvm_object_utils(riscv_unproven_store_test_stream)
  `uvm_object_new

  function void post_randomize();
    riscv_instr store_instr = riscv_instr::get_instr(SW);
    `DV_CHECK_RANDOMIZE_FATAL(store_instr)
    instr_list.push_back(store_instr);
    super.post_randomize();
  endfunction
endclass
