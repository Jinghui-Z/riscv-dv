# Hypertest 全量用例 cosim 分类

按“除非证明与 cosim 不兼容，否则可开启”的原则，对当前导入的全部
4,636 个原始用例名分类：

| 清单 | 数量 | 分类依据 |
| --- | ---: | --- |
| [case_names_random_cosim_on.txt](../src/hypertest/case_names_random_cosim_on.txt) | 4,615 | 未发现已证实的 cosim 不兼容，包含尚未执行的用例 |
| [case_names_random_cosim_off.txt](../src/hypertest/case_names_random_cosim_off.txt) | 21 | 原用例要求与当前 RTL/参考模型存在明确冲突 |

两个文件均为排序后的纯函数名列表，一行一个名称，无重复、无交集，合并等于
manifest.json 和 case_names.txt 的全部名称。[逐用例审计 CSV](../src/hypertest/cosim_classification.csv)
记录分类、原始源码位置、调用链、参考模型证据、环境依赖和既有验证状态。

本轮完成的是源码审查和分类工具验证，没有执行新的 ELF、独立 Spike 或 RTL cosim。
on 表示没有充分理由关闭比较，不表示用例自检通过、当前目标支持其全部功能，
也不表示已审定可与所有其它用例反复混入。此前的“未验证默认 off”口径已废弃。

## 审查范围与模型

用例取自 DV 内的 src/hypertest/upstream 快照，SHA256 为
51e6272f6002582bdde9dcd26f6229bf2ab7835d754535956feffd2d85477d07。
扫描全部 case 的独立函数体、可见宏和 helper 调用，避免把同一文件其它用例、
公共 reset/trap 框架或只被引用而未调用的函数误当成执行路径。
两项使用原生 SV 实现的 canonical 用例按实际 SV 移植路径审查。

cosim 以本机 projects/LinkNan 的实际集成为依据：

| 部件 | 审查版本/配置 |
| --- | --- |
| Spike | LinkNan/Spike，revision e70d9e98feb860a990ab7127202fee24863f7166，CPU_NANHU |
| 差分框架 | LinkNan/dependencies/difftest，基线 revision 17057c4f3d4f79cc11e15306176c25a5cc6925a7，含当前未提交改动 |
| 核数 | LinkNan/sim/simv/comp/vcs_cmd.sh 当前编译参数 NUM_CORES=1 |

审查读取的是实际工作树。为区分差分框架基线与本机同步补丁，本次
dependencies/difftest/src/test/csrc/difftest/difftest.cpp 的 SHA256 为
ddecb5e207dd2eabf3c21614a6f66e1890ce41033c16f3f8a9cc67fdfc9c77ee；
Spike/riscv/mmu.h 为
71ed8e76fefcc1a8daf8d9422ebb14234cc6d09aa16a444f0693827551384932。

独立预检查使用的 tool/simulator/riscv-isa-sim 与上述 cosim 参考模型不同。
独立 Spike 缺少某项 CSR、没有 TB 注入或者尚未执行，均不能单独证明
LinkNan cosim 不兼容。参考模型或核数改变后，应重新审查 off 证据；
分类脚本的 --check 只检查清单是否与当前规则一致，不自动证明外部模型未变化。

## 21 项关闭依据

| 原因 | 数量 | 可定位的冲突 |
| --- | ---: | --- |
| requires_unmodelled_ecc_csr | 20 | 18 个 cache-error/ECC 用例及 2 个正向 custom-CSR 矩阵要求成功访问 SCEC/SCEIC；RTL 实现了 0x5da/0x5db，当前 cosim Spike 未注册且访问不会自动 skip |
| requires_stale_global_tlb_entry | 1 | 要求修改全局 PTE 并切换 ASID 后仍读取旧映射；当前单核参考模型重新 walk，得到新映射 |

20 个 CSR 冲突中的 18 个定义于
ai_test_cases/memblock/exception_trap/ai_micro_cache_error_nmi_cases.c；
例如第629–630行在正常 M-mode 路径读取 SCEC/SCEIC，随后要求错误记录和注入生效。
另两个定义于 ai_test_cases/memblock/csr/ai_arch_custom_csr_cases.c：

- ai_arch_memblock_custom_csr_privilege_wpri_warl_matrix_corner：第1064–1069行的
  custom_csrs 包含 SCEC/SCEIC，第1100–1103行要求逐项成功读取。
- ai_arch_scec_sceic_baseline_field_privilege_matrix_corner：第2101–2102行读取
  两个 CSR，第2110–2116行要求无异常和有效 reset baseline。

对应模型证据在 LinkNan/Spike/riscv/csr_init.cc:662–668、
riscv/processor.cc:741–751。RTL backend/fu/NewCSR/CSRCustom.scala:30–43 注册
两个 CSR；NewCSR.scala:1017–1022 的性能计数器/AIA skip 范围不含这两个地址，
xiangshan/Bundle.scala:370–376 的 isSkipDiff 仅合并 isMMIO/isPerfCnt，
Rob.scala:1735–1739 将它传给差分框架。因而成功的 RTL CSR 访问会在参考侧产生
非法指令，不能用预期异常测试的名义放过。

TLB 冲突项为 ai_micro_memblock_global_pte_cross_asid_cached_hit_corner。
ai_test_cases/memblock/translation/ai_micro_memblock_pte_ptw_legal_cases.c:1002–1022
改写 PPN 并切换 ASID，但断言必须获得 old_value。
LinkNan/Spike/riscv/mmu.h:593–602 的 access_tlb 固定返回 miss；
dependencies/difftest/src/test/csrc/difftest/difftest.cpp:1265–1268 的成功 load
同步只在 NUM_CORES>1 时执行，当前单核构建不满足。
此结论限定当前单核配置；多核构建需要重新评估。

ai_arch_memblock_custom_csr_trap_handler_snapshot_restore_corner 只使用
已经实现的 SPFCTL/SMBLOCKCTL，保持 on。旧 PTE.U 权限触发 page fault 的用例
也保持 on：difftest.cpp:801–815 存在 guided exception 路径，不能照搬成功 load
的 TLB 差异结论。

## 保留 on 的场景与依赖

没有运行证据、原注册被注释、名称含 manual、词法命中 PMA/PBMT/代码写入、
可能残留状态、LR/SC 时间窗或潜在 RTL bug，都不自动进入 off。
原先已有 generator/GNU/Spike 成功记录的 9 项全部保留 on；
两个 canonical 场景用于发现 RTL 问题，触发 bug 不是关闭 cosim 的理由。

CSV 将以下依赖单列，不据此关闭比较：

| 依赖 | 数量 | 含义 |
| --- | ---: | --- |
| requires_tb_command_responder | 84 | 需要匹配的 VModule/前端命令 responder 及 harness 配置；独立 Spike 不能独自提供外部事件 |
| requires_external_monitor_ack | 45 | 需要 Backend IT monitor、从实际 ELF 解析的 control 地址和应答同步；与上一类重叠6项 |
| requires_h_virtual_mode | 33 | 当前 NanHu 无 H，bridge 在两侧均会拒绝 VS/VU；属于目标能力限制，不能因此声称差分比较不兼容 |

这些是当前源码识别出的依赖，不能视为所有平台要求的穷尽清单。
例如 H 项在当前 NanHu 上不能通过原测试，应选择支持 H 的目标或另行适配；
保留 on 只遵循本次 cosim 分类口径。

没有把外部事件统一判 off，是因为 LinkNan 的差分框架确有同步实现：
difftest.cpp:752–761 将中断/NMI 交给参考模型；:780–799 支持受控外部取指异常；
:1502–1519 在相应 CBO.INVAL/refill 配置下同步 golden/ref 内存。
这不证明每个 monitor 场景已经实测通过，但足以否定“依赖 TB 就一定不能 cosim”。

同样，LinkNan Spike 已实现 PMA、自定义 0x5c0/1/3/4、
保留编码 CBIE/MPP 的 WARL 行为以及 NanHu trigger match/chain 限制，
相关用例保持 on，不能按独立 Spike 的差异排除。

## 清单生成与使用

在 DV 根目录执行：

    python3 scripts/classify_hypertest_cosim.py
    python3 scripts/classify_hypertest_cosim.py --check
    python3 -m unittest discover -s scripts/tests -p test_hypertest_cosim_classification.py

本次 --check 通过，10 项单元/全目录策略检查通过，覆盖清单完整性、排序去重、
默认放行、依赖与关闭理由分离、源码边界、函数调用识别，以及正向 CSR/旧映射冲突。

脚本对全部 manifest 名称执行默认 on 分类，只应用上述已复核的冲突规则。
审计 CSV 的 evidence 为用例源码见证及 helper 路径，model_evidence 为模型反证，
environment_dependencies/dependency_evidence 为不影响开关判定的环境条件，
validation 则保留既有执行记录。

要显式用 on 文件作为随机候选，在 gen_opts 中指定：

    +enable_random_hypertest=1
    +hypertest_case_file=/绝对路径/riscv-dv/src/hypertest/case_names_random_cosim_on.txt

若还需限制 DV **自己生成**的随机/定向 store，可额外设置
`+restrict_generated_store_to_data=1` 和 `+restrict_store_to_data=0`。
这只解除源 Hypertest 与严格 store 限制的生成器冲突：源 C/ASM 保留原写入，
包括通过虚拟地址别名有意改写可执行物理页的测试；其 `FENCE.I`、异常和平台
语义仍需独立验证。不能把这一组合表述为“所有写入都不碰代码”。
完整 on 名单没有逐例完成 NanHu target、独立 Spike 或 RTL cosim 验收，
且其中可能混有 H、外部 responder、互斥页表框架或跨用例状态依赖；实际批量
随机前应收窄候选池。大随机模板的
`nanhu_big_random_hypertest_cosim_on_scoped_reference` 是默认禁用的配置示例，
小规模正向验收使用
[hypertest_scoped_store_testlist.yaml](../yaml/hypertest_scoped_store_testlist.yaml)。

清单不会自动开启或关闭 checker；off 文件也不会设置 no_iss/no-diff。
run.py 的默认候选仍为原 case_names_random.txt，本次只增加分类文件与审计工具。
生成时的 M-mode、单 hart、Sv48、ISA 和 restrict_store_to_data 等既有约束仍按
实际选中用例检查，操作方法见 [hypertest-directed-streams.md](hypertest-directed-streams.md)。
独立 Spike 预检查仍按 dv-workflow 显式执行 --iss spike；on 条目如因外部 responder
或独立模型缺失而预检查失败，应报告具体工具/平台问题，不能据此静默关闭检查，
也不能自动改成 cosim_off。
