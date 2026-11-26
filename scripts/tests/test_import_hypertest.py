"""Check inventory provenance and prevent accidental case/evidence inflation."""

import importlib.util
import csv
import io
import json
from pathlib import Path
import tempfile
import unittest


IMPORTER_PATH = Path(__file__).resolve().parents[1] / "import_hypertest.py"
SPEC = importlib.util.spec_from_file_location("import_hypertest", IMPORTER_PATH)
importer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(importer)


class ImportHypertestTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source"
        self.source.mkdir()
        self.register = self.source / "test_register.c"
        self.register.write_text("TEST_REGISTER(registered_case);\n")
        self.case_file = self.source / "ai_test_cases" / "unit" / "cases.c"
        self.case_file.parent.mkdir(parents=True)
        self.case_file.write_text("bool registered_case(void) { TEST_START(); TEST_END(); }\n")

    def inventory(self):
        return importer.make_inventory(self.source)

    def test_comments_strings_and_prototypes_cannot_create_cases(self):
        self.case_file.write_text(r'''
/*
bool block_comment_case(void) { TEST_START(); return true; }
*/
// bool line_comment_case(void) { TEST_START(); return true; }
const char *example = "bool quoted_case(void) { TEST_START(); }";
bool prototype_only(void);
bool registered_case(void) {
    const char *brace_text = "} TEST_START() { ";
    /* nested braces in a comment: } } */
    TEST_START();
    if (1) { if (2) { TEST_ASSERT("works", true); } }
    TEST_END();
}
''')
        inventory = self.inventory()
        self.assertEqual({case["name"] for case in inventory["cases"]},
                         {"registered_case"})
        self.assertEqual(len(inventory["cases"][0]["definitions"]), 1)

    def test_unregistered_test_start_is_imported_but_bool_helpers_are_not(self):
        self.case_file.write_text(r'''
static inline bool positive_helper(int x) { return x > 0; }
bool string_helper(void) { const char *x = "TEST_START()"; return x != 0; }
bool comment_helper(void) { /* TEST_START(); */ return true; }
bool registered_case(void) { TEST_START(); TEST_END(); }
bool new_unregistered_case(void) {
    TEST_START();
    TEST_ASSERT("helper checked", positive_helper(1));
    TEST_END();
}
''')
        inventory = self.inventory()
        cases = {case["name"]: case for case in inventory["cases"]}
        self.assertEqual(set(cases), {"registered_case", "new_unregistered_case"})
        self.assertFalse(cases["new_unregistered_case"]["registered"])
        self.assertEqual({item["name"] for item in inventory["non_case_bool_helpers"]},
                         {"positive_helper", "string_helper", "comment_helper"})

    def test_duplicate_and_disabled_registrations_preserve_provenance(self):
        self.register.write_text("TEST_REGISTER(registered_case);\n"
                                 "// TEST_REGISTER(registered_case);\n"
                                 "TEST_REGISTER(registered_case); // duplicate\n"
                                 "// TEST_REGISTER(disabled_case);\n")
        self.case_file.write_text(self.case_file.read_text() +
                                  "bool disabled_case(void) { TEST_START(); TEST_END(); }\n")
        inventory = self.inventory()
        cases = {case["name"]: case for case in inventory["cases"]}
        self.assertEqual(set(cases), {"registered_case", "disabled_case"})
        self.assertTrue(cases["registered_case"]["default_active"])
        self.assertFalse(cases["disabled_case"]["default_active"])
        registrations = cases["registered_case"]["registrations"]
        self.assertEqual([row["line"] for row in registrations], [1, 2, 3])
        self.assertEqual([row["active"] for row in registrations], [True, False, True])

    def test_block_commented_registration_is_not_a_disabled_case(self):
        self.register.write_text(r'''
/* An example, not an intentionally disabled registration:
TEST_REGISTER(block_comment_phantom);
*/
const char *example = "TEST_REGISTER(string_phantom);";
TEST_REGISTER(registered_case);
// TEST_REGISTER(intentionally_disabled);
''')
        inventory = self.inventory()
        self.assertEqual({case["name"] for case in inventory["cases"]},
                         {"registered_case", "intentionally_disabled"})

    def test_different_snapshot_is_rejected_before_any_file_is_overwritten(self):
        inventory = self.inventory()
        destination = self.root / "snapshot"
        importer.copy_snapshot(self.source, destination, inventory["source"]["files"], False)
        original = (destination / "test_register.c").read_bytes()
        self.register.write_text("TEST_REGISTER(changed_case);\n")
        new_file = self.source / "ai_test_cases" / "unit" / "000_new.c"
        new_file.write_text("bool added_case(void) { TEST_START(); TEST_END(); }\n")
        changed = self.inventory()
        with self.assertRaisesRegex(ValueError, "Existing snapshot differs"):
            importer.copy_snapshot(self.source, destination, changed["source"]["files"], False)
        self.assertEqual((destination / "test_register.c").read_bytes(), original)
        self.assertFalse((destination / new_file.relative_to(self.source)).exists())

    def test_snapshot_changed_after_inventory_is_rejected(self):
        inventory = self.inventory()
        self.case_file.write_text("/* source changed after hashing */\n")
        with self.assertRaisesRegex(ValueError, "Source changed during import"):
            importer.copy_snapshot(self.source, self.root / "snapshot",
                                   inventory["source"]["files"], False)

    def test_validation_cannot_credit_a_name_outside_this_snapshot(self):
        inventory = self.inventory()
        validation = self.root / "validation.json"
        validation.write_text(json.dumps({"cases": {
            "registered_case": {"validation": "spike_passed", "evidence": ["real.log"]},
            "case_from_another_snapshot": {"validation": "spike_passed"},
        }}))
        with self.assertRaisesRegex(ValueError, "unknown case names"):
            importer.merge_validation(inventory, validation)
        self.assertEqual(inventory["cases"][0]["validation"], "not_run")

    def test_validation_snapshot_mismatch_is_rejected_before_crediting_cases(self):
        inventory = self.inventory()
        validation = self.root / "validation.json"
        validation.write_text(json.dumps({
            "source_snapshot_sha256": "a_different_snapshot",
            "cases": {"registered_case": {"validation": "gen_gcc_spike_pass"}},
        }))
        with self.assertRaisesRegex(ValueError, "different source snapshot"):
            importer.merge_validation(inventory, validation)
        self.assertEqual(inventory["cases"][0]["validation"], "not_run")
        self.assertNotIn("validation_source", inventory)

    def test_full_validation_records_and_supplementary_evidence_are_preserved(self):
        inventory = self.inventory()
        validation = self.root / "validation.json"
        record = {
            "validation": "gen_gcc_spike_pass", "evidence": ["out/run/spike_sim/case.log"],
            "evidence_sha256": {"out/run/spike_sim/case.log": "log_identity"},
            "case_hits": {"registered_case": 3}, "completed_count": 3,
            "details": {"selfcheck": "original assertions retained"},
            "runs": [{"run": "out/run", "case_hits": {"registered_case": 3},
                      "basis": "injection_intervals"}],
        }
        supplementary = {
            "bridge_selftest": [{"validation": "gcc_spike_pass",
                                 "case_hits": {"bridge_only_check": 1},
                                 "evidence": ["out/selftest/spike_sim/selftest.log"],
                                 "evidence_sha256": {"out/selftest/spike_sim/selftest.log": "hash"},
                                 "counts_as_migrated_case": False}],
            "legacy": [{"test": "legacy_smoke", "validation": "gen_gcc_spike_pass",
                        "evidence": ["out/legacy/spike_sim/legacy.log"],
                        "counts_as_migrated_case": False}],
        }
        data = {"source_snapshot_sha256": inventory["source"]["snapshot_sha256"],
                "cases": {"registered_case": record}, "supplementary_checks": supplementary,
                "rejected_artifacts": [{"artifact": "not_a_pass.json", "reason": "skipped"}]}
        validation.write_text(json.dumps(data))
        importer.merge_validation(inventory, validation)
        case = inventory["cases"][0]
        self.assertEqual(case["validation_record"], record)
        self.assertEqual(case["validation_evidence_sha256"], record["evidence_sha256"])
        self.assertEqual(case["migration_status"], "source_adapter_gcc_spike_verified")
        self.assertEqual(inventory["supplementary_checks"], supplementary)
        self.assertEqual(inventory["validation_metadata"]["rejected_artifacts"],
                         data["rejected_artifacts"])
        self.assertEqual(inventory["counts"]["catalog_cases"], 1)
        rows = list(csv.DictReader(io.StringIO(importer.csv_text(inventory))))
        self.assertEqual(json.loads(rows[0]["validation_record"]), record)
        self.assertEqual(json.loads(rows[0]["validation_evidence_sha256"]),
                         record["evidence_sha256"])
        report = importer.markdown_report(inventory, "cases.csv", "manifest.json", "tests.yaml")
        self.assertIn("# Hypertest 全量移植与验证报告", report)
        self.assertIn("| gen_gcc_spike_pass | 1 |", report)
        self.assertIn("| `registered_case` | `source_adapter_gcc_spike_verified` | 3 |", report)
        self.assertIn("[case.log](<" + str(importer.REPO_ROOT / "out/run/spike_sim/case.log") + ">)",
                      report)
        self.assertIn("| `bridge_selftest` | 1 |", report)
        self.assertIn("| `legacy` | 1 |", report)
        self.assertIn("补充验证（不计入迁移 case 数量）", report)

    def test_native_full_chain_pass_updates_native_migration_status(self):
        inventory = self.inventory()
        inventory["cases"][0]["native_class"] = "native_fixture_stream"
        validation = self.root / "validation.json"
        validation.write_text(json.dumps({"cases": {
            "registered_case": {"validation": "gen_gcc_spike_pass", "evidence": []},
        }}))
        importer.merge_validation(inventory, validation)
        self.assertEqual(inventory["cases"][0]["migration_status"],
                         "implemented_gcc_spike_verified")

    def test_duplicate_validation_list_cannot_silently_replace_evidence(self):
        inventory = self.inventory()
        validation = self.root / "validation.json"
        validation.write_text(json.dumps({"cases": [
            {"name": "registered_case", "validation": "gen_gcc_spike_pass"},
            {"name": "registered_case", "validation": "not_run"},
        ]}))
        with self.assertRaisesRegex(ValueError, "duplicate case names"):
            importer.merge_validation(inventory, validation)

    def test_reviewed_pool_membership_is_independent_of_upstream_active(self):
        inventory = self.inventory()
        default_pool = self.root / "random.txt"
        restricted_pool = self.root / "restricted.txt"
        default_pool.write_text("# explicitly reviewed pool\n")
        restricted_pool.write_text("")
        importer.merge_reviewed_pools(inventory, default_pool, restricted_pool)
        case = inventory["cases"][0]
        self.assertTrue(case["default_active"])
        self.assertFalse(case["default_random_eligible"])
        self.assertFalse(case["restrict_store_reviewed"])
        self.assertEqual(case["validation"], "not_run")
        default_pool.write_text("registered_case # reviewed separately\n")
        importer.merge_reviewed_pools(inventory, default_pool, restricted_pool)
        self.assertTrue(case["default_random_eligible"])
        self.assertFalse(case["restrict_store_reviewed"])

    def test_reviewed_pools_reject_unknown_case_names_and_inconsistent_subset(self):
        default_pool = self.root / "random.txt"
        restricted_pool = self.root / "restricted.txt"
        default_pool.write_text("unknown_case\n")
        restricted_pool.write_text("")
        with self.assertRaisesRegex(ValueError, "unknown cases"):
            importer.merge_reviewed_pools(self.inventory(), default_pool, restricted_pool)
        default_pool.write_text("")
        restricted_pool.write_text("registered_case\n")
        with self.assertRaisesRegex(ValueError, "must be a subset"):
            importer.merge_reviewed_pools(self.inventory(), default_pool, restricted_pool)

    def write_minimal_link_audit(self, inventory):
        """Synthetic diagnostic evidence exercises merge gates, not a real build."""
        output = self.root / "diagnostic"
        generated = output / "generated"
        generated.mkdir(parents=True)
        (generated / "preparation.json").write_text('{"sources": []}\n')
        case = inventory["cases"][0]
        audit = {
            "diagnostic_only": True,
            "source_snapshot_sha256": inventory["source"]["snapshot_sha256"],
            "source_results": [],
            "case_results": [{"name": case["name"], "status": "object_compiled",
                              "definitions": []}],
            "isa": "diagnostic_fixture", "abi": "lp64d", "compiler": ["fixture"],
            "compiler_version": "synthetic test fixture", "common_flags": [],
            "counts": {"source_status": {}, "case_status": {"object_compiled": 1},
                       "elf_link_status": {"elf_linked": 1}},
            "limits": ["no assertion or Spike execution"],
            "link_fixture": {"source": "fixture.S", "source_sha256": "synthetic"},
            "elf_link_results": [{"name": case["name"], "catalog_test": case["catalog_test"],
                                  "status": "elf_linked", "returncode": 0,
                                  "elf_sha256": "synthetic",
                                  "case_symbol": {"name": case["name"]}}],
        }
        path = output / "audit.json"
        path.write_text(json.dumps(audit))
        return path, audit

    def test_compile_and_link_evidence_cannot_credit_execution_or_review(self):
        inventory = self.inventory()
        path, _ = self.write_minimal_link_audit(inventory)
        importer.merge_compile_audit(inventory, path)
        case = inventory["cases"][0]
        self.assertEqual(case["compile_diagnostic"]["status"], "object_compiled")
        self.assertEqual(case["elf_link_diagnostic"]["status"], "elf_linked")
        self.assertFalse(case["elf_link_diagnostic"]["assertions_run"])
        self.assertFalse(case["elf_link_diagnostic"]["spike_run"])
        self.assertFalse(case["elf_link_diagnostic"]["rtl_run"])
        self.assertEqual(case["validation"], "not_run")
        self.assertFalse(case["default_random_eligible"])
        self.assertFalse(case["restrict_store_reviewed"])

    def test_link_evidence_requires_complete_catalog_and_retained_case_symbol(self):
        inventory = self.inventory()
        path, audit = self.write_minimal_link_audit(inventory)
        audit["elf_link_results"][0]["case_symbol"]["name"] = "different_case"
        path.write_text(json.dumps(audit))
        with self.assertRaisesRegex(ValueError, "lacks successful link/symbol evidence"):
            importer.merge_compile_audit(inventory, path)
        audit["elf_link_results"] = []
        path.write_text(json.dumps(audit))
        with self.assertRaisesRegex(ValueError, "complete unique catalog"):
            importer.merge_compile_audit(inventory, path)


if __name__ == "__main__":
    unittest.main()
