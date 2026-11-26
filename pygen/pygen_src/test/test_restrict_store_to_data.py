"""Focused PyFlow address-safety checks; no ELF or ISS run is performed.

Run from the repository root with PyFlow dependencies installed:
  PYTHONPATH=pygen python3 pygen/pygen_src/test/test_restrict_store_to_data.py
Set DV_RESTRICT_TEST_TARGET=rv32imafdc to exercise the RV32 instruction set.
"""

import os
import random
import sys
import tempfile
import unittest
from importlib import import_module


_test_output = tempfile.TemporaryDirectory(prefix='dv-restrict-pyflow-test-')
sys.argv = [sys.argv[0], '--target', os.getenv('DV_RESTRICT_TEST_TARGET', 'rv64imafdc'),
            '--restrict_store_to_data', '1', '--num_of_sub_program', '0',
            '--enable_unaligned_load_store', '1',
            '--enable_floating_point', '1',
            '--log_file_name', os.path.join(_test_output.name, 'test.log')]
# The package deliberately initializes cfg through riscv_instr_pkg.
from pygen_src.riscv_instr_pkg import data_pattern_t, riscv_reg_t
from pygen_src.riscv_instr_gen_config import cfg, rcs

for _isa in rcs.supported_isa:
    import_module('pygen_src.isa.' + _isa.name.lower() + '_instr')

from pygen_src.isa.riscv_instr import riscv_instr
from pygen_src.riscv_data_page_gen import riscv_data_page_gen
from pygen_src.riscv_asm_program_gen import riscv_asm_program_gen
from pygen_src.riscv_instr_stream import riscv_rand_instr_stream
from pygen_src.riscv_load_store_instr_lib import (
    riscv_single_load_store_instr_stream, riscv_load_store_hazard_instr_stream,
    locality_e, _ACCESS_BYTES)
from pygen_src.riscv_utils import factory, validate_restricted_directed_stream


class RestrictedStoreTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        random.seed(19)
        cfg.randomize()
        riscv_instr.create_instr_list(cfg)

    def test_scalar_effective_addresses_and_live_registers(self):
        for _ in range(20):
            stream = riscv_single_load_store_instr_stream()
            stream.randomize()
            self.assertNotEqual(int(stream.rs1_reg), int(riscv_reg_t.ZERO))
            self.assertNotIn(int(stream.rs1_reg), {int(reg) for reg in cfg.reserved_regs})
            for addr, offset, instr in zip(stream.addr, stream.offset, stream.load_store_instr):
                self.assertEqual(addr, int(stream.base) + offset)
                self.assertGreaterEqual(addr, 0)
                self.assertLessEqual(addr + _ACCESS_BYTES[instr.instr_name.name],
                                     int(stream.max_load_store_offset))
                self.assertEqual(int(instr.rs1), int(stream.rs1_reg))

    def test_unaligned_last_byte_excludes_wide_accesses(self):
        stream = riscv_single_load_store_instr_stream()
        stream.randomize()
        stream.addr = [4095]
        stream.offset = [1]
        stream.base = 4094
        stream.max_load_store_offset = 4096
        for _ in range(20):
            stream.gen_load_store_instr()
            self.assertIn(stream.load_store_instr[-1].instr_name.name, ('LB', 'LBU', 'SB'))

    def test_hazard_offsets_at_both_region_edges(self):
        stream = riscv_load_store_hazard_instr_stream()
        stream.num_load_store = 30
        stream.locality = locality_e.SPARSE
        stream.max_load_store_offset = 33
        for base in (0, 32):
            stream.base = base
            stream.hazard_ratio = 100
            stream.randomize_offset()
            self.assertEqual(len(set(stream.offset)), 1)
            for offset, addr in zip(stream.offset, stream.addr):
                self.assertEqual(addr, base + offset)
                self.assertTrue(0 <= addr < 33)

    def test_unsafe_streams_fail_closed_and_default_is_unchanged(self):
        for name in ('riscv_amo_instr_stream', 'riscv_lr_sc_instr_stream',
                     'riscv_load_store_rand_addr_instr_stream', 'custom_store'):
            with self.assertRaisesRegex(ValueError, 'restrict_store_to_data'):
                factory(name)
        try:
            cfg.restrict_store_to_data = 0
            validate_restricted_directed_stream('riscv_amo_instr_stream')
        finally:
            cfg.restrict_store_to_data = 1

    def test_generated_scope_without_strict_flag(self):
        original_strict = cfg.restrict_store_to_data
        original_scoped = cfg.restrict_generated_store_to_data
        try:
            cfg.restrict_store_to_data = 0
            cfg.restrict_generated_store_to_data = 1
            self.assertTrue(cfg.restrict_dv_store_to_data)
            with self.assertRaisesRegex(ValueError, 'unsupported pyflow stream'):
                factory('riscv_amo_instr_stream')
            with self.assertRaisesRegex(ValueError, 'bounded memory stream'):
                riscv_rand_instr_stream().setup_allowed_instr(no_load_store=0)
            program = riscv_asm_program_gen()
            output = []
            program.gen_register_dump(output, 0)
            self.assertIn('__dv_register_dump_0', output[0])
            self.assertNotIn('_start', '\n'.join(output))
            cfg.restrict_generated_store_to_data = 0
            self.assertFalse(cfg.restrict_dv_store_to_data)
            validate_restricted_directed_stream('riscv_amo_instr_stream')
        finally:
            cfg.restrict_store_to_data = original_strict
            cfg.restrict_generated_store_to_data = original_scoped

    def test_generated_scope_cli_parse(self):
        original_argv = sys.argv
        try:
            sys.argv = original_argv + ['--restrict_generated_store_to_data', '1']
            self.assertEqual(cfg.parse_args().restrict_generated_store_to_data, 1)
        finally:
            sys.argv = original_argv

    def test_generic_stream_cannot_request_unbounded_stores(self):
        with self.assertRaisesRegex(ValueError, 'bounded memory stream'):
            riscv_rand_instr_stream().setup_allowed_instr(no_load_store=0)

    def test_ecall_register_dump_never_targets_code(self):
        program = riscv_asm_program_gen()
        for hart in (0, 1):
            instr = []
            program.gen_register_dump(instr, hart)
            self.assertNotIn('_start', '\n'.join(instr))
            self.assertIn('__dv_register_dump_{}'.format(hart), instr[0])
            self.assertEqual(len(instr), 33)
            program.gen_register_dump_data(hart)
            self.assertIn('__dv_register_dump_{}:'.format(hart), program.instr_stream)
        self.assertEqual(program.instr_stream.count('.zero {}'.format(32 * (rcs.XLEN // 8))), 2)

    def test_dedicated_section_exact_size_and_balanced_push_pop(self):
        original_size = int(cfg.amo_region[0].size_in_bytes)
        original_push = cfg.use_push_data_section
        try:
            cfg.amo_region[0].size_in_bytes = 33
            cfg.use_push_data_section = 1
            pages = riscv_data_page_gen()
            pages.gen_data_page(0, data_pattern_t.INCR_VAL, amo=1)
            lines = pages.data_page_str
            self.assertEqual(sum(line.startswith('.pushsection .dv_store_data.')
                                 for line in lines), 1)
            self.assertEqual(lines.count('.popsection'), 1)
            self.assertIn('.balign 4096', lines)
            values = [int(value) for line in lines if line.startswith('.byte ')
                      for value in line[len('.byte '):].split(', ')]
            self.assertEqual(values, list(range(33)))
        finally:
            cfg.amo_region[0].size_in_bytes = original_size
            cfg.use_push_data_section = original_push


if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0]])
