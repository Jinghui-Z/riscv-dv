"""Guard default-on admission and source evidence isolation."""

import collections
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import classify_hypertest_cosim as classifier


class SourceIsolationTest(unittest.TestCase):
    def test_multiline_macro_and_sibling_cases_stay_separate(self):
        source = (
            "#define POSITIVE_HELPER() \\\n"
            "    do { positive(); } while (0)\n"
            "// bool fake() { goto_priv(PRIV_VS); }\n"
            "bool ordinary() { if (1) { ordinary_helper(); } return true; }\n"
            "bool virtual_case() { goto_priv(PRIV_VS); return true; }\n"
        )
        nodes = {node.name: node for node in classifier.source_nodes("cases.c", source)}
        self.assertEqual(set(nodes), {"POSITIVE_HELPER", "ordinary", "virtual_case"})
        self.assertNotIn("goto_priv", nodes["ordinary"].text)
        self.assertIn("positive();", nodes["POSITIVE_HELPER"].text)
        self.assertEqual(nodes["virtual_case"].line, 5)

    def test_function_pointer_reference_is_not_a_call(self):
        sources = classifier.Sources.__new__(classifier.Sources)
        helper = classifier.Node("helper", "cases.c", 1, "void helper() {}")
        sources.symbols = {"helper": [helper]}
        sources.includes = {"cases.c": []}
        sources.include_cache = {}
        sources.edge_cache = {}
        reference = classifier.Node("reference", "cases.c", 2, "{ void *p = helper; }")
        call = classifier.Node("call", "cases.c", 3, "{ helper(); }")
        self.assertEqual(sources.edges(reference), [])
        self.assertEqual(sources.edges(call), [helper])

    def test_target_or_tb_dependency_alone_does_not_exclude(self):
        nodes = [
            classifier.Node("h_case", "cases.c", 1, "{ goto_priv(PRIV_VS); }"),
            classifier.Node("ai_vmodule_write_reg", "cases.c", 1, "{}"),
            classifier.Node("be_it_prepare_scenario",
                            "backend/micro/backend_it_0911_common.c", 1, "{}"),
        ]
        for node in nodes:
            with self.subTest(node=node.name):
                self.assertEqual(classifier.conflicts(node), [])
                self.assertTrue(classifier.environment_dependencies(node))

    def test_lexical_names_and_comments_are_not_incompatibility_proof(self):
        node = classifier.Node("manual_ecc_pma_hyper_test", "cases.c", 1,
                               "{ /* CSRR(CSR_SCEC); goto_priv(PRIV_VS); */ }")
        self.assertEqual(classifier.conflicts(node), [])
        self.assertEqual(classifier.environment_dependencies(node), [])


class CatalogAdmissionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.outputs, cls.records = classifier.render()
        cls.by_name = {row["case_name"]: row for row in cls.records}

    def test_lists_are_exact_sorted_partition_of_authoritative_inventory(self):
        manifest = json.loads((classifier.HYPERTEST / "manifest.json").read_text())
        names = {case["name"] for case in manifest["cases"]}
        on = self.outputs[classifier.ON].splitlines()
        off = self.outputs[classifier.OFF].splitlines()
        for values in (on, off):
            self.assertEqual(values, sorted(set(values)))
            self.assertTrue(all(values))
        self.assertFalse(set(on) & set(off))
        self.assertEqual(set(on) | set(off), names)
        self.assertEqual(names, set((classifier.HYPERTEST / "case_names.txt")
                                    .read_text().splitlines()))

    def test_every_exclusion_has_source_chain_and_model_evidence(self):
        for row in self.records:
            with self.subTest(case=row["case_name"]):
                if row["cosim_random_pool"] == "off":
                    self.assertTrue(row["evidence"])
                    self.assertTrue(row["model_evidence"])
                    self.assertIn(row["case_name"], row["evidence"])
                else:
                    self.assertEqual(row["reason"], "no_proven_cosim_incompatibility")

    def test_native_and_previously_executed_cases_remain_on(self):
        passed = [row for row in self.records if row["validation"] == "gen_gcc_spike_pass"]
        self.assertTrue(passed)
        for row in passed:
            self.assertEqual(row["cosim_random_pool"], "on", row["case_name"])
        native = [row for row in self.records if row["migration_path"] != "source_adapter"]
        self.assertTrue(native)
        self.assertTrue(all(row["cosim_random_pool"] == "on" for row in native))

    def test_no_execution_record_is_not_a_reason_to_exclude(self):
        on_unrun = [row for row in self.records
                    if row["validation"] == "not_run" and row["cosim_random_pool"] == "on"]
        self.assertGreater(len(on_unrun), 4000)
        self.assertEqual(self.by_name["time_test_1"]["cosim_random_pool"], "on")
        self.assertEqual(self.by_name[
            "ai_micro_memblock_pte_u_update_requires_sfence_old_entry_boundary_corner"
        ]["cosim_random_pool"], "on")
        self.assertEqual(self.by_name[
            "ai_arch_memblock_custom_csr_trap_handler_snapshot_restore_corner"
        ]["cosim_random_pool"], "on")

    def test_h_and_external_environment_dependencies_are_on(self):
        covered = collections.Counter()
        for row in self.records:
            if row["environment_dependencies"]:
                self.assertEqual(row["cosim_random_pool"], "on", row["case_name"])
                self.assertTrue(row["dependency_evidence"])
                covered.update(row["environment_dependencies"].split(";"))
        self.assertTrue(covered["requires_h_virtual_mode"])
        self.assertTrue(covered["requires_tb_command_responder"])
        self.assertTrue(covered["requires_external_monitor_ack"])

    def test_ecc_positive_access_and_single_core_stale_mapping_are_off(self):
        for name in (
            "ai_micro_m_icache_data_ecc_inject_fetch_progress_reports_scec_corner",
            "ai_arch_memblock_custom_csr_privilege_wpri_warl_matrix_corner",
            "ai_arch_scec_sceic_baseline_field_privilege_matrix_corner",
            "ai_micro_memblock_global_pte_cross_asid_cached_hit_corner",
        ):
            self.assertEqual(self.by_name[name]["cosim_random_pool"], "off")


if __name__ == "__main__":
    unittest.main()
