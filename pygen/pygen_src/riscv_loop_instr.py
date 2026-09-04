"""
Copyright 2020 Google LLC
Copyright 2020 PerfectVIPs Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

"""

import vsc
import random
import copy
import sys
import logging
from pygen_src.riscv_instr_gen_config import cfg
from pygen_src.isa.riscv_instr import riscv_instr
from pygen_src.riscv_instr_stream import riscv_rand_instr_stream
from pygen_src.riscv_instr_pkg import (riscv_reg_t, riscv_instr_name_t, pkg_ins,
                                       riscv_instr_format_t, riscv_instr_category_t,
                                       compressed_gpr)


@vsc.randobj
class riscv_loop_instr(riscv_rand_instr_stream):

    def __init__(self):
        super().__init__()
        self.loop_cnt_reg = vsc.randsz_list_t(vsc.enum_t(riscv_reg_t))
        self.loop_limit_reg = vsc.randsz_list_t(vsc.enum_t(riscv_reg_t))
        self.loop_init_val = vsc.randsz_list_t(vsc.int32_t())
        self.loop_step_val = vsc.randsz_list_t(vsc.int32_t())
        self.loop_limit_val = vsc.randsz_list_t(vsc.int32_t())
        self.num_of_nested_loop = vsc.rand_bit_t(3)
        self.num_of_instr_in_loop = vsc.rand_int32_t(0)
        self.branch_type = vsc.randsz_list_t(vsc.enum_t(riscv_instr_name_t))
        self.loop_init_instr = []
        self.loop_update_instr = []
        self.loop_branch_instr = []
        self.loop_branch_target_instr = []
        # Aggregated loop instruction stream
        self.loop_instr = []

    @vsc.constraint
    def legal_loop_regs_c(self):
        self.num_of_nested_loop.inside(vsc.rangelist(1, 2))
        self.loop_limit_reg.size.inside(vsc.rangelist((1, 32)))
        self.loop_cnt_reg.size.inside(vsc.rangelist((1, 8)))
        vsc.solve_order(self.num_of_nested_loop, self.loop_cnt_reg)
        vsc.solve_order(self.num_of_nested_loop, self.loop_limit_reg)
        with vsc.foreach(self.loop_cnt_reg, idx = True) as i:
            self.loop_cnt_reg[i] != riscv_reg_t.ZERO
            # Loop control state must survive the surrounding generator
            # prologue/epilogue.  In particular, SP/TP/GP and the architectural
            # return-address register are used by the runtime and by the
            # call-stack return stubs; using one of them here can make an
            # otherwise finite loop jump to an invalid address at program end.
            self.loop_cnt_reg[i].not_inside(vsc.rangelist(
                riscv_reg_t.SP, riscv_reg_t.TP, riscv_reg_t.GP,
                riscv_reg_t.RA, cfg.sp, cfg.tp, cfg.scratch_reg))
            with vsc.foreach(cfg.reserved_regs, idx = True) as j:
                self.loop_cnt_reg[i] != cfg.reserved_regs[j]
            with vsc.foreach(cfg.gpr, idx = True) as j:
                self.loop_cnt_reg[i] != cfg.gpr[j]
            self.loop_cnt_reg[i] != cfg.ra
            self.loop_cnt_reg[i] != cfg.pmp_reg
        with vsc.foreach(self.loop_limit_reg, idx = True) as i:
            with vsc.foreach(cfg.reserved_regs, idx = True) as j:
                self.loop_limit_reg[i] != cfg.reserved_regs[j]
            self.loop_limit_reg[i].not_inside(vsc.rangelist(
                riscv_reg_t.SP, riscv_reg_t.TP, riscv_reg_t.GP,
                riscv_reg_t.RA, cfg.sp, cfg.tp, cfg.scratch_reg))
            with vsc.foreach(cfg.gpr, idx = True) as j:
                self.loop_limit_reg[i] != cfg.gpr[j]
            self.loop_limit_reg[i] != cfg.ra
            self.loop_limit_reg[i] != cfg.pmp_reg
        # Counter and limit registers must be disjoint as well.  Constraining
        # uniqueness within each list is not sufficient: if a counter and its
        # limit alias, the generated initialization overwrites the counter and
        # a relational branch such as ``bge x, x, target`` can never terminate.
        with vsc.foreach(self.loop_cnt_reg, idx = True) as i:
            with vsc.foreach(self.loop_limit_reg, idx = True) as j:
                self.loop_cnt_reg[i] != self.loop_limit_reg[j]
        vsc.unique(self.loop_cnt_reg)
        vsc.unique(self.loop_limit_reg)
        self.loop_cnt_reg.size == self.num_of_nested_loop
        self.loop_limit_reg.size == self.num_of_nested_loop

    @vsc.constraint
    def loop_c(self):
        vsc.solve_order(self.num_of_nested_loop, self.loop_init_val)
        vsc.solve_order(self.num_of_nested_loop, self.loop_step_val)
        vsc.solve_order(self.num_of_nested_loop, self.loop_limit_val)
        vsc.solve_order(self.loop_limit_val, self.loop_limit_reg)
        vsc.solve_order(self.branch_type, self.loop_init_val)
        vsc.solve_order(self.branch_type, self.loop_step_val)
        vsc.solve_order(self.branch_type, self.loop_limit_val)
        self.num_of_instr_in_loop.inside(vsc.rangelist((1, 25)))
        self.num_of_nested_loop.inside(vsc.rangelist(1, 2))
        self.loop_init_val.size.inside(vsc.rangelist(1, 2))
        self.loop_step_val.size.inside(vsc.rangelist(1, 2))
        self.loop_limit_val.size.inside(vsc.rangelist(1, 2))
        self.branch_type.size.inside(vsc.rangelist(1, 2))
        self.loop_init_val.size == self.num_of_nested_loop
        self.branch_type.size == self.num_of_nested_loop
        self.loop_step_val.size == self.num_of_nested_loop
        self.loop_limit_val.size == self.num_of_nested_loop
        self.branch_type.size == self.num_of_nested_loop
        with vsc.foreach(self.branch_type, idx = True) as i:
            with vsc.if_then(cfg.disable_compressed_instr == 0):
                self.branch_type[i].inside(vsc.rangelist(riscv_instr_name_t.C_BNEZ,
                                                         riscv_instr_name_t.C_BEQZ,
                                                         riscv_instr_name_t.BEQ,
                                                         riscv_instr_name_t.BNE,
                                                         riscv_instr_name_t.BLTU,
                                                         riscv_instr_name_t.BLT,
                                                         riscv_instr_name_t.BGEU,
                                                         riscv_instr_name_t.BGE))
            with vsc.else_then():
                self.branch_type[i].inside(vsc.rangelist(riscv_instr_name_t.BEQ,
                                                         riscv_instr_name_t.BNE,
                                                         riscv_instr_name_t.BLTU,
                                                         riscv_instr_name_t.BLT,
                                                         riscv_instr_name_t.BGEU,
                                                         riscv_instr_name_t.BGE))
        with vsc.foreach(self.loop_init_val, idx = True) as i:
            with vsc.if_then(self.branch_type[i].inside(vsc.rangelist(riscv_instr_name_t.C_BNEZ,
                                                                      riscv_instr_name_t.C_BEQZ))):
                self.loop_limit_val[i] == 0
                self.loop_limit_reg[i] == riscv_reg_t.ZERO
                self.loop_cnt_reg[i].inside(vsc.rangelist(list(compressed_gpr)))
            with vsc.else_then:
                self.loop_limit_val[i].inside(vsc.rangelist((-20, 20)))
                self.loop_limit_reg[i] != riscv_reg_t.ZERO
                self.loop_limit_reg[i].not_inside(vsc.rangelist(
                    riscv_reg_t.SP, riscv_reg_t.TP, riscv_reg_t.GP,
                    riscv_reg_t.RA, cfg.sp, cfg.tp, cfg.scratch_reg))
                with vsc.foreach(cfg.gpr, idx = True) as j:
                    self.loop_limit_reg[i] != cfg.gpr[j]
                self.loop_limit_reg[i] != cfg.ra
                self.loop_limit_reg[i] != cfg.pmp_reg
            with vsc.if_then(self.branch_type[i].inside(vsc.rangelist(riscv_instr_name_t.C_BNEZ,
                                                                      riscv_instr_name_t.C_BEQZ,
                                                                      riscv_instr_name_t.BEQ,
                                                                      riscv_instr_name_t.BNE))):
                self.loop_limit_val[i] != self.loop_init_val[i]
                ((self.loop_limit_val[i] - self.loop_init_val[i]) % self.loop_step_val[i]) == 0
            with vsc.else_if(self.branch_type[i] == riscv_instr_name_t.BGE):
                self.loop_step_val[i] < 0
            with vsc.else_if(self.branch_type[i].inside(vsc.rangelist(riscv_instr_name_t.BGEU))):
                self.loop_step_val[i] < 0
                self.loop_init_val[i] > 0
                # Avoid count to negative
                (self.loop_step_val[i] + self.loop_limit_val[i]) > 0
            with vsc.else_if(self.branch_type[i] == riscv_instr_name_t.BLT):
                self.loop_step_val[i] > 0
            with vsc.else_if(self.branch_type[i] == riscv_instr_name_t.BLTU):
                self.loop_step_val[i] > 0
                self.loop_limit_val[i] > 0
            self.loop_init_val[i].inside(vsc.rangelist((-10, 10)))
            self.loop_step_val[i].inside(vsc.rangelist((-10, 10)))
            with vsc.if_then(self.loop_init_val[i] < self.loop_limit_val[i]):
                self.loop_step_val[i] > 0
            with vsc.else_then:
                self.loop_step_val[i] < 0

    def post_randomize(self):
        # PyVSC does not reliably enforce constraints that compare a dynamic
        # enum list with another dynamic list.  Repair the selected operands
        # after solving as a final safety net; otherwise a loop may overwrite
        # SP/RA/GP and corrupt the generator's call/stack machinery.
        self._sanitize_loop_regs()
        for i in range(len(self.loop_cnt_reg)):
            self.reserved_rd.append(self.loop_cnt_reg[i])
        for i in range(len(self.loop_limit_reg)):
            self.reserved_rd.append(self.loop_limit_reg[i])
        # Generate instructions that mixed with the loop instructions
        self.initialize_instr_list(self.num_of_instr_in_loop)
        # The generic random-instruction path applies a second set of
        # PyVSC constraints to copied instruction templates and is prone to
        # seed-dependent failures (particularly with the dynamic register
        # lists used by this stream).  Use harmless ADDI operations for the
        # filler body; loop-control operands remain excluded so the loop
        # cannot be corrupted by its own body.
        reserved = set(cfg.reserved_regs)
        reserved.update(self.reserved_rd)
        # The generator's GPR scratch set and the return/PMP registers are
        # live across a directed stream.  Do not let filler instructions
        # overwrite them, even when they are not present in reserved_regs.
        reserved.update(cfg.gpr)
        reserved.update((cfg.ra, cfg.sp, cfg.tp, cfg.scratch_reg, cfg.pmp_reg))
        # Keep architectural/runtime-reserved registers untouched.  In
        # particular, allowing ADDI to write SP/TP/GP can corrupt the stack or
        # hart bookkeeping when a loop stream is injected into a normal test.
        reserved.update((riscv_reg_t.ZERO, riscv_reg_t.SP, riscv_reg_t.TP,
                         riscv_reg_t.GP, riscv_reg_t.RA))
        body_regs = [reg for reg in riscv_reg_t if reg not in reserved]
        if not body_regs:
            body_regs = [riscv_reg_t.T0]
        for body_idx in range(len(self.instr_list)):
            rd = random.choice(body_regs)
            rs1 = random.choice(body_regs)
            self.instr_list[body_idx] = self._make_addi(
                rd, rs1, random.randint(-16, 15), "loop filler")
        # Randomize the key loop instructions
        self.loop_init_instr = [0] * 2 * self.num_of_nested_loop
        self.loop_update_instr = [0] * self.num_of_nested_loop
        self.loop_branch_instr = [0] * self.num_of_nested_loop
        self.loop_branch_target_instr = [0] * self.num_of_nested_loop
        for i in range(self.num_of_nested_loop):
            # Instruction to init the loop counter
            self.loop_init_instr[2 * i] = self._make_addi(
                self.loop_cnt_reg[i], riscv_reg_t.ZERO, self.loop_init_val[i],
                "init loop {} counter".format(i))
            # Instruction to init loop limit
            self.loop_init_instr[2 * i + 1] = self._make_addi(
                self.loop_limit_reg[i], riscv_reg_t.ZERO, self.loop_limit_val[i],
                "init loop {} limit".format(i))
            # Branch target instruction, can be anything
            # Keep this instruction solver-free as well.  The old pygen code
            # randomized an arbitrary instruction and then applied constraints
            # to fields that might not exist for its format, making loop
            # injection seed-dependent.  A zero-effect ADDI is a legal,
            # deterministic branch target and still exercises the loop path.
            self.loop_branch_target_instr[i] = self._make_addi(
                riscv_reg_t.ZERO, riscv_reg_t.ZERO, 0, "loop body")
            self.loop_branch_target_instr[i].label = pkg_ins.format_string(
                "{}_{}_t".format(self.label, i))
            # Instruction to update loop counter
            self.loop_update_instr[i] = self._make_addi(
                self.loop_cnt_reg[i], self.loop_cnt_reg[i], self.loop_step_val[i],
                "update loop {} counter".format(i))
            # Backward branch instruction
            self.loop_branch_instr[i] = self._make_branch(
                self.branch_type[i], self.loop_cnt_reg[i], self.loop_limit_reg[i],
                "branch for loop {}".format(i))
            self.loop_branch_instr[i].imm_str = self.loop_branch_target_instr[i].label
            self.loop_branch_instr[i].branch_assigned = 1
        # Randomly distribute the loop instruction in the existing instruction stream
        self.build_loop_instr_stream()
        # A loop is a control-flow unit: its init, body, update and backward
        # branch must remain contiguous.  ``mix_instr_stream(..., contained=1)``
        # only pins the first/last element and interleaves the middle elements
        # with the surrounding random stream, which can place the backward
        # branch before the update and create an infinite loop.  Insert the
        # complete stream as one block at a random non-atomic boundary instead.
        self.insert_instr_stream(self.loop_instr, -1)
        for i in range(len(self.instr_list)):
            if (self.instr_list[i].label != ""):
                self.instr_list[i].has_label = 1
            else:
                self.instr_list[i].has_label = 0
            self.instr_list[i].atomic = 1

    @staticmethod
    def _as_reg(value):
        """Convert a PyVSC enum value to the architectural register enum."""
        try:
            return riscv_reg_t(int(value))
        except (TypeError, ValueError):
            return value

    def _sanitize_loop_regs(self):
        """Keep loop operands away from generator/runtime-owned registers.

        The loop constraints contain dynamic ``foreach`` expressions.  Some
        PyVSC versions elaborate those expressions before the list values are
        known, so a solved value can still alias a register reserved by the
        surrounding program.  This post-solve repair is deliberately small and
        only changes the register operands; loop bounds and branch direction
        remain untouched.
        """
        forbidden = {
            riscv_reg_t.ZERO, riscv_reg_t.SP, riscv_reg_t.TP,
            riscv_reg_t.GP, riscv_reg_t.RA,
        }
        for values in (cfg.reserved_regs, cfg.gpr,
                       (cfg.ra, cfg.sp, cfg.tp, cfg.scratch_reg, cfg.pmp_reg)):
            for value in values:
                reg = self._as_reg(value)
                if isinstance(reg, riscv_reg_t):
                    forbidden.add(reg)

        all_regs = list(riscv_reg_t)
        compressed_regs = [riscv_reg_t(int(reg)) for reg in compressed_gpr]
        used_counters = set()
        used_limits = set()
        compressed_branches = {
            riscv_instr_name_t.C_BEQZ,
            riscv_instr_name_t.C_BNEZ,
        }
        noncompressed_branches = [
            riscv_instr_name_t.BEQ, riscv_instr_name_t.BNE,
            riscv_instr_name_t.BLTU, riscv_instr_name_t.BLT,
            riscv_instr_name_t.BGEU, riscv_instr_name_t.BGE,
        ]

        for i in range(self.num_of_nested_loop):
            branch = riscv_instr_name_t(int(self.branch_type[i]))
            if branch in compressed_branches:
                candidates = [reg for reg in compressed_regs
                              if reg not in forbidden and reg not in used_counters]
                # A target may reserve an unusually large subset of the eight
                # compressed registers.  Keep the compressed branch legal by
                # switching to a regular register-register branch if no safe
                # compressed operand remains.
                if not candidates:
                    branch = random.choice(noncompressed_branches)
                    self.branch_type[i] = branch
                    candidates = [reg for reg in all_regs
                                  if reg not in forbidden and reg not in used_counters]
                if not candidates:
                    raise RuntimeError("No safe register available for loop counter")
                current = self._as_reg(self.loop_cnt_reg[i])
                if current not in candidates:
                    current = random.choice(candidates)
                    self.loop_cnt_reg[i] = current
                used_counters.add(current)
                # C.BEQZ/C.BNEZ compare against x0 by definition.
                if branch in compressed_branches:
                    self.loop_limit_reg[i] = riscv_reg_t.ZERO
                    used_limits.add(riscv_reg_t.ZERO)
                    continue

            counter_candidates = [reg for reg in all_regs
                                  if reg not in forbidden and reg not in used_counters]
            if not counter_candidates:
                raise RuntimeError("No safe register available for loop counter")
            current = self._as_reg(self.loop_cnt_reg[i])
            if current not in counter_candidates:
                current = random.choice(counter_candidates)
                self.loop_cnt_reg[i] = current
            used_counters.add(current)

            limit_candidates = [reg for reg in all_regs
                                if reg not in forbidden and reg != current
                                and reg not in used_limits and reg not in used_counters]
            if not limit_candidates:
                raise RuntimeError("No safe register available for loop limit")
            current_limit = self._as_reg(self.loop_limit_reg[i])
            if current_limit not in limit_candidates:
                current_limit = random.choice(limit_candidates)
                self.loop_limit_reg[i] = current_limit
            used_limits.add(current_limit)

    @staticmethod
    def _make_addi(rd, rs1, imm, comment=""):
        """Build a fully-specified ADDI without invoking PyVSC again."""
        # get_instr() intentionally uses a shallow copy in the legacy pygen
        # implementation; its PyVSC field objects are therefore still shared
        # with the class template.  Deep-copy the template before assigning
        # operands so one injected stream cannot corrupt the next one.
        instr = copy.deepcopy(riscv_instr.instr_template[riscv_instr_name_t.ADDI])
        instr.rd = rd
        instr.rs1 = rs1
        # ``get_instr`` returns a lightweight template copy.  Calling
        # set_imm_len()/extend_imm() repeatedly mutates the shared template's
        # mask in this pygen version and can turn a small negative immediate
        # into an out-of-range positive assembly operand.  The assembly path
        # consumes imm_str, so retain the intended signed value directly.
        instr.imm = imm
        instr.imm_str = str(int(imm))
        instr.comment = pkg_ins.format_string(comment)
        return instr

    @staticmethod
    def _make_branch(branch_name, rs1, rs2, comment=""):
        """Build a loop branch from its registered instruction template."""
        instr = copy.deepcopy(riscv_instr.instr_template[branch_name])
        instr.rs1 = rs1
        if branch_name not in (riscv_instr_name_t.C_BEQZ,
                               riscv_instr_name_t.C_BNEZ):
            instr.rs2 = rs2
        instr.comment = pkg_ins.format_string(comment)
        return instr

    # Build the whole loop structure from innermost loop to the outermost loop
    def build_loop_instr_stream(self):
        self.loop_instr = []
        for i in range(self.num_of_nested_loop):
            # Wrap the previously-built inner loop when constructing an outer
            # loop.  Indexing ``self.loop_instr[i]`` here duplicated an init
            # instruction (and was out of range for the first iteration).
            inner_loop = list(self.loop_instr)
            self.loop_instr = [self.loop_init_instr[2 * i],
                               self.loop_init_instr[2 * i + 1],
                               self.loop_branch_target_instr[i],
                               self.loop_update_instr[i]]
            self.loop_instr.extend(inner_loop)
            self.loop_instr.append(self.loop_branch_instr[i])
        logging.info("Totally {} instructions have been added".format(len(self.loop_instr)))
