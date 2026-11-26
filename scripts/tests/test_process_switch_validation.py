"""Runtime evidence checks for native process-switch stream validation."""

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


MODULE_PATH = Path(__file__).resolve().parents[1] / "process_switch_validation.py"
SPEC = importlib.util.spec_from_file_location("process_switch_validation", MODULE_PATH)
validator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validator)


def retired(pc):
    return "core   0: 3 0x{:016x} (0x00000013)\n".format(pc)


def decoded(pc):
    return "core   0: 0x{:016x} (0x00000013) nop\n".format(pc)


def tohost(value=1):
    return "core   0: 3 0x0000000080000100 (0x00533023) mem 0x80005000 0x{:x}\n".format(value)


class ProcessSwitchValidationTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.assembly = self.root / "test.S"
        self.log = self.root / "test.log"
        self.elf = self.root / "test.o"
        self.assembly.write_text("# PROCESS_SWITCH_INJECTION: a_begin a_pass\n"
                                 "# PROCESS_SWITCH_SCENARIO: same_asid a_scenario\n"
                                 "# PROCESS_SWITCH_FAIL: a_fail\n")
        self.symbols = {"a_begin": 0x1000, "a_pass": 0x1010, "a_scenario": 0x1004,
                        "a_fail": 0x1020, "tohost": 0x80005000}

    def validate(self, content, minimum_hits=0):
        self.log.write_text(content)
        with patch.object(validator, "_nm_symbols", return_value=self.symbols):
            return validator.validate_spike(self.log, self.elf, self.assembly, minimum_hits)

    def passed_injection(self):
        return retired(0x1000) + retired(0x1004) + retired(0x1010)

    def test_only_committed_entries_count_and_artifact_records_hits(self):
        content = (decoded(0x1000) + retired(0x1000) + decoded(0x1004) + retired(0x1004) +
                   decoded(0x1010) + retired(0x1010) + tohost())
        report = self.validate(content, minimum_hits=1)
        self.assertEqual(report["status"], "passed")
        self.assertEqual(report["injection_hits"], {"a_begin": 1})
        self.assertEqual(report["scenario_hits"], {"same_asid": 1})
        self.assertEqual(json.loads(self.log.with_suffix(".process-switch-validation.json")
                                    .read_text()), report)

    def test_decoded_but_unretired_markers_are_not_hits(self):
        content = decoded(0x1000) + decoded(0x1004) + decoded(0x1010) + tohost()
        with self.assertRaisesRegex(RuntimeError, "Only 0.*minimum is 1"):
            self.validate(content, minimum_hits=1)

    def test_skipped_injection_is_reported_without_claiming_coverage(self):
        report = self.validate(tohost())
        self.assertEqual(report["status"], "not_reached")
        self.assertEqual(report["injection_hits"], {"a_begin": 0})
        self.assertEqual(report["scenario_hits"], {"same_asid": 0})
        self.assertEqual(report["skipped_injections"], ["a_begin"])

    def test_started_injection_requires_pass_even_when_minimum_zero(self):
        with self.assertRaisesRegex(RuntimeError, "no successful return"):
            self.validate(retired(0x1000) + retired(0x1004) + tohost())

    def test_completed_check_requires_normal_dv_exit(self):
        with self.assertRaisesRegex(RuntimeError, "no normal DV tohost pass"):
            self.validate(self.passed_injection(), minimum_hits=1)

    def test_tohost_before_injection_does_not_count_as_final_completion(self):
        with self.assertRaisesRegex(RuntimeError, "no normal DV tohost pass"):
            self.validate(tohost() + self.passed_injection())

    def test_failing_tohost_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "failing tohost value 0x3"):
            self.validate(self.passed_injection() + tohost(3))

    def test_fail_marker_is_rejected_before_htif_write(self):
        with self.assertRaisesRegex(RuntimeError, "fail marker executed"):
            self.validate(retired(0x1000) + retired(0x1020))

    def test_orphan_pass_and_nested_begin_are_rejected(self):
        for content, message in ((retired(0x1010), "pass without matching begin"),
                                 (retired(0x1000) * 2, "Nested process-switch begin")):
            with self.subTest(message=message), self.assertRaisesRegex(RuntimeError, message):
                self.validate(content + tohost())

    def test_missing_symbol_and_aliased_boundaries_are_rejected(self):
        del self.symbols["a_scenario"]
        with self.assertRaisesRegex(RuntimeError, "Missing.*a_scenario"):
            self.validate(self.passed_injection() + tohost())
        self.symbols["a_scenario"] = 0x1004
        self.symbols["a_pass"] = 0x1000
        with self.assertRaisesRegex(RuntimeError, "PCs must be distinct"):
            self.validate(self.passed_injection() + tohost())

    def test_expected_page_fault_allowed_but_illegal_trap_is_not(self):
        report = self.validate(retired(0x1000) + "core 0: exception trap_store_page_fault\n" +
                               retired(0x1004) + retired(0x1010) + tohost())
        self.assertEqual(report["status"], "passed")
        with self.assertRaisesRegex(RuntimeError, "Illegal instruction inside"):
            self.validate(retired(0x1000) + "core 0: exception trap_illegal_instruction\n" +
                          retired(0x1010) + tohost())

    def test_multiple_injections_record_skipped_instance(self):
        with self.assembly.open("a") as asm:
            asm.write("# PROCESS_SWITCH_INJECTION: b_begin b_pass\n")
        self.symbols.update({"b_begin": 0x2000, "b_pass": 0x2010})
        report = self.validate(self.passed_injection() + tohost(), minimum_hits=1)
        self.assertEqual(report["status"], "passed_partial")
        self.assertEqual(report["skipped_injections"], ["b_begin"])
        with self.assertRaisesRegex(RuntimeError, "Only 1.*minimum is 2"):
            self.validate(self.passed_injection() + tohost(), minimum_hits=2)

    def test_repeated_completed_visits_count(self):
        report = self.validate(self.passed_injection() * 2 + tohost(), minimum_hits=2)
        self.assertEqual(report["injection_hits"], {"a_begin": 2})
        self.assertEqual(report["scenario_hits"], {"same_asid": 2})

    def test_malformed_or_duplicate_metadata_is_rejected(self):
        for text, message in (("# PROCESS_SWITCH_INJECTION: a_begin\n", "Malformed"),
                              ("# PROCESS_SWITCH_INJECTION: a_begin a_begin\n", "distinct"),
                              ("# PROCESS_SWITCH_SCENARIO: same_asid a_scenario\n", "no injection")):
            self.assembly.write_text(text)
            with self.subTest(text=text), self.assertRaisesRegex(RuntimeError, message):
                self.validate(tohost())

    def test_unmarked_disabled_program_needs_no_nm_or_log(self):
        self.assembly.write_text("addi a0, a0, 1\n")
        with patch.object(validator, "_nm_symbols", side_effect=AssertionError):
            self.assertEqual(validator.validate_spike(self.log, self.elf, self.assembly), {})
        with self.assertRaisesRegex(RuntimeError, "No process-switch injections generated"):
            validator.validate_spike(self.log, self.elf, self.assembly, minimum_hits=1)

    def test_invalid_minimum_rejected(self):
        for minimum in (-1, True, "1"):
            with self.subTest(minimum=minimum), self.assertRaisesRegex(RuntimeError, "non-negative integer"):
                self.validate(tohost(), minimum_hits=minimum)

    def test_missing_optional_assembly_is_noop_but_required_injection_fails(self):
        self.assembly.unlink()
        with patch.object(validator, "_nm_symbols", side_effect=AssertionError):
            self.assertEqual(validator.validate_spike(self.log, self.elf, self.assembly), {})
        with self.assertRaisesRegex(RuntimeError, "required process-switch assembly"):
            validator.validate_spike(self.log, self.elf, self.assembly, minimum_hits=1)


if __name__ == "__main__":
    unittest.main()
