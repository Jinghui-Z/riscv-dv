#!/usr/bin/env python3
"""Collect existing generator/GNU/Spike evidence without executing any tools.

Only cases with a completed runtime interval enter the importer-compatible
``cases`` map. Static insertion, an ELF alone, or a successful program which
skipped an injection never establishes a case pass. Paths in the output are
relative to the DV repository; hashes identify the evidence actually read.
"""

import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re

try:
    from scripts.hypertest_adapter import (CASE_RE, PC_RE, SYNTHETIC_CASE_SOURCES,
                                            injection_metadata)
except ModuleNotFoundError as error:
    if error.name not in ("scripts", "scripts.hypertest_adapter"):
        raise
    from hypertest_adapter import (CASE_RE, PC_RE, SYNTHETIC_CASE_SOURCES,
                                   injection_metadata)


REPO_ROOT = Path(__file__).resolve().parents[1]
ARTIFACT_SUFFIX = ".hypertest-validation.json"
GOOD_STATUSES = {"passed", "passed_partial"}
SUMMARY_RE = re.compile(r"^\s*UVM_(WARNING|ERROR|FATAL)\s*:\s*(\d+)\s*$",
                        re.MULTILINE)


def relative(path, repo):
    return Path(os.path.relpath(Path(path).resolve(), repo)).as_posix()


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_file(path):
    if not path.is_file() or path.stat().st_size == 0:
        raise ValueError("Missing or empty evidence file: {}".format(path))
    return path


def require_elf(path):
    require_file(path)
    with path.open("rb") as stream:
        if stream.read(4) != b"\x7fELF":
            raise ValueError("Linked output does not have ELF magic: {}".format(path))


def recorded_path(value, repo, run):
    if not isinstance(value, str) or not value:
        raise ValueError("Validation record is missing an artifact path")
    path = Path(value)
    candidates = [path] if path.is_absolute() else [repo / path, run / path]
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise ValueError("Recorded evidence path does not exist: {}".format(value))


def contained(path, root):
    return path == root or root in path.parents


def uvm_evidence(run, assembly):
    """Find the successful batch which explicitly emitted this exact .S file."""
    generated = re.compile(r"(?<![A-Za-z_0-9])" + re.escape(assembly.name) +
                           r"\s+is generated\b")
    for path in sorted(run.glob("sim_*.log")):
        text = path.read_text(encoding="utf-8", errors="replace")
        if not generated.search(text):
            continue
        counts = {severity: [] for severity in ("WARNING", "ERROR", "FATAL")}
        for severity, value in SUMMARY_RE.findall(text):
            counts[severity].append(int(value))
        if (all(values and all(value == 0 for value in values)
                for values in counts.values()) and
                "TEST PASSED" in text and "TEST GENERATION DONE" in text and
                "TEST FAILED" not in text):
            return path.resolve()
    raise ValueError("No matching completed generator log with UVM W/E/F all zero for {}"
                     .format(assembly.name))


def positive_count(value):
    return isinstance(value, int) and not isinstance(value, bool) and value > 0


def interval_valid(interval, lines):
    if not isinstance(interval, dict):
        return False
    begin, passed = interval.get("begin_line"), interval.get("pass_line")
    return (positive_count(begin) and positive_count(passed) and
            begin < passed <= len(lines) and
            PC_RE.match(lines[begin - 1]) is not None and
            PC_RE.match(lines[passed - 1]) is not None)


def completed_cases(record, assembly, lines):
    """Return case completion counts, checking new IDs before legacy counters."""
    entries = injection_metadata(assembly)
    if entries or "injections" in record:
        if not entries or record.get("injections") != entries:
            raise ValueError("Injection metadata disagrees with generated assembly")
        hits = record.get("injection_hits")
        intervals = record.get("injection_intervals")
        if not isinstance(hits, dict) or not isinstance(intervals, list):
            raise ValueError("Missing runtime injection hits/intervals")
        index = {entry["id"]: entry for entry in entries}
        if set(hits) != set(index):
            raise ValueError("Injection hit IDs disagree with generated assembly")
        observed = Counter()
        for interval in intervals:
            if not interval_valid(interval, lines):
                raise ValueError("Invalid injection runtime interval")
            entry = index.get(interval.get("injection_id"))
            if (not entry or interval.get("case") != entry["case"] or
                    interval.get("kind") != entry["kind"] or
                    interval.get("original_selfcheck_passed") is not True):
                raise ValueError("Injection interval does not prove its original selfcheck")
            observed[entry["id"]] += 1
        cases = Counter()
        for identifier, entry in index.items():
            count = hits[identifier]
            if (not isinstance(count, int) or isinstance(count, bool) or count < 0 or
                    count != observed[identifier]):
                raise ValueError("Injection hit count disagrees with completed intervals")
            cases[entry["case"]] += count
        return +cases, "injection_intervals"

    names = CASE_RE.findall(assembly.read_text(encoding="utf-8"))
    if not names or Counter(record.get("static_cases", [])) != Counter(names):
        raise ValueError("Legacy source case metadata disagrees with assembly")
    intervals = record.get("intervals")
    declared = record.get("case_hits")
    if not isinstance(intervals, list) or not isinstance(declared, dict):
        raise ValueError("Missing legacy source hits/intervals")
    completed = Counter()
    for interval in intervals:
        if (not interval_valid(interval, lines) or
                interval.get("original_selfcheck_passed", True) is not True):
            raise ValueError("Invalid legacy source selfcheck interval")
        hits = interval.get("case_hits")
        if not isinstance(hits, dict) or not hits:
            raise ValueError("Legacy interval has no actual case entry")
        for name, count in hits.items():
            if name not in names or not positive_count(count):
                raise ValueError("Invalid legacy interval case hit")
            completed[name] += count
    if dict(completed) != declared:
        raise ValueError("Legacy case hits disagree with completed intervals")
    return completed, "legacy_source_intervals"


def evidence(paths, repo):
    names = sorted({relative(path, repo) for path in paths})
    return {"evidence": names,
            "evidence_sha256": {name: sha256(repo / name) for name in names}}


def inspect_artifact(artifact, run, repo):
    record = json.loads(artifact.read_text(encoding="utf-8"))
    if (not isinstance(record, dict) or record.get("status") not in GOOD_STATUSES or
            record.get("program_pass") is not True):
        raise ValueError("Spike validation does not record a completed passing program")
    log = artifact.with_name(artifact.name[:-len(ARTIFACT_SUFFIX)] + ".log").resolve()
    require_file(log)
    if recorded_path(record.get("log"), repo, run) != log:
        raise ValueError("Spike validation points at a different log")
    assembly = recorded_path(record.get("assembly"), repo, run)
    elf = recorded_path(record.get("elf"), repo, run)
    if not contained(elf, run) or elf.stem != log.stem or assembly.stem != elf.stem:
        raise ValueError("ELF/assembly/Spike paths do not belong to the same run test")
    require_file(assembly)
    require_elf(elf)
    lines = log.read_text(encoding="utf-8", errors="replace").splitlines()
    if any("***FAILED***" in line or "*** FAILED ***" in line for line in lines):
        raise ValueError("Spike log reports program failure")
    cases, basis = completed_cases(record, assembly, lines)
    if not cases:
        raise ValueError("No hypertest injection actually completed")
    files = [artifact, log, assembly, elf]
    synthetic_only = set(cases).issubset(SYNTHETIC_CASE_SOURCES)
    if synthetic_only:
        stage = "gcc_spike_pass"
    else:
        if not contained(assembly, run / "asm_test"):
            raise ValueError("Migrated case does not use a generated assembly in this run")
        files.append(uvm_evidence(run, assembly))
        stage = "gen_gcc_spike_pass"
    details = evidence(files, repo)
    details.update({"run": relative(run, repo), "validation": stage,
                    "basis": basis, "case_hits": dict(cases)})
    return cases, details


def legacy_supplement(run, repo, handled_logs):
    """Record ordinary DV regression checks separately from migrated cases."""
    results = []
    for log in sorted((run / "spike_sim").glob("*.log")):
        if log.resolve() in handled_logs:
            continue
        assembly = run / "asm_test" / (log.stem + ".S")
        elf = assembly.with_suffix(".o")
        try:
            require_file(assembly)
            text = assembly.read_text(encoding="utf-8")
            if "HYPERTEST_CASE:" in text or "HYPERTEST_INJECTION:" in text:
                continue
            require_elf(elf)
            generator = uvm_evidence(run, assembly)
            lines = require_file(log).read_text(encoding="utf-8", errors="replace").splitlines()
            if any("***FAILED***" in line or "*** FAILED ***" in line for line in lines):
                continue
            # Spike symbol traces identify the normal DV completion routine.
            # An arbitrary store of 1 elsewhere is not a pass indication.
            pending = False
            completed = False
            illegal_count = 0
            for index, line in enumerate(lines):
                if "exception trap_illegal_instruction" in line:
                    illegal_count += 1
                    following = lines[index + 1] if index + 1 < len(lines) else ""
                    match = re.search(r"\btval\s+0x([0-9a-fA-F]+)\b", following)
                    if illegal_count > 1 or not match or int(match.group(1), 16) != 0x5006b:
                        raise ValueError("Unexpected legacy illegal-instruction trap")
                if ">>>>" in line:
                    pending = bool(re.search(r">>>>\s+write_tohost\s*$", line))
                if pending:
                    match = re.search(r"\bmem\s+0x[0-9a-fA-F]+\s+0x([0-9a-fA-F]+)\b", line)
                    if match:
                        if int(match.group(1), 16) != 1:
                            raise ValueError("Legacy program wrote failing completion status")
                        completed = True
                        pending = False
            if not completed:
                continue
            details = evidence([assembly, elf, generator, log], repo)
            details.update({"validation": "gen_gcc_spike_pass",
                            "test": log.stem, "run": relative(run, repo),
                            "basis": "normal_DV_write_tohost_execution",
                            "counts_as_migrated_case": False})
            results.append(details)
        except (OSError, ValueError):
            continue
    return results


def collect(run_dirs, repo=REPO_ROOT, manifest=None):
    repo = Path(repo).resolve()
    manifest = Path(manifest) if manifest else repo / "src/hypertest/manifest.json"
    catalog = json.loads(manifest.read_text(encoding="utf-8"))
    known = {item["name"] for item in catalog["cases"]}
    runs = sorted({Path(run).resolve() for run in run_dirs})
    result = {"schema_version": 1, "cases": {},
              "source_snapshot_sha256": catalog.get("source", {}).get("snapshot_sha256"),
              "evidence_path_base": "riscv-dv repository",
              "input_directories": [relative(run, repo) for run in runs],
              "supplementary_checks": {"bridge_selftest": [], "legacy": []},
              "rejected_artifacts": [],
              "claims": "Completed generator/GNU/Spike selfchecks only; no RTL, "
                        "external stimulus, or unexecuted-case coverage claim."}
    for run in runs:
        if not run.is_dir():
            raise ValueError("Run output directory does not exist: {}".format(run))
        handled = set()
        for artifact in sorted((run / "spike_sim").glob("*" + ARTIFACT_SUFFIX)):
            handled.add(artifact.with_name(artifact.name[:-len(ARTIFACT_SUFFIX)] + ".log").resolve())
            try:
                cases, details = inspect_artifact(artifact, run, repo)
                unknown = set(cases) - known - set(SYNTHETIC_CASE_SOURCES)
                if unknown:
                    raise ValueError("Unknown original case(s): " + ", ".join(sorted(unknown)))
                for name, count in cases.items():
                    if name in SYNTHETIC_CASE_SOURCES:
                        supplemental = dict(details)
                        supplemental["counts_as_migrated_case"] = False
                        result["supplementary_checks"]["bridge_selftest"].append(supplemental)
                        continue
                    item = result["cases"].setdefault(name, {
                        "validation": "gen_gcc_spike_pass", "evidence": [],
                        "evidence_sha256": {}, "runs": [], "completed_count": 0})
                    item["evidence"] = sorted(set(item["evidence"]) | set(details["evidence"]))
                    item["evidence_sha256"].update(details["evidence_sha256"])
                    item["runs"].append(details)
                    item["completed_count"] += count
            except (OSError, ValueError, TypeError, KeyError, RuntimeError) as error:
                result["rejected_artifacts"].append({"artifact": relative(artifact, repo),
                                                       "reason": str(error)})
        result["supplementary_checks"]["legacy"].extend(legacy_supplement(run, repo, handled))
    result["counts"] = {"validated_cases": len(result["cases"]),
                        "catalog_cases": len(known),
                        "rejected_artifacts": len(result["rejected_artifacts"]),
                        "bridge_selftests": len(result["supplementary_checks"]["bridge_selftest"]),
                        "legacy_checks": len(result["supplementary_checks"]["legacy"])}
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dirs", nargs="+", type=Path,
                        help="existing run.py output directories")
    parser.add_argument("--output", required=True, type=Path,
                        help="importer-compatible validation JSON to write")
    parser.add_argument("--repo", type=Path, default=REPO_ROOT,
                        help="repository used as evidence path base")
    parser.add_argument("--manifest", type=Path,
                        help="catalog manifest (defaults to repo/src/hypertest/manifest.json)")
    args = parser.parse_args(argv)
    try:
        data = collect(args.run_dirs, args.repo, args.manifest)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n",
                               encoding="utf-8")
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, "error: {}\n".format(error))
    print(json.dumps(data["counts"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
