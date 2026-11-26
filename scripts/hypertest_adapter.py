"""Compile the vendored hypertest cases referenced by a generated DV program.

The source marker is a build dependency, not evidence of execution.  Spike
validation below independently checks the case entries, bridge return values,
and the normal DV tohost completion.
"""

from collections import Counter
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
from types import SimpleNamespace


ASM_COMMENT_PREFIX = r"^[ \t]*(?:(?:[A-Za-z_.$][A-Za-z_0-9.$]*|[0-9]+):[ \t]*)?#"
CASE_RE = re.compile(ASM_COMMENT_PREFIX +
                     r"[ \t]*HYPERTEST_CASE:[ \t]*([A-Za-z_][A-Za-z_0-9]*)[ \t]*$",
                     re.MULTILINE)
INJECTION_RE = re.compile(
    ASM_COMMENT_PREFIX + r"[ \t]*HYPERTEST_INJECTION:[ \t]*(native|source)[ \t]+"
    r"([A-Za-z_][A-Za-z_0-9]*)[ \t]+([A-Za-z_.$][A-Za-z_0-9.$]*)[ \t]+"
    r"([A-Za-z_.$][A-Za-z_0-9.$]*)[ \t]*$", re.MULTILINE)
PC_RE = re.compile(r"^core\s+0:\s+0x([0-9a-fA-F]+)\s+\(0x([0-9a-fA-F]+)\)")
MEM_RE = re.compile(r"\bmem\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\b")
ILLEGAL_RE = re.compile(r"\bexception\s+trap_illegal_instruction\b")
PLATFORM_HEADER = """#ifndef PLATFORM_H
#define PLATFORM_H
#define MEM_BASE (0x80000000UL)
#define MEM_SIZE (0x10000000UL)
/* Smrnmi is a DUT capability, not an assumption of the Spike platform. */
#endif
"""
SYNTHETIC_CASE_SOURCES = {
    "dv_hypertest_selftest_clobber_case": "bridge_selftest.c",
}
BOOL_DEF_RE = re.compile(
    r"^[ \t]*(?:(?:static|inline|extern|__inline__|__inline)\s+)*"
    r"bool\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\(", re.MULTILINE)
NON_CODE_RE = re.compile(
    r"/\*.*?\*/|//[^\n]*|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'", re.DOTALL)


def case_names(assembly_path):
    """Return every static injection, including repeated instances."""
    return CASE_RE.findall(Path(assembly_path).read_text(encoding="utf-8"))


def injection_metadata(assembly_path):
    text = Path(assembly_path).read_text(encoding="utf-8")
    entries = [{"kind": kind, "case": case, "id": begin,
                "begin_symbol": begin, "pass_symbol": passed}
               for kind, case, begin, passed in INJECTION_RE.findall(text)]
    if len(entries) != len(re.findall(ASM_COMMENT_PREFIX + r"[ \t]*HYPERTEST_INJECTION:", text,
                                      flags=re.MULTILINE)):
        raise RuntimeError("Malformed HYPERTEST_INJECTION metadata")
    symbols = [entry[key] for entry in entries for key in ("begin_symbol", "pass_symbol")]
    if len(set(symbols)) != len(symbols):
        raise RuntimeError("Every hypertest injection requires distinct begin/pass symbols")
    if entries and Counter(CASE_RE.findall(text)) != Counter(
            entry["case"] for entry in entries if entry["kind"] == "source"):
        raise RuntimeError("Source HYPERTEST_INJECTION metadata does not match HYPERTEST_CASE dependencies")
    return entries


def _load_planner(upstream):
    path = upstream / "compile_elf.py"
    # dataclasses consult sys.modules while decorating the imported classes.
    name = "_dv_hypertest_planner_" + hashlib.sha256(str(path).encode()).hexdigest()[:16]
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load hypertest source planner: {}".format(path))
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    previous_bytecode = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    try:
        spec.loader.exec_module(module)
    except BaseException:
        sys.modules.pop(name, None)
        raise
    finally:
        sys.dont_write_bytecode = previous_bytecode
    return module


def _compiler_command(gcc):
    if isinstance(gcc, (tuple, list)):
        return [str(arg) for arg in gcc]
    # RISCV_GCC may include a compiler launcher. Preserve an existing filename
    # with spaces as one argument; otherwise parse without ever invoking a shell.
    return [str(gcc)] if Path(gcc).is_file() else shlex.split(str(gcc))


def _require_bridge_isa(isa):
    match = re.match(r"^rv64([a-z0-9]+)(?:_|$)", isa.lower())
    extensions = set(match.group(1)) if match else set()
    if "g" in extensions:
        extensions.update("imafd")
    # Full V implies Zve64d and its scalar floating-point dependencies.
    if "v" in extensions:
        extensions.update("fd")
    if not match or not {"f", "d", "v"}.issubset(extensions):
        raise ValueError("Source hypertest bridge requires an RV64 GNU ISA with F, D and "
                         "full V for architectural-state preservation; requested -march={}. "
                         "Use a matching target/toolchain configuration; the adapter will not "
                         "silently enable extensions.".format(isa))


def _write_json(path, payload):
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8")


def prepare_source(source, upstream, prepared):
    """Prepare a counted framework/return-protocol adaptation, never edit input.

    Return ``(source_path, extra_compiler_flags, adaptation_records)``. Callers
    performing compile censuses must use this same boundary and compiler flags
    as normal DV builds. No assertion is removed or replaced with success.
    """
    source, upstream, prepared = Path(source), Path(upstream), Path(prepared)
    original = source.read_text(encoding="utf-8")
    text = original
    adaptations = []
    flags = ["-include", "stddef.h"] if source.suffix == ".c" else []
    if source == upstream / "asm" / "handlers.S":
        count = text.count("(REGLEN*31)")
        if count != 2:
            raise RuntimeError("Expected exactly 2 hypertest trap-stack adjustments, found {} in {}"
                               .format(count, source))
        text = text.replace("(REGLEN*31)", "(REGLEN*32)")
        adaptations.append({"kind": "trap_stack_alignment", "source": str(source),
                            "old_bytes": 248, "new_bytes": 256, "replacements": count})
    elif source == upstream / "src" / "rvh_test.c":
        text, count = re.subn(
            r"^([ \t]*)return_from_exception\(temp_priv, curr_priv, cause, epc\);[ \t]*$",
            r"\1return return_from_exception(temp_priv, curr_priv, cause, epc);",
            text, flags=re.MULTILINE)
        if count != 3:
            raise RuntimeError("Expected exactly 3 hypertest C handler return fixes, found {} in {}"
                               .format(count, source))
        flags += ["-Dreset_state=dv_hyp_legacy_reset_state",
                  "-Dgoto_priv=dv_hyp_legacy_goto_priv"]
        adaptations.append({"kind": "handler_return_value", "source": str(source),
                            "replacements": count})
    elif (source.suffix == ".c" and source.is_relative_to(upstream) and
          source.relative_to(upstream).parts[0] in ("ai_test_cases", "manual_test_cases")):
        masked = NON_CODE_RE.sub(
            lambda match: "".join("\n" if char == "\n" else " "
                                  for char in match.group()), text)
        stack, closing_braces = [], {}
        for position, char in enumerate(masked):
            if char == "{":
                stack.append(position)
            elif char == "}" and stack:
                closing_braces[stack.pop()] = position
        additions = []
        for match in BOOL_DEF_RE.finditer(masked):
            depth, position = 1, match.end()
            while position < len(masked) and depth:
                char = masked[position]
                depth += (char == "(") - (char == ")")
                position += 1
            if depth:
                continue
            opening = masked.find("{", position)
            semicolon = masked.find(";", position)
            if opening < 0 or (0 <= semicolon < opening):
                continue
            closing = closing_braces.get(opening)
            if closing is None:
                continue
            body = masked[opening:closing]
            if (not re.search(r"\bTEST_START\s*\(", body) or
                    re.search(r"\bTEST_END\s*\(", body)):
                continue
            additions.append(closing)
            adaptations.append({"kind": "test_end_return_protocol", "source": str(source),
                                "function": match.group("name"),
                                "line": text.count("\n", 0, match.start()) + 1,
                                "result": "original test_status"})
        for position in sorted(set(additions), reverse=True):
            text = (text[:position] +
                    "\n    /* DV adapter: complete the original TEST_START return protocol. */\n"
                    "    goto_priv(PRIV_M);\n    reset_state();\n    return test_status;\n" +
                    text[position:])
    if text == original:
        return source, flags, adaptations
    prepared.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(str(source.resolve()).encode() + text.encode()).hexdigest()[:16]
    path = prepared / (source.stem + "_" + digest + source.suffix)
    if not path.exists():
        path.write_text(text, encoding="utf-8")
    # Quoted includes in the original file must still resolve beside that file.
    flags += ["-iquote", str(source.parent)]
    return path, flags, adaptations


def _page_table_core(upstream, case_sources):
    users = {"page_tables.h": [], "dynamic_page_tables.h": []}
    for source in sorted(set(case_sources)):
        text = source.read_text(encoding="utf-8", errors="replace")
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.DOTALL)
        for header in users:
            if re.search(r'^\s*#\s*include\s*[<"]' + re.escape(header) + r'[>"]',
                         text, flags=re.MULTILINE):
                users[header].append(str(source))
    if users["dynamic_page_tables.h"]:
        if users["page_tables.h"]:
            raise ValueError("Cannot mix static and dynamic hypertest page-table cases "
                             "in one ELF; static sources: {}; dynamic sources: {}"
                             .format(", ".join(users["page_tables.h"]),
                                     ", ".join(users["dynamic_page_tables.h"])))
        return upstream / "src" / "dynamic_page_tables.c"
    return upstream / "src" / "page_tables.c"


def build_objects(repo, assembly_path, gcc, output_dir, isa, abi,
                  debug_cmd=None, extra_cflags=None):
    """Return objects and linker options for marked cases, or [] for normal DV.

    ``isa`` and ``abi`` must be the *resolved final* GNU settings, including any
    per-test override. ``extra_cflags`` carries additional per-test compiler
    options; this adapter never silently widens the requested ISA.
    """
    assembly = Path(assembly_path)
    if debug_cmd and not assembly.is_file():
        return []
    names = case_names(assembly)
    if not names:
        return []
    repo = Path(repo).resolve()
    upstream = repo / "src" / "hypertest" / "upstream"
    bridge = repo / "src" / "hypertest"
    build_root = Path(output_dir).resolve() / "hypertest"
    cache_dir = build_root / "objects"
    cache_dir.mkdir(parents=True, exist_ok=True)
    identifier = hashlib.sha256(str(assembly.resolve()).encode()).hexdigest()[:12]
    log_path = build_root / (assembly.stem[:80] + "_" + identifier + ".build.json")
    record = {"assembly": str(assembly.resolve()), "cases": names,
              "isa": isa, "abi": abi, "status": "planning", "commands": []}
    try:
        _require_bridge_isa(isa)
        planner = _load_planner(upstream)
        plan, errors = planner.build_case_source_plan(
            [SimpleNamespace(name=name) for name in dict.fromkeys(names)
             if name not in SYNTHETIC_CASE_SOURCES])
        if errors:
            raise ValueError("Unknown or ambiguous hypertest case(s): " +
                             "; ".join("{}: {}".format(k, v)
                                       for k, v in sorted(errors.items())))
        sources = {upstream / "src" / name
                   for name in ("rvh_test.c", "instruction.c")}
        sources.update((bridge / "bridge.c", bridge / "bridge.S"))
        sources.update(bridge / SYNTHETIC_CASE_SOURCES[name]
                       for name in names if name in SYNTHETIC_CASE_SOURCES)
        case_sources = set()
        for case_plan in plan.values():
            sources.update(case_plan.c_sources)
            sources.update(case_plan.asm_sources)
            case_sources.update(case_plan.c_sources)
        page_table_core = _page_table_core(upstream, case_sources)
        sources.add(page_table_core)
        record["page_table_core"] = str(page_table_core)

        prepared = build_root / "generated"
        prepared.mkdir(exist_ok=True)
        sources.add(upstream / "asm" / "handlers.S")
        record["source_adaptations"] = []
        platform_dir = prepared / "include"
        platform_dir.mkdir(exist_ok=True)
        platform_path = platform_dir / "platform.h"
        if not platform_path.exists() or platform_path.read_text() != PLATFORM_HEADER:
            platform_path.write_text(PLATFORM_HEADER, encoding="utf-8")

        flags = list(extra_cflags or [])
        if isinstance(extra_cflags, str):
            flags = shlex.split(extra_cflags)
        common = (_compiler_command(gcc) + ["-march=" + isa, "-mabi=" + abi] +
                  flags + ["-mcmodel=medany", "-O2", "-ffunction-sections",
                           "-fdata-sections", "-fno-tree-vectorize", "-fno-builtin",
                           "-ffreestanding", "-Werror=return-type",
                           "-msmall-data-limit=0", "-DLOG_LEVEL=0",
                           "-I" + str(platform_dir), "-I" + str(upstream / "inc"),
                           "-I" + str(upstream / "platform" / "spike" / "inc"),
                           "-I" + str(bridge)])
        prepared_sources = []
        for original_source in sorted(sources):
            if not original_source.is_file():
                raise FileNotFoundError("Missing hypertest source: {}".format(original_source))
            source, source_flags, adaptations = prepare_source(
                original_source, upstream, prepared)
            record["source_adaptations"].extend(adaptations)
            prepared_sources.append((original_source, source, source_flags))
        headers_hash = hashlib.sha256(PLATFORM_HEADER.encode())
        for header in sorted(set(upstream.rglob("*.h")) | set(bridge.glob("*.h"))):
            headers_hash.update(str(header).encode())
            headers_hash.update(header.read_bytes())
        compiler = shutil.which(common[0])
        if compiler:
            stat = Path(compiler).stat()
            headers_hash.update("{}:{}:{}".format(compiler, stat.st_size,
                                                stat.st_mtime_ns).encode())
        objects = []
        for original_source, source, source_flags in prepared_sources:
            base_command = common + source_flags + ["-c", str(source)]
            digest = hashlib.sha256(headers_hash.digest())
            digest.update(source.read_bytes())
            digest.update(json.dumps(base_command).encode())
            obj = cache_dir / (source.stem[:64] + "_" + digest.hexdigest()[:24] + ".o")
            command = base_command + ["-o", str(obj)]
            entry = {"source": str(source), "original_source": str(original_source),
                     "command": command,
                     "object": str(obj), "cached": obj.is_file()}
            record["commands"].append(entry)
            if debug_cmd:
                debug_cmd.write(shlex.join(command) + "\n\n")
            elif not obj.is_file():
                # A failed compiler must never leave a reusable cache entry.
                temp_obj = obj.with_suffix(".{}.tmp.o".format(os.getpid()))
                actual_command = base_command + ["-o", str(temp_obj)]
                entry["executed_command"] = actual_command
                result = subprocess.run(actual_command, text=True, capture_output=True)
                entry.update(returncode=result.returncode, stdout=result.stdout,
                             stderr=result.stderr)
                if result.returncode or not temp_obj.is_file():
                    temp_obj.unlink(missing_ok=True)
                    raise RuntimeError("Hypertest compilation failed for {} ({}):\n{}\n{}"
                                       .format(source, result.returncode,
                                               result.stdout, result.stderr))
                temp_obj.replace(obj)
            objects.append(str(obj))
        record["status"] = "commands-only" if debug_cmd else "compiled"
        record["link_arguments"] = objects + ["-Wl,--gc-sections", "-lgcc"]
        return record["link_arguments"]
    except BaseException as exc:
        record["status"] = "failed"
        record["error"] = str(exc)
        raise
    finally:
        _write_json(log_path, record)


def _nm_symbols(elf, nm=None):
    if nm is None:
        nm = os.environ.get("RISCV_NM")
    if nm is None:
        gcc = os.environ.get("RISCV_GCC", "")
        candidate = gcc[:-3] + "nm" if gcc.endswith("gcc") else ""
        nm = (candidate if candidate and shutil.which(candidate) else
              shutil.which("riscv64-unknown-elf-nm") or shutil.which("nm"))
    if not nm:
        raise RuntimeError("No nm executable available to validate hypertest execution")
    result = subprocess.run(_compiler_command(nm) + ["-n", str(elf)],
                            text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError("Cannot inspect hypertest ELF symbols: " + result.stderr)
    symbols = {}
    for line in result.stdout.splitlines():
        match = re.match(r"^\s*([0-9a-fA-F]+)\s+\S\s+(\S+)\s*$", line)
        if match:
            symbols[match.group(2)] = int(match.group(1), 16)
    return symbols


def _validate_spike_injections(log, elf, assembly, entries, nm, require_all):
    """Match each static wrapper separately, including repeated loop visits."""
    log = Path(log)
    artifact = log.with_suffix(".hypertest-validation.json")
    record = {"assembly": str(assembly), "elf": str(elf), "log": str(log),
              "static_cases": [entry["case"] for entry in entries],
              "injections": entries, "injection_hits": {entry["id"]: 0 for entry in entries},
              "case_hits": {}, "intervals": [], "injection_intervals": [],
              "status": "failed",
              "pass_semantics": "original_selfcheck_passed for each completed injection; "
                                "native instructions receive no illegal-trap exemption"}
    try:
        symbols = _nm_symbols(elf, nm)
        source_entries = [entry for entry in entries if entry["kind"] == "source"]
        required = {entry[key] for entry in entries for key in ("begin_symbol", "pass_symbol")}
        if source_entries:
            required.update(("dv_hypertest_begin", "dv_hypertest_pass", "dv_hypertest_fail"))
            required.update(entry["case"] for entry in source_entries)
        missing = required - symbols.keys()
        if missing:
            raise RuntimeError("Missing hypertest ELF symbols: " + ", ".join(sorted(missing)))
        marker_addresses = [symbols[name] for name in required
                            if name not in {entry["case"] for entry in source_entries}]
        if len(set(marker_addresses)) != len(marker_addresses):
            raise RuntimeError("Hypertest injection marker PCs must be distinct")
        begins = {symbols[entry["begin_symbol"]]: entry for entry in entries}
        passes = {symbols[entry["pass_symbol"]]: entry for entry in entries}
        common_begin = symbols.get("dv_hypertest_begin") if source_entries else None
        common_pass = symbols.get("dv_hypertest_pass") if source_entries else None
        common_fail = symbols.get("dv_hypertest_fail") if source_entries else None
        lines = log.read_text(encoding="utf-8", errors="replace").splitlines()
        active = None
        common_start = None
        function_hits = 0
        illegal_traps = []
        ignored = set()
        case_hits = Counter()
        last_pc = None
        program_pass = False
        for index, line in enumerate(lines):
            if "***FAILED***" in line or "*** FAILED ***" in line:
                raise RuntimeError("Spike reports a failed program at line {}".format(index + 1))
            match = PC_RE.match(line)
            if match:
                pc = int(match.group(1), 16)
                last_pc = pc
                if pc == common_fail:
                    raise RuntimeError("Hypertest failed at Spike line {}".format(index + 1))
                if pc in begins:
                    if active is not None:
                        raise RuntimeError("Nested hypertest injection at Spike line {}".format(index + 1))
                    active = {"entry": begins[pc], "start": index, "common_passes": 0}
                    program_pass = False
                elif pc in passes:
                    entry = passes[pc]
                    if active is None or active["entry"]["id"] != entry["id"]:
                        raise RuntimeError("Hypertest injection pass without matching begin at line {}"
                                           .format(index + 1))
                    if common_start is not None:
                        raise RuntimeError("Injection returned before its source selfcheck passed")
                    if entry["kind"] == "source" and active["common_passes"] != 1:
                        raise RuntimeError("Source injection must complete exactly one bridge selfcheck")
                    record["injection_hits"][entry["id"]] += 1
                    case_hits[entry["case"]] += 1
                    record["injection_intervals"].append({
                        "injection_id": entry["id"], "kind": entry["kind"], "case": entry["case"],
                        "begin_line": active["start"] + 1, "pass_line": index + 1,
                        "original_selfcheck_passed": True})
                    active = None
                elif pc == common_begin:
                    if active is None or active["entry"]["kind"] != "source":
                        raise RuntimeError("Source bridge began outside its source injection")
                    if common_start is not None:
                        raise RuntimeError("Nested hypertest bridge begin")
                    common_start = index
                    function_hits = 0
                    illegal_traps = []
                elif pc == common_pass:
                    if common_start is None or active is None:
                        raise RuntimeError("Hypertest pass without begin at Spike line {}".format(index + 1))
                    if not function_hits:
                        raise RuntimeError("Hypertest returned without executing its selected case")
                    active["common_passes"] += 1
                    record["intervals"].append({
                        "injection_id": active["entry"]["id"],
                        "begin_line": common_start + 1, "pass_line": index + 1,
                        "case_hits": {active["entry"]["case"]: function_hits},
                        "original_selfcheck_passed": True, "illegal_traps": illegal_traps})
                    ignored.update(range(common_start, index + 1))
                    common_start = None
                elif (common_start is not None and active is not None and
                      pc == symbols[active["entry"]["case"]]):
                    function_hits += 1
                elif pc == symbols.get("dv_hypertest_program_pass") and active is None:
                    program_pass = True
            if common_start is not None and ILLEGAL_RE.search(line):
                following = lines[index + 1] if index + 1 < len(lines) else ""
                epc_match = re.search(r"\bepc\s+0x([0-9a-fA-F]+)\b", following)
                tval_match = re.search(r"\btval\s+0x([0-9a-fA-F]+)\b", following)
                if not tval_match:
                    raise RuntimeError("Hypertest illegal trap at line {} has no tval "
                                       "on its following line".format(index + 1))
                trap_pc = int(epc_match.group(1), 16) if epc_match else last_pc
                illegal_traps.append({"line": index + 1,
                                      "pc": "0x{:x}".format(trap_pc)
                                            if trap_pc is not None else None,
                                      "tval": "0x{:x}".format(int(tval_match.group(1), 16))})
            elif active is not None and active["entry"]["kind"] == "native" and ILLEGAL_RE.search(line):
                raise RuntimeError("Unexpected illegal instruction in native hypertest injection {} at line {}"
                                   .format(active["entry"]["id"], index + 1))
            mem_match = MEM_RE.search(line)
            if mem_match and int(mem_match.group(1), 16) == symbols.get("tohost"):
                value = int(mem_match.group(2), 16)
                if value and value != 1:
                    raise RuntimeError("Program wrote failing tohost value 0x{:x} at line {}"
                                       .format(value, index + 1))
                if value == 1 and active is None:
                    program_pass = True
        if active is not None or common_start is not None:
            raise RuntimeError("Hypertest injection begin has no successful return before end of Spike log")
        skipped = [entry["id"] for entry in entries if not record["injection_hits"][entry["id"]]]
        record["case_hits"] = dict(case_hits)
        record["skipped_injection_ids"] = skipped
        record["skipped_injection_count"] = len(skipped)
        skipped_ids = set(skipped)
        record["skipped_cases"] = dict(Counter(entry["case"] for entry in entries
                                               if entry["id"] in skipped_ids))
        if require_all and skipped:
            raise RuntimeError("Hypertest injection instances were skipped: " + ", ".join(skipped))
        if not program_pass:
            raise RuntimeError("Spike log has no normal DV tohost pass after the hypertest cases")
        completed = sum(record["injection_hits"].values())
        record["status"] = "not_reached" if not completed else "passed_partial" if skipped else "passed"
        record["program_pass"] = True
        return ignored
    except BaseException as exc:
        record["error"] = str(exc)
        raise
    finally:
        _write_json(artifact, record)


def validate_spike(log, elf, assembly, nm=None, require_all=True):
    """Validate real case execution; return 0-based lines owned by passed cases.

    These are the only lines the runner may exclude from its ordinary strict
    illegal-instruction checks. An expected illegal instruction inside a case
    remains guarded by the original case's assertions and successful return.
    An empty, skipped, failed or unterminated case is never an exemption.
    ``require_all=False`` permits ordinary random branches to skip injections,
    recording them as skipped/not_reached rather than claiming case coverage.
    """
    entries = injection_metadata(assembly)
    if entries:
        return _validate_spike_injections(log, elf, assembly, entries, nm, require_all)
    names = case_names(assembly)
    if not names:
        return set()
    log = Path(log)
    artifact = log.with_suffix(".hypertest-validation.json")
    record = {"assembly": str(assembly), "elf": str(elf), "log": str(log),
              "static_cases": names, "case_hits": {}, "intervals": [],
              "status": "failed",
              "pass_semantics": "original_selfcheck_passed; original case boolean/assertions "
                                "accepted the observed behavior, not a per-instruction ISA "
                                "or external-stimulus coverage claim"}
    try:
        symbols = _nm_symbols(elf, nm)
        required = {"dv_hypertest_begin", "dv_hypertest_pass", "dv_hypertest_fail"}
        missing = (required | set(names)) - symbols.keys()
        if missing:
            raise RuntimeError("Missing hypertest ELF symbols: " + ", ".join(sorted(missing)))
        begin_pc, pass_pc, fail_pc = (symbols[key] for key in
                                     ("dv_hypertest_begin", "dv_hypertest_pass",
                                      "dv_hypertest_fail"))
        if len({begin_pc, pass_pc, fail_pc}) != 3:
            raise RuntimeError("Hypertest execution marker symbols must be distinct")
        case_pcs = {}
        for name in dict.fromkeys(names):
            case_pcs.setdefault(symbols[name], []).append(name)
        lines = log.read_text(encoding="utf-8", errors="replace").splitlines()
        start = None
        current_hits = Counter()
        current_illegals = []
        hits = Counter()
        ignored_lines = set()
        last_pass = None
        program_pass = False
        last_pc = None
        for index, line in enumerate(lines):
            if "***FAILED***" in line or "*** FAILED ***" in line:
                raise RuntimeError("Spike reports a failed program at line {}".format(index + 1))
            match = PC_RE.match(line)
            if match:
                pc = int(match.group(1), 16)
                last_pc = pc
                if pc == fail_pc:
                    raise RuntimeError("Hypertest failed at Spike line {}".format(index + 1))
                if pc == begin_pc:
                    if start is not None:
                        raise RuntimeError("Nested hypertest begin at Spike line {}".format(index + 1))
                    start = index
                    current_hits.clear()
                    current_illegals.clear()
                    program_pass = False
                elif pc == pass_pc:
                    if start is None:
                        raise RuntimeError("Hypertest pass without begin at Spike line {}".format(index + 1))
                    if not current_hits:
                        raise RuntimeError("Hypertest returned without executing a selected case")
                    hits.update(current_hits)
                    record["intervals"].append({"begin_line": start + 1,
                                                "pass_line": index + 1,
                                                "case_hits": dict(current_hits),
                                                "original_selfcheck_passed": True,
                                                "illegal_traps": list(current_illegals)})
                    ignored_lines.update(range(start, index + 1))
                    last_pass = index
                    start = None
                elif start is not None:
                    current_hits.update(case_pcs.get(pc, []))
                elif pc == symbols.get("dv_hypertest_program_pass"):
                    program_pass = True
            if start is not None and ILLEGAL_RE.search(line):
                following = lines[index + 1] if index + 1 < len(lines) else ""
                epc_match = re.search(r"\bepc\s+0x([0-9a-fA-F]+)\b", following)
                tval_match = re.search(r"\btval\s+0x([0-9a-fA-F]+)\b", following)
                if not tval_match:
                    raise RuntimeError("Hypertest illegal trap at line {} has no tval "
                                       "on its following line".format(index + 1))
                trap_pc = int(epc_match.group(1), 16) if epc_match else last_pc
                current_illegals.append({"line": index + 1,
                                         "pc": "0x{:x}".format(trap_pc)
                                               if trap_pc is not None else None,
                                         "tval": "0x{:x}".format(int(tval_match.group(1), 16))})
            mem_match = MEM_RE.search(line)
            if mem_match and int(mem_match.group(1), 16) == symbols.get("tohost"):
                value = int(mem_match.group(2), 16)
                if value and value != 1:
                    raise RuntimeError("Program wrote failing tohost value 0x{:x} at line {}"
                                       .format(value, index + 1))
                if value == 1 and start is None:
                    program_pass = True
        if start is not None:
            raise RuntimeError("Hypertest begin has no successful return before end of Spike log")
        record["case_hits"] = dict(hits)
        missing_hits = Counter(names) - hits
        record["skipped_cases"] = dict(missing_hits)
        record["skipped_injection_count"] = max(
            sum(missing_hits.values()), len(names) - len(record["intervals"]))
        if require_all and not record["intervals"]:
            raise RuntimeError("No hypertest case executed in Spike")
        if require_all and missing_hits:
            raise RuntimeError("Hypertest injections were skipped: " +
                               ", ".join("{} ({} missing)".format(name, count)
                                         for name, count in sorted(missing_hits.items())))
        if require_all and len(record["intervals"]) < len(names):
            raise RuntimeError("Some static hypertest injections did not reach the bridge")
        if not program_pass:
            raise RuntimeError("Spike log has no normal DV tohost pass after the hypertest cases")
        if not record["intervals"]:
            record["status"] = "not_reached"
        elif record["skipped_injection_count"]:
            record["status"] = "passed_partial"
        else:
            record["status"] = "passed"
        record["program_pass"] = True
        return ignored_lines
    except BaseException as exc:
        record["error"] = str(exc)
        raise
    finally:
        _write_json(artifact, record)
