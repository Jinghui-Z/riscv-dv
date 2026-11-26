"""Keep the generator's dynamic-page-table case list aligned with sources."""

from pathlib import Path
import re
import unittest


REPO = Path(__file__).resolve().parents[2]


class HypertestRandomPoolPageTablesTest(unittest.TestCase):
    def test_generator_dynamic_cases_match_vendored_source(self):
        source = (REPO / "src/hypertest/upstream/manual_test_cases/special/"
                  "dynamic_page_table_test.c").read_text(encoding="utf-8")
        generator = (REPO / "src/riscv_asm_program_gen.sv").read_text(encoding="utf-8")
        function = re.search(
            r"virtual function bit is_dynamic_page_table_hypertest_case\(string case_name\);"
            r"(.*?)\bendfunction", generator, re.DOTALL)
        self.assertIsNotNone(function)
        source_names = set(re.findall(
            r"^bool\s+([A-Za-z_][A-Za-z_0-9]*)\s*\(", source, re.MULTILINE))
        generator_names = set(re.findall(r'"([A-Za-z_][A-Za-z_0-9]*)"',
                                         function.group(1)))
        self.assertEqual(generator_names, source_names)

        pool = set((REPO / "src/hypertest/case_names_random_cosim_on.txt")
                   .read_text(encoding="utf-8").splitlines())
        self.assertTrue(generator_names <= pool)


if __name__ == "__main__":
    unittest.main()
