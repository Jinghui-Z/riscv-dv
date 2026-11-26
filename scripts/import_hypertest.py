#!/usr/bin/env python3
"""Snapshot hypertest sources and inventory every existing test without running it.

The register parser follows compile_elf.py, including commented registrations.
The source parser additionally finds unregistered bool functions containing
TEST_START(). Risk hints are review prompts, never automatic platform eligibility.
"""

import argparse
import collections
import csv
import hashlib
import io
import json
import re
import shutil
import stat
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT_DIRS = (
    "ai_test_cases", "manual_test_cases", "inc", "src", "asm", "platform/spike"
)
SNAPSHOT_FILES = ("LICENSE", "compile_elf.py", "test_register.c")
GENERATED_DIR_NAMES = {"build", "out", "results", ".git", "__pycache__",
                       ".tmp", "case_elf_asm"}
REGISTER_RE = re.compile(
    r"^(?P<indent>\s*)(?://\s*)?TEST_REGISTER\((?P<name>[^)]+)\)\s*;?\s*(?://.*)?$"
)
ACTIVE_REGISTER_RE = re.compile(r"^\s*TEST_REGISTER\(")
BOOL_DEF_RE = re.compile(
    r"^[ \t]*(?:(?:static|inline|extern|__inline__|__inline)\s+)*"
    r"bool\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\(", re.MULTILINE
)
NON_CODE_RE = re.compile(
    r"/\*.*?\*/|//[^\n]*|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'",
    re.DOTALL,
)
COMMENT_RE = re.compile(
    r"(?P<literal>\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*')"
    r"|(?P<comment>/\*.*?\*/|//[^\n]*)", re.DOTALL
)
NATIVE_CLASSES = {
    "ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_corner":
        "riscv_hypertest_sv48_canonical_stream",
    "ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_x_deny_corner":
        "riscv_hypertest_sv48_canonical_x_deny_stream",
}
CATALOG_GCC_ISA = (
    "rv64imafdcv_zba_zbb_zbc_zbs_zbkb_zbkc_zbkx_zcb_zicond_zimop_zcmop_"
    "zicbom_zicbop_zicboz_zknd_zkne_zknh_zksed_zksh_zvbb_zicsr_zifencei"
)


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def mask_non_code(text):
    """Keep offsets and newlines intact for definition locations and braces."""
    return NON_CODE_RE.sub(
        lambda match: "".join("\n" if char == "\n" else " "
                              for char in match.group()), text
    )


def strip_comments(text):
    return COMMENT_RE.sub(
        lambda match: match.group("literal") if match.group("literal")
        else "".join("\n" if char == "\n" else " "
                     for char in match.group()), text
    )


def git_read(source, *arguments):
    try:
        result = subprocess.run(
            ["git", "-C", str(source), *arguments], capture_output=True,
            text=True, check=False, timeout=30
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    return result.stdout.rstrip("\n") if result.returncode == 0 else None


def snapshot_inventory(source):
    files = []
    skipped_symlinks = []
    missing_paths = []
    candidates = []
    for relative in SNAPSHOT_DIRS + SNAPSHOT_FILES:
        path = source / relative
        if path.is_symlink():
            skipped_symlinks.append(relative)
        elif path.is_dir():
            # Path.rglob does not follow directory symlinks. Check every result
            # as well so a file symlink can never escape this source snapshot.
            for child in path.rglob("*"):
                if any(part in GENERATED_DIR_NAMES
                       for part in child.relative_to(source).parts):
                    continue
                if child.is_symlink():
                    skipped_symlinks.append(child.relative_to(source).as_posix())
                elif child.is_file():
                    candidates.append(child)
        elif path.is_file():
            candidates.append(path)
        else:
            missing_paths.append(relative)
    for path in sorted(set(candidates)):
        metadata = path.stat()
        files.append({
            "path": path.relative_to(source).as_posix(),
            "sha256": sha256_file(path),
            "size": metadata.st_size,
            "mode": format(stat.S_IMODE(metadata.st_mode), "04o"),
        })
    digest = hashlib.sha256(
        json.dumps(files, sort_keys=True, separators=(",", ":")).encode("utf-8")
    ).hexdigest()
    return files, sorted(skipped_symlinks), missing_paths, digest


def registration_inventory(source):
    registrations = collections.defaultdict(list)
    text = (source / "test_register.c").read_text(encoding="utf-8")
    # Keep // registrations as disabled catalog entries, but do not treat code
    # inside a C block comment or string as a registration. Preserve line/offset
    # positions so metadata still points at the original source, including when
    # a real registration has an inline /* explanation */.
    masked = NON_CODE_RE.sub(
        lambda match: match.group() if match.group().startswith("//") else
        "".join("\n" if char == "\n" else " " for char in match.group()), text
    )
    original_lines = text.splitlines()
    for line_number, line in enumerate(masked.splitlines(), 1):
        match = REGISTER_RE.match(line)
        if not match:
            continue
        registrations[match.group("name").strip()].append({
            "path": "test_register.c",
            "line": line_number,
            "active": bool(ACTIVE_REGISTER_RE.match(line)),
            "text": original_lines[line_number - 1].strip(),
        })
    return registrations


def function_inventory(source):
    """Locate bool definitions, excluding prototypes and commented-out code."""
    definitions = collections.defaultdict(list)
    bodies = {}
    for root in ("ai_test_cases", "manual_test_cases"):
        directory = source / root
        if not directory.is_dir() or directory.is_symlink():
            continue
        for path in sorted(directory.rglob("*.c")):
            if path.is_symlink():
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            masked = mask_non_code(text)
            # Brace pairs avoid repeatedly scanning the tail of very large
            # source files for each of their many test definitions.
            stack = []
            brace_end = {}
            for position, char in enumerate(masked):
                if char == "{":
                    stack.append(position)
                elif char == "}" and stack:
                    brace_end[stack.pop()] = position
            for match in BOOL_DEF_RE.finditer(masked):
                depth = 1
                position = match.end()
                while position < len(masked) and depth:
                    char = masked[position]
                    depth += (char == "(") - (char == ")")
                    position += 1
                if depth:
                    continue
                # After the closing signature, a semicolon is a declaration;
                # optional GCC attributes may precede the opening body brace.
                opening = masked.find("{", position)
                semicolon = masked.find(";", position)
                if opening < 0 or (0 <= semicolon < opening):
                    continue
                closing = brace_end.get(opening)
                if closing is None:
                    continue
                name = match.group("name")
                relative = path.relative_to(source).as_posix()
                line = text.count("\n", 0, match.start()) + 1
                body = text[opening:closing + 1]
                contains_start = bool(re.search(
                    r"\bTEST_START\s*\(", masked[opening:closing + 1]
                ))
                record = {
                    "name": name, "path": relative, "line": line,
                    "has_test_start": contains_start,
                    "static": bool(re.search(r"\bstatic\b", match.group())),
                }
                definitions[name].append(record)
                bodies[(relative, line)] = body
    return definitions, bodies


def review_hints(name, definitions, registrations, bodies):
    """Conservative lexical hints; do not assert that any case is runnable."""
    hints = []
    body = "\n".join(
        bodies.get((item["path"], item["line"]), "") for item in definitions
    )
    code = strip_comments(body)
    # "manual_test_cases" identifies authorship, not a manual/nongate execution
    # tier. Only the remaining path, case name, notes and code provide hints.
    source_paths = " ".join("/".join(item["path"].split("/")[1:])
                            for item in definitions)
    register_notes = " ".join(item["text"] for item in registrations)
    candidates = (
        ("possible_h_extension", r"\bCSR_(?:H(?:STATUS|EDELEG|IDELEG|IE|TIMEDELTAH?|"
         r"COUNTEREN|GEIE|GEIP|ENVCFGH?|STATEEN[0-3]H?|VICTL|VIP|IP|TVAL|TINST|"
         r"GATP|CONTEXT)|VS(?:STATUS|IE|TVEC|SCRATCH|EPC|CAUSE|TVAL|ATP|IP))\b|"
         r"\bPRIV_V[SU]\b|\b(?:hfence|hlv|hlvx|hsv)(?:\b|[._])", code),
        ("manual_or_external_stimulus", r"manual|nongate|vmodule|inject|"
         r"bus.?corrupt|debug.?agent|external|no.?response",
         name + " " + source_paths + " " + register_notes + " " + code),
        ("reset_or_terminal_behavior", r"\b(?:exit|abort)\s*\(|"
         r"\b(?:warm_reset|cold_reset|system_reset|request_reset)\b|"
         r"while\s*\(\s*1\s*\)|for\s*\(\s*;\s*;\s*\)", code),
        ("possible_program_code_write", r"self.?mod|fence\.i|"
         r"(?:write(?:16|32|64)|memcpy)\s*\([^;\n]*(?:code|instr|text|exec)", code),
        ("pma_pbmt_or_device_semantics", r"\bPMA[A-Z0-9_]*\b|"
         r"\bCSR_PMA[A-Z0-9_]*\b|PBMT|[Pp]bmt|[Mm][Mm][Ii][Oo]", code),
    )
    for kind, pattern, haystack in candidates:
        match = re.search(pattern, haystack, re.IGNORECASE
                          if kind == "manual_or_external_stimulus" else 0)
        if match:
            hints.append({"kind": kind, "evidence": match.group()[:120],
                          "classification": "needs_review"})
    if not definitions:
        hints.append({"kind": "missing_definition", "evidence": name,
                      "classification": "needs_review"})
    if len(definitions) > 1:
        hints.append({"kind": "multiple_definitions", "evidence": name,
                      "classification": "needs_review"})
    return hints


def make_inventory(source):
    files, symlinks, missing, tree_hash = snapshot_inventory(source)
    registrations = registration_inventory(source)
    definitions, bodies = function_inventory(source)
    case_names = set(registrations)
    case_names.update(name for name, items in definitions.items()
                      if any(item["has_test_start"] for item in items))
    cases = []
    for name in sorted(case_names):
        locations = definitions.get(name, [])
        registered = registrations.get(name, [])
        native = NATIVE_CLASSES.get(name)
        source_status = "imported_unverified" if locations else "unresolved_definition"
        paths = sorted({item["path"] for item in locations})
        first_path = paths[0] if paths else "<missing>"
        module = "/".join(first_path.split("/")[:2])
        cases.append({
            "name": name,
            "catalog_test": catalog_test_name(name),
            "module": module,
            "definitions": locations,
            "registered": bool(registered),
            "default_active": any(item["active"] for item in registered),
            "default_random_eligible": False,
            "restrict_store_reviewed": False,
            "registration_count": len(registered),
            "registration_duplicate_count": max(0, len(registered) - 1),
            "registrations": registered,
            "migration_path": "native" if native else "source_adapter",
            "migration_status": "implemented_unverified" if native else source_status,
            "source_adapter": {"status": source_status,
                               "sources": paths},
            "native_class": native,
            "validation": "not_run",
            "review_hints": review_hints(name, locations, registered, bodies),
        })
    helpers = [item for name, items in sorted(definitions.items())
               if name not in case_names for item in items]
    active = {name for name, rows in registrations.items()
              if any(row["active"] for row in rows)}
    counts = {
        "registration_lines": sum(len(rows) for rows in registrations.values()),
        "registered_unique_cases": len(registrations),
        "default_active_unique_cases": len(active),
        "commented_only_registered_cases": len(set(registrations) - active),
        "duplicate_registration_names": sum(len(rows) > 1
                                            for rows in registrations.values()),
        "duplicate_registration_lines": sum(max(0, len(rows) - 1)
                                            for rows in registrations.values()),
        "unregistered_test_start_cases": len(case_names - set(registrations)),
        "catalog_cases": len(cases),
        "native_cases": sum(bool(item["native_class"]) for item in cases),
        "source_adapter_cases": len(cases),
        "non_case_bool_helper_definitions": len(helpers),
        "missing_definition_cases": sum(not item["definitions"] for item in cases),
        "multiple_definition_cases": sum(len(item["definitions"]) > 1 for item in cases),
        "cases_with_review_hints": sum(bool(item["review_hints"]) for item in cases),
        "source_files": len(files),
        "source_bytes": sum(item["size"] for item in files),
    }
    return {
        "schema_version": 1,
        "scope": "all registered cases including commented registrations, plus "
                 "unregistered bool definitions containing TEST_START",
        "claims": {
            "sources": "verbatim snapshot; every catalog case has a source-adapter entry",
            "execution": "not_run unless an explicit external validation record is merged; "
                         "imported source is not proof of build, Spike, or RTL execution",
            "review_hints": "lexical review prompts only; not hardware eligibility decisions",
        },
        "source": {
            "path": str(source),
            "git_revision": git_read(source, "rev-parse", "HEAD"),
            "git_status_short": (git_read(source, "status", "--short") or "").splitlines(),
            "snapshot_sha256": tree_hash,
            "snapshot_hash_algorithm": "SHA256 of sorted compact JSON file records "
                                       "(path,sha256,size,mode), UTF-8",
            "included_roots": list(SNAPSHOT_DIRS + SNAPSHOT_FILES),
            "excluded_symlinks": symlinks,
            "missing_snapshot_paths": missing,
            "files": files,
        },
        "counts": counts,
        "cases": cases,
        "non_case_bool_helpers": helpers,
    }


def catalog_test_name(name):
    # The original repository includes case names longer than NAME_MAX.
    # Preserve the full name in metadata/plusargs, but keep runner paths short.
    slug = re.sub(r"[^A-Za-z0-9_]", "_", name)[:80]
    suffix = hashlib.sha256(name.encode("utf-8")).hexdigest()[:12]
    return f"hypertest_case_{slug}_{suffix}"


def merge_validation(inventory, path):
    if not path.is_file():
        return
    data = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(data, dict) and "source_snapshot_sha256" in data:
        if data["source_snapshot_sha256"] != inventory["source"]["snapshot_sha256"]:
            raise ValueError("Validation records belong to a different source snapshot")
    if isinstance(data, dict) and "cases" in data and "supplementary_checks" in data:
        supplementary = data["supplementary_checks"]
        if not isinstance(supplementary, dict) or any(
                not isinstance(rows, list) or any(not isinstance(row, dict) for row in rows)
                for rows in supplementary.values()):
            raise ValueError("supplementary_checks must map categories to lists of records")
    records = data.get("cases", data) if isinstance(data, dict) else data
    if isinstance(records, list):
        rows = records
        records = {row["name"]: row for row in rows}
        if len(records) != len(rows):
            raise ValueError("Validation records contain duplicate case names")
    if not isinstance(records, dict):
        raise ValueError("validation JSON must map case names to records or contain a cases list")
    index = {item["name"]: item for item in inventory["cases"]}
    unknown = sorted(set(records) - set(index))
    if unknown:
        raise ValueError("validation JSON contains unknown case names: " + ", ".join(unknown[:5]))
    normalized = {}
    for name, record in records.items():
        if isinstance(record, str):
            record = {"validation": record}
        if not isinstance(record, dict):
            raise ValueError(f"Invalid validation record for {name}")
        validation = record.get("validation", record.get("status"))
        if not isinstance(validation, str) or not validation:
            raise ValueError(f"Missing validation/status string for {name}")
        normalized[name] = (validation, record)
    for name, (validation, record) in normalized.items():
        index[name]["validation"] = validation
        index[name]["validation_evidence"] = record.get("evidence", [])
        index[name]["validation_evidence_sha256"] = record.get("evidence_sha256", {})
        # Keep the complete collector record: runtime hits, completion counts,
        # per-run details, evidence hashes and future fields must remain auditable.
        index[name]["validation_record"] = record
        if validation == "gen_gcc_spike_pass":
            index[name]["migration_status"] = (
                "implemented_gcc_spike_verified" if index[name]["native_class"] else
                "source_adapter_gcc_spike_verified")
    inventory["validation_source"] = {"path": str(path), "sha256": sha256_file(path)}
    if isinstance(data, dict) and "cases" in data:
        inventory["validation_metadata"] = {key: value for key, value in data.items()
                                            if key != "cases"}
        if "supplementary_checks" in data:
            inventory["supplementary_checks"] = data["supplementary_checks"]


def merge_reviewed_pools(inventory, default_random_path, restricted_path):
    """Attach reviewed eligibility only from the explicit runner allowlists."""
    cases = {item["name"]: item for item in inventory["cases"]}
    provenance = {}
    pools = {}
    for field, path in (("default_random_eligible", default_random_path),
                        ("restrict_store_reviewed", restricted_path)):
        names = [line.split("#", 1)[0].strip()
                 for line in path.read_text(encoding="utf-8").splitlines()]
        names = [name for name in names if name]
        if len(names) != len(set(names)):
            raise ValueError(f"Duplicate case names in reviewed allowlist: {path}")
        unknown = sorted(set(names) - set(cases))
        if unknown:
            raise ValueError(f"Reviewed allowlist {path} contains unknown cases: " +
                             ", ".join(unknown[:5]))
        pools[field] = set(names)
        for item in inventory["cases"]:
            item[field] = item["name"] in pools[field]
        provenance[field] = {"path": str(path.resolve()), "sha256": sha256_file(path),
                             "count": len(names)}
        inventory["counts"][field + "_cases"] = len(names)
    if not pools["restrict_store_reviewed"] <= pools["default_random_eligible"]:
        raise ValueError("Restricted reviewed pool must be a subset of the default random pool")
    inventory["reviewed_pool_sources"] = provenance
    inventory["claims"]["random_eligibility"] = (
        "default_random_eligible and restrict_store_reviewed come only from explicit "
        "reviewed allowlists; upstream default_active, compile success and lexical hints "
        "do not imply eligibility")


def merge_compile_audit(inventory, path):
    """Attach object-stage evidence without changing execution validation."""
    audit = json.loads(path.read_text(encoding="utf-8"))
    if not audit.get("diagnostic_only"):
        raise ValueError("Compile audit must explicitly declare diagnostic_only")
    if audit.get("source_snapshot_sha256") != inventory["source"]["snapshot_sha256"]:
        raise ValueError("Compile audit belongs to a different source snapshot")
    cases = {item["name"]: item for item in inventory["cases"]}
    results = {item["name"]: item for item in audit.get("case_results", [])}
    if len(results) != len(audit.get("case_results", [])) or set(results) != set(cases):
        raise ValueError("Compile audit case names must match the complete unique catalog")
    source_hashes = {item["path"]: item["sha256"]
                     for item in inventory["source"]["files"]}
    preparation_path = path.parent / "generated/preparation.json"
    preparation = json.loads(preparation_path.read_text(encoding="utf-8"))
    prepared = {item["source_key"]: item for item in preparation["sources"]}
    sources = {}
    for original in audit["source_results"]:
        item = dict(original)
        evidence = prepared[item["source_key"]]
        original_hash = evidence["source_sha256"]
        snapshot_hash = source_hashes.get(item["source_key"])
        if snapshot_hash is not None and snapshot_hash != original_hash:
            raise ValueError("Compile source hash differs from snapshot: " + item["source_key"])
        if item["source_sha256"] != evidence["prepared_sha256"]:
            raise ValueError("Prepared source hashes disagree: " + item["source_key"])
        item["original_source_sha256"] = original_hash
        item["compiled_source_sha256"] = evidence["prepared_sha256"]
        sources[item["source_key"]] = item
    for name, result in results.items():
        evidence = []
        adaptations = []
        for definition in result["definitions"]:
            key = definition["definition"]["path"]
            source = sources.get(key)
            if source:
                evidence.append({
                    "source_key": key, "status": source["status"],
                    "original_source_sha256": source["original_source_sha256"],
                    "compiled_source_sha256": source["compiled_source_sha256"],
                    "compiled_source": source["compiled_source"],
                    "object": source["object"], "compiler_stdout": source["stdout"],
                    "compiler_stderr": source["stderr"], "nm": source.get("nm"),
                    "symbols": definition["symbols"],
                })
                adaptations.extend(item for item in source.get("adaptations", [])
                                   if item.get("function", name) == name)
        cases[name]["compile_diagnostic"] = {
            "status": result["status"], "sources": evidence,
            "adaptations": adaptations, "audit": str(path),
            "elf_linked": False, "spike_run": False, "rtl_run": False,
        }
    adaptations = collections.Counter(
        item["kind"] for source in sources.values() for item in source.get("adaptations", [])
    )
    inventory["compile_diagnostic"] = {
        "audit": str(path), "audit_sha256": sha256_file(path),
        "preparation": str(preparation_path),
        "preparation_sha256": sha256_file(preparation_path),
        "diagnostic_only": True, "isa": audit["isa"], "abi": audit["abi"],
        "compiler": audit["compiler"], "compiler_version": audit["compiler_version"],
        "common_flags": audit["common_flags"], "counts": audit["counts"],
        "limits": audit["limits"], "adaptation_counts": dict(adaptations),
        "source_results": list(sources.values()),
    }
    if "elf_link_results" in audit:
        link_rows = audit["elf_link_results"]
        links = {item["name"]: item for item in link_rows}
        if len(links) != len(link_rows) or set(links) != set(cases):
            raise ValueError("ELF link audit case names must match the complete unique catalog")
        link_counts = dict(collections.Counter(item["status"] for item in link_rows))
        if link_counts != audit["counts"].get("elf_link_status"):
            raise ValueError("ELF link audit counts disagree with per-case results")
        for name, result in links.items():
            if result.get("catalog_test") != cases[name]["catalog_test"]:
                raise ValueError("ELF link audit catalog name differs: " + name)
            if result["status"] == "elf_linked" and (
                    result.get("returncode") != 0 or not result.get("elf_sha256") or
                    result.get("case_symbol", {}).get("name") != name):
                raise ValueError("ELF link audit lacks successful link/symbol evidence: " + name)
            cases[name]["elf_link_diagnostic"] = {
                **result, "audit": str(path), "diagnostic_only": True,
                "spike_run": False, "assertions_run": False, "rtl_run": False,
            }
        inventory["elf_link_diagnostic"] = {
            "audit": str(path), "audit_sha256": sha256_file(path),
            "diagnostic_only": True, "counts": link_counts,
            "fixture": audit["link_fixture"], "limits": audit["limits"],
            "elapsed_seconds": audit.get("link_elapsed_seconds"),
            "object_audit_reused_from": audit.get("object_audit_reused_from"),
            "spike_run": False, "assertions_run": False, "rtl_run": False,
        }


def catalog_testlist_text(inventory):
    lines = [
        "# Generated by scripts/import_hypertest.py. These are selectable entry points,",
        "# not declarations that the cases pass Spike or are supported by NanHu.",
        "# Select an individual test through the catalog_test field in manifest/CSV.",
        "# Keep this catalog separate from the target's default testlist.",
        "# Use Sv48 generator compile options for target nanhu_v5_1.", "",
    ]
    for item in inventory["cases"]:
        description = (f"Source-adapter entry for {item['name']}; "
                       f"validation={item['validation']}. Check platform and review hints first.")
        lines.extend([
            f"- test: {item['catalog_test']}",
            "  description: " + json.dumps(description),
            "  iterations: 1", "  gen_test: riscv_instr_base_test", "  gen_opts: >",
            "    +instr_cnt=100", "    +num_of_sub_program=0", "    +boot_mode=m",
            "    +no_branch_jump=1", "    +no_csr_instr=1", "    +no_ebreak=1",
            "    +no_ecall=1", "    +no_dret=1", "    +no_wfi=1",
            "    +enable_random_directed_instr=0", "    +enable_random_hypertest=1",
            "    +random_hypertest_ratio=10",
            f"    +hypertest_case_names={item['name']}",
            "  gcc_opts: >", f"    -march={CATALOG_GCC_ISA}", "    -mabi=lp64d", "",
        ])
    return "\n".join(lines)


def csv_text(inventory):
    stream = io.StringIO(newline="")
    fields = ("name", "catalog_test", "module", "definition_paths", "definition_lines", "registered",
              "default_active", "default_random_eligible", "restrict_store_reviewed",
              "registration_lines", "registration_duplicate_count",
              "migration_path", "migration_status", "native_class", "source_adapter_status",
              "compile_status", "compile_objects", "compile_source_hashes",
              "compile_adaptations", "compile_audit", "elf_link_status", "elf_link_elf",
              "elf_link_sha256", "elf_link_closure_sources", "elf_link_stdout",
              "elf_link_stderr", "elf_link_error", "elf_link_audit", "validation",
              "validation_evidence", "validation_evidence_sha256", "validation_record",
              "review_hints")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    for item in inventory["cases"]:
        compile_evidence = item.get("compile_diagnostic", {})
        link_evidence = item.get("elf_link_diagnostic", {})
        writer.writerow({
            "name": item["name"], "catalog_test": item["catalog_test"],
            "module": item["module"],
            "definition_paths": ";".join(row["path"] for row in item["definitions"]),
            "definition_lines": ";".join(str(row["line"]) for row in item["definitions"]),
            "registered": int(item["registered"]),
            "default_active": int(item["default_active"]),
            "default_random_eligible": int(item["default_random_eligible"]),
            "restrict_store_reviewed": int(item["restrict_store_reviewed"]),
            "registration_lines": ";".join(str(row["line"])
                                           for row in item["registrations"]),
            "registration_duplicate_count": item["registration_duplicate_count"],
            "migration_path": item["migration_path"],
            "migration_status": item["migration_status"],
            "native_class": item["native_class"] or "",
            "source_adapter_status": item["source_adapter"]["status"],
            "compile_status": compile_evidence.get("status", "not_run"),
            "compile_objects": ";".join(row["object"]
                                        for row in compile_evidence.get("sources", [])),
            "compile_source_hashes": json.dumps([
                {"source": row["source_key"], "original": row["original_source_sha256"],
                 "compiled": row["compiled_source_sha256"]}
                for row in compile_evidence.get("sources", [])], ensure_ascii=False),
            "compile_adaptations": json.dumps(compile_evidence.get("adaptations", []),
                                               ensure_ascii=False),
            "compile_audit": compile_evidence.get("audit", ""),
            "elf_link_status": link_evidence.get("status", "not_run"),
            "elf_link_elf": link_evidence.get("elf", ""),
            "elf_link_sha256": link_evidence.get("elf_sha256", ""),
            "elf_link_closure_sources": ";".join(link_evidence.get("closure_sources", [])),
            "elf_link_stdout": link_evidence.get("stdout", ""),
            "elf_link_stderr": link_evidence.get("stderr", ""),
            "elf_link_error": link_evidence.get("error", "") or
                              "; ".join(link_evidence.get("first_errors", [])),
            "elf_link_audit": link_evidence.get("audit", ""),
            "validation": item["validation"],
            "validation_evidence": json.dumps(item.get("validation_evidence", []),
                                              ensure_ascii=False),
            "validation_evidence_sha256": json.dumps(
                item.get("validation_evidence_sha256", {}), ensure_ascii=False),
            "validation_record": json.dumps(item.get("validation_record", {}),
                                            ensure_ascii=False),
            "review_hints": ";".join(hint["kind"] for hint in item["review_hints"]),
        })
    return stream.getvalue()


def markdown_evidence_links(record):
    links = []
    evidence = record.get("evidence", [])
    runtime = [value for value in evidence
               if value.endswith(".hypertest-validation.json")]
    for value in runtime or evidence:
        path = Path(value)
        if not path.is_absolute():
            path = REPO_ROOT / path
        label = Path(value).name.replace("[", r"\[").replace("]", r"\]")
        links.append(f"[{label}](<{path.as_posix()}>)")
    return "<br>".join(links) or "未提供链接"


def markdown_report(inventory, csv_path, manifest_path, testlist_path):
    counts = inventory["counts"]
    modules = collections.defaultdict(collections.Counter)
    for item in inventory["cases"]:
        row = modules[item["module"]]
        row["cases"] += 1
        row["registered"] += item["registered"]
        row["default_active"] += item["default_active"]
        row["native"] += bool(item["native_class"])
        row["review"] += bool(item["review_hints"])
    lines = [
        "# Hypertest 全量移植与验证报告", "",
        "本报告由 `scripts/import_hypertest.py` 从当前源工作树生成。所有注册用例（含",
        "注释注册）及未注册但含 `TEST_START()` 的 bool 用例均进入 source-adapter catalog；",
        "其余 bool helper 单列审计，不计作测试用例。重复注册按名称去重，保留全部原始行。", "",
        "报告按源码/独立 DV 入口接通与实际执行验证两个维度记录结果。全量源入口和",
        "catalog testlist 负责逐项选择，执行通过数量只来自实际完成自检的证据。", "",
        "源码已接入 catalog 不等于这些用例均可在 NanHu 或 Spike 上执行。逐用例",
        "validation 初始为 `not_run`；只有显式 validation JSON 才能合并执行证据。",
        "native 和 source-adapter 的初始迁移状态分别为 `implemented_unverified` 和",
        "`imported_unverified`；完成 generator/GNU/Spike 的条目会更新为相应 verified 状态。",
        "本次实际结果以独立的 `migration_status`、`validation` 列和下方执行证据表为准，",
        "对象编译或 ELF 链接不能替代执行结论，Spike 通过也不代表 RTL 验证完成。",
        "平台约束、H 扩展、外部注入、reset/终止和程序写代码等",
        "词法提示仅标记 `needs_review`，不自动推断可运行性或替代人工审核。", "",
        "## 来源证据", "",
        f"- 源路径：`{inventory['source']['path']}`",
        f"- Git revision：`{inventory['source']['git_revision'] or 'unavailable'}`",
        f"- 原样快照 SHA256：`{inventory['source']['snapshot_sha256']}`",
        f"- 快照文件：{counts['source_files']}，共 {counts['source_bytes']} bytes。",
        "- manifest 保存逐文件 SHA256、大小、权限、源工作树 dirty 状态及被排除的软链。",
        "- 只快照 case、共享头文件/框架/汇编、Spike 平台以及必要的源码/注册/许可证文件；",
        "  build、results、仓库外文件和软链不导入。", "",
        "## 数量", "",
        "| 项目 | 数量 |", "| --- | ---: |",
    ]
    labels = (
        ("原始注册行", "registration_lines"),
        ("去重后已注册 case", "registered_unique_cases"),
        ("默认 active case", "default_active_unique_cases"),
        ("仅注释注册 case", "commented_only_registered_cases"),
        ("重复注册的额外行", "duplicate_registration_lines"),
        ("未注册 TEST_START case", "unregistered_test_start_cases"),
        ("catalog case 合计", "catalog_cases"),
        ("原生 directed stream case", "native_cases"),
        ("source-adapter 源入口（包含上述 native case，不另加总）", "source_adapter_cases"),
        ("非 case bool helper 定义", "non_case_bool_helper_definitions"),
        ("缺失定义 case", "missing_definition_cases"),
        ("有多处定义 case", "multiple_definition_cases"),
        ("带 needs_review 提示 case", "cases_with_review_hints"),
    )
    lines.extend(f"| {label} | {counts[key]} |" for label, key in labels)
    pools = inventory.get("reviewed_pool_sources")
    if pools:
        default_pool = pools["default_random_eligible"]
        restricted_pool = pools["restrict_store_reviewed"]
        lines.extend([
            "", "## 随机选择与 store 限制审核名单", "",
            f"全部 {counts['catalog_cases']} 个 catalog case 均可通过显式名称选择；",
            f"默认随机池只选择审核名单中的 {default_pool['count']} 项，",
            f"`+restrict_store_to_data=1` 的默认池只选择其中已审核的 "
            f"{restricted_pool['count']} 项。", "",
            "`default_active` 仅表示上游注册表默认启用状态；`default_random_eligible`",
            "表示 DV 默认随机池成员；`restrict_store_reviewed` 表示 store 限制已审核成员。",
            "这些字段用途不同；对象编译、ELF 链接和词法提示均不能自动授予随机池资格。", "",
            f"- 默认随机池：`{default_pool['path']}`，SHA256 "
            f"`{default_pool['sha256']}`。",
            f"- restrict 审核池：`{restricted_pool['path']}`，SHA256 "
            f"`{restricted_pool['sha256']}`。", "",
            "显式选择 catalog 用例仍须确认 H 扩展、locked PMP、外部激励和平台地址等",
            "运行前提。名单用于控制默认随机抽样，不表示 catalog 中其它用例已经通过执行。", "",
        ])
    lines.extend(["", "## 按模块", "",
                  "| 模块 | case | 已注册 | 默认 active | native | needs_review |",
                  "| --- | ---: | ---: | ---: | ---: | ---: |"])
    for name, row in sorted(modules.items()):
        lines.append(f"| {name} | {row['cases']} | {row['registered']} | "
                     f"{row['default_active']} | {row['native']} | {row['review']} |")
    compile_audit = inventory.get("compile_diagnostic")
    if compile_audit:
        source_counts = compile_audit["counts"]["source_status"]
        case_counts = compile_audit["counts"]["case_status"]
        lines.extend([
            "", "## 全量 GNU 对象编译诊断", "",
            f"审计日志：`{compile_audit['audit']}`。这里只执行 `gcc -c` 和 `nm`，",
            "该对象阶段不执行原断言、Spike 或 RTL。`object_compiled` 表示相应源码",
            "编译完成且对象中存在该 case 的函数符号；不计入执行通过数量。", "",
            "| 源文件状态 | 数量 |", "| --- | ---: |",
        ])
        lines.extend(f"| {name} | {count} |" for name, count in sorted(source_counts.items()))
        lines.extend(["", "| case 对象状态 | 数量 |", "| --- | ---: |"])
        lines.extend(f"| {name} | {count} |" for name, count in sorted(case_counts.items()))
        lines.extend([
            "", "编译使用与运行 adapter 相同的 `prepare_source`，原样快照没有修改。",
            "派生副本包含共享 trap handler 返回值/栈对齐修复，以及仅对含真实",
            "`TEST_START` 而缺少 `TEST_END` 的 bool case 补充返回协议，保留原",
            "`test_status` 和所有断言。C 文件额外强制包含 `stddef.h` 并启用",
            "`-Werror=return-type`；副本保留原目录的 quoted-header 搜索路径。", "",
            "| 派生副本适配记录 | 数量 |", "| --- | ---: |",
        ])
        lines.extend(f"| {name} | {count} |" for name, count in
                     sorted(compile_audit["adaptation_counts"].items()))
        lines.extend([
            "", "manifest 的顶层 `compile_diagnostic.source_results` 保存每个源文件的",
            "原始/派生 SHA256、命令、对象、compiler/nm 日志及适配记录；每个 case 的",
            "`compile_diagnostic` 和 CSV 的 `compile_*` 列保存对应对象证据。", "",
        ])
        for source in compile_audit["source_results"]:
            if source["status"] != "compiled":
                lines.append(f"- `{source['source_key']}`：`{source['status']}`；"
                             f"{source.get('configuration_note', '')} 日志：`{source['stderr']}`。")
        lines.extend([
            "", "上述独立 flash payload 的 POINT 配置需求不代表 bool case 编译失败；",
            "它需要原专用 runner 提供配置。对象诊断也不验证外部中断、debug、reset、",
            "PMA/MMIO 响应以及 H 扩展是否受目标硬件支持。", "",
        ])
    link_audit = inventory.get("elf_link_diagnostic")
    if link_audit:
        lines.extend([
            "", "## 全量逐 case ELF 链接诊断", "",
            f"审计日志：`{link_audit['audit']}`。为每个 catalog case 单独计算 C/ASM",
            "依赖闭包，复用上述已编译对象，用最小启动 fixture 与 DV linker script",
            "执行最终静态链接，并通过 `nm` 确认 case、bridge 和入口符号保留。", "",
            "| ELF 链接状态 | case |", "| --- | ---: |",
        ])
        lines.extend(f"| {name} | {count} |" for name, count in
                     sorted(link_audit["counts"].items()))
        lines.extend([
            "", "这一步没有执行生成器随机程序、原用例断言、Spike 或 RTL。",
            "`elf_linked` 只表示依赖闭包可以链接；不计为架构或平台执行通过。",
            "每 case 的 `elf_link_diagnostic` 保存 ELF/SHA256、链接命令、依赖闭包、",
            "compiler/nm 日志和失败原因；CSV 的 `elf_link_*` 列提供索引。", "",
            f"启动 fixture：`{link_audit['fixture']['source']}`，SHA256 "
            f"`{link_audit['fixture']['source_sha256']}`。", "",
        ])
        failures = [item for item in inventory["cases"]
                    if item["elf_link_diagnostic"]["status"] != "elf_linked"]
        if failures:
            for item in failures:
                result = item["elf_link_diagnostic"]
                reason = result.get("error", "") or "; ".join(result.get("first_errors", []))
                lines.append(f"- `{item['name']}`：`{result['status']}`；{reason} "
                             f"日志：`{result.get('stderr', '')}`。")
        else:
            lines.append("本次完整 catalog 未出现链接失败。")
    lines.extend(["", "## 执行证据状态", "", "| validation | case |", "| --- | ---: |"])
    states = collections.Counter(item["validation"] for item in inventory["cases"])
    lines.extend(f"| {name} | {count} |" for name, count in sorted(states.items()))
    validation_source = inventory.get("validation_source")
    if validation_source:
        lines.extend([
            "", f"执行记录：`{validation_source['path']}`，SHA256 "
            f"`{validation_source['sha256']}`。完整逐 case 记录（含 evidence SHA256、",
            "实际运行命中、完成次数及每轮 details）保存在 `validation_record`；",
            "collection 元数据及被拒绝的证据保存在 `validation_metadata`。", "",
        ])
    passed = [item for item in inventory["cases"]
              if item["validation"] == "gen_gcc_spike_pass"]
    if passed:
        lines.extend([
            "", "### 已通过 generator/GNU/Spike 的 catalog case", "",
            "以下每行对应一个实际完成原自检的 catalog case；多次运行仍只计一个 case。", "",
            "| case | migration_status | 完成次数 | evidence |",
            "| --- | --- | ---: | --- |",
        ])
        for item in passed:
            record = item["validation_record"]
            lines.append(f"| `{item['name']}` | `{item['migration_status']}` | "
                         f"{record.get('completed_count', '未提供')} | "
                         f"{markdown_evidence_links(record)} |")
    supplementary = inventory.get("supplementary_checks")
    if supplementary is not None:
        lines.extend([
            "", "### 补充验证（不计入迁移 case 数量）", "",
            "bridge selftest 验证适配桥，legacy 验证普通 DV 回归。完整记录保存在",
            "manifest 的 `supplementary_checks`；这些记录不增加 catalog case 或通过 case 数。", "",
            "| 类别 | 记录数 |", "| --- | ---: |",
        ])
        for kind, records in sorted(supplementary.items()):
            lines.append(f"| `{kind}` | {len(records)} |")
        if any(supplementary.values()):
            lines.extend(["", "| 类别 | 检查 | validation | evidence |",
                          "| --- | --- | --- | --- |"])
            for kind, records in sorted(supplementary.items()):
                for record in records:
                    name = record.get("test") or ", ".join(record.get("case_hits", {})) or \
                           record.get("run", "未提供")
                    lines.append(f"| `{kind}` | `{name}` | "
                                 f"`{record.get('validation', '未提供')}` | "
                                 f"{markdown_evidence_links(record)} |")
    lines.extend([
        "", "## 逐用例清单与复现", "",
        "本轮工具、命令、seed、输出目录与限制见 [实际验证记录](hypertest-validation-runs.md)，",
        "随机生成与独立运行方法见 [使用说明](hypertest-directed-streams.md)。", "",
        f"逐 case 的定义位置、注册位置、迁移路径和 validation 见 `{csv_path}`。",
        f"完整结构化数据及非 case helper 审计见 `{manifest_path}`。", "",
        f"可逐项选择的测试入口见 `{testlist_path}`，`catalog_test` 列给出短测试名，",
        "`hypertest_case_names` 保留完整原始名称。全量和 active-only 名单分别见",
        "`src/hypertest/case_names.txt` 和 `src/hypertest/case_names_active.txt`。",
        "catalog testlist 不加入 target 的默认 testlist，避免无意一键运行数千项。", "",
        "```bash",
        "python3 scripts/import_hypertest.py --source /path/to/riscv-hyp-tests-nhv5 \\",
        "  --inventory-only",
        "python3 scripts/import_hypertest.py --source /path/to/riscv-hyp-tests-nhv5",
        "```", "",
        "可用 `--compile-audit out/<本次审计目录>/audit.json` 合并完整对象编译和可选 ELF 链接证据；",
        "importer 会核对快照 SHA256、源文件哈希和完整 case 名称集合，拒绝混用。", "",
        "可选 `--validation src/hypertest/validation.json` 合并逐用例外部记录；JSON",
        '格式为 `{"cases": {"原case名": {"validation": "状态", '
        '"evidence": ["日志路径"]}}}`。未知 case 名称会报错，避免错配证据。', "",
        "若 validation JSON 提供 `source_snapshot_sha256`，必须与本次原样快照一致。",
        "完整 case record、`evidence_sha256`、每轮实际命中及 `supplementary_checks`",
        "会原样保留；补充检查不参与迁移 case 通过计数。", "",
        "相同源文件内容和权限会得到相同 snapshot SHA256。importer 不运行原仓库的",
        "编译或仿真程序；manifest 中的执行结论需由后续完整 generator/GNU/Spike",
        "运行及独立 RTL 验证证据更新。导入器再次运行会重建未验证清单，应在更新",
        "验证记录前保留该次运行对应的 manifest。", "",
    ])
    return "\n".join(lines)


def reject_symlink_components(path):
    for parent in (path, *path.parents):
        if parent.is_symlink():
            raise ValueError(f"Output path contains a symlink: {parent}")


def copy_snapshot(source, destination, records, replace_existing):
    reject_symlink_components(destination)
    expected = {item["path"] for item in records}
    if destination.exists():
        for existing in destination.rglob("*"):
            if existing.is_symlink():
                raise ValueError(f"Existing snapshot contains a symlink: {existing}")
            if existing.is_file() and existing.relative_to(destination).as_posix() not in expected:
                raise ValueError(f"Existing snapshot has an extra file: {existing}; use a new --dest")
    # Check all conflicts first so a different old snapshot cannot be partially
    # overwritten before the user chooses a new destination or explicit update.
    for item in records:
        target_file = destination / item["path"]
        reject_symlink_components(target_file)
        if target_file.exists() and sha256_file(target_file) != item["sha256"]:
            if not replace_existing:
                raise ValueError(f"Existing snapshot differs: {target_file}; "
                                 "use a new --dest or explicitly --replace-existing")
    for item in records:
        source_file = source / item["path"]
        target_file = destination / item["path"]
        target_file.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_file, target_file)
        if sha256_file(target_file) != item["sha256"]:
            raise ValueError(f"Source changed during import: {source_file}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="existing hypertest repository root")
    parser.add_argument("--dest", type=Path,
                        default=REPO_ROOT / "src/hypertest/upstream")
    parser.add_argument("--manifest", type=Path,
                        default=REPO_ROOT / "src/hypertest/manifest.json")
    parser.add_argument("--report", type=Path,
                        default=REPO_ROOT / "docs/hypertest-migration-report.md")
    parser.add_argument("--csv", type=Path,
                        default=REPO_ROOT / "docs/hypertest-migration.csv")
    parser.add_argument("--case-names", type=Path,
                        default=REPO_ROOT / "src/hypertest/case_names.txt")
    parser.add_argument("--active-case-names", type=Path,
                        default=REPO_ROOT / "src/hypertest/case_names_active.txt")
    parser.add_argument("--default-random-names", type=Path,
                        default=REPO_ROOT / "src/hypertest/case_names_random.txt",
                        help="reviewed default random pool; independent of upstream active entries")
    parser.add_argument("--restricted-names", type=Path,
                        default=REPO_ROOT / "src/hypertest/case_names_restricted.txt",
                        help="reviewed subset for restrict_store_to_data random selection")
    parser.add_argument("--testlist", type=Path,
                        default=REPO_ROOT / "yaml/hypertest_catalog_testlist.yaml")
    parser.add_argument("--validation", type=Path,
                        default=REPO_ROOT / "src/hypertest/validation.json",
                        help="optional external per-case validation records to merge")
    parser.add_argument("--compile-audit", type=Path,
                        help="complete check_hypertest_compile.py object/optional-link audit to merge")
    parser.add_argument("--inventory-only", action="store_true",
                        help="print complete JSON inventory; do not write any file")
    parser.add_argument("--replace-existing", action="store_true",
                        help="explicitly allow updating different files in an existing snapshot")
    args = parser.parse_args()
    source = args.source.resolve()
    if not (source / "test_register.c").is_file():
        parser.error(f"Not a hypertest source root: {source}")
    try:
        inventory = make_inventory(source)
        merge_reviewed_pools(inventory, args.default_random_names, args.restricted_names)
        merge_validation(inventory, args.validation)
        if args.compile_audit:
            merge_compile_audit(inventory, args.compile_audit)
        if args.inventory_only:
            print(json.dumps(inventory, ensure_ascii=False, indent=2))
            return 0
        destination = args.dest.absolute()
        if destination == source or source in destination.parents:
            raise ValueError("Snapshot destination must be outside the source repository")
        outputs = (args.manifest, args.report, args.csv, args.case_names,
                   args.active_case_names, args.testlist)
        for output in outputs:
            reject_symlink_components(output.absolute())
            if output.resolve() == source or source in output.resolve().parents:
                raise ValueError("Reports must be outside the source repository")
            if output.resolve() == destination or destination in output.resolve().parents:
                raise ValueError("Reports must be outside the verbatim snapshot")
        if len({output.absolute() for output in outputs}) != len(outputs):
            raise ValueError("Each generated output requires a distinct path")
        copy_snapshot(source, destination, inventory["source"]["files"],
                      args.replace_existing)
        inventory["snapshot_destination"] = str(destination)
        for output in outputs:
            output.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(
            json.dumps(inventory, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )
        args.csv.write_text(csv_text(inventory), encoding="utf-8")
        args.report.write_text(markdown_report(inventory, args.csv, args.manifest,
                                              args.testlist), encoding="utf-8")
        args.case_names.write_text(
            "".join(item["name"] + "\n" for item in inventory["cases"]), encoding="utf-8"
        )
        args.active_case_names.write_text(
            "".join(item["name"] + "\n" for item in inventory["cases"]
                    if item["default_active"]), encoding="utf-8"
        )
        args.testlist.write_text(catalog_testlist_text(inventory), encoding="utf-8")
        print(json.dumps({"counts": inventory["counts"],
                          "snapshot_sha256": inventory["source"]["snapshot_sha256"],
                          "manifest": str(args.manifest),
                          "report": str(args.report), "csv": str(args.csv)},
                         ensure_ascii=False, indent=2))
    except (OSError, ValueError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
