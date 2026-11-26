"""Reject incomplete evidence before publishing per-case validation claims."""

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "collect_hypertest_validation.py"
SPEC = importlib.util.spec_from_file_location("collect_hypertest_validation", SCRIPT)
collector = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(collector)


class CollectorTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name)
        self.run = self.repo / "out" / "run"
        (self.run / "asm_test").mkdir(parents=True)
        (self.run / "spike_sim").mkdir()
        self.manifest = self.repo / "manifest.json"
        self.manifest.write_text(json.dumps({"cases": [{"name": "case_a"}, {"name": "case_b"}],
                                             "source": {"snapshot_sha256": "snapshot"}}))
        self.assembly = self.run / "asm_test" / "test_0.S"
        self.elf = self.assembly.with_suffix(".o")
        self.log = self.run / "spike_sim" / "test_0.log"
        self.artifact = self.log.with_suffix(".hypertest-validation.json")
        self.generator = self.run / "sim_test_0.log"
        self.assembly.write_text("# HYPERTEST_CASE: case_a\naddi x1, x1, 1\n")
        self.elf.write_bytes(b"\x7fELF" + bytes(60))
        self.log.write_text("core 0: 0x1000 (0x00000013) nop\n"
                            "core 0: 0x2000 (0x00000013) nop\n"
                            "core 0: 0x1010 (0x00000013) nop\n")
        self.record = {"assembly": str(self.assembly), "elf": str(self.elf),
                       "log": str(self.log), "static_cases": ["case_a"],
                       "status": "passed", "program_pass": True,
                       "case_hits": {"case_a": 1},
                       "intervals": [{"begin_line": 1, "pass_line": 3,
                                      "case_hits": {"case_a": 1}}]}
        self.write_record()
        self.write_uvm()

    def write_record(self):
        self.artifact.write_text(json.dumps(self.record))

    def write_uvm(self, warning=0, assembly=None):
        self.generator.write_text("{} is generated\nTEST PASSED\nTEST GENERATION DONE\n"
                                  "UVM_WARNING : {}\nUVM_ERROR : 0\nUVM_FATAL : 0\n"
                                  .format(assembly or self.assembly, warning))

    def collect(self):
        return collector.collect([self.run], self.repo, self.manifest)

    def test_legacy_source_pass_requires_full_evidence_and_hashes(self):
        result = self.collect()
        self.assertEqual(result["cases"]["case_a"]["validation"], "gen_gcc_spike_pass")
        self.assertEqual(result["cases"]["case_a"]["completed_count"], 1)
        proof = result["cases"]["case_a"]
        self.assertEqual(len(proof["evidence"]), 5)
        self.assertTrue(all(len(value) == 64 for value in proof["evidence_sha256"].values()))
        self.assertTrue(all(not Path(value).is_absolute() for value in proof["evidence"]))

    def test_empty_json_cannot_publish_pass(self):
        self.artifact.write_text("{}")
        self.assertEqual(self.collect()["cases"], {})

    def test_elf_without_generator_summary_cannot_publish_pass(self):
        self.generator.unlink()
        result = self.collect()
        self.assertEqual(result["cases"], {})
        self.assertIn("generator log", result["rejected_artifacts"][0]["reason"])

    def test_any_nonzero_uvm_count_rejects(self):
        self.write_uvm(warning=1)
        self.assertEqual(self.collect()["cases"], {})

    def test_different_test_generator_summary_cannot_publish_pass(self):
        self.write_uvm(assembly="other_0.S")
        self.assertEqual(self.collect()["cases"], {})

    def test_non_elf_linked_file_cannot_publish_pass(self):
        self.elf.write_bytes(b"not an ELF")
        self.assertEqual(self.collect()["cases"], {})

    def test_program_pass_without_case_interval_cannot_publish_pass(self):
        self.record["case_hits"] = {}
        self.record["intervals"] = []
        self.write_record()
        self.assertEqual(self.collect()["cases"], {})

    def test_failed_program_cannot_publish_a_previously_completed_case(self):
        self.record["program_pass"] = False
        self.write_record()
        self.assertEqual(self.collect()["cases"], {})

    def new_metadata(self):
        self.assembly.write_text("# HYPERTEST_INJECTION: native case_a a_begin a_pass\n"
                                 "# HYPERTEST_INJECTION: native case_b b_begin b_pass\n")
        self.record.update({"injections": collector.injection_metadata(self.assembly),
                            "injection_hits": {"a_begin": 1, "b_begin": 0},
                            "injection_intervals": [{"injection_id": "a_begin", "kind": "native",
                                                      "case": "case_a", "begin_line": 1,
                                                      "pass_line": 3,
                                                      "original_selfcheck_passed": True}],
                            "status": "passed_partial", "static_cases": ["case_a", "case_b"]})
        self.write_record()

    def test_new_metadata_records_only_completed_injection(self):
        self.new_metadata()
        result = self.collect()
        self.assertEqual(set(result["cases"]), {"case_a"})
        self.assertEqual(result["cases"]["case_a"]["runs"][0]["basis"], "injection_intervals")

    def test_new_metadata_does_not_fall_back_to_legacy_claims(self):
        self.new_metadata()
        self.record["injection_intervals"] = []
        self.write_record()
        self.assertEqual(self.collect()["cases"], {})

    def test_interval_outside_real_log_cannot_publish_pass(self):
        self.record["intervals"][0]["pass_line"] = 100
        self.write_record()
        self.assertEqual(self.collect()["cases"], {})

    def test_synthetic_is_separate_even_without_generator(self):
        name = "dv_hypertest_selftest_clobber_case"
        self.assembly.write_text("# HYPERTEST_CASE: " + name + "\n")
        self.record.update({"static_cases": [name], "case_hits": {name: 1},
                            "intervals": [{"begin_line": 1, "pass_line": 3,
                                           "case_hits": {name: 1}}]})
        self.write_record()
        self.generator.unlink()
        result = self.collect()
        self.assertEqual(result["cases"], {})
        self.assertEqual(result["supplementary_checks"]["bridge_selftest"][0]["validation"],
                         "gcc_spike_pass")

    def test_cannot_borrow_elf_from_different_run(self):
        wrong_elf = self.repo / "other.o"
        wrong_elf.write_bytes(self.elf.read_bytes())
        self.record["elf"] = str(wrong_elf)
        self.write_record()
        self.assertEqual(self.collect()["cases"], {})

    def test_multiple_directories_merge_without_double_counting_inputs(self):
        result = collector.collect([self.run, self.run], self.repo, self.manifest)
        self.assertEqual(result["cases"]["case_a"]["completed_count"], 1)


if __name__ == "__main__":
    unittest.main()
