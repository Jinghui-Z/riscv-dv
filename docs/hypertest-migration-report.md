# Hypertest 全量移植与验证报告

本报告由 `scripts/import_hypertest.py` 从当前源工作树生成。所有注册用例（含
注释注册）及未注册但含 `TEST_START()` 的 bool 用例均进入 source-adapter catalog；
其余 bool helper 单列审计，不计作测试用例。重复注册按名称去重，保留全部原始行。

报告按源码/独立 DV 入口接通与实际执行验证两个维度记录结果。全量源入口和
catalog testlist 负责逐项选择，执行通过数量只来自实际完成自检的证据。

源码已接入 catalog 不等于这些用例均可在 NanHu 或 Spike 上执行。逐用例
validation 初始为 `not_run`；只有显式 validation JSON 才能合并执行证据。
native 和 source-adapter 的初始迁移状态分别为 `implemented_unverified` 和
`imported_unverified`；完成 generator/GNU/Spike 的条目会更新为相应 verified 状态。
本次实际结果以独立的 `migration_status`、`validation` 列和下方执行证据表为准，
对象编译或 ELF 链接不能替代执行结论，Spike 通过也不代表 RTL 验证完成。
平台约束、H 扩展、外部注入、reset/终止和程序写代码等
词法提示仅标记 `needs_review`，不自动推断可运行性或替代人工审核。

## 来源证据

- 源路径：`/nfs/home/zhongjinghui/tool/ISG/riscv-hyp-tests-nhv5`
- Git revision：`92f5d78859d1c63940a76adb6d92c04629dba0f6`
- 原样快照 SHA256：`51e6272f6002582bdde9dcd26f6229bf2ab7835d754535956feffd2d85477d07`
- 快照文件：391，共 22199297 bytes。
- manifest 保存逐文件 SHA256、大小、权限、源工作树 dirty 状态及被排除的软链。
- 只快照 case、共享头文件/框架/汇编、Spike 平台以及必要的源码/注册/许可证文件；
  build、results、仓库外文件和软链不导入。

## 数量

| 项目 | 数量 |
| --- | ---: |
| 原始注册行 | 4749 |
| 去重后已注册 case | 4443 |
| 默认 active case | 944 |
| 仅注释注册 case | 3499 |
| 重复注册的额外行 | 306 |
| 未注册 TEST_START case | 193 |
| catalog case 合计 | 4636 |
| 原生 directed stream case | 2 |
| source-adapter 源入口（包含上述 native case，不另加总） | 4636 |
| 非 case bool helper 定义 | 414 |
| 缺失定义 case | 0 |
| 有多处定义 case | 0 |
| 带 needs_review 提示 case | 896 |

## 随机选择与 store 限制审核名单

全部 4636 个 catalog case 均可通过显式名称选择；
默认随机池只选择审核名单中的 6 项，
`+restrict_store_to_data=1` 的默认池只选择其中已审核的 2 项。

`default_active` 仅表示上游注册表默认启用状态；`default_random_eligible`
表示 DV 默认随机池成员；`restrict_store_reviewed` 表示 store 限制已审核成员。
这些字段用途不同；对象编译、ELF 链接和词法提示均不能自动授予随机池资格。

- 默认随机池：`/nfs/home/zhongjinghui/tool/ISG/riscv-dv/src/hypertest/case_names_random.txt`，SHA256 `513bd5ce46b7af1d5fb64f3dedc92d977596df030a425e2a4e7d2d7fbf245e7b`。
- restrict 审核池：`/nfs/home/zhongjinghui/tool/ISG/riscv-dv/src/hypertest/case_names_restricted.txt`，SHA256 `e47ade87c4e782aadeaae771592b95fdf7c2cc853f7cd8ef8b91d63359926c15`。

显式选择 catalog 用例仍须确认 H 扩展、locked PMP、外部激励和平台地址等
运行前提。名单用于控制默认随机抽样，不表示 catalog 中其它用例已经通过执行。


## 按模块

| 模块 | case | 已注册 | 默认 active | native | needs_review |
| --- | ---: | ---: | ---: | ---: | ---: |
| ai_test_cases/backend | 220 | 220 | 78 | 0 | 49 |
| ai_test_cases/frontend | 231 | 176 | 24 | 2 | 50 |
| ai_test_cases/interrupt | 235 | 235 | 150 | 0 | 85 |
| ai_test_cases/memblock | 3313 | 3188 | 669 | 0 | 521 |
| ai_test_cases/mix | 178 | 178 | 23 | 0 | 65 |
| manual_test_cases/csr_priv | 133 | 124 | 0 | 0 | 50 |
| manual_test_cases/exception | 36 | 36 | 0 | 0 | 1 |
| manual_test_cases/interrupt | 43 | 39 | 0 | 0 | 15 |
| manual_test_cases/memory | 216 | 216 | 0 | 0 | 53 |
| manual_test_cases/special | 9 | 9 | 0 | 0 | 0 |
| manual_test_cases/vector | 22 | 22 | 0 | 0 | 7 |

## 全量 GNU 对象编译诊断

审计日志：`out/hypertest_catalog_compile_20261016_041652_1940400/audit.json`。这里只执行 `gcc -c` 和 `nm`，
该对象阶段不执行原断言、Spike 或 RTL。`object_compiled` 表示相应源码
编译完成且对象中存在该 case 的函数符号；不计入执行通过数量。

| 源文件状态 | 数量 |
| --- | ---: |
| compiled | 349 |
| external_payload_requires_config | 1 |

| case 对象状态 | 数量 |
| --- | ---: |
| object_compiled | 4636 |

编译使用与运行 adapter 相同的 `prepare_source`，原样快照没有修改。
派生副本包含共享 trap handler 返回值/栈对齐修复，以及仅对含真实
`TEST_START` 而缺少 `TEST_END` 的 bool case 补充返回协议，保留原
`test_status` 和所有断言。C 文件额外强制包含 `stddef.h` 并启用
`-Werror=return-type`；副本保留原目录的 quoted-header 搜索路径。

| 派生副本适配记录 | 数量 |
| --- | ---: |
| handler_return_value | 1 |
| test_end_return_protocol | 168 |
| trap_stack_alignment | 1 |

manifest 的顶层 `compile_diagnostic.source_results` 保存每个源文件的
原始/派生 SHA256、命令、对象、compiler/nm 日志及适配记录；每个 case 的
`compile_diagnostic` 和 CSV 的 `compile_*` 列保存对应对象证据。

- `ai_test_cases/backend/boundary/flash/boot_boundary.S`：`external_payload_requires_config`；Standalone flash boot payload requires POINT and its dedicated runner; this source does not define any catalog bool case. 日志：`/nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_catalog_compile_20261016_041132_1874414/logs/ai_test_cases/backend/boundary/flash/boot_boundary.S.stderr.log`。

上述独立 flash payload 的 POINT 配置需求不代表 bool case 编译失败；
它需要原专用 runner 提供配置。对象诊断也不验证外部中断、debug、reset、
PMA/MMIO 响应以及 H 扩展是否受目标硬件支持。


## 全量逐 case ELF 链接诊断

审计日志：`out/hypertest_catalog_compile_20261016_041652_1940400/audit.json`。为每个 catalog case 单独计算 C/ASM
依赖闭包，复用上述已编译对象，用最小启动 fixture 与 DV linker script
执行最终静态链接，并通过 `nm` 确认 case、bridge 和入口符号保留。

| ELF 链接状态 | case |
| --- | ---: |
| elf_linked | 4636 |

这一步没有执行生成器随机程序、原用例断言、Spike 或 RTL。
`elf_linked` 只表示依赖闭包可以链接；不计为架构或平台执行通过。
每 case 的 `elf_link_diagnostic` 保存 ELF/SHA256、链接命令、依赖闭包、
compiler/nm 日志和失败原因；CSV 的 `elf_link_*` 列提供索引。

启动 fixture：`/nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_catalog_compile_20261016_041652_1940400/generated/link_fixture.S`，SHA256 `f64e6ff68566c64183b2b8f569d8dfaf4cb7658deea58fb229ca32878a866c96`。

本次完整 catalog 未出现链接失败。

## 执行证据状态

| validation | case |
| --- | ---: |
| gen_gcc_spike_pass | 9 |
| not_run | 4627 |

执行记录：`src/hypertest/validation.json`，SHA256 `d064eff5f602d67372df40525e9776db3b62f707f6acd3a5bc23387f52c6d5eb`。完整逐 case 记录（含 evidence SHA256、
实际运行命中、完成次数及每轮 details）保存在 `validation_record`；
collection 元数据及被拒绝的证据保存在 `validation_metadata`。


### 已通过 generator/GNU/Spike 的 catalog case

以下每行对应一个实际完成原自检的 catalog case；多次运行仍只计一个 case。

| case | migration_status | 完成次数 | evidence |
| --- | --- | ---: | --- |
| `ai_arch_scalar_illegal_field_encoding_matrix_corner` | `source_adapter_gcc_spike_verified` | 2 | [hypertest_random_pool_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/spike_sim/hypertest_random_pool_smoke_0.hypertest-validation.json>)<br>[hypertest_case_ai_arch_scalar_illegal_field_encoding_matrix_corner_575bef15938b_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_source_101603_r2/spike_sim/hypertest_case_ai_arch_scalar_illegal_field_encoding_matrix_corner_575bef15938b_0.hypertest-validation.json>) |
| `ai_arch_vector_zvbb_full_opcode_sew_reference_matrix_corner` | `source_adapter_gcc_spike_verified` | 1 | [hypertest_case_ai_arch_vector_zvbb_full_opcode_sew_reference_matrix_corner_887da060dc08_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_representative_101605/spike_sim/hypertest_case_ai_arch_vector_zvbb_full_opcode_sew_reference_matrix_corner_887da060dc08_0.hypertest-validation.json>) |
| `ai_micro_backend_issuequeue_long_alu_dependency_chain_corner` | `source_adapter_gcc_spike_verified` | 3 | [hypertest_random_pool_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/spike_sim/hypertest_random_pool_smoke_0.hypertest-validation.json>)<br>[hypertest_case_ai_micro_backend_issuequeue_long_alu_dependency_chain_corner_1024bfce8c10_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_source_101603_r2/spike_sim/hypertest_case_ai_micro_backend_issuequeue_long_alu_dependency_chain_corner_1024bfce8c10_0.hypertest-validation.json>) |
| `ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_corner` | `implemented_gcc_spike_verified` | 6 | [hypertest_sv48_canonical_explicit_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_native_all_101602_r4/spike_sim/hypertest_sv48_canonical_explicit_0.hypertest-validation.json>)<br>[hypertest_sv48_mixed_restricted_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_native_all_101602_r4/spike_sim/hypertest_sv48_mixed_restricted_smoke_0.hypertest-validation.json>)<br>[hypertest_sv48_random_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_native_all_101602_r4/spike_sim/hypertest_sv48_random_smoke_0.hypertest-validation.json>)<br>[hypertest_random_pool_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/spike_sim/hypertest_random_pool_smoke_0.hypertest-validation.json>) |
| `ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_x_deny_corner` | `implemented_gcc_spike_verified` | 7 | [hypertest_sv48_canonical_x_deny_explicit_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_native_all_101602_r4/spike_sim/hypertest_sv48_canonical_x_deny_explicit_0.hypertest-validation.json>)<br>[hypertest_sv48_mixed_restricted_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_native_all_101602_r4/spike_sim/hypertest_sv48_mixed_restricted_smoke_0.hypertest-validation.json>)<br>[hypertest_sv48_random_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_native_all_101602_r4/spike_sim/hypertest_sv48_random_smoke_0.hypertest-validation.json>)<br>[hypertest_random_pool_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/spike_sim/hypertest_random_pool_smoke_0.hypertest-validation.json>) |
| `ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner` | `source_adapter_gcc_spike_verified` | 3 | [hypertest_random_pool_smoke_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/spike_sim/hypertest_random_pool_smoke_0.hypertest-validation.json>)<br>[hypertest_case_ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner_dce82b71a3d2_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_representative_101605/spike_sim/hypertest_case_ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner_dce82b71a3d2_0.hypertest-validation.json>) |
| `test_sv39_mode` | `source_adapter_gcc_spike_verified` | 1 | [hypertest_case_test_sv39_mode_5637cf558125_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_representative_101605/spike_sim/hypertest_case_test_sv39_mode_5637cf558125_0.hypertest-validation.json>) |
| `test_sv48_mode` | `source_adapter_gcc_spike_verified` | 1 | [hypertest_case_test_sv48_mode_c19f070fa402_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_representative_101605/spike_sim/hypertest_case_test_sv48_mode_c19f070fa402_0.hypertest-validation.json>) |
| `time_test_1` | `source_adapter_gcc_spike_verified` | 1 | [hypertest_case_time_test_1_87a116b229e3_0.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_representative_101605/spike_sim/hypertest_case_time_test_1_87a116b229e3_0.hypertest-validation.json>) |

### 补充验证（不计入迁移 case 数量）

bridge selftest 验证适配桥，legacy 验证普通 DV 回归。完整记录保存在
manifest 的 `supplementary_checks`；这些记录不增加 catalog case 或通过 case 数。

| 类别 | 记录数 |
| --- | ---: |
| `bridge_selftest` | 1 |
| `legacy` | 2 |

| 类别 | 检查 | validation | evidence |
| --- | --- | --- | --- |
| `bridge_selftest` | `dv_hypertest_selftest_clobber_case` | `gcc_spike_pass` | [bridge_selftest_main.hypertest-validation.json](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_bridge_101604_r4/spike_sim/bridge_selftest_main.hypertest-validation.json>) |
| `legacy` | `riscv_arithmetic_basic_test_0` | `gen_gcc_spike_pass` | [riscv_arithmetic_basic_test_0.S](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_legacy_101606_r2/asm_test/riscv_arithmetic_basic_test_0.S>)<br>[riscv_arithmetic_basic_test_0.o](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_legacy_101606_r2/asm_test/riscv_arithmetic_basic_test_0.o>)<br>[sim_riscv_arithmetic_basic_test_0.log](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_legacy_101606_r2/sim_riscv_arithmetic_basic_test_0.log>)<br>[riscv_arithmetic_basic_test_0.log](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_legacy_101606_r2/spike_sim/riscv_arithmetic_basic_test_0.log>) |
| `legacy` | `hypertest_random_pool_disabled_0` | `gen_gcc_spike_pass` | [hypertest_random_pool_disabled_0.S](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/asm_test/hypertest_random_pool_disabled_0.S>)<br>[hypertest_random_pool_disabled_0.o](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/asm_test/hypertest_random_pool_disabled_0.o>)<br>[sim_hypertest_random_pool_disabled_0.log](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/sim_hypertest_random_pool_disabled_0.log>)<br>[hypertest_random_pool_disabled_0.log](</nfs/home/zhongjinghui/tool/ISG/riscv-dv/out/hypertest_random_pool_101607_r4/spike_sim/hypertest_random_pool_disabled_0.log>) |

## 逐用例清单与复现

本轮工具、命令、seed、输出目录与限制见 [实际验证记录](hypertest-validation-runs.md)，
随机生成与独立运行方法见 [使用说明](hypertest-directed-streams.md)。

逐 case 的定义位置、注册位置、迁移路径和 validation 见 `/nfs/home/zhongjinghui/tool/ISG/riscv-dv/docs/hypertest-migration.csv`。
完整结构化数据及非 case helper 审计见 `/nfs/home/zhongjinghui/tool/ISG/riscv-dv/src/hypertest/manifest.json`。

可逐项选择的测试入口见 `/nfs/home/zhongjinghui/tool/ISG/riscv-dv/yaml/hypertest_catalog_testlist.yaml`，`catalog_test` 列给出短测试名，
`hypertest_case_names` 保留完整原始名称。全量和 active-only 名单分别见
`src/hypertest/case_names.txt` 和 `src/hypertest/case_names_active.txt`。
catalog testlist 不加入 target 的默认 testlist，避免无意一键运行数千项。

```bash
python3 scripts/import_hypertest.py --source /path/to/riscv-hyp-tests-nhv5 \
  --inventory-only
python3 scripts/import_hypertest.py --source /path/to/riscv-hyp-tests-nhv5
```

可用 `--compile-audit out/<本次审计目录>/audit.json` 合并完整对象编译和可选 ELF 链接证据；
importer 会核对快照 SHA256、源文件哈希和完整 case 名称集合，拒绝混用。

可选 `--validation src/hypertest/validation.json` 合并逐用例外部记录；JSON
格式为 `{"cases": {"原case名": {"validation": "状态", "evidence": ["日志路径"]}}}`。未知 case 名称会报错，避免错配证据。

若 validation JSON 提供 `source_snapshot_sha256`，必须与本次原样快照一致。
完整 case record、`evidence_sha256`、每轮实际命中及 `supplementary_checks`
会原样保留；补充检查不参与迁移 case 通过计数。

相同源文件内容和权限会得到相同 snapshot SHA256。importer 不运行原仓库的
编译或仿真程序；manifest 中的执行结论需由后续完整 generator/GNU/Spike
运行及独立 RTL 验证证据更新。导入器再次运行会重建未验证清单，应在更新
验证记录前保留该次运行对应的 manifest。
