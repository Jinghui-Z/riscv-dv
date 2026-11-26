"""Validate execution of native process-switch streams in Spike retire logs.

Assembly markers describe what could run. Only committed begin/pass PCs and
normal HTIF completion establish that the generated self-check actually ran.
"""

from collections import Counter
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess


_COMMENT = r"^[ \t]*(?:(?:[A-Za-z_.$][A-Za-z_0-9.$]*|[0-9]+):[ \t]*)?#[ \t]*"
_SYMBOL = r"([A-Za-z_.$][A-Za-z_0-9.$]*)"
_INJECTION = re.compile(_COMMENT + r"PROCESS_SWITCH_INJECTION:[ \t]*" +
                        _SYMBOL + r"[ \t]+" + _SYMBOL + r"[ \t]*$", re.MULTILINE)
_SCENARIO = re.compile(_COMMENT + r"PROCESS_SWITCH_SCENARIO:[ \t]*" +
                       _SYMBOL + r"[ \t]+" + _SYMBOL + r"[ \t]*$", re.MULTILINE)
_FAIL = re.compile(_COMMENT + r"PROCESS_SWITCH_FAIL:[ \t]*" +
                   _SYMBOL + r"[ \t]*$", re.MULTILINE)
_COMMIT = re.compile(r"^core\s+0:\s+[0-3]\s+0x([0-9a-fA-F]+)\s+\(0x[0-9a-fA-F]+\)")
_WRITE = re.compile(r"\bmem\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\b")


def _nm_symbols(elf):
    """Use the selected RISC-V toolchain without invoking a shell."""
    nm = os.environ.get("RISCV_NM")
    if nm:
        command = [nm] if Path(nm).is_file() else shlex.split(nm)
    else:
        gcc = os.environ.get("RISCV_GCC", "")
        gcc_command = [gcc] if Path(gcc).is_file() else shlex.split(gcc)
        candidate = next((part[:-3] + "nm" for part in reversed(gcc_command)
                          if part.endswith("gcc")), None)
        executable = ((candidate if candidate and shutil.which(candidate) else None)
                      or shutil.which("riscv64-unknown-elf-nm") or shutil.which("nm"))
        if not executable:
            raise RuntimeError("No nm executable available for process-switch validation")
        command = [executable]
    try:
        result = subprocess.run(command + ["-n", str(elf)], text=True,
                                capture_output=True, check=False)
    except OSError as exc:
        raise RuntimeError("Cannot inspect process-switch ELF: {}".format(exc)) from exc
    if result.returncode:
        raise RuntimeError("Cannot inspect process-switch ELF: " + result.stderr.strip())
    symbols = {}
    for line in result.stdout.splitlines():
        match = re.match(r"^\s*([0-9a-fA-F]+)\s+\S\s+(\S+)\s*$", line)
        if match:
            symbols[match.group(2)] = int(match.group(1), 16)
    return symbols


def _metadata(text):
    found = []
    for kind, pattern in (("INJECTION", _INJECTION), ("SCENARIO", _SCENARIO),
                          ("FAIL", _FAIL)):
        entries = pattern.findall(text)
        count = len(re.findall(_COMMENT + "PROCESS_SWITCH_" + kind + ":", text,
                               re.MULTILINE))
        if len(entries) != count:
            raise RuntimeError("Malformed PROCESS_SWITCH_{} metadata".format(kind))
        found.append(entries)
    injections, scenarios, failures = found
    boundaries = [symbol for pair in injections for symbol in pair]
    if len(boundaries) != len(set(boundaries)):
        raise RuntimeError("Process-switch begin/pass symbols must be distinct")
    scenario_symbols = [symbol for _, symbol in scenarios]
    if len(scenario_symbols) != len(set(scenario_symbols)):
        raise RuntimeError("Process-switch scenario symbols must be distinct")
    if (len(failures) != len(set(failures)) or
            set(failures) & (set(boundaries) | set(scenario_symbols))):
        raise RuntimeError("Process-switch fail symbols must be distinct")
    if not injections and (scenarios or failures):
        raise RuntimeError("Process-switch scenario/fail metadata has no injection")
    return injections, scenarios, failures


def validate_spike(log, elf, assembly, minimum_hits=0):
    """Return execution evidence, or raise RuntimeError for incomplete checks.

    Branches may skip a whole injection. Such an injection gets zero hits and
    is reported as skipped, while a started injection must reach its own pass.
    This function grants no exemptions from the runner's strict trap checker.
    """
    if type(minimum_hits) is not int or minimum_hits < 0:
        raise RuntimeError("min_process_switch_hits must be a non-negative integer")
    try:
        text = Path(assembly).read_text(encoding="utf-8")
    except FileNotFoundError as exc:
        # Directed asm_test/c_test flows do not create the generator's
        # prefix.S. They have no native injection unless explicitly required.
        if minimum_hits == 0:
            return {}
        raise RuntimeError("Cannot read required process-switch assembly: {}".format(exc)) from exc
    except OSError as exc:
        raise RuntimeError("Cannot read process-switch assembly: {}".format(exc)) from exc
    injections, scenarios, failures = _metadata(text)
    if not injections:
        if minimum_hits:
            raise RuntimeError("No process-switch injections generated; minimum is {}"
                               .format(minimum_hits))
        return {}

    log = Path(log)
    artifact = log.with_suffix(".process-switch-validation.json")
    record = {"assembly": str(assembly), "elf": str(elf), "log": str(log),
              "status": "failed", "injection_hits": {}, "scenario_hits": {},
              "skipped_injections": [], "intervals": [], "minimum_hits": minimum_hits,
              "pass_semantics": "native self-check returned and normal DV HTIF pass; "
                                "scenario hits are retired entry markers"}
    try:
        symbols = _nm_symbols(elf)
        required = {symbol for pair in injections for symbol in pair}
        required.update(symbol for _, symbol in scenarios)
        required.update(failures)
        required.add("tohost")
        missing = required - symbols.keys()
        if missing:
            raise RuntimeError("Missing process-switch ELF symbols: " + ", ".join(sorted(missing)))
        boundaries = [symbol for pair in injections for symbol in pair]
        if len({symbols[symbol] for symbol in boundaries}) != len(boundaries):
            raise RuntimeError("Process-switch begin/pass PCs must be distinct")
        begin_pcs = {symbols[begin]: begin for begin, _ in injections}
        pass_pcs = {symbols[passed]: begin for begin, passed in injections}
        fail_pcs = {symbols[symbol] for symbol in failures}
        if fail_pcs & (set(begin_pcs) | set(pass_pcs)):
            raise RuntimeError("Process-switch fail PCs overlap begin/pass PCs")
        scenario_pcs = {}
        for name, symbol in scenarios:
            scenario_pcs.setdefault(symbols[symbol], []).append(name)
        hits = Counter({begin: 0 for begin, _ in injections})
        scenario_hits = Counter({name: 0 for name, _ in scenarios})
        active = None
        active_line = None
        current_scenarios = Counter()
        completed_scenarios = Counter()
        program_pass = False
        with log.open(encoding="utf-8", errors="replace") as log_file:
            for index, line in enumerate(log_file, 1):
                if "***FAILED***" in line or "*** FAILED ***" in line:
                    raise RuntimeError("Spike reports failure at line {}".format(index))
                if active and "exception trap_illegal_instruction" in line:
                    raise RuntimeError("Illegal instruction inside process-switch injection at line {}"
                                       .format(index))
                match = _COMMIT.match(line)
                if not match:
                    continue
                pc = int(match.group(1), 16)
                if pc in fail_pcs:
                    raise RuntimeError("Process-switch fail marker executed at line {}".format(index))
                if pc in begin_pcs:
                    if active is not None:
                        raise RuntimeError("Nested process-switch begin at line {}".format(index))
                    active = begin_pcs[pc]
                    active_line = index
                    current_scenarios.clear()
                    program_pass = False
                if pc in scenario_pcs:
                    if active is None:
                        raise RuntimeError("Process-switch scenario outside injection at line {}"
                                           .format(index))
                    current_scenarios.update(scenario_pcs[pc])
                if pc in pass_pcs:
                    if active != pass_pcs[pc]:
                        raise RuntimeError("Process-switch pass without matching begin at line {}"
                                           .format(index))
                    hits[active] += 1
                    completed_scenarios.update(current_scenarios)
                    record["intervals"].append({"injection": active, "begin_line": active_line,
                                                "pass_line": index,
                                                "scenario_hits": dict(current_scenarios)})
                    active = None
                for write in _WRITE.finditer(line):
                    if int(write.group(1), 16) == symbols["tohost"]:
                        value = int(write.group(2), 16)
                        if value not in (0, 1):
                            raise RuntimeError("Program wrote failing tohost value 0x{:x} at line {}"
                                               .format(value, index))
                        if value == 1 and active is None:
                            program_pass = True
        record["injection_hits"] = dict(hits)
        scenario_hits.update(completed_scenarios)
        record["scenario_hits"] = dict(scenario_hits)
        record["skipped_injections"] = [name for name, count in hits.items() if not count]
        if active is not None:
            raise RuntimeError("Process-switch begin {} has no successful return".format(active))
        if sum(hits.values()) < minimum_hits:
            raise RuntimeError("Only {} process-switch injections executed; minimum is {}"
                               .format(sum(hits.values()), minimum_hits))
        if not program_pass:
            raise RuntimeError("Spike log has no normal DV tohost pass after process-switch checks")
        record["program_pass"] = True
        record["status"] = ("not_reached" if not sum(hits.values()) else
                            "passed_partial" if record["skipped_injections"] else "passed")
        return record
    except (OSError, ValueError) as exc:
        record["error"] = str(exc)
        raise RuntimeError("Cannot validate process-switch execution: {}".format(exc)) from exc
    except RuntimeError as exc:
        record["error"] = str(exc)
        raise
    finally:
        artifact.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
