#!/usr/bin/env python3
"""Classify every imported hypertest; absence of proven incompatibility means on.

This is a static source review of the current DV + LinkNan cosim Spike environment,
not a claim that the on cases have passed RTL. Off entries retain a reachable
source witness and a specific model/environment conflict. Compilation, missing
execution evidence, manual naming and lexical review hints are not exclusions.
"""

import argparse
import collections
import csv
from dataclasses import dataclass
import io
import json
from pathlib import Path
import re

from import_hypertest import function_inventory, mask_non_code


ROOT = Path(__file__).resolve().parents[1]
HYPERTEST = ROOT / "src" / "hypertest"
UPSTREAM = HYPERTEST / "upstream"
ON = HYPERTEST / "case_names_random_cosim_on.txt"
OFF = HYPERTEST / "case_names_random_cosim_off.txt"
AUDIT = HYPERTEST / "cosim_classification.csv"
TOKEN = re.compile(r"\b[A-Za-z_]\w*\b")
DIRECTIVE = re.compile(r"^[ \t]*\#(?:\\\n|[^\n])*", re.MULTILINE)


@dataclass
class Node:
    name: str
    path: str
    line: int
    text: str
    kind: str = "function"

    @property
    def key(self):
        return (self.path, self.name, self.line)


def blank(text):
    return "".join("\n" if ch == "\n" else " " for ch in text)


def source_nodes(path, text):
    """Index function bodies and multiline macros without mixing sibling cases.

    Both preprocessor branches are indexed; reaching a symbol is a review aid,
    not execution proof. Only reviewed, unconditional conflict sites exclude.
    """
    nodes = []
    for match in DIRECTIVE.finditer(text):
        macro = re.match(r"\s*#\s*define\s+(\w+)(.*)", match.group(), re.DOTALL)
        if macro:
            nodes.append(Node(macro[1], path, text.count("\n", 0, match.start()) + 1,
                              match.group(), "macro"))
    masked = DIRECTIVE.sub(lambda m: blank(m.group()), mask_non_code(text))
    depth = 0
    boundary = 0
    opening = None
    function = None
    for pos, char in enumerate(masked):
        if char == "{" and depth == 0:
            prefix = masked[boundary:pos]
            candidates = [m for m in re.finditer(r"\b(\w+)\s*\(", prefix)
                          if m[1] not in {"__attribute__", "__attribute", "sizeof",
                                          "if", "for", "while", "switch", "return"}]
            # Initializers and type definitions are not function definitions.
            if prefix.rstrip().endswith(")") and "=" not in prefix and candidates:
                match = candidates[0]
                function = match[1]
                opening = boundary + len(prefix) - len(prefix.lstrip())
        if char == "{":
            depth += 1
        elif char == "}" and depth:
            depth -= 1
            if depth == 0:
                if function:
                    nodes.append(Node(function, path,
                                      text.count("\n", 0, opening) + 1,
                                      text[opening:pos + 1]))
                function = None
                opening = None
                boundary = pos + 1
        elif char == ";" and depth == 0:
            boundary = pos + 1
    return nodes


class Sources:
    def __init__(self):
        self.texts = {}
        self.nodes = {}
        self.symbols = collections.defaultdict(list)
        self.includes = {}
        for path in sorted(UPSTREAM.rglob("*")):
            if path.suffix not in {".c", ".h"}:
                continue
            relative = path.relative_to(UPSTREAM).as_posix()
            text = path.read_text(encoding="utf-8", errors="replace")
            self.texts[relative] = text
            self.includes[relative] = re.findall(r'^\s*#include\s*[<"]([^>"]+)',
                                                text, re.MULTILINE)
            for node in source_nodes(relative, text):
                self.nodes[node.key] = node
                self.symbols[node.name].append(node)
        self.include_cache = {}
        self.edge_cache = {}
        definitions, bodies = function_inventory(UPSTREAM)
        # Source-authored bool definitions are authoritative, including files
        # containing top-level inline asm or macro-generated helper functions.
        for name, items in definitions.items():
            for item in items:
                path = item["path"]
                if any(n.path == path and n.kind == "function"
                       for n in self.symbols[name]):
                    continue
                body = bodies[(path, item["line"])]
                offset = sum(len(line) for line in
                             self.texts[path].splitlines(keepends=True)[:item["line"] - 1])
                opening = self.texts[path].find(body, offset)
                node = Node(name, path, self.texts[path].count("\n", 0, opening) + 1, body)
                self.nodes[node.key] = node
                self.symbols[name].append(node)

    def headers(self, path):
        if path in self.include_cache:
            return self.include_cache[path]
        seen = set()
        pending = [path]
        while pending:
            current = pending.pop()
            if current in seen:
                continue
            seen.add(current)
            for name in self.includes.get(current, []):
                for candidate in (str(Path(current).parent / name), "inc/" + name):
                    if candidate in self.texts:
                        pending.append(candidate)
                        break
        self.include_cache[path] = seen
        return seen

    def edges(self, node):
        if node.key in self.edge_cache:
            return self.edge_cache[node.key]
        # Framework calls are replaced/guarded by bridge.c; indexing the legacy
        # reset or generic trap handlers would incorrectly taint every case.
        skip = {"TEST_START", "TEST_END", "TEST_ASSERT", "TEST_SETUP_EXCEPT",
                "goto_priv", "reset_state", "test_setup_except_function",
                "INFO", "DEBUG", "VERBOSE", "DETAIL", "WARN", "printf",
                "ERROR", "test_print_assert_failure_context", "is_user"}
        code = mask_non_code(node.text)
        tokens = set(TOKEN.findall(code)) - skip - {node.name}
        calls = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", code))
        result = []
        visible = self.headers(node.path)
        for token in sorted(tokens):
            definitions = self.symbols.get(token, [])
            # A reference to a function name is not an execution edge; only
            # calls and macro expansions can reach an incompatible helper.
            if token not in calls and not any(n.kind == "macro" for n in definitions):
                continue
            local = [n for n in definitions if n.path == node.path]
            included = [n for n in definitions if n.path in visible]
            if local:
                result.extend(local)
            elif included:
                result.extend(included)
            else:
                # A unique external definition can be followed across C files.
                external = [n for n in definitions if n.kind == "function" and
                            not re.match(r"\s*static\b", n.text)]
                if len(external) == 1:
                    result.extend(external)
        self.edge_cache[node.key] = result
        return result

    def reachable(self, root):
        seen = {}
        pending = [(root, root.name)]
        while pending:
            node, chain = pending.pop()
            if node.key in seen:
                continue
            seen[node.key] = chain
            yield node, chain
            pending.extend((child, chain + " -> " + child.name)
                           for child in reversed(self.edges(node)))


# These sites drive testbench-only transports. Their standalone Spike precheck
# needs an external responder, but that alone is not a cosim incompatibility.
EXTERNAL_SITES = {
    "ai_vmodule_write_reg", "ai_imv_vmodule_write_reg", "ai_ibc_vmodule_write_reg",
    "be_vm_write", "fe_vm_raw", "ai_frontend_it_vmodule_write_reg",
}


def conflicts(node):
    """Return proven environment conflicts, never generic 'needs review' hints."""
    code = mask_non_code(node.text)
    if node.kind != "function":
        return []
    findings = []
    # LinkNan RTL implements this ECC injection interface, but its cosim Spike
    # does not register SCEC (0x5da) or SCEIC (0x5db). These tests need valid
    # accesses and injection records, not the illegal CSR trap Spike produces.
    if node.path.endswith("/ai_micro_cache_error_nmi_cases.c"):
        csr_access = re.search(r"\b(?:CSRR|CSRW|CSRS|CSRC)\s*\(\s*"
                               r"CSR_(?:SCEC|SCEIC)\b", code)
        if csr_access:
            findings.append(("requires_unmodelled_ecc_csr",
                             "LinkNan Spike csr_init.cc lacks SCEC 0x5da and SCEIC 0x5db; this test requires ECC injection and records",
                             csr_access.start()))
    # The generic CSR switch also contains expected-illegal accesses. Exclude
    # only these two reviewed positive matrices, not every caller of the switch.
    positive_csr_matrices = {
        "ai_arch_memblock_custom_csr_privilege_wpri_warl_matrix_corner":
            "ai_arch_custom_read_csr(custom_csrs[i])",
        "ai_arch_scec_sceic_baseline_field_privilege_matrix_corner":
            "CSRR(CSR_SCEC)",
    }
    if (node.path.endswith("/ai_arch_custom_csr_cases.c") and
            node.name in positive_csr_matrices):
        access = code.find(positive_csr_matrices[node.name])
        if access >= 0:
            findings.append(("requires_unmodelled_ecc_csr",
                             "This positive CSR matrix requires successful SCEC 0x5da/SCEIC 0x5db access; LinkNan Spike does not register them and raises illegal instruction",
                             access))
    if (node.path.endswith("/ai_micro_memblock_pte_ptw_legal_cases.c") and
            node.name == "ai_micro_memblock_global_pte_cross_asid_cached_hit_corner"):
        assertion = code.find("observed_cross_asid == old_value")
        if assertion >= 0:
            findings.append(("requires_stale_global_tlb_entry",
                             "Case demands old global-PTE mapping after leaf rewrite/ASID switch; LinkNan Spike mmu.h access_tlb always misses and current NUM_CORES=1 disables successful-load synchronization",
                             assertion))
    return findings


def environment_dependencies(node):
    """Target support, TB setup and precheck gaps are not cosim verdicts."""
    if node.kind != "function":
        return []
    result = []
    virtual = re.search(r"\bgoto_priv\s*\(\s*PRIV_V[SU]\s*\)",
                        mask_non_code(node.text))
    if virtual:
        result.append(("requires_h_virtual_mode",
                       "Current NanHu has no H and bridge.c rejects VS/VU in both models; target unsupported, not proof of a cosim mismatch",
                       virtual.start()))
    if node.name in EXTERNAL_SITES:
        result.append(("requires_tb_command_responder",
                       "Requires the matching RTL TB responder and VMODULE harness configuration; standalone Spike cannot supply the event", 0))
    if (node.path.endswith("/backend_it_0911_common.c") and
            node.name in {"be_it_prepare_scenario", "be_it_finish_timeout"}):
        result.append(("requires_external_monitor_ack",
                       "Requires Backend IT monitor and ELF control address; current runner is no-diff, but CBO.INVAL reference-memory sync means incompatibility is not proven", 0))
    return result


MODEL_EVIDENCE = {
    "requires_unmodelled_ecc_csr": "LinkNan Spike/riscv/csr_init.cc:662-668 lacks 0x5da/0x5db; Spike/riscv/processor.cc:741-751 traps missing CSR; RTL backend/fu/NewCSR/CSRCustom.scala:30-43 implements both",
    "requires_stale_global_tlb_entry": "LinkNan Spike/riscv/mmu.h:593-602; dependencies/difftest/src/test/csrc/difftest/difftest.cpp:1265-1268; sim/simv/comp/vcs_cmd.sh NUM_CORES=1",
}


def partition(manifest, sources, case_bodies):
    records = []
    cases = manifest["cases"]
    names = {case["name"] for case in cases}
    if len(names) != len(cases):
        raise ValueError("Manifest contains duplicate cases")
    if names != set((HYPERTEST / "case_names.txt").read_text().splitlines()):
        raise ValueError("Manifest and case_names.txt disagree")
    for case in sorted(cases, key=lambda c: c["name"]):
        findings = []
        dependencies = []
        root_nodes = []
        for definition in case["definitions"]:
            body = case_bodies.get((definition["path"], definition["line"]))
            if body is None:
                raise ValueError("Cannot isolate case body: " + case["name"])
            text = sources.texts[definition["path"]]
            offset = sum(len(line) for line in
                         text.splitlines(keepends=True)[:definition["line"] - 1])
            opening = text.find(body, offset)
            if opening < 0:
                raise ValueError("Cannot locate case body: " + case["name"])
            root_nodes.append(Node(case["name"], definition["path"],
                                   text.count("\n", 0, opening) + 1, body))
        # Native SV templates are different implementations from the C adapter.
        if not case["native_class"]:
            for root in root_nodes:
                for node, chain in sources.reachable(root):
                    for reason, explanation, offset in conflicts(node):
                        line = node.line + node.text.count("\n", 0, offset)
                        findings.append((reason, explanation,
                                         "{}:{}".format(node.path, line), chain))
                    for reason, explanation, offset in environment_dependencies(node):
                        line = node.line + node.text.count("\n", 0, offset)
                        dependencies.append((reason, explanation,
                                             "{}:{}".format(node.path, line), chain))
        findings = sorted(set(findings))
        dependencies = sorted(set(dependencies))
        records.append({
            "case_name": case["name"],
            "cosim_random_pool": "off" if findings else "on",
            "reason": ";".join(sorted({f[0] for f in findings})) if findings else
                      "no_proven_cosim_incompatibility",
            "evidence": " | ".join(f[2] + " [" + f[3] + "]: " + f[1]
                                     for f in findings),
            "model_evidence": " | ".join(MODEL_EVIDENCE[reason]
                                        for reason in sorted({f[0] for f in findings})),
            "environment_dependencies": ";".join(sorted({d[0] for d in dependencies})),
            "dependency_evidence": " | ".join(d[2] + " [" + d[3] + "]: " + d[1]
                                              for d in dependencies),
            "validation": case["validation"],
            "migration_path": case["migration_path"],
            "source": ";".join(d["path"] + ":" + str(d["line"])
                                for d in case["definitions"]),
            "review_hints_not_verdicts": ";".join(h["kind"] for h in case["review_hints"]),
        })
    return records


def render():
    manifest = json.loads((HYPERTEST / "manifest.json").read_text())
    sources = Sources()
    _, case_bodies = function_inventory(UPSTREAM)
    records = partition(manifest, sources, case_bodies)
    on = {r["case_name"] for r in records if r["cosim_random_pool"] == "on"}
    off = {r["case_name"] for r in records if r["cosim_random_pool"] == "off"}
    assert not on & off and len(on | off) == len(records)
    buffer = io.StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=list(records[0]), lineterminator="\n")
    writer.writeheader()
    writer.writerows(records)
    outputs = {ON: "".join(n + "\n" for n in sorted(on)),
               OFF: "".join(n + "\n" for n in sorted(off)), AUDIT: buffer.getvalue()}
    return outputs, records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify outputs without writing")
    args = parser.parse_args()
    outputs, records = render()
    for path, text in outputs.items():
        if args.check:
            if path.read_text(encoding="utf-8") != text:
                raise ValueError("Stale classification: " + str(path))
        else:
            path.write_text(text, encoding="utf-8")
    counts = collections.Counter(r["cosim_random_pool"] for r in records)
    print("catalog={} cosim_on={} cosim_off={} checked={}".format(
        len(records), counts["on"], counts["off"], args.check))
    print(dict(collections.Counter(r["reason"] for r in records if r["cosim_random_pool"] == "off")))


if __name__ == "__main__":
    main()
