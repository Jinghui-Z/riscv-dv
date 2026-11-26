"""Regression checks for the per-ELF random Hypertest hit requirement."""

import json
from pathlib import Path
import tempfile
import unittest

import run


class HypertestMinHitsTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.log = Path(self.temp_dir.name) / "case.log"
        self.report = self.log.with_suffix(".hypertest-validation.json")
        self.test = {"test": "branch_mix", "min_hypertest_hits": 1}

    def write_hits(self, hits):
        self.report.write_text(json.dumps({"injection_hits": hits}))

    def test_executed_injection_passes(self):
        self.write_hits({"first": 1, "second": 0})
        run.check_hypertest_min_hits(str(self.log), self.test)

    def test_all_skipped_injections_fail(self):
        self.write_hits({"first": 0, "second": 0})
        with self.assertRaises(SystemExit):
            run.check_hypertest_min_hits(str(self.log), self.test)

    def test_missing_validation_report_fails(self):
        with self.assertRaises(SystemExit):
            run.check_hypertest_min_hits(str(self.log), self.test)


if __name__ == "__main__":
    unittest.main()
