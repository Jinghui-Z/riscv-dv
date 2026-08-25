/*
 * Copyright 2020 Google LLC
 * Copyright 2020 Andes Technology Co., Ltd.
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


// Base class for the RISC-V vector ISA. Legacy v0.9 output remains available,
// while use_vector_1_0 selects the ratified instruction set and syntax.
class riscv_vector_instr extends riscv_floating_point_instr;

  rand riscv_vreg_t vs1;
  rand riscv_vreg_t vs2;
  rand riscv_vreg_t vs3;
  rand riscv_vreg_t vd;
  rand va_variant_t va_variant;
  rand bit          vm;
  rand bit          wd;
  rand bit [10:0]   eew;
  bit               has_vd = 1'b1;
  bit               has_vs1 = 1'b1;
  bit               has_vs2 = 1'b1;
  bit               has_vs3 = 1'b1;
  bit               has_vm = 1'b0;
  bit               has_va_variant;
  bit               is_widening_instr;
  bit               is_narrowing_instr;
  bit               is_legacy_narrowing_instr;
  bit               is_reduction_instr;
  bit               is_widening_reduction_instr;
  bit               is_quad_widening_instr;
  bit               is_convert_instr;
  bit               is_whole_register_mem;
  bit               is_mask_register_mem;
  int unsigned      whole_register_count;
  int unsigned      fixed_mem_eew;
  int unsigned      extension_factor;
  va_variant_t      allowed_va_variants[$];
  string            sub_extension;
  rand bit [2:0]    nfields; // Used by segmented load/store
  rand bit [3:0]    emul;      // Register footprint of the memory/index operand
  rand bit [3:0]    data_emul; // Register footprint of the loaded/stored data

  constraint avoid_reserved_vregs_c {
    if (has_vd && (m_cfg.vector_cfg.reserved_vregs.size() > 0)) {
      !(vd inside {m_cfg.vector_cfg.reserved_vregs});
    }
  }

  constraint va_variant_c {
    if (has_va_variant) {
      va_variant inside {allowed_va_variants};
    }
  }

  // Section 3.3.2: Vector Register Grouping (vlmul)
  // Instructions specifying a vector operand with an odd-numbered vector register will raisean
  // illegal instruction exception.
  // TODO: Exclude the instruction that ignore VLMUL
  constraint operand_group_c {
    if (!is_whole_register_mem && !is_mask_register_mem &&
        (extension_factor == 0) && (instr_name != VRGATHEREI16) &&
        !m_cfg.vector_cfg.vtype.fractional_lmul &&
        (m_cfg.vector_cfg.vtype.vlmul > 1)) {
      if (m_cfg.use_vector_1_0 && is_reduction_instr) {
        // Reduction inputs and results are scalars in element zero. Only the
        // vs2 vector operand uses the current LMUL register grouping.
        vs2 % m_cfg.vector_cfg.vtype.vlmul == 0;
      } else {
        vd  % m_cfg.vector_cfg.vtype.vlmul == 0;
        vs1 % m_cfg.vector_cfg.vtype.vlmul == 0;
        vs2 % m_cfg.vector_cfg.vtype.vlmul == 0;
        vs3 % m_cfg.vector_cfg.vtype.vlmul == 0;
      }
    }
  }

  // Section 11.2: Widening Vector Arithmetic Instructions
  constraint widening_instr_c {
    if (is_widening_instr) {
     2 * m_cfg.vector_cfg.vtype.vsew <= ELEN;
     if (!m_cfg.use_vector_1_0 || !is_widening_reduction_instr) {
       if (!m_cfg.vector_cfg.vtype.fractional_lmul) {
         m_cfg.vector_cfg.vtype.vlmul <= 4;
       }
       if (!m_cfg.vector_cfg.vtype.fractional_lmul) {
         // The destination group has twice the current integer LMUL.
         vd % (m_cfg.vector_cfg.vtype.vlmul * 2) == 0;
         !(vs1 inside {[vd : vd + m_cfg.vector_cfg.vtype.vlmul * 2 - 1]});
         !(vs2 inside {[vd : vd + m_cfg.vector_cfg.vtype.vlmul * 2 - 1]});
       } else {
         // A fractional source and its widened destination each occupy at most
         // one architectural register for mf2/mf4/mf8.
         vd != vs1;
         vd != vs2;
       }
       (vm == 0) -> (vd != 0);
       // Double-width result, first source double-width, second source single-width
       if (!m_cfg.vector_cfg.vtype.fractional_lmul &&
           (va_variant inside {WV, WF, WX})) {
         vs2 % (m_cfg.vector_cfg.vtype.vlmul * 2) == 0;
       }
     }
    }
  }

  // Section 11.3: Narrowing Vector Arithmetic Instructions
  constraint narrowing_instr_c {
    if ((m_cfg.use_vector_1_0 && is_narrowing_instr) ||
        (!m_cfg.use_vector_1_0 && is_legacy_narrowing_instr)) {
      2 * m_cfg.vector_cfg.vtype.vsew <= ELEN;
      if (!m_cfg.vector_cfg.vtype.fractional_lmul) {
        m_cfg.vector_cfg.vtype.vlmul <= 4;
      }
      if (!m_cfg.vector_cfg.vtype.fractional_lmul) {
        vs2 % (m_cfg.vector_cfg.vtype.vlmul * 2) == 0;
        !(vd inside {[vs2 : vs2 + m_cfg.vector_cfg.vtype.vlmul * 2 - 1]});
      } else {
        vd != vs2;
      }
      // The destination vector register group cannot overlap the mask register
      // if used, unless LMUL=1 (implemented in vmask_overlap_c)
    }
  }

  // 12.3. Vector Integer Add-with-Carry / Subtract-with-Borrow Instructions
  constraint add_sub_with_carry_c {
    if (!m_cfg.vector_cfg.vtype.fractional_lmul &&
        (m_cfg.vector_cfg.vtype.vlmul > 1)) {
      // For vadc and vsbc, an illegal instruction exception is raised if the
      // destination vector register is v0 and LMUL> 1
      if (instr_name inside {VADC, VSBC}) {
        vd != 0;
      }
      // For vmadc and vmsbc, an illegal instruction exception is raised if the
      // destination vector register overlaps asource vector register group and LMUL > 1
      if (instr_name inside {VMADC, VMSBC}) {
        vd != vs2;
        vd != vs1;
      }
    }
  }

  // 12.7. Vector Integer Comparison Instructions
  // For all comparison instructions, an illegal instruction exception is raised if the
  // destination vector register overlaps a source vector register group and LMUL > 1
  constraint compare_instr_c {
    if (category == COMPARE) {
      vd != vs2;
      vd != vs1;
    }
  }

  // 16.8. Vector Iota Instruction
  // An illegal instruction exception is raised if the destination vector register group
  // overlaps the source vector mask register. If the instruction is masked, an illegal
  // instruction exception is issued if the destination vector register group overlaps v0.
  constraint vector_itoa_c {
    if (instr_name == VIOTA_M) {
      vd != vs2;
      (vm == 0) -> (vd != 0);
    }
  }

  // 16.9. Vector Element Index Instruction
  // The vs2 eld of the instruction must be set to v0, otherwise the encoding is reserved
  constraint vector_element_index_c {
    if (instr_name == VID_V) {
      vs2 == 0;
      // TODO; Check if this constraint is needed
      vd != vs2;
    }
  }

  // Section 17.3  Vector Slide Instructions
  // The destination vector register group for vslideup cannot overlap the vector register
  // group of the source vector register group or the mask register
  constraint vector_slide_c {
    if (instr_name inside {VSLIDEUP, VSLIDE1UP, VSLIDEDOWN, VSLIDE1DOWN,
                           VFSLIDE1UP, VFSLIDE1DOWN}) {
      vd != vs2;
      vd != vs1;
      (vm == 0) -> (vd != 0);
    }
  }

  // Section 17.4: Vector Register Gather Instruction
  // For any vrgather instruction, the destination vector register group cannot overlap
  // with the source vector register group
  constraint vector_gather_c {
    if (instr_name inside {VRGATHER, VRGATHEREI16}) {
      vd != vs2;
      vd != vs1;
      (vm == 0) -> (vd != 0);
    }
  }

  // vrgatherei16.vv uses SEW/LMUL for data and EEW=16 with
  // EMUL=(16/SEW)*LMUL for its index operand. Keep all three groups
  // disjoint so every generated overlap is legal and restartable.
  constraint vrgatherei16_c {
    if (instr_name == VRGATHEREI16) {
      if (m_cfg.vector_cfg.vtype.fractional_lmul) {
        m_cfg.vector_cfg.vtype.vsew * m_cfg.vector_cfg.vtype.vlmul <= 128;
        vd != vs2;
        vd != vs1;
        if (m_cfg.vector_cfg.vtype.vsew != 16) {
          vs1 != vs2;
        }
      } else {
        16 * m_cfg.vector_cfg.vtype.vlmul <=
            8 * m_cfg.vector_cfg.vtype.vsew;
        128 * m_cfg.vector_cfg.vtype.vlmul >=
              m_cfg.vector_cfg.vtype.vsew;
        if (m_cfg.vector_cfg.vtype.vlmul > 1) {
          vd  % m_cfg.vector_cfg.vtype.vlmul == 0;
          vs2 % m_cfg.vector_cfg.vtype.vlmul == 0;
        }
        if (16 * m_cfg.vector_cfg.vtype.vlmul >
            m_cfg.vector_cfg.vtype.vsew) {
          vs1 % ((16 * m_cfg.vector_cfg.vtype.vlmul) /
                 m_cfg.vector_cfg.vtype.vsew) == 0;
          (int'(vd) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs1)) ||
          (int'(vs1) + (16 * m_cfg.vector_cfg.vtype.vlmul) /
                       m_cfg.vector_cfg.vtype.vsew <= int'(vd));
          if (m_cfg.vector_cfg.vtype.vsew != 16) {
            (int'(vs2) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs1)) ||
            (int'(vs1) + (16 * m_cfg.vector_cfg.vtype.vlmul) /
                         m_cfg.vector_cfg.vtype.vsew <= int'(vs2));
          }
        } else {
          (int'(vd) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs1)) ||
          (int'(vs1) + 1 <= int'(vd));
          if (m_cfg.vector_cfg.vtype.vsew != 16) {
            (int'(vs2) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs1)) ||
            (int'(vs1) + 1 <= int'(vs2));
          }
        }
        (int'(vd) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs2)) ||
        (int'(vs2) + m_cfg.vector_cfg.vtype.vlmul <= int'(vd));
      }
    }
  }

  // vzext/vsext source EEW is SEW/factor and source EMUL is LMUL/factor.
  // Conservative non-overlap keeps the generated forms legal for restart.
  constraint vector_integer_extension_c {
    if (extension_factor != 0) {
      m_cfg.vector_cfg.vtype.vsew >= 8 * extension_factor;
      if (m_cfg.vector_cfg.vtype.fractional_lmul) {
        m_cfg.vector_cfg.vtype.vlmul * extension_factor <= 8;
        vd != vs2;
      } else {
        if (m_cfg.vector_cfg.vtype.vlmul > 1) {
          vd % m_cfg.vector_cfg.vtype.vlmul == 0;
        }
        if (m_cfg.vector_cfg.vtype.vlmul > extension_factor) {
          vs2 % (m_cfg.vector_cfg.vtype.vlmul / extension_factor) == 0;
          (int'(vd) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs2)) ||
          (int'(vs2) + m_cfg.vector_cfg.vtype.vlmul / extension_factor <= int'(vd));
        } else {
          (int'(vd) + m_cfg.vector_cfg.vtype.vlmul <= int'(vs2)) ||
          (int'(vs2) + 1 <= int'(vd));
        }
      }
    }
  }

  // Section 17.5: Vector compress instruction
  // The destination vector register group cannot overlap the source vector register
  // group or the source vector mask register
  constraint vector_compress_c {
    if (instr_name == VCOMPRESS) {
      vd != vs2;
      vd != vs1;
      (vm == 0) -> (vd != 0);
    }
  }

  // Section 7.8. Vector Load/Store Segment Instructions
  // The LMUL setting must be such that LMUL * NFIELDS <= 8
  // Vector register numbers accessed by the segment load or store would increment
  // cannot past 31
  constraint nfields_c {
    if (check_sub_extension(sub_extension, "zvlsseg")) {
      // Segment mnemonics start at two fields. A data EMUL of eight cannot
      // satisfy the architectural NFIELDS*EMUL limit.
      data_emul < 8;
      nfields > 0;
      (nfields + 1) * data_emul <= 8;
      if (category == LOAD) {
        vd + (nfields + 1) * data_emul - 1 <= 31;
      }
      if (category == STORE) {
        vs3 + (nfields + 1) * data_emul - 1 <= 31;
      }
    }
  }

  constraint vmv_alignment_c {
    if (instr_name == VMV2R_V) {
      int'(vs2) % 2 == 0;
      int'(vd)  % 2 == 0;
    }
    if (instr_name == VMV4R_V) {
      int'(vs2) % 4 == 0;
      int'(vd)  % 4 == 0;
    }
    if (instr_name == VMV8R_V) {
      int'(vs2) % 8 == 0;
      int'(vd)  % 8 == 0;
    }
  }

  // Whole-register transfers ignore vtype/vl and use the encoded EEW only as
  // a load hint. Mask transfers always operate as unmasked byte accesses.
  constraint special_memory_c {
    if (is_whole_register_mem) {
      vm == 1'b1;
      eew == fixed_mem_eew;
      emul == whole_register_count;
      data_emul == whole_register_count;
      fixed_mem_eew <= ELEN;
      if (whole_register_count > 1) {
        if (category == LOAD) {
          int'(vd) % whole_register_count == 0;
        } else {
          int'(vs3) % whole_register_count == 0;
        }
      }
      if (category == LOAD) {
        int'(vd) + whole_register_count <= 32;
      } else {
        int'(vs3) + whole_register_count <= 32;
      }
    }
    if (is_mask_register_mem) {
      vm == 1'b1;
      eew == 8;
      emul == 1;
      data_emul == 1;
    }
  }

  /////////////////// Vector mask constraint ///////////////////

  // Section 5.3
  // The destination vector register group for a masked vector instruction can only overlap
  // the source mask register (v0) when LMUL=1
  constraint vmask_overlap_c {
    (vm == 0) && !(m_cfg.use_vector_1_0 && is_reduction_instr) &&
        !m_cfg.vector_cfg.vtype.fractional_lmul &&
        (m_cfg.vector_cfg.vtype.vlmul > 1) -> (vd != 0);
  }

  constraint vector_mask_enable_c {
    // Below instruction is always masked
    if (instr_name inside {VMERGE, VFMERGE, VADC, VSBC}) {
      vm == 1'b0;
    }
  }

  // Carry-in forms consume the v0 mask operand; the ordinary forms encode no
  // mask operand.  The suffix and the vm bit must describe the same form.
  constraint carry_variant_vm_c {
    if (instr_name inside {VMADC, VMSBC}) {
      if (va_variant inside {VVM, VXM, VIM}) {
        vm == 1'b0;
      } else if (va_variant inside {VV, VX, VI}) {
        vm == 1'b1;
      }
    }
  }

  constraint vector_mask_disable_c {
    // (vm=0) is reserved for below ops
    if (instr_name inside {VMV, VFMV, VCOMPRESS, VFMV_F_S, VFMV_S_F, VMV_X_S, VMV_S_X,
                           VMV1R_V, VMV2R_V, VMV4R_V, VMV8R_V}) {
      vm == 1'b1;
    }
  }

  // 16.1. Vector Mask-Register Logical Instructions
  // No vector mask for these instructions
  constraint vector_mask_instr_c {
    if (instr_name inside {[VMAND_MM : VMXNOR_MM]}) {
      vm == 1'b1;
    }
  }

  constraint disable_floating_point_varaint_c {
    if (!m_cfg.vector_cfg.vec_fp) {
      !(va_variant inside {VF, WF});
    }
  }

  constraint vector_load_store_mask_overlap_c {
    // TODO: Check why this is needed?
    if ((category == STORE) && !is_whole_register_mem && !is_mask_register_mem) {
      (vm == 0) -> (vs3 != 0);
      vs2 != vs3;
    }
    // 7.8.3 For vector indexed segment loads, the destination vector register groups
    // cannot overlap the source vectorregister group (specied by vs2), nor can they
    // overlap the mask register if masked
    // AMO instruction uses indexed address mode
    if (format inside {VLX_FORMAT, VAMO_FORMAT}) {
      vd != vs2;
    }
    if (m_cfg.use_vector_1_0 && (format == VLX_FORMAT)) {
      if (sub_extension == "zvlsseg") {
        (vd + (nfields + 1) * data_emul <= vs2) ||
        (vs2 + emul <= vd);
      } else {
        (vd + data_emul <= vs2) || (vs2 + emul <= vd);
      }
    }
  }

  // load/store EEW/EMUL and corresponding register grouping constraints
  constraint load_store_solve_order_c {
    solve eew before emul;
    solve emul before data_emul;
    solve data_emul before nfields;
    solve data_emul before vd;
    solve data_emul before vs3;
    solve emul before vd;
    solve emul before vs1;
    solve emul before vs2;
    solve emul before vs3;
  }

  constraint load_store_eew_emul_c {
    if ((category inside {LOAD, STORE, AMO}) &&
        !is_whole_register_mem && !is_mask_register_mem) {
      eew inside {m_cfg.vector_cfg.legal_eew};
      if (m_cfg.use_vector_1_0) {
        // V 1.0 memory width encodings only name 8/16/32/64-bit EEWs.
        eew inside {8, 16, 32, 64};
      }
      if (m_cfg.vector_cfg.vtype.fractional_lmul) {
        if (eew > m_cfg.vector_cfg.vtype.vsew * m_cfg.vector_cfg.vtype.vlmul) {
          emul == eew / (m_cfg.vector_cfg.vtype.vsew * m_cfg.vector_cfg.vtype.vlmul);
        } else {
          emul == 1;
        }
      } else {
        if (eew * m_cfg.vector_cfg.vtype.vlmul > m_cfg.vector_cfg.vtype.vsew) {
          emul == (eew * m_cfg.vector_cfg.vtype.vlmul) /
                  m_cfg.vector_cfg.vtype.vsew;
        } else {
          emul == 1;
        }
      }

      // For indexed accesses, EEW and EMUL describe the index vector. The
      // loaded/stored data retains SEW and LMUL. Other memory forms use the
      // memory operand's EMUL for the data register group as well.
      if (m_cfg.use_vector_1_0 && (format inside {VLX_FORMAT, VSX_FORMAT})) {
        if (m_cfg.vector_cfg.vtype.fractional_lmul) {
          data_emul == 1;
        } else {
          data_emul == m_cfg.vector_cfg.vtype.vlmul;
        }
      } else {
        data_emul == emul;
      }

      if ((format inside {VLX_FORMAT, VSX_FORMAT}) && m_cfg.use_vector_1_0) {
        if (emul > 1) {
          vs2 % emul == 0;
        }
        if (data_emul > 1) {
          vd  % data_emul == 0;
          vs3 % data_emul == 0;
        }
      } else if (emul > 1) {
        vd  % emul == 0;
        vs1 % emul == 0;
        vs2 % emul == 0;
        vs3 % emul == 0;
      }
    }
  }

  // Some temporarily constraint to avoid illegal instruction
  // TODO: Review these constraints
  constraint temp_c {
    (vm == 0) && !(m_cfg.use_vector_1_0 && is_reduction_instr) -> (vd != 0);
  }

  `uvm_object_utils(riscv_vector_instr)
  `uvm_object_new

  // OPIVI instructions encode a five-bit immediate. Arithmetic/logical
  // immediates are sign-extended, while shifts and element indices use an
  // unsigned value. Without this override, VA_FORMAT leaves imm_len at zero
  // and every generated .vi instruction collapses to immediate zero.
  virtual function void set_imm_len();
    if (format == VA_FORMAT) begin
      imm_len = 5;
      if (instr_name inside {VSLL, VSRL, VSRA, VNSRL, VNSRA,
                             VSSRL, VSSRA, VNCLIPU, VNCLIP,
                             VSLIDEUP, VSLIDEDOWN, VRGATHER}) begin
        imm_type = UIMM;
      end else begin
        imm_type = IMM;
      end
      imm_mask = 32'hffff_ffff << imm_len;
    end else begin
      super.set_imm_len();
    end
  endfunction : set_imm_len

  // Filter unsupported instructions based on configuration
  virtual function bit is_supported(riscv_instr_gen_config cfg);
    string name = instr_name.name();
    if (cfg.use_vector_1_0) begin
      // Vector AMOs were removed before V 1.0. vpopc.m was renamed vcpop.m.
      if ((format == VAMO_FORMAT) || (instr_name == VPOPC_M)) begin
        return 1'b0;
      end
    end else begin
      if (instr_name inside {VLUXEI_V, VLUXSEGEI_V}) begin
        // These names were introduced by the ratified V 1.0 indexed-load split.
        return 1'b0;
      end
      if ((instr_name == VCPOP_M) || is_whole_register_mem || is_mask_register_mem ||
          (extension_factor != 0) ||
          (instr_name inside {VSMUL, VFRSQRT7_V, VFREC7_V, VFWADD, VFWSUB,
                              VRGATHEREI16, VFSLIDE1UP, VFSLIDE1DOWN, VFREDMIN_VS,
                              VFCVT_RTZ_XU_F_V, VFCVT_RTZ_X_F_V,
                              VFWCVT_RTZ_XU_F_V, VFWCVT_RTZ_X_F_V,
                              VFNCVT_RTZ_XU_F_W, VFNCVT_RTZ_X_F_W})) begin
        return 1'b0;
      end
    end
    if (is_whole_register_mem && (fixed_mem_eew > ELEN)) begin
      return 1'b0;
    end
    if (extension_factor != 0) begin
      if (cfg.vector_cfg.vtype.vsew < 8 * extension_factor) begin
        return 1'b0;
      end
      if (cfg.vector_cfg.vtype.fractional_lmul &&
          (cfg.vector_cfg.vtype.vlmul * extension_factor > 8)) begin
        return 1'b0;
      end
    end
    if (instr_name == VRGATHEREI16) begin
      if (cfg.vector_cfg.vtype.fractional_lmul) begin
        if (cfg.vector_cfg.vtype.vsew * cfg.vector_cfg.vtype.vlmul > 128) begin
          return 1'b0;
        end
      end else if ((16 * cfg.vector_cfg.vtype.vlmul >
                    8 * cfg.vector_cfg.vtype.vsew) ||
                   (128 * cfg.vector_cfg.vtype.vlmul <
                    cfg.vector_cfg.vtype.vsew)) begin
        return 1'b0;
      end
    end
    if ((sub_extension == "zvlsseg") && !cfg.vector_cfg.enable_zvlsseg) begin
      return 1'b0;
    end
    if (cfg.use_vector_1_0 && (instr_name inside {VLEFF_V, VLSEGEFF_V}) &&
        !cfg.vector_cfg.enable_fault_only_first_load) begin
      return 1'b0;
    end
    // 19.2.2. Vector Add with Carry/Subtract with Borrow Reserved under EDIV>1
    if ((cfg.vector_cfg.vtype.vediv > 1) &&
        (instr_name inside {VADC, VSBC, VMADC, VMSBC})) begin
      return 1'b0;
    end
    if (is_widening_instr ||
        (cfg.use_vector_1_0 ? is_narrowing_instr : is_legacy_narrowing_instr)) begin
      if (!cfg.vector_cfg.vec_narrowing_widening ||
          (2 * cfg.vector_cfg.vtype.vsew > ELEN) ||
          (!cfg.vector_cfg.vtype.fractional_lmul &&
           (cfg.vector_cfg.vtype.vlmul > 4) &&
           !(cfg.use_vector_1_0 && is_widening_reduction_instr))) begin
        return 1'b0;
      end
    end
    if (!cfg.vector_cfg.vec_quad_widening && is_quad_widening_instr) begin
      return 1'b0;
    end
    // Older vector assemblers used by the legacy path reject these forms.
    // They are ratified V 1.0 instructions and must remain available there.
    if (!cfg.use_vector_1_0 &&
        (instr_name inside {VWMACCSU, VMERGE, VFMERGE, VMADC, VMSBC})) begin
      return 1'b0;
    end
    // The standard vector floating-point instructions treat 16-bit, 32-bit, 64-bit,
    // and 128-bit elements as IEEE-754/2008-compatible values. If the current SEW does
    // not correspond to a supported IEEE floating-pointtype, an illegal instruction
    // exception is raised
    if (!cfg.vector_cfg.vec_fp) begin
      if ((!cfg.use_vector_1_0 || (instr_name != VFIRST_M)) &&
          ((name.substr(0, 1) == "VF") || (name.substr(0, 2) == "VMF"))) begin
        return 1'b0;
      end
    end
    return 1'b1;
  endfunction

  virtual function string get_instr_name();
    string name = super.get_instr_name();
    if ((m_cfg != null) && m_cfg.use_vector_1_0) begin
      case (instr_name)
        // V 1.0 split indexed accesses into ordered and unordered forms.
        VLUXEI_V:    name = "VLUXEI.V";
        VLXEI_V:     name = "VLOXEI.V";
        VLUXSEGEI_V: name = "VLUXSEGEI.V";
        VSXEI_V:     name = "VSOXEI.V";
        VLXSEGEI_V:  name = "VLOXSEGEI.V";
        VSXSEGEI_V:  name = "VSOXSEGEI.V";
        VSUXSEGEI_V: name = "VSUXSEGEI.V";
        VFREDSUM_VS:   name = "VFREDUSUM.VS";
        VFWREDSUM_VS:  name = "VFWREDUSUM.VS";
        VMANDNOT_MM:   name = "VMANDN.MM";
        VMORNOT_MM:    name = "VMORN.MM";
        default: ;
      endcase
    end
    if ((category inside {LOAD, STORE}) &&
        !is_whole_register_mem && !is_mask_register_mem) begin
      // Add eew before ".v" or "ff.v" suffix
      if (instr_name inside {VLEFF_V, VLSEGEFF_V}) begin
        name = name.substr(0, name.len() - 5);
        name = $sformatf("%0s%0dFF.V", name, eew);
      end else begin
        name = name.substr(0, name.len() - 3);
        name = $sformatf("%0s%0d.V", name, eew);
      end
      `uvm_info(`gfn, $sformatf("%0s -> %0s", super.get_instr_name(), name), UVM_LOW)
    end
    return name;
  endfunction

  // Convert the instruction to assembly code
  virtual function string convert2asm(string prefix = "");
    string asm_str;
    case (format)
      VS2_FORMAT: begin
        if (instr_name == VID_V) begin
          asm_str = $sformatf("vid.v %s", vd.name());
        end else if (instr_name inside {VPOPC_M, VCPOP_M, VFIRST_M}) begin
          asm_str = $sformatf("%0s %0s,%0s", get_instr_name(), rd.name(), vs2.name());
        end else begin
          asm_str = $sformatf("%0s %0s,%0s", get_instr_name(), vd.name(), vs2.name());
        end
      end
      VA_FORMAT: begin
        if (instr_name == VMV) begin
          case (va_variant)
            VV: asm_str = $sformatf("vmv.v.v %s,%s", vd.name(), vs1.name());
            VX: asm_str = $sformatf("vmv.v.x %s,%s", vd.name(), rs1.name());
            VI: asm_str = $sformatf("vmv.v.i %s,%s", vd.name(), imm_str);
            default: `uvm_info(`gfn, $sformatf("Unsupported va_variant %0s", va_variant), UVM_LOW)
          endcase
        end else if (instr_name == VFMV) begin
          asm_str = $sformatf("vfmv.v.f %s,%s", vd.name(), fs1.name());
        end else if (instr_name == VMV_X_S) begin
          asm_str = $sformatf("vmv.x.s %s,%s", rd.name(), vs2.name());
        end else if (instr_name == VMV_S_X) begin
          asm_str = $sformatf("vmv.s.x %s,%s", vd.name(), rs1.name());
        end else if (instr_name == VFMV_F_S) begin
          asm_str = $sformatf("vfmv.f.s %s,%s", fd.name(), vs2.name());
        end else if (instr_name == VFMV_S_F) begin
          asm_str = $sformatf("vfmv.s.f %s,%s", vd.name(), fs1.name());
        end else begin
          if (!has_va_variant) begin
            asm_str = $sformatf("%0s ", get_instr_name());
            asm_str = format_string(asm_str, MAX_INSTR_STR_LEN);
            asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), vs2.name(), vs1.name())};
          end else begin
            asm_str = $sformatf("%0s.%0s ", get_instr_name(), va_variant.name());
            asm_str = format_string(asm_str, MAX_INSTR_STR_LEN);
            case (va_variant) inside
              WV, VV, VVM, VM: begin
                asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), vs2.name(), vs1.name())};
              end
              WI, VI, VIM: begin
                asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), vs2.name(), imm_str)};
              end
              VF, WF, VFM: begin
                if (instr_name inside {VFMADD, VFNMADD, VFMACC, VFNMACC, VFNMSUB, VFWNMSAC,
                                       VFWMACC, VFMSUB, VFMSAC, VFNMSAC, VFWNMACC, VFWMSAC}) begin
                  asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), fs1.name(), vs2.name())};
                end else begin
                  asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), vs2.name(), fs1.name())};
                end
              end
              WX, VX, VXM: begin
                if (instr_name inside {VMADD, VNMSUB, VMACC, VNMSAC, VWMACCSU, VWMACCU,
                                       VWMACCUS, VWMACC}) begin
                  asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), rs1.name(), vs2.name())};
                end else begin
                  asm_str = {asm_str, $sformatf("%0s,%0s,%0s", vd.name(), vs2.name(), rs1.name())};
                end
              end
            endcase
          end
        end
      end
      VL_FORMAT: begin
        if (sub_extension == "zvlsseg") begin
          asm_str = $sformatf("%0s %s,(%s)", add_nfields(get_instr_name(), "vlseg"),
                                             vd.name(), rs1.name());
        end else begin
          asm_str = $sformatf("%0s %s,(%s)", get_instr_name(), vd.name(), rs1.name());
        end
      end
      VS_FORMAT: begin
        if (sub_extension == "zvlsseg") begin
          asm_str = $sformatf("%0s %s,(%s)", add_nfields(get_instr_name(), "vsseg"),
                                             vs3.name(), rs1.name());
        end else begin
          asm_str = $sformatf("%0s %s,(%s)", get_instr_name(), vs3.name(), rs1.name());
        end
      end
      VLS_FORMAT: begin
        if (sub_extension == "zvlsseg") begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s", add_nfields(get_instr_name(), "vlsseg"),
                                                   vd.name(), rs1.name(), rs2.name());
        end else begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s", get_instr_name(),
                                                   vd.name(), rs1.name(), rs2.name());
        end
      end
      VSS_FORMAT: begin
        if (sub_extension == "zvlsseg") begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s", add_nfields(get_instr_name(), "vssseg"),
                                                   vs3.name(), rs1.name(), rs2.name());
        end else begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s", get_instr_name(),
                                                   vs3.name(), rs1.name(), rs2.name());
        end
      end
      VLX_FORMAT: begin
        if (sub_extension == "zvlsseg") begin
          string prefix;
          if ((m_cfg != null) && m_cfg.use_vector_1_0) begin
            prefix = (instr_name == VLUXSEGEI_V) ? "vluxseg" : "vloxseg";
          end else begin
            prefix = "vlxseg";
          end
          asm_str = $sformatf("%0s %0s,(%0s),%0s", add_nfields(get_instr_name(), prefix),
                                                   vd.name(), rs1.name(), vs2.name());
        end else begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s", get_instr_name(),
                                                   vd.name(), rs1.name(), vs2.name());
        end
      end
      VSX_FORMAT: begin
        if (sub_extension == "zvlsseg") begin
          string seg_prefix;
          if ((m_cfg != null) && m_cfg.use_vector_1_0) begin
            seg_prefix = (instr_name == VSUXSEGEI_V) ? "vsuxseg" : "vsoxseg";
          end else begin
            seg_prefix = (instr_name == VSUXSEGEI_V) ? "vsuxseg" : "vsxseg";
          end
          asm_str = $sformatf("%0s %0s,(%0s),%0s", add_nfields(get_instr_name(), seg_prefix),
                                                   vs3.name(), rs1.name(), vs2.name());
        end else begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s", get_instr_name(),
                                                   vs3.name(), rs1.name(), vs2.name());
        end
      end
      VAMO_FORMAT: begin
        if (wd) begin
          asm_str = $sformatf("%0s %0s,(%0s),%0s,%0s", get_instr_name(), vd.name(),
                                                   rs1.name(), vs2.name(), vd.name());
        end else begin
          asm_str = $sformatf("%0s x0,(%0s),%0s,%0s", get_instr_name(),
                                                  rs1.name(), vs2.name(), vs3.name());
        end
      end
      default: begin
        `uvm_fatal(`gfn, $sformatf("Unsupported format %0s", format.name()))
      end
    endcase
    // Add vector mask
    asm_str = {asm_str, vec_vm_str()};
    if(comment != "") begin
      asm_str = {asm_str, " #",comment};
    end
    return asm_str.tolower();
  endfunction : convert2asm

  function void pre_randomize();
    super.pre_randomize();
    vs1.rand_mode(has_vs1);
    vs2.rand_mode(has_vs2);
    vs3.rand_mode(has_vs3);
    vd.rand_mode(has_vd);
    if (!(category inside {LOAD, STORE, AMO})) begin
      load_store_solve_order_c.constraint_mode(0);
    end
  endfunction : pre_randomize

  virtual function void set_rand_mode();
    string name = instr_name.name();
    has_rs1 = 1;
    has_rs2 = 0;
    has_rd  = 0;
    has_fs1 = 0;
    has_fs2 = 0;
    has_fs3 = 0;
    has_fd  = 0;
    has_imm = 0;
    case (instr_name)
      VLM_V, VSM_V: begin
        is_mask_register_mem = 1'b1;
        fixed_mem_eew = 8;
      end
      VL1RE8_V, VS1R_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 1;
        fixed_mem_eew = 8;
      end
      VL1RE16_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 1;
        fixed_mem_eew = 16;
      end
      VL1RE32_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 1;
        fixed_mem_eew = 32;
      end
      VL1RE64_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 1;
        fixed_mem_eew = 64;
      end
      VL2RE8_V, VS2R_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 2;
        fixed_mem_eew = 8;
      end
      VL2RE16_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 2;
        fixed_mem_eew = 16;
      end
      VL2RE32_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 2;
        fixed_mem_eew = 32;
      end
      VL2RE64_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 2;
        fixed_mem_eew = 64;
      end
      VL4RE8_V, VS4R_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 4;
        fixed_mem_eew = 8;
      end
      VL4RE16_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 4;
        fixed_mem_eew = 16;
      end
      VL4RE32_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 4;
        fixed_mem_eew = 32;
      end
      VL4RE64_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 4;
        fixed_mem_eew = 64;
      end
      VL8RE8_V, VS8R_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 8;
        fixed_mem_eew = 8;
      end
      VL8RE16_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 8;
        fixed_mem_eew = 16;
      end
      VL8RE32_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 8;
        fixed_mem_eew = 32;
      end
      VL8RE64_V: begin
        is_whole_register_mem = 1'b1;
        whole_register_count = 8;
        fixed_mem_eew = 64;
      end
      VZEXT_VF2, VSEXT_VF2: extension_factor = 2;
      VZEXT_VF4, VSEXT_VF4: extension_factor = 4;
      VZEXT_VF8, VSEXT_VF8: extension_factor = 8;
      default: ;
    endcase
    if (is_whole_register_mem || is_mask_register_mem) begin
      has_vs1 = 1'b0;
      has_vs2 = 1'b0;
      if (category == LOAD) begin
        has_vs3 = 1'b0;
      end else begin
        has_vd = 1'b0;
      end
    end
    if (sub_extension != "zvlsseg") begin
      nfields.rand_mode(0);
    end
    if ((name.substr(0, 1) == "VW") || (name.substr(0, 2) == "VFW")) begin
      is_widening_instr = 1'b1;
    end
    if (name.substr(0, 2) == "VQW") begin
      is_quad_widening_instr = 1'b1;
      is_widening_instr = 1'b1;
    end
    // Preserve the legacy prefix classification for v0.9 output. Ratified V
    // uses the explicit list so negative multiply-accumulate instructions
    // (vnms*/vfnm*) are not mistaken for narrowing operations.
    if ((name.substr(0, 1) == "VN") || (name.substr(0, 2) == "VFN")) begin
      is_legacy_narrowing_instr = 1'b1;
    end
    if (instr_name inside {VNSRL, VNSRA, VNCLIPU, VNCLIP,
                           VFNCVT_XU_F_W, VFNCVT_X_F_W,
                           VFNCVT_RTZ_XU_F_W, VFNCVT_RTZ_X_F_W,
                           VFNCVT_F_XU_W, VFNCVT_F_X_W,
                           VFNCVT_F_F_W, VFNCVT_ROD_F_F_W}) begin
      is_narrowing_instr = 1'b1;
    end
    if (instr_name inside {VREDSUM_VS, VREDMAXU_VS, VREDMAX_VS,
                           VREDMINU_VS, VREDMIN_VS, VREDAND_VS,
                           VREDOR_VS, VREDXOR_VS, VWREDSUMU_VS,
                           VWREDSUM_VS, VFREDOSUM_VS, VFREDSUM_VS,
                           VFREDMAX_VS, VFREDMIN_VS, VFWREDOSUM_VS,
                           VFWREDSUM_VS}) begin
      is_reduction_instr = 1'b1;
    end
    if (instr_name inside {VWREDSUMU_VS, VWREDSUM_VS,
                           VFWREDOSUM_VS, VFWREDSUM_VS}) begin
      is_widening_reduction_instr = 1'b1;
    end
    if (uvm_is_match("*CVT*", name)) begin
      is_convert_instr = 1'b1;
      has_vs1 = 1'b0;
    end
    if (allowed_va_variants.size() > 0) begin
      has_va_variant = 1;
    end
    // Set the rand mode based on the superset of all VA variants
    if (format == VA_FORMAT) begin
      has_imm = (VI inside {allowed_va_variants}) ||
                (VIM inside {allowed_va_variants}) ||
                (WI inside {allowed_va_variants});
      has_rs1 = 1'b1;
      has_fs1 = 1'b1;
    end
    if (format == VS2_FORMAT) begin
      has_vs1 = 1'b0;
      has_vs3 = 1'b0;
      if (instr_name inside {VPOPC_M, VCPOP_M, VFIRST_M}) begin
        has_rd = 1'b1;
        has_vd = 1'b0;
      end
    end
  endfunction : set_rand_mode

  virtual function string vec_vm_str();
    if (vm) begin
      return "";
    end else begin
      if (instr_name inside {VMERGE, VFMERGE, VADC, VSBC, VMADC, VMSBC}) begin
        return ",v0";
      end else begin
        return ",v0.t";
      end
    end
  endfunction

  function string add_nfields(string instr_name, string prefix);
    string suffix = instr_name.substr(prefix.len(), instr_name.len() - 1);
    return $sformatf("%0s%0d%0s", prefix, nfields + 1, suffix);
  endfunction

  function string add_eew(string instr_name, string prefix);
    string suffix = instr_name.substr(prefix.len(), instr_name.len() - 1);
    return $sformatf("%0s%0d%0s", prefix,  eew, suffix);
  endfunction

  function bit check_sub_extension(string s, string literal);
    return s == literal;
  endfunction

endclass : riscv_vector_instr
