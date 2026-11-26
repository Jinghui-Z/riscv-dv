#!/usr/bin/env python3
"""Diagnose GNU object compilation and optional per-case ELF linking.

The default stops at objects; --link adds minimal-fixture per-case closure links.
Neither mode runs an ELF or proves assertions, Spike or RTL success.
Each source gets its command, object, compiler logs and defined-function nm log.
"""

import argparse
import concurrent.futures
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from types import SimpleNamespace

import hypertest_adapter as adapter
from import_hypertest import CATALOG_GCC_ISA


REPO_ROOT = Path(__file__).resolve().parents[1]
LINK_FIXTURE = """.section .text.dv_hyp_link_fixture,"ax",@progbits
.option push
.option norelax
.globl _start
.type _start,@function
_start:
    la sp, dv_hypertest_stack_top
    la a0, dv_hypertest_selected_case
    call dv_hypertest_call
1:  j 1b
.size _start, .-_start
.option pop
.section .tohost,"aw",@progbits
.balign 64
.globl tohost
tohost: .dword 0
.balign 64
.globl fromhost
fromhost: .dword 0
.section .note.GNU-stack,"",@progbits
"""


def digest_file(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n",
                    encoding="utf-8")


def run_capture(command, timeout):
    try:
        process = subprocess.run(command, text=True, capture_output=True,
                                 timeout=timeout, check=False)
        return process.returncode, process.stdout, process.stderr
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout or ""
        stderr = error.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode("utf-8", errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        return -1, stdout, stderr + f"\nCompiler timeout after {timeout}s\n"
    except OSError as error:
        return -1, "", str(error) + "\n"


def function_symbols(nm_text):
    functions = []
    for line in nm_text.splitlines():
        # GNU nm -P: name type value [size]. Include local/static text symbols;
        # their visibility is recorded and does not establish linker reachability.
        match = re.match(r"^(\S+)\s+([TtWw])\s+([0-9a-fA-F]+)(?:\s+(\S+))?\s*$", line)
        if match:
            functions.append({"name": match.group(1), "type": match.group(2),
                              "value": match.group(3),
                              "size": match.group(4),
                              "visibility": "global" if match.group(2).isupper()
                              else "local"})
    return functions


def compile_source(job, common, nm_command, output, timeout):
    source = job["compiled_source"]
    key = job["source_key"]
    object_path = output / "objects" / (key + ".o")
    log_stem = output / "logs" / key
    object_path.parent.mkdir(parents=True, exist_ok=True)
    log_stem.parent.mkdir(parents=True, exist_ok=True)
    stdout_path = Path(str(log_stem) + ".stdout.log")
    stderr_path = Path(str(log_stem) + ".stderr.log")
    nm_stdout = Path(str(log_stem) + ".nm.stdout.log")
    nm_stderr = Path(str(log_stem) + ".nm.stderr.log")
    command = common + job.get("source_flags", []) + ["-c", str(source),
                                                       "-o", str(object_path)]
    start = time.monotonic()
    returncode, stdout, stderr = run_capture(command, timeout)
    stdout_path.write_text(stdout, encoding="utf-8")
    stderr_path.write_text(stderr, encoding="utf-8")
    result = {
        "source_key": key, "group": job["group"],
        "source": str(job["original_source"]),
        "compiled_source": str(source), "source_sha256": digest_file(source),
        "object": str(object_path), "command": command,
        "returncode": returncode, "stdout": str(stdout_path),
        "stderr": str(stderr_path), "elapsed_seconds": round(time.monotonic() - start, 3),
        "status": "compiled" if returncode == 0 and object_path.is_file() else "failed",
        "defined_functions": [],
        "adaptations": job.get("adaptations", []),
    }
    if result["status"] == "compiled":
        command = nm_command + ["-P", "--defined-only", str(object_path)]
        code, stdout, stderr = run_capture(command, timeout)
        nm_stdout.write_text(stdout, encoding="utf-8")
        nm_stderr.write_text(stderr, encoding="utf-8")
        result["nm"] = {"command": command, "returncode": code,
                        "stdout": str(nm_stdout), "stderr": str(nm_stderr)}
        if code == 0:
            result["defined_functions"] = function_symbols(stdout)
        else:
            result["status"] = "nm_failed"
    else:
        errors = [line for line in stderr.splitlines()
                  if "error:" in line.lower() or "Error:" in line]
        result["first_errors"] = errors[:5] or stderr.splitlines()[-5:]
        if key == "ai_test_cases/backend/boundary/flash/boot_boundary.S" and (
                "POINT must name one boundary test" in stderr):
            result["status"] = "external_payload_requires_config"
            result["configuration_note"] = (
                "Standalone flash boot payload requires POINT and its dedicated runner; "
                "this source does not define any catalog bool case."
            )
    write_json(Path(str(log_stem) + ".json"), result)
    return result


def prepare_jobs(repo, output):
    upstream = repo / "src/hypertest/upstream"
    generated = output / "generated"
    includes = generated / "include"
    includes.mkdir(parents=True)
    (includes / "platform.h").write_text(adapter.PLATFORM_HEADER, encoding="utf-8")
    jobs = []
    for directory in ("ai_test_cases", "manual_test_cases", "src", "asm"):
        for path in sorted((upstream / directory).rglob("*")):
            if path.is_symlink() or path.suffix not in (".c", ".S"):
                continue
            key = path.relative_to(upstream).as_posix()
            compiled, flags, adaptations = adapter.prepare_source(path, upstream, generated)
            job = {"source_key": key, "original_source": path,
                   "compiled_source": compiled, "source_flags": flags,
                   "adaptations": adaptations,
                   "group": "case" if directory.endswith("test_cases") else "core"}
            jobs.append(job)
    for name in ("bridge.c", "bridge.S"):
        path = repo / "src/hypertest" / name
        compiled, flags, adaptations = adapter.prepare_source(path, upstream, generated)
        jobs.append({"source_key": "bridge/" + name, "original_source": path,
                     "compiled_source": compiled, "source_flags": flags,
                     "adaptations": adaptations, "group": "bridge"})
    write_json(generated / "preparation.json", {
        "platform_header": adapter.PLATFORM_HEADER, "snapshot_modified": False,
        "sources": [{"source_key": job["source_key"],
                     "source": str(job["original_source"]),
                     "source_sha256": digest_file(job["original_source"]),
                     "prepared_source": str(job["compiled_source"]),
                     "prepared_sha256": digest_file(job["compiled_source"]),
                     "source_flags": job["source_flags"],
                     "adaptations": job["adaptations"]} for job in jobs],
    })
    return jobs, includes


def make_case_results(manifest, source_results):
    sources = {row["source_key"]: row for row in source_results}
    rows = []
    for case in manifest["cases"]:
        matches = []
        for definition in case["definitions"]:
            source = sources.get(definition["path"])
            symbols = [] if source is None else [
                symbol for symbol in source["defined_functions"]
                if symbol["name"] == case["name"]
            ]
            matches.append({"definition": definition,
                            "source_status": source["status"] if source else "not_scheduled",
                            "object": source["object"] if source else None,
                            "symbols": symbols})
        if any(row["source_status"] == "compiled" and row["symbols"] for row in matches):
            status = "object_compiled"
        elif any(row["source_status"] == "failed" for row in matches):
            status = "source_compile_failed"
        elif any(row["source_status"] == "nm_failed" for row in matches):
            status = "nm_failed"
        elif matches and all(row["source_status"] == "compiled" for row in matches):
            status = "symbol_not_emitted"
        else:
            status = "source_not_scheduled"
        rows.append({"name": case["name"], "catalog_test": case.get("catalog_test"),
                     "status": status, "definitions": matches,
                     "elf_linked": False, "spike_run": False, "rtl_run": False})
    return rows


def link_case(job, compiler, nm_command, fixture, repo, output, isa, abi, timeout):
    name = job["name"]
    stem = job["catalog_test"]
    elf = output / "elf" / (stem + ".elf")
    logs = output / "link_logs" / stem
    elf.parent.mkdir(parents=True, exist_ok=True)
    logs.parent.mkdir(parents=True, exist_ok=True)
    stdout_path = Path(str(logs) + ".stdout.log")
    stderr_path = Path(str(logs) + ".stderr.log")
    record = {"name": name, "catalog_test": stem, "elf": str(elf),
              "objects": job.get("objects", []),
              "page_table_core": job.get("page_table_core"),
              "closure_sources": job.get("closure_sources", []),
              "stdout": str(stdout_path), "stderr": str(stderr_path),
              "spike_run": False, "rtl_run": False}
    if job.get("error"):
        record.update(status=job.get("error_status", "planning_failed"),
                      error=job["error"])
        stdout_path.write_text("", encoding="utf-8")
        stderr_path.write_text(job["error"] + "\n", encoding="utf-8")
        write_json(Path(str(logs) + ".json"), record)
        return record
    command = compiler + [
        "-march=" + isa, "-mabi=" + abi, "-mcmodel=medany", "-nostdlib",
        "-nostartfiles", "-static", "-Wl,--gc-sections", "-Wl,--no-undefined",
        "-Wl,--undefined=" + name,
        "-Wl,--defsym=dv_hypertest_selected_case=" + name,
        "-T", str(repo / "scripts/link.ld"), str(fixture),
    ] + job["objects"] + ["-lgcc", "-o", str(elf)]
    start = time.monotonic()
    code, stdout, stderr = run_capture(command, timeout)
    stdout_path.write_text(stdout, encoding="utf-8")
    stderr_path.write_text(stderr, encoding="utf-8")
    record.update(command=command, returncode=code,
                  elapsed_seconds=round(time.monotonic() - start, 3))
    if code == 0 and elf.is_file():
        nm_stdout = Path(str(logs) + ".nm.stdout.log")
        nm_stderr = Path(str(logs) + ".nm.stderr.log")
        nm_args = nm_command + ["-P", "--defined-only", str(elf)]
        code, stdout, stderr = run_capture(nm_args, timeout)
        nm_stdout.write_text(stdout, encoding="utf-8")
        nm_stderr.write_text(stderr, encoding="utf-8")
        defined = {row["name"]: row for row in function_symbols(stdout)}
        required = {name, "dv_hypertest_call", "dv_hypertest_run_case", "_start"}
        missing = sorted(required - set(defined))
        record["nm"] = {"command": nm_args, "returncode": code,
                        "stdout": str(nm_stdout), "stderr": str(nm_stderr)}
        record["case_symbol"] = defined.get(name)
        record["status"] = "elf_linked" if code == 0 and not missing else "symbol_check_failed"
        if missing:
            record["error"] = "Missing retained function symbols: " + ", ".join(missing)
        record["elf_sha256"] = digest_file(elf)
    else:
        record["status"] = "link_failed"
        record["first_errors"] = [line for line in stderr.splitlines()
                                  if "undefined reference" in line or "error:" in line
                                  or "multiple definition" in line][:8]
        if not record["first_errors"]:
            record["first_errors"] = stderr.splitlines()[-5:]
    write_json(Path(str(logs) + ".json"), record)
    return record


def run_links(record, manifest, repo, output, compiler, nm_command, args):
    generated = output / "generated"
    generated.mkdir(exist_ok=True)
    source = generated / "link_fixture.S"
    source.write_text(LINK_FIXTURE, encoding="utf-8")
    fixture = generated / "link_fixture.o"
    command = compiler + ["-march=" + args.isa, "-mabi=" + args.abi,
                          "-mcmodel=medany", "-c", str(source), "-o", str(fixture)]
    code, stdout, stderr = run_capture(command, args.timeout)
    (generated / "link_fixture.stdout.log").write_text(stdout, encoding="utf-8")
    (generated / "link_fixture.stderr.log").write_text(stderr, encoding="utf-8")
    if code or not fixture.is_file():
        raise RuntimeError("Cannot compile diagnostic link fixture: " + stderr)
    record["link_fixture"] = {"source": str(source), "source_sha256": digest_file(source),
                              "object": str(fixture), "command": command}
    upstream = repo / "src/hypertest/upstream"
    print("Planning complete per-case source/assembly closures...", flush=True)
    planner = adapter._load_planner(upstream)
    plans, errors = planner.build_case_source_plan(
        [SimpleNamespace(name=item["name"]) for item in manifest["cases"]])
    sources = {row["source_key"]: row for row in record["source_results"]}
    page_cores = {}
    jobs = []
    base = {"src/rvh_test.c", "src/instruction.c", "asm/handlers.S",
            "bridge/bridge.c", "bridge/bridge.S"}
    for case in manifest["cases"]:
        job = {"name": case["name"], "catalog_test": case["catalog_test"]}
        if case["name"] in errors:
            job.update(error=errors[case["name"]], error_status="planning_failed")
            jobs.append(job)
            continue
        plan = plans[case["name"]]
        key = tuple(sorted(str(path) for path in plan.c_sources))
        try:
            if key not in page_cores:
                page_cores[key] = adapter._page_table_core(upstream, plan.c_sources)
            page = page_cores[key]
            closure = set(base)
            closure.add(page.relative_to(upstream).as_posix())
            closure.update(path.relative_to(upstream).as_posix()
                           for path in (*plan.c_sources, *plan.asm_sources))
            job["closure_sources"] = sorted(closure)
            job["page_table_core"] = str(page)
            incomplete = [name for name in closure
                          if name not in sources or sources[name]["status"] != "compiled"]
            if incomplete:
                requires_config = any(sources.get(name, {}).get("status") ==
                                      "external_payload_requires_config" for name in incomplete)
                job.update(error="Closure object unavailable: " + ", ".join(sorted(incomplete)),
                           error_status="requires_config" if requires_config else "object_unavailable")
            else:
                job["objects"] = [sources[name]["object"] for name in sorted(closure)]
        except (OSError, ValueError) as error:
            job.update(error=str(error), error_status="planning_failed")
        jobs.append(job)
    record["elf_link_results"] = []
    started = time.monotonic()
    last_progress = started
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(link_case, job, compiler, nm_command, fixture, repo,
                                   output, args.isa, args.abi, args.timeout) for job in jobs]
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            record["elf_link_results"].append(result)
            count = len(record["elf_link_results"])
            now = time.monotonic()
            if count % 100 == 0 or now - last_progress > 15 or count == len(jobs):
                failed = sum(row["status"] != "elf_linked" for row in record["elf_link_results"])
                print(f"Link progress {count}/{len(jobs)}, nonlinked={failed}, "
                      f"elapsed={now - started:.1f}s", flush=True)
                last_progress = now
    record["elf_link_results"].sort(key=lambda row: row["name"])
    record["counts"]["elf_link_status"] = dict(Counter(
        row["status"] for row in record["elf_link_results"]))
    record["link_elapsed_seconds"] = round(time.monotonic() - started, 3)
    record["limits"] = [
        "per-case ELF closure linked with a minimal diagnostic fixture; not a random generated program",
        "no Spike execution", "no assertion result", "no RTL execution",
        "link success does not establish target extension or platform support",
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=REPO_ROOT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--gcc", default=os.environ.get("RISCV_GCC") or
                        shutil.which("riscv64-unknown-elf-gcc"))
    parser.add_argument("--nm", default=os.environ.get("RISCV_NM"))
    parser.add_argument("--jobs", type=int, default=8, choices=range(1, 9))
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--isa", default=CATALOG_GCC_ISA)
    parser.add_argument("--abi", default="lp64d")
    parser.add_argument("--link", action="store_true",
                        help="also link every case closure against a minimal diagnostic fixture")
    parser.add_argument("--reuse-audit", type=Path,
                        help="reuse objects and preparation from a complete earlier object audit")
    args = parser.parse_args()
    if not args.gcc:
        parser.error("Set RISCV_GCC or --gcc")
    if args.timeout < 1:
        parser.error("--timeout must be positive")
    repo = args.repo.resolve()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    output = (args.output or repo / "out" /
              f"hypertest_catalog_compile_{stamp}_{os.getpid()}").absolute()
    output.mkdir(parents=True, exist_ok=False)
    manifest_path = repo / "src/hypertest/manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    compiler = adapter._compiler_command(args.gcc)
    nm = args.nm
    if not nm and compiler[-1].endswith("gcc"):
        candidate = compiler[-1][:-3] + "nm"
        if shutil.which(candidate):
            nm = candidate
    nm = nm or shutil.which("riscv64-unknown-elf-nm") or shutil.which("nm")
    if not nm:
        parser.error("Set RISCV_NM or --nm")
    if args.reuse_audit:
        old = json.loads(args.reuse_audit.read_text(encoding="utf-8"))
        if old.get("source_snapshot_sha256") != manifest["source"]["snapshot_sha256"]:
            parser.error("Reused object audit belongs to a different source snapshot")
        if old.get("isa") != args.isa or old.get("abi") != args.abi or old.get("compiler") != compiler:
            parser.error("Reused audit compiler/ISA/ABI differs from this invocation")
        if {row["name"] for row in old["case_results"]} != {row["name"] for row in manifest["cases"]}:
            parser.error("Reused object audit has a different case catalog")
        for row in old["source_results"]:
            if row["status"] == "compiled" and not Path(row["object"]).is_file():
                parser.error("Missing reused object: " + row["object"])
            if digest_file(Path(row["compiled_source"])) != row["source_sha256"]:
                parser.error("Prepared source changed since object compilation: " + row["compiled_source"])
        record = old
        record["output"] = str(output)
        record["object_audit_reused_from"] = {"path": str(args.reuse_audit),
                                             "sha256": digest_file(args.reuse_audit)}
        generated = output / "generated"
        generated.mkdir()
        shutil.copy2(args.reuse_audit.parent / "generated/preparation.json",
                     generated / "preparation.json")
        print(f"Reusing {len(old['source_results'])} source objects; output={output}", flush=True)
        if args.link:
            run_links(record, manifest, repo, output, compiler,
                      adapter._compiler_command(nm), args)
        write_json(output / "audit.json", record)
        print(json.dumps({"counts": record["counts"], "audit": str(output / "audit.json")},
                         indent=2), flush=True)
        return 0
    jobs, include = prepare_jobs(repo, output)
    upstream = repo / "src/hypertest/upstream"
    common = compiler + [
        "-march=" + args.isa, "-mabi=" + args.abi, "-mcmodel=medany", "-O2",
        "-ffunction-sections", "-fdata-sections", "-fno-tree-vectorize",
        "-fno-builtin", "-ffreestanding", "-Werror=return-type",
        "-msmall-data-limit=0", "-DLOG_LEVEL=0",
        "-I" + str(include), "-I" + str(upstream / "inc"),
        "-I" + str(upstream / "platform/spike/inc"), "-I" + str(repo / "src/hypertest"),
    ]
    _, version, version_error = run_capture(compiler + ["--version"], args.timeout)
    record = {
        "schema_version": 1, "diagnostic_only": True, "output": str(output),
        "source_manifest": str(manifest_path), "manifest_sha256": digest_file(manifest_path),
        "source_snapshot_sha256": manifest["source"]["snapshot_sha256"],
        "isa": args.isa, "abi": args.abi, "compiler": compiler,
        "compiler_version": version, "compiler_version_stderr": version_error,
        "common_flags": common, "parallel_jobs": args.jobs,
        "limits": ["objects only; no ELF link", "no Spike execution",
                   "no assertion result", "no RTL execution",
                   "local function symbols do not prove global linker reachability"],
        "source_results": [], "case_results": [],
    }
    write_json(output / "audit.json", record)
    print(f"Object-only audit: {len(jobs)} sources; output={output}", flush=True)
    started = time.monotonic()
    last_progress = started
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        pending = {executor.submit(compile_source, job, common,
                                   adapter._compiler_command(nm), output, args.timeout): job
                   for job in jobs}
        for future in concurrent.futures.as_completed(pending):
            result = future.result()
            record["source_results"].append(result)
            count = len(record["source_results"])
            now = time.monotonic()
            if count % 25 == 0 or now - last_progress > 15 or count == len(jobs):
                failed = sum(row["status"] != "compiled" for row in record["source_results"])
                print(f"Progress {count}/{len(jobs)}, failed={failed}, "
                      f"elapsed={now - started:.1f}s", flush=True)
                last_progress = now
    record["source_results"].sort(key=lambda row: row["source_key"])
    record["case_results"] = make_case_results(manifest, record["source_results"])
    source_counts = dict(Counter(
        row["status"] for row in record["source_results"]))
    case_counts = dict(Counter(
        row["status"] for row in record["case_results"]))
    record["counts"] = {"sources_total": len(jobs), "source_status": source_counts,
                        "cases_total": len(record["case_results"]),
                        "case_status": case_counts}
    record["elapsed_seconds"] = round(time.monotonic() - started, 3)
    record["status"] = "completed_with_compile_failures" if any(
        row["status"] != "compiled" for row in record["source_results"]) else "completed"
    if args.link:
        run_links(record, manifest, repo, output, compiler,
                  adapter._compiler_command(nm), args)
    write_json(output / "audit.json", record)
    print(json.dumps({"status": record["status"], "counts": record["counts"],
                      "audit": str(output / "audit.json")}, indent=2), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
