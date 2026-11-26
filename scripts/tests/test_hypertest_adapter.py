"""Behavior checks for the hypertest build and execution boundaries."""

import importlib.util
import io
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch


ADAPTER_PATH = Path(__file__).resolve().parents[1] / "hypertest_adapter.py"
SPEC = importlib.util.spec_from_file_location("hypertest_adapter", ADAPTER_PATH)
adapter = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(adapter)


def pc(address, opcode="00000013"):
    return "core   0: 0x{:016x} (0x{}) nop\n".format(address, opcode)


class SpikeValidationTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.assembly = self.root / "test.S"
        self.log = self.root / "test.log"
        self.elf = self.root / "test.o"
        self.assembly.write_text("# HYPERTEST_CASE: case_a\n")
        self.symbols = {"dv_hypertest_begin": 0x1000,
                        "dv_hypertest_pass": 0x1010,
                        "dv_hypertest_fail": 0x1020,
                        "case_a": 0x2000, "case_b": 0x3000,
                        "tohost": 0x80005000}

    def validate(self, content, **kwargs):
        self.log.write_text(content)
        with patch.object(adapter, "_nm_symbols", return_value=self.symbols):
            return adapter.validate_spike(self.log, self.elf, self.assembly, **kwargs)

    def program_pass(self):
        return (pc(0x4000) + "core   0: 3 0x4000 (0x23) "
                "mem 0x0000000080005000 0x0000000000000001\n")

    def test_only_passed_case_interval_is_exempt(self):
        content = (pc(0x800) + "core 0: exception trap_illegal_instruction\n" +
                   pc(0x1000) + pc(0x2000) +
                   "core 0: exception trap_illegal_instruction\n" +
                   "core 0: epc 0x2000, tval 0xffffffff\n" +
                   pc(0x1010) + "core 0: exception trap_illegal_instruction\n" +
                   self.program_pass())
        self.assertEqual(self.validate(content), {2, 3, 4, 5, 6})
        artifact = json.loads(self.log.with_suffix(".hypertest-validation.json").read_text())
        self.assertEqual(artifact["status"], "passed")
        self.assertEqual(artifact["case_hits"], {"case_a": 1})
        self.assertEqual(artifact["intervals"][0]["illegal_traps"],
                         [{"line": 5, "pc": "0x2000", "tval": "0xffffffff"}])
        self.assertTrue(artifact["intervals"][0]["original_selfcheck_passed"])

    def test_missing_begin_never_passes(self):
        with self.assertRaisesRegex(RuntimeError, "No hypertest case executed"):
            self.validate(pc(0x2000) + self.program_pass())

    def test_missing_pass_never_exempts_expected_illegal(self):
        with self.assertRaisesRegex(RuntimeError, "no successful return"):
            self.validate(pc(0x1000) + pc(0x2000) +
                          "core 0: exception trap_illegal_instruction\n"
                          "core 0: epc 0x2000, tval 0xffffffff\n")

    def test_fail_marker_never_exempted(self):
        with self.assertRaisesRegex(RuntimeError, "Hypertest failed"):
            self.validate(pc(0x1000) + pc(0x2000) + pc(0x1020))

    def test_symbol_existence_is_not_case_execution(self):
        with self.assertRaisesRegex(RuntimeError, "without executing"):
            self.validate(pc(0x1000) + pc(0x2222) + pc(0x1010))

    def test_each_static_injection_requires_a_hit(self):
        self.assembly.write_text("# HYPERTEST_CASE: case_a\n# HYPERTEST_CASE: case_b\n")
        with self.assertRaisesRegex(RuntimeError, "case_b .*missing"):
            self.validate(pc(0x1000) + pc(0x2000) + pc(0x1010) + self.program_pass())

    def test_normal_dv_completion_is_required(self):
        with self.assertRaisesRegex(RuntimeError, "no normal DV tohost pass"):
            self.validate(pc(0x1000) + pc(0x2000) + pc(0x1010))

    def test_failing_tohost_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "failing tohost"):
            self.validate(pc(0x1000) + pc(0x2000) + pc(0x1010) +
                          "core 0: 3 0x4000 (0x23) mem 0x80005000 0x3\n")

    def test_pass_without_begin_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "pass without begin"):
            self.validate(pc(0x1010))

    def test_random_branch_can_skip_some_injections_without_claiming_their_coverage(self):
        self.assembly.write_text("# HYPERTEST_CASE: case_a\n# HYPERTEST_CASE: case_b\n")
        self.assertEqual(self.validate(pc(0x1000) + pc(0x2000) + pc(0x1010) +
                                       self.program_pass(), require_all=False), {0, 1, 2})
        artifact = json.loads(self.log.with_suffix(".hypertest-validation.json").read_text())
        self.assertEqual(artifact["status"], "passed_partial")
        self.assertEqual(artifact["skipped_cases"], {"case_b": 1})

    def test_all_skipped_injections_have_no_exemptions_and_no_pass_claim(self):
        self.assertEqual(self.validate(self.program_pass(), require_all=False), set())
        artifact = json.loads(self.log.with_suffix(".hypertest-validation.json").read_text())
        self.assertEqual(artifact["status"], "not_reached")
        self.assertEqual(artifact["case_hits"], {})

    def test_partial_mode_still_rejects_entered_cases_without_return(self):
        with self.assertRaisesRegex(RuntimeError, "no successful return"):
            self.validate(pc(0x1000) + pc(0x2000), require_all=False)

    def test_partial_mode_still_rejects_fail_marker(self):
        with self.assertRaisesRegex(RuntimeError, "Hypertest failed"):
            self.validate(pc(0x1000) + pc(0x2000) + pc(0x1020), require_all=False)

    def test_all_skipped_program_still_needs_normal_completion(self):
        with self.assertRaisesRegex(RuntimeError, "no normal DV tohost pass"):
            self.validate(pc(0x4000), require_all=False)

    def test_passed_case_cannot_exempt_an_unidentifiable_illegal_trap(self):
        with self.assertRaisesRegex(RuntimeError, "has no tval"):
            self.validate(pc(0x1000) + pc(0x2000) +
                          "core 0: exception trap_illegal_instruction\n" +
                          pc(0x1010) + self.program_pass())

    def test_unmarked_legacy_log_does_not_need_nm(self):
        self.assembly.write_text("addi a0, a0, 1\n")
        with patch.object(adapter, "_nm_symbols", side_effect=AssertionError):
            self.assertEqual(adapter.validate_spike(self.log, self.elf, self.assembly), set())

    def metadata(self, kind, name="case_a", tag="injection_a", begin=0x5000, passed=0x5010):
        self.symbols[tag + "_begin"] = begin
        self.symbols[tag + "_pass"] = passed
        dependency = "# HYPERTEST_CASE: " + name + "\n" if kind == "source" else ""
        return dependency + "# HYPERTEST_INJECTION: {} {} {}_begin {}_pass\n".format(kind, name, tag, tag)

    def test_native_injection_needs_no_c_function_or_common_bridge_symbols(self):
        self.assembly.write_text(self.metadata("native", name="native_case"))
        self.symbols = {name: value for name, value in self.symbols.items()
                        if name in ("injection_a_begin", "injection_a_pass", "tohost")}
        self.assertEqual(self.validate(pc(0x5000) + pc(0x5555) + pc(0x5010) +
                                       self.program_pass()), set())
        artifact = json.loads(self.log.with_suffix(".hypertest-validation.json").read_text())
        self.assertEqual(artifact["case_hits"], {"native_case": 1})
        self.assertEqual(artifact["injection_hits"], {"injection_a_begin": 1})

    def test_entered_native_injection_without_pass_fails_even_in_partial_mode(self):
        self.assembly.write_text(self.metadata("native"))
        with self.assertRaisesRegex(RuntimeError, "no successful return"):
            self.validate(pc(0x5000) + pc(0x5555), require_all=False)

    def test_native_injection_can_never_consume_an_illegal_instruction(self):
        self.assembly.write_text(self.metadata("native"))
        with self.assertRaisesRegex(RuntimeError, "illegal instruction in native"):
            self.validate(pc(0x5000) + pc(0x5555) +
                          "core 0: exception trap_illegal_instruction\n" +
                          "core 0: epc 0x5555, tval 0xffffffff\n" +
                          pc(0x5010) + self.program_pass())

    def test_repeated_loop_visits_cannot_cover_a_skipped_static_instance(self):
        self.assembly.write_text(self.metadata("native") +
                                  self.metadata("native", tag="injection_b", begin=0x6000,
                                                passed=0x6010))
        loop = pc(0x5000) + pc(0x5555) + pc(0x5010)
        with self.assertRaisesRegex(RuntimeError, "instances were skipped.*injection_b_begin"):
            self.validate(loop + loop + self.program_pass())
        self.assertEqual(self.validate(loop + loop + self.program_pass(), require_all=False), set())
        artifact = json.loads(self.log.with_suffix(".hypertest-validation.json").read_text())
        self.assertEqual(artifact["status"], "passed_partial")
        self.assertEqual(artifact["injection_hits"], {"injection_a_begin": 2, "injection_b_begin": 0})
        self.assertEqual(artifact["skipped_injection_ids"], ["injection_b_begin"])

    def test_source_wrapper_requires_its_actual_bridge_selfcheck(self):
        self.assembly.write_text(self.metadata("source"))
        with self.assertRaisesRegex(RuntimeError, "exactly one bridge selfcheck"):
            self.validate(pc(0x5000) + pc(0x5010) + self.program_pass())

    def test_source_metadata_exempts_only_its_passed_common_bridge(self):
        self.assembly.write_text(self.metadata("source"))
        content = (pc(0x5000) + pc(0x1000) + pc(0x2000) +
                   "core 0: exception trap_illegal_instruction\n" +
                   "core 0: epc 0x2000, tval 0xffffffff\n" + pc(0x1010) +
                   pc(0x5010) + self.program_pass())
        self.assertEqual(self.validate(content), {1, 2, 3, 4, 5})
        artifact = json.loads(self.log.with_suffix(".hypertest-validation.json").read_text())
        self.assertEqual(artifact["case_hits"], {"case_a": 1})

    def test_wrapper_pass_cannot_complete_another_instance(self):
        self.assembly.write_text(self.metadata("native") +
                                  self.metadata("native", tag="injection_b", begin=0x6000,
                                                passed=0x6010))
        with self.assertRaisesRegex(RuntimeError, "without matching begin"):
            self.validate(pc(0x5000) + pc(0x6010) + self.program_pass(), require_all=False)

    def test_native_metadata_can_follow_a_gnu_numeric_label(self):
        self.assembly.write_text("1: " + self.metadata("native"))
        self.assertEqual(self.validate(pc(0x5000) + pc(0x5010) + self.program_pass()), set())

    def test_source_markers_can_follow_main_and_local_symbol_labels(self):
        metadata = self.metadata("source").splitlines()
        self.assembly.write_text("main:             " + metadata[0] + "\n.Lmetadata: " +
                                  metadata[1] + "\n")
        self.assertEqual(adapter.case_names(self.assembly), ["case_a"])
        self.assertEqual(self.validate(pc(0x5000) + pc(0x1000) + pc(0x2000) +
                                       pc(0x1010) + pc(0x5010) + self.program_pass()), {1, 2, 3})

    def test_assembly_strings_and_instruction_comments_are_not_dependency_markers(self):
        self.assembly.write_text('.asciz "main: # HYPERTEST_CASE: string_phantom"\n'
                                  '.ascii "1: # HYPERTEST_INJECTION: native phantom b p"\n'
                                  'addi a0, a0, 1 # HYPERTEST_CASE: instruction_phantom\n')
        self.assertEqual(adapter.case_names(self.assembly), [])
        self.assertEqual(adapter.injection_metadata(self.assembly), [])

    def test_malformed_labelled_injection_is_rejected(self):
        self.assembly.write_text("main: # HYPERTEST_INJECTION: native case_a missing_pass\n")
        with self.assertRaisesRegex(RuntimeError, "Malformed HYPERTEST_INJECTION"):
            adapter.injection_metadata(self.assembly)


class BuildObjectsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.assembly = self.root / "case.S"
        self.assembly.write_text("# HYPERTEST_CASE: case_a\n")
        self.bridge = self.root / "src" / "hypertest"
        self.upstream = self.bridge / "upstream"
        for relative in ("src/rvh_test.c", "src/instruction.c", "src/page_tables.c",
                         "inc/rvh_test.h", "ai_test_cases/a.c"):
            path = self.upstream / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("/* fixture */\n")
        (self.upstream / "src" / "rvh_test.c").write_text(
            "\n".join("unsigned {}(void) {{\n    "
                      "return_from_exception(temp_priv, curr_priv, cause, epc);\n}}"
                      .format(name) for name in ("mhandler", "hshandler", "vshandler")))
        handlers = self.upstream / "asm" / "handlers.S"
        handlers.parent.mkdir(parents=True)
        handlers.write_text("add sp, sp, -(REGLEN*31)\nadd sp, sp, (REGLEN*31)\n")
        for relative in ("bridge.c", "bridge.S"):
            (self.bridge / relative).write_text("/* bridge fixture */\n")
        self.output = self.root / "out"
        self.case_source = self.upstream / "ai_test_cases" / "a.c"
        self.plan = {"case_a": SimpleNamespace(c_sources=(self.case_source,), asm_sources=())}
        self.planner = SimpleNamespace(build_case_source_plan=lambda tests: (self.plan, {}))
        self.executed = []

    def compiler(self, command, **kwargs):
        self.executed.append(command)
        Path(command[command.index("-o") + 1]).write_bytes(b"object")
        return subprocess.CompletedProcess(command, 0, "", "")

    def build(self, **kwargs):
        with patch.object(adapter, "_load_planner", return_value=self.planner):
            with patch.object(adapter.subprocess, "run", side_effect=self.compiler):
                return adapter.build_objects(self.root, self.assembly, "fake-gcc", self.output,
                                             "rv64gcv_zicsr", "lp64", **kwargs)

    def test_build_caches_matching_sources_but_respects_header_changes(self):
        result = self.build()
        self.assertEqual(result[-2:], ["-Wl,--gc-sections", "-lgcc"])
        self.assertEqual(len(self.executed), 7)
        for command in self.executed:
            self.assertIn("-march=rv64gcv_zicsr", command)
            self.assertIn("-mabi=lp64", command)
            self.assertIn("-Werror=return-type", command)
        rvh_command = next(c for c in self.executed
                           if "-Dreset_state=dv_hyp_legacy_reset_state" in c)
        self.assertIn("-Dreset_state=dv_hyp_legacy_reset_state", rvh_command)
        self.assertIn("-Dgoto_priv=dv_hyp_legacy_goto_priv", rvh_command)
        self.build()
        self.assertEqual(len(self.executed), 7)
        (self.upstream / "inc" / "rvh_test.h").write_text("/* changed header */")
        self.build()
        self.assertEqual(len(self.executed), 14)

    def test_unknown_case_fails_instead_of_building_everything(self):
        self.planner.build_case_source_plan = lambda tests: ({}, {"case_a": "not found"})
        with self.assertRaisesRegex(ValueError, "Unknown or ambiguous"):
            self.build()
        self.assertFalse(self.executed)
        artifact = next((self.output / "hypertest").glob("*.build.json"))
        self.assertEqual(json.loads(artifact.read_text())["status"], "failed")

    def test_compile_failure_is_recorded_and_not_cached(self):
        with patch.object(adapter, "_load_planner", return_value=self.planner):
            with patch.object(adapter.subprocess, "run", return_value=
                              subprocess.CompletedProcess([], 1, "", "bad instruction")):
                with self.assertRaisesRegex(RuntimeError, "bad instruction"):
                    adapter.build_objects(self.root, self.assembly, "fake-gcc", self.output,
                                          "rv64gcv_zicsr", "lp64")
        self.assertEqual(list((self.output / "hypertest" / "objects").glob("*.o")), [])

    def test_unmarked_legacy_assembly_has_no_extra_dependencies(self):
        self.assembly.write_text("nop\n")
        with patch.object(adapter, "_load_planner", side_effect=AssertionError):
            self.assertEqual(adapter.build_objects(self.root, self.assembly, "fake-gcc",
                                                  self.output, "rv32i", "ilp32"), [])

    def test_source_bridge_reports_its_isa_requirement_before_assembler_errors(self):
        with self.assertRaisesRegex(ValueError, "requires an RV64 GNU ISA with F, D and full V"):
            adapter.build_objects(self.root, self.assembly, "fake-gcc", self.output,
                                  "rv64imafdc_zicsr", "lp64d")

    def test_debug_records_commands_without_claiming_compilation(self):
        debug = io.StringIO()
        self.build(debug_cmd=debug)
        self.assertFalse(self.executed)
        self.assertIn("-march=rv64gcv_zicsr", debug.getvalue())
        artifact = next((self.output / "hypertest").glob("*.build.json"))
        self.assertEqual(json.loads(artifact.read_text())["status"], "commands-only")

    def test_runner_empty_debug_argument_executes_the_compiler(self):
        self.build(debug_cmd="")
        self.assertEqual(len(self.executed), 7)

    def test_main_label_on_first_case_marker_still_compiles_its_dependencies(self):
        self.assembly.write_text("main:             # HYPERTEST_CASE: case_a\n")
        self.build()
        self.assertEqual(len(self.executed), 7)
        self.assertTrue(any(str(self.case_source) in command for command in self.executed))

    def test_changed_handler_structure_cannot_be_silently_rewritten(self):
        (self.upstream / "src" / "rvh_test.c").write_text("/* incompatible source */")
        with self.assertRaisesRegex(RuntimeError, "exactly 3 hypertest C handler return fixes"):
            self.build()
        self.assertFalse(self.executed)

    def test_dynamic_cases_select_the_matching_core(self):
        self.case_source.write_text("#include <dynamic_page_tables.h>\n")
        (self.upstream / "src" / "dynamic_page_tables.c").write_text("/* dynamic core */")
        self.build()
        sources = {c[c.index("-c") + 1] for c in self.executed}
        self.assertIn(str(self.upstream / "src" / "dynamic_page_tables.c"), sources)
        self.assertNotIn(str(self.upstream / "src" / "page_tables.c"), sources)

    def test_mixed_static_and_dynamic_cases_fail_with_sources(self):
        self.case_source.write_text("#include <dynamic_page_tables.h>\n")
        static_source = self.case_source.with_name("static.c")
        static_source.write_text("#include <page_tables.h>\n")
        self.plan["case_a"] = SimpleNamespace(c_sources=(self.case_source, static_source),
                                            asm_sources=())
        with self.assertRaisesRegex(ValueError, "Cannot mix static and dynamic"):
            self.build()
        self.assertFalse(self.executed)


class PrepareSourceTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.upstream = self.root / "upstream"
        self.source = self.upstream / "manual_test_cases" / "unit" / "legacy.c"
        self.source.parent.mkdir(parents=True)
        self.prepared = self.root / "generated"

    @unittest.skipUnless(shutil.which("cc"), "native C compiler required for assertion propagation")
    def test_missing_end_protocol_preserves_failed_assertion_and_local_include(self):
        (self.source.parent / "common.h").write_text(
            "typedef _Bool bool;\n#define PRIV_M 3\n"
            "#define TEST_START() bool test_status = 1\n"
            "#define TEST_ASSERT(message, condition) test_status = test_status && (condition)\n"
            "static void goto_priv(int value) { (void)value; }\n"
            "static void reset_state(void) {}\n")
        original = r'''#include "common.h"
bool failed_case(void) {
    TEST_START();
    TEST_ASSERT("failure must survive", 0);
    const char *example = "} TEST_END() {";
    (void)example;
    /* TEST_END(); */
}
bool successful_case(void) {
    TEST_START();
    if (1) { TEST_ASSERT("success", 1); }
}
bool bool_helper(void) { return 1; }
int main(void) {
    return failed_case() == 0 && successful_case() == 1 && bool_helper() ? 0 : 1;
}
'''
        self.source.write_text(original)
        source, flags, adaptations = adapter.prepare_source(self.source, self.upstream,
                                                           self.prepared)
        self.assertEqual(self.source.read_text(), original)
        self.assertEqual({item["function"] for item in adaptations},
                         {"failed_case", "successful_case"})
        executable = self.root / "assertion_check"
        result = subprocess.run([shutil.which("cc"), "-Werror=return-type", *flags,
                                 str(source), "-o", str(executable)], capture_output=True,
                                text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(subprocess.run([str(executable)]).returncode, 0)

    def test_existing_test_end_and_unmarked_bool_helper_are_not_rewritten(self):
        self.source.write_text("bool completed(void) { TEST_START(); TEST_END(); }\n"
                               "bool helper(void) { return 1; }\n")
        source, flags, adaptations = adapter.prepare_source(self.source, self.upstream,
                                                           self.prepared)
        self.assertEqual(source, self.source)
        self.assertEqual(adaptations, [])

    def test_assembly_never_receives_c_stddef_header(self):
        assembly = self.source.with_suffix(".S")
        assembly.write_text("nop\n")
        _, flags, _ = adapter.prepare_source(assembly, self.upstream, self.prepared)
        self.assertNotIn("-include", flags)


if __name__ == "__main__":
    unittest.main()
