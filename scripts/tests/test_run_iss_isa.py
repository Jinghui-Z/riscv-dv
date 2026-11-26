"""Keep an ISS extension override scoped to its selected test."""

import unittest
from unittest import mock

import run


class TestIssIsaOverride(unittest.TestCase):
    def test_one_test_override_does_not_change_next_test(self):
        tests = [
            {"test": "rnmi", "iterations": 1, "iss_isa": "rv64i_smrnmi"},
            {"test": "normal", "iterations": 1},
        ]

        def command_for_isa(iss, iss_yaml, isa, priv, setting_dir, debug_cmd):
            return "spike --isa={} -l <elf>".format(isa)

        with mock.patch.object(run, "parse_iss_yaml", side_effect=command_for_isa) as parse, \
             mock.patch.object(run, "run_cmd_output"), \
             mock.patch.object(run, "run_cmd") as execute, \
             mock.patch.object(run, "check_hypertest_min_hits"):
            run.iss_sim(tests, "/tmp/dv-iss-isa-unit", "spike", "unused.yaml", "",
                        "rv64i", "msu", "", 120, False, False)

        self.assertEqual([call.args[2] for call in parse.call_args_list],
                         ["rv64i_smrnmi", "rv64i"])
        self.assertIn("--isa=rv64i_smrnmi", execute.call_args_list[0].args[0])
        self.assertIn("--isa=rv64i ", execute.call_args_list[1].args[0])


if __name__ == "__main__":
    unittest.main()
