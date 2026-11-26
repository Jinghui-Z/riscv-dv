# Hypertest 移植实际验证记录

## 2026-11-04 大随机批量模板修复

`nanhu_big_random_hypertest_branch_mix` 原来从完整 cosim-on 池抽取源用例，
但 `gcc_opts` 的 `-march=rv64imafdc_zicsr_zifencei` 缺少 V；源桥接要保存
并恢复向量状态，故在 10 批生成完后首个 GCC 阶段被拒绝。现已给两个大随机
Hypertest 入口设置包含 F/D/V 的 NanHu GNU ISA，不降低桥接检查要求。
生成器还按 ELF 固定静态或动态页表候选，避免完整池在一个 ELF 中混合
`page_tables.h` 和 `dynamic_page_tables.h`。桥接 BSS 每例清零改为对齐后的
64B 宽 store 展开；原 120 秒 Spike 日志的清零 PC 不是死循环证据。

默认脚本 `dv_big_random_hyp_dir.sh` 现在运行
`nanhu_big_random_hypertest_branch_mix_reviewed`，使用六个此前逐项通过 Spike
的候选；完整池仍由 `TEST=nanhu_big_random_hypertest_branch_mix` 显式选择。
固定 seed `20261104`、10 批、每例约 10k 基础指令的完整
`gen,gcc_compile,iss_sim --iss spike` 运行目录为
`out/nanhu_branch_mix_reviewed_batch10_20261104`：10 个 `.S`、10 个 ELF、
10 个 `.bin` 均生成；10 份验证 JSON 均为 `passed_partial`，共 256 次注入
实际命中、44 个静态注入因随机分支跳过。全部生成日志的
UVM_WARNING/UVM_ERROR/UVM_FATAL 均为 0。`scripts/tests` 的 85 个 Python
unittest 通过，VCS 生成器 compile-only 与桥接自测也通过。

完整 4,615 项 cosim-on 名单是候选全集，尚未逐项完成此 NanHu/Spike
集成验证。保持原名单不变：固定 seed 的完整池运行中，
`instruction_page_fault_9` 和另一个 LR/SC 源用例曾使 Spike 返回
`dv_hypertest_fail`；因此不能把默认 10 批通过外推为完整池通过。

## 2026-10-16 基线验证

全量 4,636 项已完成对象编译和逐项 ELF 链接诊断，实际执行了其中 9 个不同
原用例的生成/GNU/Spike 自检。其余 4,627 项为 `not_run`，没有进行 LinkNan RTL
或 cosim 验证。完整名称、状态和证据见
[迁移报告](hypertest-migration-report.md)、[CSV](hypertest-migration.csv) 和
`src/hypertest/validation.json`。

## 工具与工作树

- DV 基线 revision：`b5bdc9b6d71459ff5318dc334335a2b994238045`，使用包含本次修改
  及用户已有改动的工作树；并非该 revision 的干净树。
- hypertest 源 revision：`92f5d78859d1c63940a76adb6d92c04629dba0f6`，其源工作树
  dirty 状态与全部导入文件的 SHA256 记录在 manifest。
- VCS：`U-2023.03`；GNU：`riscv64-unknown-linux-gnu-gcc 15.1.0`；
  Spike：`1.1.1-dev`，使用用户指定的安装版本，preflight 已验证支持 Zicclsm。
- 所有正常 ELF 命令显式使用 `--steps gen,gcc_compile,iss_sim --iss spike
  --iss_timeout 120`。每组实际运行前均以同一 target/testlist/test/seed/output
  通过 `dv_preflight.py`；下面的输出目录互不复用。

## 完整运行组

| 组 | target / testlist | test | seed | 输出目录 | 结果 |
| --- | --- | --- | ---: | --- | --- |
| 整数与负向编码 | `nanhu_v5_1` / `yaml/hypertest_catalog_testlist.yaml` | 下列两个入口 | 101603 | `out/hypertest_source_101603_r2` | 2 ELF 全链通过 |
| Vector、嵌套异常、页表、手工 CSR | `nanhu_v5_1` / 同上 | 下列五个入口 | 101605 | `out/hypertest_representative_101605` | 5 ELF 全链通过 |
| 原生定向与 restrict 混合 | `nanhu_v5_1` / `yaml/hypertest_testlist.yaml` | `all` | 101602 | `out/hypertest_native_all_101602_r4` | 4 ELF 全链通过，全部实例实际命中 |
| 默认随机池与关闭对照 | `nanhu_v5_1` / `yaml/hypertest_random_testlist.yaml` | `all` | 101607 | `out/hypertest_random_pool_101607_r4` | 2 ELF 全链通过，开启项实际完成 8 个实例，关闭项无 hypertest marker/对象 |
| 桥接合成自测 | `nanhu_v5_1` / `yaml/hypertest_bridge_selftest.yaml` | `hypertest_bridge_selftest` | 101604 | `out/hypertest_bridge_101604_r4` | GNU/Spike 通过，不计原用例 |
| 相邻 legacy target | `rv64imafdc` / target 默认 testlist | `riscv_arithmetic_basic_test`，`--iterations 1` | 101606 | `out/hypertest_legacy_101606_r2` | 1 ELF 全链通过，不计原用例 |

NanHu 各组均使用 `--simulator vcs --cmp_opts='+define+NANHU_V5_1_SV48'`；legacy
不传该 define。生成器各日志的 `UVM_WARNING/UVM_ERROR/UVM_FATAL` 均为 0。
合成自测使用汇编入口，不调用随机生成器；它比较两轮 Initial/Clean FS/VS 状态
下全部 32 个 FP、32 个 V 寄存器、向量控制及 M/S CSR 的恢复。

整数与负向编码组的 `--test` 为以下两项的逗号拼接：

```text
hypertest_case_ai_micro_backend_issuequeue_long_alu_dependency_chain_corner_1024bfce8c10
hypertest_case_ai_arch_scalar_illegal_field_encoding_matrix_corner_575bef15938b
```

代表性组的 `--test` 为以下五项的逗号拼接：

```text
hypertest_case_ai_arch_vector_zvbb_full_opcode_sew_reference_matrix_corner_887da060dc08
hypertest_case_ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner_dce82b71a3d2
hypertest_case_test_sv39_mode_5637cf558125
hypertest_case_test_sv48_mode_c19f070fa402
hypertest_case_time_test_1_87a116b229e3
```

原生显式测试已关闭普通随机 branch，以保证对应场景不被跳过。随机池测试所选
8 个实例覆盖非法编码、整数依赖链、嵌套异常和两种 canonical 边界；Zvbb 本轮
没有被抽中，其执行证据来自独立 Vector 入口。随机候选存在不表示已执行。

## 全量静态诊断

最终证据为 `out/hypertest_catalog_compile_20261016_041652_1940400/audit.json`。
349 个源码编译成功，所有 4,636 个 case 的对象定义符号均存在，全部 4,636 个
case 依赖闭包均可独立链接，ELF 通过 `nm` 检查。另一个独立 flash 启动 payload
需要专用 runner 的 `POINT` 配置，不定义 catalog 的 bool case，单独记录为
`external_payload_requires_config`。

静态诊断可通过 `scripts/check_hypertest_compile.py --link` 重建；默认使用 8 个
并行任务。它使用最小启动 fixture，不执行原断言、Spike 或 RTL，也不作为
4,636 项运行通过的证据。共享框架适配和 168 个缺失 TEST_END 的函数尾补返回
均记录在审计中，原快照不变。

## 汇总与重建报告

以下命令已实际执行，从 ELF、生成器日志和实际执行区间汇总出 9 个原用例，
并保存证据 SHA256；另记桥接自测和两个普通 DV 对照，不计入原用例数量：

```bash
python3 scripts/collect_hypertest_validation.py \
  out/hypertest_source_101603_r2 \
  out/hypertest_representative_101605 \
  out/hypertest_native_all_101602_r4 \
  out/hypertest_random_pool_101607_r4 \
  out/hypertest_bridge_101604_r4 \
  out/hypertest_legacy_101606_r2 \
  --output src/hypertest/validation.json

python3 scripts/import_hypertest.py \
  --source /path/to/riscv-hyp-tests-nhv5 \
  --compile-audit out/hypertest_catalog_compile_20261016_041652_1940400/audit.json \
  --validation src/hypertest/validation.json
```

生成/回放的完整命令模板见[使用说明](hypertest-directed-streams.md)。复现时
更换新的输出目录，固定本表的 seed 和 test 选择。运行证据区分每个静态注入
实例，循环重复不会填补其它实例的跳过；通过只说明原用例自检成功。

脚本回归：`python3 -m unittest discover -s scripts/tests -p 'test_*hypertest*.py'
-q`，70 项通过；`git diff --check` 通过。
