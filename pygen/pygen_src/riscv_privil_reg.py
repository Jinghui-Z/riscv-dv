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

import logging
import vsc
from importlib import import_module
from pygen_src.riscv_instr_pkg import privileged_level_t, reg_field_access_t, privileged_reg_t
from pygen_src.riscv_reg import riscv_reg
from pygen_src.riscv_instr_gen_config import cfg
rcs = import_module("pygen_src.target." + cfg.argv.target + ".riscv_core_setting")


# RISC-V privileged register class
@vsc.randobj
class riscv_privil_reg(riscv_reg):
    def __init__(self):
        super().__init__()

    def init_reg(self, reg_name):
        super().init_reg(reg_name)
        # ---------------Machine mode register ----------------
        # Machine status Register
        if reg_name == privileged_reg_t.MSTATUS:
            self.privil_level = privileged_level_t.M_LEVEL
            self.add_field("UIE", 1, reg_field_access_t.WARL)
            self.add_field("SIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI0", 1, reg_field_access_t.WPRI)
            self.add_field("MIE", 1, reg_field_access_t.WARL)
            self.add_field("UPIE", 1, reg_field_access_t.WARL)
            self.add_field("SPIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI1", 1, reg_field_access_t.WPRI)
            self.add_field("MPIE", 1, reg_field_access_t.WARL)
            self.add_field("SPP", 1, reg_field_access_t.WLRL)
            self.add_field("VS", 2, reg_field_access_t.WARL)
            self.add_field("MPP", 2, reg_field_access_t.WLRL)
            self.add_field("FS", 2, reg_field_access_t.WARL)
            self.add_field("XS", 2, reg_field_access_t.WARL)
            self.add_field("MPRV", 1, reg_field_access_t.WARL)
            self.add_field("SUM", 1, reg_field_access_t.WARL)
            self.add_field("MXR", 1, reg_field_access_t.WARL)
            self.add_field("TVM", 1, reg_field_access_t.WARL)
            self.add_field("TW", 1, reg_field_access_t.WARL)
            self.add_field("TSR", 1, reg_field_access_t.WARL)
            if rcs.XLEN == 32:
                self.add_field("WPRI3", 8, reg_field_access_t.WPRI)
            else:
                self.add_field("WPRI3", 9, reg_field_access_t.WPRI)
                self.add_field("UXL", 2, reg_field_access_t.WARL)
                self.add_field("SXL", 2, reg_field_access_t.WARL)
                self.add_field("WPRI4", rcs.XLEN - 37, reg_field_access_t.WPRI)
            self.add_field("SD", 1, reg_field_access_t.WARL)
        # Machine interrupt-enable register
        elif reg_name == privileged_reg_t.MIE:
            self.privil_level = privileged_level_t.M_LEVEL
            self.add_field("USIE", 1, reg_field_access_t.WARL)
            self.add_field("SSIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI0", 1, reg_field_access_t.WPRI)
            self.add_field("MSIE", 1, reg_field_access_t.WARL)
            self.add_field("UTIE", 1, reg_field_access_t.WARL)
            self.add_field("STIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI1", 1, reg_field_access_t.WPRI)
            self.add_field("MTIE", 1, reg_field_access_t.WARL)
            self.add_field("UEIE", 1, reg_field_access_t.WARL)
            self.add_field("SEIE", 1, reg_field_access_t.WARL)
            self.add_field("WPEI2", 1, reg_field_access_t.WPRI)
            self.add_field("MEIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI3", rcs.XLEN - 12, reg_field_access_t.WPRI)
        # Supervisor status register.  Keep the field order low-to-high so
        # riscv_reg.get_val() reconstructs the architectural CSR layout.
        elif reg_name == privileged_reg_t.SSTATUS:
            self.privil_level = privileged_level_t.S_LEVEL
            self.add_field("UIE", 1, reg_field_access_t.WARL)
            self.add_field("SIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI0", 2, reg_field_access_t.WPRI)
            self.add_field("UPIE", 1, reg_field_access_t.WARL)
            self.add_field("SPIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI1", 2, reg_field_access_t.WPRI)
            self.add_field("SPP", 1, reg_field_access_t.WARL)
            self.add_field("WPRI2", 4, reg_field_access_t.WPRI)
            self.add_field("FS", 2, reg_field_access_t.WARL)
            self.add_field("XS", 2, reg_field_access_t.WARL)
            self.add_field("WPRI3", 1, reg_field_access_t.WPRI)
            self.add_field("SUM", 1, reg_field_access_t.WARL)
            self.add_field("MXR", 1, reg_field_access_t.WARL)
            if rcs.XLEN == 32:
                # RV32 places SD at bit 31; bits 20:30 are WPRI (11 bits).
                self.add_field("WPRI4", 11, reg_field_access_t.WPRI)
            else:
                self.add_field("WPRI4", 12, reg_field_access_t.WPRI)
                self.add_field("UXL", 2, reg_field_access_t.WARL)
                self.add_field("WPRI5", 29, reg_field_access_t.WPRI)
            self.add_field("SD", 1, reg_field_access_t.WARL)
        # User status register.  U-mode trap CSRs are optional, but modeling
        # USTATUS keeps explicit U-mode boot generation well-defined when a
        # target advertises user traps.
        elif reg_name == privileged_reg_t.USTATUS:
            self.privil_level = privileged_level_t.U_LEVEL
            self.add_field("UIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI0", 3, reg_field_access_t.WPRI)
            self.add_field("UPIE", 1, reg_field_access_t.WARL)
            if rcs.XLEN == 32:
                # UIE[0], UPIE[4], SD[31].
                self.add_field("WPRI1", 26, reg_field_access_t.WPRI)
            else:
                # UIE[0], UPIE[4], SD[63].
                self.add_field("WPRI1", 58, reg_field_access_t.WPRI)
            self.add_field("SD", 1, reg_field_access_t.WARL)
        # Supervisor interrupt-enable register.
        elif reg_name == privileged_reg_t.SIE:
            self.privil_level = privileged_level_t.S_LEVEL
            self.add_field("USIE", 1, reg_field_access_t.WARL)
            self.add_field("SSIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI0", 2, reg_field_access_t.WPRI)
            self.add_field("UTIE", 1, reg_field_access_t.WARL)
            self.add_field("STIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI1", 2, reg_field_access_t.WPRI)
            self.add_field("UEIE", 1, reg_field_access_t.WARL)
            self.add_field("SEIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI2", rcs.XLEN - 10, reg_field_access_t.WPRI)
        # User interrupt-enable register.
        elif reg_name == privileged_reg_t.UIE:
            self.privil_level = privileged_level_t.U_LEVEL
            self.add_field("USIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI0", 3, reg_field_access_t.WPRI)
            self.add_field("UTIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI1", 3, reg_field_access_t.WPRI)
            self.add_field("UEIE", 1, reg_field_access_t.WARL)
            self.add_field("WPRI2", rcs.XLEN - 9, reg_field_access_t.WPRI)
        # Supervisor address translation and protection register.
        elif reg_name == privileged_reg_t.SATP:
            self.privil_level = privileged_level_t.S_LEVEL
            if rcs.XLEN == 32:
                self.add_field("PPN", 22, reg_field_access_t.WARL)
                self.add_field("ASID", 9, reg_field_access_t.WARL)
                self.add_field("MODE", 1, reg_field_access_t.WARL)
            else:
                self.add_field("PPN", 44, reg_field_access_t.WARL)
                self.add_field("ASID", 16, reg_field_access_t.WARL)
                self.add_field("MODE", 4, reg_field_access_t.WARL)
        else:
            logging.error("reg %0s is not supported yet", reg_name.name)
