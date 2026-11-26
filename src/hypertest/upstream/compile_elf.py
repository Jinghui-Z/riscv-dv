#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
RISC-V 单独编译测试脚本 (Python版本)

用法示例:
  python3 compile_elf.py [--plat PLAT] 250 300
  python3 compile_elf.py [--plat PLAT] all
  python3 compile_elf.py [--plat PLAT] --include-commented --match '^ai_'
  python3 compile_elf.py [--plat PLAT] --include-commented --name-file ai_names.txt
  python3 compile_elf.py [--plat PLAT] --name case_a --name case_b --jobs 8

PLAT 可选: spike(默认), nemu, linknan
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

try:
    import fcntl
except ImportError:  # pragma: no cover - this repo is normally used on Linux.
    fcntl = None

# 配置
REPO_ROOT = Path(__file__).resolve().parent
SOURCE_FILE = REPO_ROOT / "test_register.c"
OUTPUT_DIR = REPO_ROOT / "case_elf_asm"
VALID_PLATS = ["spike", "nemu", "linknan"]
DEFAULT_PLAT = "spike"
DEFAULT_CROSS_COMPILE = "riscv64-unknown-elf-"
ARTIFACT_MAP_FILE = "artifact_name_map.json"
ARTIFACT_SUFFIXES = (".ELF", ".asm")
FALLBACK_NAME_MAX = 255
ALIAS_HASH_LEN = 16
ALIAS_SEPARATOR = "__"

TEST_REGISTER_RE = re.compile(
    r"^(?P<indent>\s*)(?://\s*)?TEST_REGISTER\((?P<name>[^)]+)\)\s*;?\s*(?://.*)?$"
)
ACTIVE_TEST_REGISTER_RE = re.compile(r"^\s*TEST_REGISTER\(")
DEFAULT_TMP_ROOT = REPO_ROOT / ".tmp" / "hyptest_compile"
DEFAULT_TMPDIR = DEFAULT_TMP_ROOT / "tmp"
FAST_REGISTER_DIR = DEFAULT_TMP_ROOT / "register"
LOCK_DIR = DEFAULT_TMP_ROOT / "locks"
DEFAULT_JOBS = max(1, min(os.cpu_count() or 1, 16))
MAKE_OUTPUT_TAIL_LINES = 80
MAX_TEST_LIST_LINES = 120
MAX_SOURCE_ERROR_LINES = 50
PROGRESS_BAR_WIDTH = 32
PROGRESS_LOG_INTERVAL_SEC = 5.0
PROGRESS_LOG_STEP = 50
CASE_SOURCE_DIRS = ("manual_test_cases", "ai_test_cases")
CASE_DEF_RE = re.compile(r"^\s*bool\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(")
CASE_REF_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
ASM_GLOBAL_RE = re.compile(r"^\s*\.global\s+([A-Za-z_.$][A-Za-z0-9_.$]*)\b")
ASM_SYMBOL_RE = re.compile(r"\b([A-Za-z_.$][A-Za-z0-9_.$]*)\b")
C_BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)
C_LINE_COMMENT_RE = re.compile(r"//.*?$", re.MULTILINE)
C_STRING_OR_CHAR_RE = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


@dataclass(frozen=True)
class RegisteredTest:
    line_num: int
    name: str
    active: bool
    indent: str


@dataclass(frozen=True)
class CaseSourcePlan:
    c_sources: tuple
    asm_sources: tuple


@dataclass(frozen=True)
class BuildFailure:
    case_name: str
    reason: str
    command: tuple = ()
    stdout: str = ""
    stderr: str = ""


def parse_register_line(line):
    """解析单行 TEST_REGISTER，返回 (indent, name, active) 或 None。"""
    match = TEST_REGISTER_RE.match(line.rstrip("\n"))
    if not match:
        return None

    return (
        match.group("indent"),
        match.group("name").strip(),
        ACTIVE_TEST_REGISTER_RE.match(line) is not None,
    )


def load_source_lines():
    """读取 test_register.c 的全部行。"""
    try:
        with SOURCE_FILE.open("r", encoding="utf-8") as f:
            return f.readlines()
    except FileNotFoundError:
        print(f"错误: 找不到文件 {SOURCE_FILE}")
        return None


def load_registered_tests():
    """读取并解析 test_register.c 中所有 TEST_REGISTER 行。"""
    lines = load_source_lines()
    if lines is None:
        return None, None

    tests = []
    for idx, line in enumerate(lines, 1):
        parsed = parse_register_line(line)
        if not parsed:
            continue
        indent, name, active = parsed
        tests.append(RegisteredTest(idx, name, active, indent))

    return lines, tests


def get_component_name_max(path):
    """返回目录允许的单文件名最大长度。"""
    try:
        name_max = os.pathconf(path, "PC_NAME_MAX")
        if isinstance(name_max, int) and name_max > 0:
            return name_max
    except (AttributeError, OSError, ValueError):
        pass
    return FALLBACK_NAME_MAX


def make_empty_artifact_map():
    """创建空的产物映射结构。"""
    return {
        "version": 1,
        "artifacts": {},
    }


def load_artifact_map(output_path):
    """读取平台产物映射。"""
    map_path = output_path / ARTIFACT_MAP_FILE
    if not map_path.exists():
        return make_empty_artifact_map()

    try:
        with map_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError):
        return make_empty_artifact_map()

    artifacts = data.get("artifacts")
    if not isinstance(artifacts, dict):
        artifacts = {}

    normalized = {}
    for case_name, artifact_stem in artifacts.items():
        if isinstance(case_name, str) and isinstance(artifact_stem, str):
            normalized[case_name] = artifact_stem

    return {
        "version": 1,
        "artifacts": normalized,
    }


def save_artifact_map(output_path, plat, artifact_map):
    """保存平台产物映射。"""
    map_path = output_path / ARTIFACT_MAP_FILE
    payload = {
        "version": 1,
        "platform": plat,
        "artifacts": artifact_map["artifacts"],
    }
    with map_path.open("w", encoding="utf-8") as f:
        json.dump(payload, f, ensure_ascii=True, indent=2, sort_keys=True)
        f.write("\n")


def display_path(path):
    """尽量用仓库相对路径显示，输出更短也更稳定。"""
    path = Path(path)
    try:
        return str(path.resolve().relative_to(REPO_ROOT))
    except (OSError, ValueError):
        return str(path)


def read_env_value(*names):
    """读取第一个非空环境变量。"""
    for name in names:
        value = os.environ.get(name)
        if value and value.strip():
            return value.strip()
    return ""


def resolve_input_path(path_text, base=REPO_ROOT):
    """解析用户输入路径；相对路径统一按仓库根目录处理。"""
    path = Path(os.path.expandvars(str(path_text))).expanduser()
    if path.is_absolute():
        return path
    return base / path


def normalize_case_name_token(token):
    """把命令行/文件中的 case token 规整成 TEST_REGISTER 名字。"""
    name = token.strip()
    if not name:
        return ""
    name = name.split("#", 1)[0].split("//", 1)[0].strip()
    if not name:
        return ""
    name = Path(name).name
    if name.endswith(".ELF"):
        name = name[:-4]
    return name.strip()


def resolve_artifact_stem(test_name, output_path, artifact_map):
    """为 case 生成可落盘的稳定产物名。"""
    max_suffix_len = max(len(suffix) for suffix in ARTIFACT_SUFFIXES)
    name_max = get_component_name_max(output_path)
    max_stem_len = max(1, name_max - max_suffix_len)

    existing = artifact_map["artifacts"].get(test_name)
    if isinstance(existing, str) and len(existing) <= max_stem_len:
        return existing

    if len(test_name) <= max_stem_len:
        return test_name

    taken_stems = {
        stem: case_name
        for case_name, stem in artifact_map["artifacts"].items()
        if case_name != test_name
    }
    digest = hashlib.sha1(test_name.encode("utf-8")).hexdigest()

    for hash_len in range(ALIAS_HASH_LEN, len(digest) + 1):
        suffix = f"{ALIAS_SEPARATOR}{digest[:hash_len]}"
        prefix_len = max_stem_len - len(suffix)
        if prefix_len <= 0:
            continue
        artifact_stem = f"{test_name[:prefix_len]}{suffix}"
        owner = taken_stems.get(artifact_stem)
        if owner is None or owner == test_name:
            return artifact_stem

    raise RuntimeError(f"无法为测试名生成稳定短别名: {test_name}")


def print_usage():
    """打印使用说明。"""
    print("RISC-V 单独编译测试脚本 (Python版本)")
    print("=" * 40)
    print("用法:")
    print(f"  {sys.argv[0]} [--plat PLAT] 250 300")
    print(f"  {sys.argv[0]} [--plat PLAT] all")
    print(
        f"  {sys.argv[0]} [--plat PLAT] --include-commented --match '^ai_'"
    )
    print(
        f"  {sys.argv[0]} [--plat PLAT] --include-commented --name-file ai_names.txt"
    )
    print(f"  {sys.argv[0]} [--plat PLAT] --name case_a --name case_b --jobs 8")
    print(f"  {sys.argv[0]} [--plat PLAT] --include-commented all --plan-only")
    print()
    print(f"PLAT 可选: {', '.join(VALID_PLATS)}  (默认: {DEFAULT_PLAT})")
    print("默认不会自动 make clean，如需首次 clean 请显式加 --clean。")
    print("默认使用临时小注册源文件，不改写 test_register.c；如需旧行为加 --rewrite-register。")
    print(f"默认 CROSS_COMPILE 为 {DEFAULT_CROSS_COMPILE}，可用环境变量覆盖。")
    print(f"默认 TMPDIR 为 {display_path(DEFAULT_TMPDIR)}，避免 gcc 临时文件写爆系统 /tmp。")
    print(f"默认 make 并行任务数为 {DEFAULT_JOBS}，可用 --jobs 调整。")
    print("默认 LOG_LEVEL=4，可用 --log-level 2 减少 RTL 仿真的 UART 输出。")
    print("默认不生成 .o.c/readelf/bin；如需调试加 --keep-preprocessed --keep-readelf --keep-bin。")
    print("--plan-only 只打印 case 到源文件的链接计划，不执行 make。")
    print()

    lines, tests = load_registered_tests()
    if lines is None:
        return

    print("当前 test_register.c 中的 TEST_REGISTER 行示例:")
    for test in tests[:10]:
        prefix = "" if test.active else "// "
        print(f"{test.line_num}:{prefix}TEST_REGISTER({test.name});")
    if len(tests) > 10:
        print("...")


def build_parser():
    """构建命令行参数解析器。"""
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--plat", default=DEFAULT_PLAT)
    parser.add_argument("--log-level", type=int, choices=range(6), default=4,
                        help="Harness LOG_LEVEL passed to make (default: 4).")
    parser.add_argument(
        "--include-commented",
        action="store_true",
        help="选择范围中包含已注释的 TEST_REGISTER。",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="开始批量编译前先执行一次 make clean。",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=DEFAULT_JOBS,
        help=f"传给 make 的并行任务数，默认 CPU 数量 ({DEFAULT_JOBS})。",
    )
    parser.add_argument(
        "--tmpdir",
        default=read_env_value("HYPTEST_TMPDIR", "TMPDIR") or str(DEFAULT_TMPDIR),
        help="编译临时目录，会传给 make/gcc 的 TMPDIR，默认 .tmp/hyptest_compile/tmp。",
    )
    parser.add_argument(
        "--cross-compile",
        default=read_env_value("HYPTEST_CROSS_COMPILE", "CROSS_COMPILE") or DEFAULT_CROSS_COMPILE,
        help=f"RISC-V 工具链前缀，默认 {DEFAULT_CROSS_COMPILE}，也可用 HYPTEST_CROSS_COMPILE/CROSS_COMPILE 覆盖。",
    )
    parser.add_argument(
        "--rewrite-register",
        action="store_true",
        help="兼容旧行为：逐 case 改写 test_register.c。默认使用临时小注册源文件，不修改 test_register.c。",
    )
    parser.add_argument(
        "--keep-preprocessed",
        action="store_true",
        help="保留每个对象的预处理 .o.c 调试文件。默认关闭以减少编译 IO。",
    )
    parser.add_argument(
        "--keep-readelf",
        action="store_true",
        help="生成 build/<plat>/rvh_test.elf.txt。默认关闭以减少批量编译开销。",
    )
    parser.add_argument(
        "--skip-asm",
        action="store_true",
        help="不生成/复制 asm 反汇编，只导出 ELF。默认仍保留 asm，方便调试。",
    )
    parser.add_argument(
        "--keep-bin",
        action="store_true",
        help="生成 build/<plat>/rvh_test.bin。默认不生成，case gate 只需要导出的 ELF/asm。",
    )
    parser.add_argument(
        "--plan-only",
        action="store_true",
        help="只解析选择结果和链接源文件计划，不执行 make。",
    )
    parser.add_argument(
        "--name",
        action="append",
        default=[],
        help="按测试名精确选择，可重复传入。",
    )
    parser.add_argument(
        "--name-file",
        action="append",
        default=[],
        help="从文件读取测试名，一行一个，支持 # 注释，可重复传入。",
    )
    parser.add_argument(
        "--match",
        action="append",
        default=[],
        help="按正则匹配测试名，可重复传入。",
    )
    parser.add_argument(
        "--exclude",
        action="append",
        default=[],
        help="按正则排除测试名，可重复传入。",
    )
    parser.add_argument("--help", action="store_true")
    parser.add_argument("targets", nargs="*")
    return parser


def load_names_from_files(paths):
    """从名字文件读取测试名。"""
    names = []
    for path_str in paths:
        path = resolve_input_path(path_str)
        try:
            with path.open("r", encoding="utf-8") as f:
                for raw in f:
                    line = normalize_case_name_token(raw)
                    if not line:
                        continue
                    names.append(line)
        except FileNotFoundError:
            raise FileNotFoundError(f"找不到名字文件: {display_path(path)}")
    return names


def compile_patterns(patterns, flag_name):
    """编译正则表达式。"""
    compiled = []
    for pattern in patterns:
        try:
            compiled.append(re.compile(pattern))
        except re.error as err:
            raise ValueError(f"{flag_name} 正则无效: {pattern} ({err})") from err
    return compiled


def parse_args():
    """解析命令行参数。"""
    parser = build_parser()
    args = parser.parse_args()

    if args.help:
        print_usage()
        return {"help": True}

    if args.plat not in VALID_PLATS:
        print(f"错误: 无效的平台 '{args.plat}'，可选: {', '.join(VALID_PLATS)}")
        print_usage()
        return None

    try:
        file_names = load_names_from_files(args.name_file)
        include_patterns = compile_patterns(args.match, "--match")
        exclude_patterns = compile_patterns(args.exclude, "--exclude")
    except (FileNotFoundError, ValueError) as err:
        print(f"错误: {err}")
        return None

    positive_selectors = bool(args.name or file_names or include_patterns)

    if args.targets:
        if args.targets[0] == "all":
            if len(args.targets) != 1:
                print("错误: all 模式不能再追加其他位置参数")
                print_usage()
                return None
            mode = "all"
            start_line = None
            end_line = None
        else:
            if len(args.targets) > 2:
                print("错误: 行号模式最多支持两个位置参数")
                print_usage()
                return None
            try:
                start_line = int(args.targets[0])
                end_line = int(args.targets[1]) if len(args.targets) == 2 else start_line
            except ValueError:
                print("错误: 行号模式的位置参数必须是整数")
                print_usage()
                return None
            if start_line <= 0 or end_line <= 0 or end_line < start_line:
                print("错误: 行号范围不合法")
                return None
            mode = "range"
    elif positive_selectors:
        mode = "select"
        start_line = None
        end_line = None
    else:
        print_usage()
        return None

    return {
        "plat": args.plat,
        "mode": mode,
        "start_line": start_line,
        "end_line": end_line,
        "include_commented": args.include_commented,
        "clean": args.clean,
        "jobs": max(1, args.jobs),
        "log_level": args.log_level,
        "tmpdir": resolve_input_path(args.tmpdir),
        "cross_compile": args.cross_compile,
        "rewrite_register": args.rewrite_register,
        "keep_preprocessed": args.keep_preprocessed,
        "keep_readelf": args.keep_readelf,
        "skip_asm": args.skip_asm,
        "keep_bin": args.keep_bin,
        "plan_only": args.plan_only,
        "exact_names": set(
            name
            for name in (
                normalize_case_name_token(raw_name)
                for raw_name in args.name + file_names
            )
            if name
        ),
        "include_patterns": include_patterns,
        "exclude_patterns": exclude_patterns,
    }


def filter_by_mode(tests, options):
    """按 all / range / select 三种模式筛出候选集。"""
    if options["include_commented"]:
        visible_tests = list(tests)
    else:
        visible_tests = [test for test in tests if test.active]

    mode = options["mode"]
    if mode == "all" or mode == "select":
        return visible_tests

    start_line = options["start_line"]
    end_line = options["end_line"]
    return [
        test for test in visible_tests
        if start_line <= test.line_num <= end_line
    ]


def apply_name_filters(tests, options):
    """按 name / match / exclude 过滤测试。"""
    selected = list(tests)
    exact_names = options["exact_names"]
    include_patterns = options["include_patterns"]
    exclude_patterns = options["exclude_patterns"]

    if exact_names:
        selected = [test for test in selected if test.name in exact_names]

    if include_patterns:
        selected = [
            test for test in selected
            if any(pattern.search(test.name) for pattern in include_patterns)
        ]

    if exclude_patterns:
        selected = [
            test for test in selected
            if not any(pattern.search(test.name) for pattern in exclude_patterns)
        ]

    return selected


def select_tests(tests, options):
    """综合参数选择最终测试集。"""
    selected = filter_by_mode(tests, options)
    selected = apply_name_filters(selected, options)
    selected.sort(key=lambda test: test.line_num)
    return selected


def dedupe_selected_tests(tests):
    """按 case 名去重，避免 all/include-commented 重复编译同一函数。"""
    selected = []
    seen = {}
    duplicates = []

    for test in tests:
        first = seen.get(test.name)
        if first is not None:
            duplicates.append((test, first))
            continue
        seen[test.name] = test
        selected.append(test)

    return selected, duplicates


def print_selected_tests(tests):
    """打印选中测试，范围很大时只显示头尾，避免日志被测试清单淹没。"""
    print(f"找到以下测试 ({len(tests)} 个):")
    if len(tests) <= MAX_TEST_LIST_LINES:
        visible = tests
        omitted = 0
    else:
        head_count = MAX_TEST_LIST_LINES - 20
        visible = tests[:head_count]
        omitted = len(tests) - MAX_TEST_LIST_LINES

    for test in visible:
        prefix = "" if test.active else "// "
        print(f"{test.line_num}:{prefix}TEST_REGISTER({test.name});")

    if len(tests) > MAX_TEST_LIST_LINES:
        print(f"... 省略 {omitted} 个测试 ...")
        for test in tests[-20:]:
            prefix = "" if test.active else "// "
            print(f"{test.line_num}:{prefix}TEST_REGISTER({test.name});")


def build_case_source_index():
    """扫描测试源文件，建立 case 函数名到源文件的映射。"""
    case_to_source = {}
    duplicates = {}
    for dirname in CASE_SOURCE_DIRS:
        source_root = REPO_ROOT / dirname
        if not source_root.exists():
            continue
        for path in sorted(source_root.rglob("*.c")):
            try:
                lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
            except OSError:
                continue
            for line_num, line in enumerate(lines, 1):
                match = CASE_DEF_RE.match(line)
                if not match or not is_case_definition(lines, line_num - 1):
                    continue
                case_name = match.group(1)
                if case_name in case_to_source:
                    duplicates.setdefault(case_name, [case_to_source[case_name]]).append(
                        (path, line_num)
                    )
                else:
                    case_to_source[case_name] = (path, line_num)
    return case_to_source, duplicates


def build_asm_symbol_index():
    """扫描手写测试汇编，建立全局符号到 .S 源文件的映射。"""
    symbol_to_source = {}
    duplicates = {}

    for dirname in CASE_SOURCE_DIRS:
        source_root = REPO_ROOT / dirname
        if not source_root.exists():
            continue
        for path in sorted(source_root.rglob("*.S")):
            try:
                lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
            except OSError:
                continue
            for line_num, line in enumerate(lines, 1):
                match = ASM_GLOBAL_RE.match(line)
                if not match:
                    continue
                symbol = match.group(1)
                if symbol in symbol_to_source:
                    duplicates.setdefault(symbol, [symbol_to_source[symbol]]).append(
                        (path, line_num)
                    )
                else:
                    symbol_to_source[symbol] = (path, line_num)

    return symbol_to_source, duplicates


def strip_line_comments(line):
    """去掉单行 // 注释；这里只用于区分 prototype 和函数定义。"""
    return line.split("//", 1)[0].strip()


def is_case_definition(lines, start_index):
    """判断 bool case_name(...) 是否是真函数定义，而不是前向声明。"""
    paren_depth = 0
    saw_open = False

    for line in lines[start_index:]:
        code = strip_line_comments(line)
        if not code:
            continue
        for char in code:
            if char == "(":
                paren_depth += 1
                saw_open = True
            elif char == ")" and paren_depth > 0:
                paren_depth -= 1
            elif saw_open and paren_depth == 0:
                if char == "{":
                    return True
                if char == ";":
                    return False

        if saw_open and paren_depth == 0:
            if "{" in code:
                return True
            if ";" in code:
                return False

    return False


def strip_c_non_code(text):
    """移除注释和字符串，避免依赖扫描被说明文字误导。"""
    text = C_BLOCK_COMMENT_RE.sub(" ", text)
    text = C_LINE_COMMENT_RE.sub(" ", text)
    text = C_STRING_OR_CHAR_RE.sub('""', text)
    return text


def find_indexed_references(path, token_re, source_index, duplicates, cache=None):
    """查找源码中引用到的已索引符号。"""
    if cache is not None and path in cache:
        return cache[path]

    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        if cache is not None:
            cache[path] = frozenset()
        return set()

    references = set()
    for match in token_re.finditer(strip_c_non_code(text)):
        name = match.group(1)
        if name in source_index and name not in duplicates:
            references.add(name)

    references = frozenset(references)
    if cache is not None:
        cache[path] = references
    return references


def find_asm_references(path, symbol_to_source, duplicates, asm_reference_cache=None):
    """查找 C 源文件中引用到的手写汇编全局符号。"""
    return find_indexed_references(
        path,
        ASM_SYMBOL_RE,
        symbol_to_source,
        duplicates,
        cache=asm_reference_cache,
    )


def find_case_references(path, case_to_source, duplicates, reference_cache=None):
    """查找一个测试源文件中引用到的其它 case 函数名。"""
    return find_indexed_references(
        path,
        CASE_REF_RE,
        case_to_source,
        duplicates,
        cache=reference_cache,
    )


def expand_case_source_dependencies(
    source_paths,
    case_to_source,
    duplicates,
    reference_cache=None,
):
    """把选中 case 源文件中直接/间接调用的其它 case 源文件也纳入链接。"""
    resolved = set(source_paths)
    pending = list(source_paths)

    while pending:
        path = pending.pop()
        for name in find_case_references(
            path,
            case_to_source,
            duplicates,
            reference_cache=reference_cache,
        ):
            dep_path = case_to_source[name][0]
            if dep_path in resolved:
                continue
            resolved.add(dep_path)
            pending.append(dep_path)

    return sorted(resolved, key=lambda p: display_path(p))


def resolve_case_asm_sources(
    case_sources,
    symbol_to_source,
    asm_duplicates,
    asm_reference_cache,
):
    """根据 case C 源文件引用的汇编全局符号，找到需要链接的 .S 源文件。"""
    asm_sources = set()

    for path in case_sources:
        for symbol in find_asm_references(
            path,
            symbol_to_source,
            asm_duplicates,
            asm_reference_cache=asm_reference_cache,
        ):
            asm_sources.add(symbol_to_source[symbol][0])

    return sorted(asm_sources, key=lambda p: display_path(p))


def format_case_source_locations(locations):
    """把重复定义位置格式化成短字符串。"""
    return ", ".join(f"{display_path(path)}:{line_num}" for path, line_num in locations)


def resolve_case_sources_for_test(
    test,
    case_to_source,
    duplicates,
    reference_cache,
    dependency_cache,
    symbol_to_source,
    asm_duplicates,
    asm_reference_cache,
    asm_dependency_cache,
):
    """为单个 case 找到需要链接的测试源文件，不再回退全工程链接。"""
    if test.name in duplicates:
        return None, (
            "case 函数定义不唯一: "
            f"{format_case_source_locations(duplicates[test.name])}"
        )

    source_info = case_to_source.get(test.name)
    if source_info is None:
        return None, "未找到唯一的 bool case_name(...) 函数定义"

    source_path = source_info[0]
    case_sources = dependency_cache.get(source_path)
    if case_sources is None:
        case_sources = expand_case_source_dependencies(
            [source_path],
            case_to_source,
            duplicates,
            reference_cache=reference_cache,
        )
        dependency_cache[source_path] = case_sources

    case_asm_sources = asm_dependency_cache.get(tuple(case_sources))
    if case_asm_sources is None:
        case_asm_sources = resolve_case_asm_sources(
            case_sources,
            symbol_to_source,
            asm_duplicates,
            asm_reference_cache,
        )
        asm_dependency_cache[tuple(case_sources)] = case_asm_sources
    return (
        CaseSourcePlan(
            c_sources=tuple(case_sources),
            asm_sources=tuple(case_asm_sources),
        ),
        "",
    )


def build_case_source_plan(selected_tests):
    """为选中 case 预先建立逐 case 源文件链接计划。"""
    case_to_source, duplicates = build_case_source_index()
    symbol_to_source, asm_duplicates = build_asm_symbol_index()
    reference_cache = {}
    dependency_cache = {}
    asm_reference_cache = {}
    asm_dependency_cache = {}
    source_plan = {}
    source_errors = {}

    for test in selected_tests:
        if test.name in source_plan or test.name in source_errors:
            continue
        case_plan, error = resolve_case_sources_for_test(
            test,
            case_to_source,
            duplicates,
            reference_cache,
            dependency_cache,
            symbol_to_source,
            asm_duplicates,
            asm_reference_cache,
            asm_dependency_cache,
        )
        if error:
            source_errors[test.name] = error
        else:
            source_plan[test.name] = case_plan

    return source_plan, source_errors


def summarize_source_sets(source_plan):
    """汇总不同 case 共用的源文件集合。"""
    return {
        (
            tuple(display_path(path) for path in plan.c_sources),
            tuple(display_path(path) for path in plan.asm_sources),
        )
        for plan in source_plan.values()
    }


def print_source_plan_summary(source_plan, source_errors):
    """打印链接计划摘要，避免 main 中堆太多展示逻辑。"""
    planned_source_sets = summarize_source_sets(source_plan)
    print("链接策略: 逐 case 仅链接其所在测试源文件")
    print(f"  可定位 case: {len(source_plan)}")
    print(f"  不可定位/重名 case: {len(source_errors)}")
    print(f"  唯一源文件集合: {len(planned_source_sets)}")
    if source_errors:
        print("  以下 case 将跳过，避免回退全工程链接:")
        for idx, (name, reason) in enumerate(source_errors.items(), 1):
            if idx > MAX_SOURCE_ERROR_LINES:
                print(f"  ... 省略 {len(source_errors) - MAX_SOURCE_ERROR_LINES} 个 ...")
                break
            print(f"  - {name}: {reason}")
    else:
        sample_sources = sorted(planned_source_sets)[:10]
        if sample_sources:
            print("  源文件集合示例:")
            for c_sources, asm_sources in sample_sources:
                parts = []
                if c_sources:
                    parts.append("C: " + ", ".join(c_sources))
                if asm_sources:
                    parts.append("ASM: " + ", ".join(asm_sources))
                print("  - " + " | ".join(parts))


def print_source_plan_detail(selected_tests, source_plan, source_errors):
    """打印每个 case 的源文件计划，用于 --plan-only 审查。"""
    print("case 源文件计划:")
    if len(selected_tests) <= MAX_TEST_LIST_LINES:
        visible_tests = selected_tests
        tail_tests = []
    else:
        head_count = MAX_TEST_LIST_LINES - 20
        visible_tests = selected_tests[:head_count]
        tail_tests = selected_tests[-20:]

    def print_one(test):
        plan = source_plan.get(test.name)
        if plan is None:
            print(f"- {test.name}: ERROR: {source_errors.get(test.name, '无法定位源文件')}")
            return
        print(f"- {test.name}")
        for path in plan.c_sources:
            print(f"  C: {display_path(path)}")
        for path in plan.asm_sources:
            print(f"  ASM: {display_path(path)}")

    for test in visible_tests:
        print_one(test)

    if tail_tests:
        omitted = len(selected_tests) - MAX_TEST_LIST_LINES
        print(f"... 省略 {omitted} 个 case 源文件计划 ...")
        for test in tail_tests:
            print_one(test)


def render_register_line(test, active):
    """生成标准化的 TEST_REGISTER 行。"""
    prefix = "" if active else "// "
    return f"{test.indent}{prefix}TEST_REGISTER({test.name});\n"


def write_selected_test(original_lines, tests, selected_test):
    """重写 test_register.c，仅保留一个活动的 TEST_REGISTER。"""
    test_by_line = {test.line_num: test for test in tests}
    updated_lines = list(original_lines)

    for line_num, test in test_by_line.items():
        updated_lines[line_num - 1] = render_register_line(
            test, active=(line_num == selected_test.line_num)
        )

    with SOURCE_FILE.open("w", encoding="utf-8") as f:
        f.writelines(updated_lines)


def create_backup():
    """创建临时备份文件，避免覆盖用户自己的 backup。"""
    fd, backup_path = tempfile.mkstemp(
        prefix="test_register.",
        suffix=".compile_elf.bak",
        dir=REPO_ROOT,
    )
    os.close(fd)
    Path(backup_path).unlink(missing_ok=True)
    shutil.copy2(SOURCE_FILE, backup_path)
    return Path(backup_path)


def restore_source(backup_file):
    """恢复源文件并删除临时备份。"""
    if backup_file and backup_file.exists():
        shutil.copy2(backup_file, SOURCE_FILE)
        backup_file.unlink(missing_ok=True)


class CompileLock:
    """同平台串行编译，避免多个脚本共用 build/<plat> 互相覆盖。"""

    def __init__(self, plat):
        self.plat = plat
        self.path = LOCK_DIR / f"{plat}.lock"
        self.handle = None

    def __enter__(self):
        LOCK_DIR.mkdir(parents=True, exist_ok=True)
        self.handle = self.path.open("w", encoding="utf-8")
        if fcntl is None:
            print("警告: 当前 Python 不支持 fcntl，无法启用编译锁")
            return self
        fcntl.flock(self.handle, fcntl.LOCK_EX)
        print(f"编译锁: {display_path(self.path)}")
        self.handle.write(f"pid={os.getpid()}\n")
        self.handle.flush()
        return self

    def __exit__(self, exc_type, exc, tb):
        if self.handle is not None:
            if fcntl is not None:
                fcntl.flock(self.handle, fcntl.LOCK_UN)
            self.handle.close()
        return False


def resolve_fast_register_stem(test_name):
    """为临时 register 源文件生成稳定且不超长的文件名。"""
    FAST_REGISTER_DIR.mkdir(parents=True, exist_ok=True)
    max_stem_len = max(1, get_component_name_max(FAST_REGISTER_DIR) - len(".c"))
    if len(test_name) <= max_stem_len:
        return test_name

    digest = hashlib.sha1(test_name.encode("utf-8")).hexdigest()
    suffix = f"{ALIAS_SEPARATOR}{digest[:ALIAS_HASH_LEN]}"
    prefix_len = max(1, max_stem_len - len(suffix))
    return f"{test_name[:prefix_len]}{suffix}"


def create_fast_register_source(test):
    """为单 case 生成很小的临时 register 源文件。"""
    path = FAST_REGISTER_DIR / f"{resolve_fast_register_stem(test.name)}.c"
    text = (
        "#include <rvh_test.h>\n\n"
        "/* Generated by compile_elf.py. Do not edit. */\n"
        f"/* Original case: {test.name} */\n"
        f"TEST_REGISTER({test.name});\n"
    )
    path.write_text(text, encoding="utf-8")
    return path


def make_env(tmpdir):
    """构建 make 子进程环境，避免 gcc 临时文件落到系统 /tmp。"""
    tmp_path = Path(tmpdir)
    tmp_path.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["TMPDIR"] = str(tmp_path.resolve())
    return env


def tail_text(text, max_lines=MAKE_OUTPUT_TAIL_LINES):
    """返回文本尾部，便于进度结束后统一展示失败原因。"""
    if not text:
        return ""
    lines = text.splitlines()
    if len(lines) > max_lines:
        lines = [f"... ({len(lines) - max_lines} lines omitted)"] + lines[-max_lines:]
    return "\n".join(lines)


def print_indented_block(label, text):
    """打印缩进后的多行块。"""
    if not text:
        return
    print(f"  {label}:")
    for line in text.splitlines():
        print(f"    {line}")


def run_silent(command, env):
    """运行命令并捕获输出。"""
    return subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
        env=env,
        cwd=str(REPO_ROOT),
    )


def remove_link_outputs(plat):
    """移除固定输出目标，避免切换注册源/源文件集合时 make 误判已最新。"""
    target = REPO_ROOT / "build" / plat / "rvh_test"
    for suffix in (".elf", ".asm", ".bin", ".elf.txt"):
        (target.with_suffix(suffix)).unlink(missing_ok=True)


def run_make(
    plat,
    options,
    clean=False,
    register_source=None,
    case_sources=None,
    case_asm_sources=None,
):
    """执行 make 命令，返回 (ok, failure)。"""
    try:
        env = make_env(options["tmpdir"])
        if clean:
            clean_command = ["make", "clean", f"PLAT={plat}"]
            clean_result = run_silent(
                clean_command,
                env=env,
            )
            if clean_result.returncode != 0:
                return False, BuildFailure(
                    case_name="",
                    reason="make clean 失败",
                    command=tuple(clean_command),
                    stdout=tail_text(clean_result.stdout),
                    stderr=tail_text(clean_result.stderr),
                )

        if (
            register_source is not None
            or case_sources is not None
            or case_asm_sources is not None
        ):
            remove_link_outputs(plat)

        command = [
            "make",
            f"-j{options['jobs']}",
            f"PLAT={plat}",
            f"LOG_LEVEL={options.get('log_level', 4)}",
            "CASE_LINK_MODE=selected",
            f"CROSS_COMPILE={options['cross_compile']}",
            f"SAVE_PREPROCESSED={1 if options['keep_preprocessed'] else 0}",
            f"GENERATE_BIN={1 if options['keep_bin'] else 0}",
            f"GENERATE_ASM={0 if options['skip_asm'] else 1}",
            f"GENERATE_READELF={1 if options['keep_readelf'] else 0}",
        ]
        vmodule = read_env_value("VMODULE")
        if vmodule:
            command.append(f"VMODULE={vmodule}")
        if register_source is not None:
            command.append(f"TEST_REGISTER_SRC={display_path(register_source)}")
        if case_sources is not None:
            command.append(
                "CASE_C_SRCS=" + " ".join(display_path(path) for path in case_sources)
            )
        if case_asm_sources is not None:
            command.append(
                "CASE_ASM_SRCS="
                + " ".join(display_path(path) for path in case_asm_sources)
            )

        result = run_silent(command, env=env)
        if result.returncode != 0:
            return False, BuildFailure(
                case_name="",
                reason="make 失败",
                command=tuple(command),
                stdout=tail_text(result.stdout),
                stderr=tail_text(result.stderr),
            )
        return True, None
    except Exception as err:
        return False, BuildFailure(
            case_name="",
            reason=f"编译错误: {err}",
        )


def copy_output_files(test_name, plat, artifact_map, export_asm=True):
    """复制输出文件，并在必要时为超长名字生成稳定别名。"""
    output_path = OUTPUT_DIR / plat
    output_path.mkdir(parents=True, exist_ok=True)
    artifact_stem = resolve_artifact_stem(test_name, output_path, artifact_map)
    copy_errors = []

    elf_src = REPO_ROOT / "build" / plat / "rvh_test.elf"
    if not elf_src.exists():
        copy_errors.append(f"ELF 缺失: {display_path(elf_src)}")
    else:
        try:
            shutil.copy2(elf_src, output_path / f"{artifact_stem}.ELF")
        except Exception as err:
            copy_errors.append(f"ELF 复制失败: {err}")

    asm_src = REPO_ROOT / "build" / plat / "rvh_test.asm"
    if export_asm and asm_src.exists():
        try:
            shutil.copy2(asm_src, output_path / f"{artifact_stem}.asm")
        except Exception as err:
            copy_errors.append(f"asm 复制失败: {err}")
    elif export_asm:
        copy_errors.append(f"asm 缺失: {display_path(asm_src)}")

    if not copy_errors:
        artifact_map["artifacts"][test_name] = artifact_stem
        save_artifact_map(output_path, plat, artifact_map)

    return artifact_stem, copy_errors


class ProgressReporter:
    """批量编译进度显示；TTY 单行刷新，非 TTY 定期落日志。"""

    def __init__(self, total):
        self.total = max(1, total)
        self.enabled = total > 1
        self.is_tty = sys.stdout.isatty()
        self.last_log_time = time.monotonic()

    def update(self, done, success_count, fail_count, current_name=""):
        if not self.enabled:
            return

        percent = done / self.total
        filled = min(PROGRESS_BAR_WIDTH, int(PROGRESS_BAR_WIDTH * percent))
        bar = "#" * filled + "-" * (PROGRESS_BAR_WIDTH - filled)
        current = f" current={current_name}" if current_name else ""
        message = (
            f"[{bar}] {done}/{self.total} "
            f"({percent * 100:5.1f}%) ok={success_count} fail={fail_count}{current}"
        )

        if self.is_tty:
            width = shutil.get_terminal_size((120, 20)).columns
            sys.stdout.write("\r\033[K" + message[: max(1, width - 1)])
            sys.stdout.flush()
            return

        now = time.monotonic()
        if (
            done == self.total
            or done == 1
            or done % PROGRESS_LOG_STEP == 0
            or now - self.last_log_time >= PROGRESS_LOG_INTERVAL_SEC
        ):
            print(message)
            self.last_log_time = now

    def finish(self):
        if self.enabled and self.is_tty:
            sys.stdout.write("\n")
            sys.stdout.flush()


def print_failure_details(failures):
    """在进度完成后统一打印失败详情。"""
    if not failures:
        return

    print("失败详情:")
    for failure in failures:
        print(f"- {failure.case_name}: {failure.reason}")
        if failure.command:
            print(f"  command: {' '.join(failure.command)}")
        print_indented_block("stdout", failure.stdout)
        print_indented_block("stderr", failure.stderr)


def main():
    """主函数。"""
    options = parse_args()
    if options is None:
        return 1
    if options.get("help"):
        return 0

    print(f"目标平台: {options['plat']}")
    if options["clean"]:
        print("构建策略: 首次编译前执行一次 clean")
    else:
        print("构建策略: 连续编译，默认不执行 clean")
    print(f"并行任务: -j{options['jobs']}")
    print(f"工具链前缀: {options['cross_compile']}")
    print(f"临时目录: {display_path(options['tmpdir'])}")
    if options["rewrite_register"]:
        print("注册策略: 改写 test_register.c（兼容旧模式）")
    else:
        print("注册策略: 临时小注册源文件，不改写 test_register.c")
    print(
        "调试产物: "
        f"preprocessed={'on' if options['keep_preprocessed'] else 'off'}, "
        f"asm={'off' if options['skip_asm'] else 'on'}, "
        f"readelf={'on' if options['keep_readelf'] else 'off'}, "
        f"bin={'on' if options['keep_bin'] else 'off'}"
    )
    if options["plan_only"]:
        print("执行模式: plan-only，只打印链接计划，不执行 make")

    original_lines, tests = load_registered_tests()
    if original_lines is None:
        return 1

    selected_tests = select_tests(tests, options)
    if not selected_tests:
        print("没有找到任何测试")
        return 1

    selected_tests, duplicate_selected_tests = dedupe_selected_tests(selected_tests)
    print_selected_tests(selected_tests)
    if duplicate_selected_tests:
        print(
            f"去重: 跳过 {len(duplicate_selected_tests)} 个重复 TEST_REGISTER 名字，"
            "仅编译首次出现项"
        )
    print()

    source_plan, source_errors = build_case_source_plan(selected_tests)
    print_source_plan_summary(source_plan, source_errors)
    print()

    if options["plan_only"]:
        print_source_plan_detail(selected_tests, source_plan, source_errors)
        return 0 if not source_errors else 2

    OUTPUT_DIR.mkdir(exist_ok=True)

    backup_file = create_backup() if options["rewrite_register"] else None
    success_count = 0
    fail_count = 0
    failed_tests = []
    failures = []
    progress = ProgressReporter(len(selected_tests))
    clean_pending = options["clean"]
    artifact_output_path = OUTPUT_DIR / options["plat"]
    artifact_output_path.mkdir(parents=True, exist_ok=True)
    artifact_map = load_artifact_map(artifact_output_path)

    try:
        with CompileLock(options["plat"]):
            for idx, test in enumerate(selected_tests, 1):
                case_link_sources = source_plan.get(test.name)
                if case_link_sources is None:
                    reason = source_errors.get(test.name, "无法定位源文件")
                    fail_count += 1
                    failed_tests.append(test.name)
                    failures.append(BuildFailure(test.name, f"跳过: {reason}"))
                    progress.update(idx, success_count, fail_count, test.name)
                    continue
                case_sources = case_link_sources.c_sources
                case_asm_sources = case_link_sources.asm_sources

                register_source = None
                if options["rewrite_register"]:
                    write_selected_test(original_lines, tests, test)
                else:
                    register_source = create_fast_register_source(test)

                ok, failure = run_make(
                    options["plat"],
                    options,
                    clean=clean_pending,
                    register_source=register_source,
                    case_sources=case_sources,
                    case_asm_sources=case_asm_sources,
                )
                if ok:
                    artifact_stem, copy_errors = copy_output_files(
                        test.name,
                        options["plat"],
                        artifact_map,
                        export_asm=not options["skip_asm"],
                    )
                    if copy_errors:
                        fail_count += 1
                        failed_tests.append(test.name)
                        failures.append(
                            BuildFailure(
                                test.name,
                                "产物导出失败: " + "; ".join(copy_errors),
                            )
                        )
                    else:
                        success_count += 1
                else:
                    fail_count += 1
                    failed_tests.append(test.name)
                    if failure is None:
                        failure = BuildFailure(test.name, "未知编译失败")
                    else:
                        failure = BuildFailure(
                            test.name,
                            failure.reason,
                            failure.command,
                            failure.stdout,
                            failure.stderr,
                        )
                    failures.append(failure)

                clean_pending = False
                progress.update(idx, success_count, fail_count, test.name)
    finally:
        progress.finish()
        restore_source(backup_file)

    print()
    print(f"编译完成: 成功 {success_count}, 失败 {fail_count}")
    if failed_tests:
        print("失败用例:")
        for name in failed_tests:
            print(f"  - {name}")
        print_failure_details(failures)
    print(f"输出文件在: {display_path(OUTPUT_DIR / options['plat'])}/")
    return 0 if fail_count == 0 else 2


if __name__ == "__main__":
    sys.exit(main())
