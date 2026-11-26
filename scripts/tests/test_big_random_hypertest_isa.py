"""Guard the batch template's source Hypertest bridge ISA contract."""

from pathlib import Path
import re
import unittest

import yaml

from scripts.hypertest_adapter import _require_bridge_isa


REPO = Path(__file__).resolve().parents[2]


class BigRandomHypertestIsaTest(unittest.TestCase):
    def test_source_pool_entries_supply_full_bridge_isa(self):
        entries = yaml.safe_load(
            (REPO / "yaml/big_random_feature_matrix.yaml").read_text(encoding="utf-8"))
        checked = []
        for entry in entries:
            gen_opts = entry.get("gen_opts", "")
            if "+enable_random_hypertest=1" not in gen_opts or "+hypertest_case_file=" not in gen_opts:
                continue
            march = re.search(r"(?:^|\s)-march=([^\s]+)", entry.get("gcc_opts", ""))
            self.assertIsNotNone(march, entry["test"])
            with self.subTest(test=entry["test"]):
                _require_bridge_isa(march.group(1))
            checked.append(entry["test"])
        self.assertIn("nanhu_big_random_hypertest_branch_mix", checked)
        self.assertIn("nanhu_big_random_hypertest_branch_mix_reviewed", checked)


if __name__ == "__main__":
    unittest.main()
