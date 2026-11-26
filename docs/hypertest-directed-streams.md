# Hypertest 全量用例与随机定向流使用说明

已将 `riscv-hyp-tests-nhv5` 的全部 4,636 个已有用例接入 DV：4,443 个去重注册
用例（含注释注册）和 193 个未注册但含 `TEST_START()` 的用例。每个用例都有
独立 testlist 入口和原始函数名，可被独立运行或加入随机候选池。两个 canonical
边界场景实现为原生 SV 模板，其余保留 C/ASM、原断言和异常处理框架，通过桥接
在同一个 DV ELF 中调用。源仓库无需放在 DV 旁边，完整快照在
`src/hypertest/upstream/`，原文件不作就地修改。

[迁移报告](hypertest-migration-report.md) 与
[逐用例 CSV](hypertest-migration.csv) 记录全部名称、定义/注册位置、独立测试名、
移植路径、编译/链接诊断和实际执行证据。接入入口和对象编译成功不等于全部用例
都能在当前 NanHu/Spike 平台运行；H、PMP 锁位、外部中断、设备和 reset 等条件
需要按用例核实，尚未执行的条目保留 `not_run`。
本轮工具版本、命令选择、seed 和结果见[实际验证记录](hypertest-validation-runs.md)。

## 开关与插入方式

| `gen_opts` | 默认值 | 含义 |
| --- | --- | --- |
| `+enable_random_hypertest=1` | `0` | 允许移植的 hypertest 定向流，并开启独立随机候选池 |
| `+random_hypertest_ratio=N` | `4` | 每 1000 条基础随机指令插入的 hypertest 流数量，范围 `0..1000` |
| `+hypertest_case_names=a,b` | 空 | 从指定的原始用例函数名中等权抽样 |
| `+hypertest_case_file=/abs/names.txt` | 由 runner 补充 | 从文件逐行读取原始用例函数名 |

只需设置 `+enable_random_hypertest=1`，不需要配置 `directed_instr_N`。
默认候选为 `src/hypertest/case_names_random.txt` 中 6 个经过状态影响审核和
完整生成/编译/Spike 验证的用例：两个原生边界场景，以及 ALU 依赖链、非法编码
矩阵、Zvbb 矩阵、M-mode 嵌套异常。默认池包含 C 桥接，因此 GNU ISA 必须含 F/D/V；
Zvbb 用例还要求 target 支持 RVV/ZVBB，参见 `hypertest_random_testlist.yaml`。
插入位置也由现有 directed-stream 机制随机选择。

原生模板为：

- `riscv_hypertest_sv48_canonical_stream`：boundary 页具有 X 权限。
- `riscv_hypertest_sv48_canonical_x_deny_stream`：boundary 页没有 X 权限。

这是普通随机定向流之外的独立候选池，不受
`+enable_random_directed_instr` 的开关或普通 `directed_instr_N` 白名单影响。
两种随机注入可以同时开启，也可以只开启 hypertest。

候选优先级为 `hypertest_case_names`、显式 `hypertest_case_file`、runner 提供的
默认文件。候选文件只放函数名，每行一个，不放注释。完整池在 `case_names.txt`；
`case_names_active.txt` 为原仓库 active 的 944 项，active 不表示可安全混入 DV。
希望随机选择其它已导入场景时，提供经过平台及状态审核的自定义候选文件。

开启 `+restrict_store_to_data=1` 时，runner 默认改用
`case_names_restricted.txt` 中的两个原生场景。任意 C 用例的写地址没有通用静态
边界证明，因此显式选择 C 用例会报错，不能借该开关声称其不写代码。
显式提供候选文件也要接受同样的逐流限制。

如果目标是**只约束 DV 生成器自己产生的随机/定向 store**，而保留导入用例
原有的 C/ASM 写入，可改为设置 `+restrict_generated_store_to_data=1`、
`+restrict_store_to_data=0`。这会解除源 Hypertest 的严格写地址冲突，
不会替源用例增加 `FENCE.I`，也不保证运行时不向可执行物理页写入。
源用例可能通过 SATP/PTE 改映射或物理别名修改代码；必须逐例检查其同步、
自检、平台依赖与 Spike/RTL cosim 结果。若两项同时为 1，严格模式优先，
源 C/ASM 仍被拒绝。完整 cosim-on 名单只表示未证实差分不兼容，包含未验证的
H、外部 responder 和状态耦合场景，不能直接作为 NanHu 的通过名单。
具体范围和命令见[store 地址限制说明](restrict-store-to-data.md)。

各 main/sub-program 按自身基础指令数计算
`floor(instr_cnt * random_hypertest_ratio / 1000)`；ratio 大于零时 main 至少插入
一个，sub-program 没有最少插入要求。kernel/debug program 不自动插入。
ratio 为零时完全停止独立随机池插入。
这里计数的是完整定向流，流本身的设置、检查、恢复指令和辅助代码会增加最终指令数。

要固定选择某个版本，可使用普通显式定向选项，同时保留功能总开关：

```yaml
gen_opts: >
  +enable_random_hypertest=1
  +random_hypertest_ratio=0
  +enable_random_directed_instr=0
  +num_of_sub_program=0
  +directed_instr_0=riscv_hypertest_sv48_canonical_stream,1
```

此时独立随机池关闭，显式 directed 采用原有密度语义。上述配置在 main 中至少
插入一次；若需要验证两个版本都命中，应分别运行两个显式测试，而不能依赖
随机池的一次抽样。

## 两项原生边界场景的复现内容和判定

两个版本都映射低 canonical 区的最后一页，并通过 JALR 进入
`0x0000_7fff_ffff_fff0`。完整的页尾 32B 对齐块没有分支或跳转指令。
从入口开始执行 `a0 += 16`、`a0 += 32`、向 marker 存储 `a0`、`a0 += 64`，
随后顺序到达非 canonical 地址 `0x0000_8000_0000_0000`。

| 检查 | `a0` 初值 | 期望 `a0` | 期望 marker | 异常期望 |
| --- | --- | --- | --- | --- |
| 第一次跳转（cold） | `1` | `113` | `49` | `mcause=12`，`mepc=mtval=0x0000_8000_0000_0000` |
| 第二次跳转（warm） | `10` | `122` | `58` | 同上 |
| 合法 alias 对照，仅 X 允许版 | `0x55` | `0x55` | `0x55` | 无异常 |

boundary 对应的 VPN 路径始终有效，并指向实际存在的 RAM 代码页。
X 允许版在两次 fault 检查后，再访问合法的
`0xffff_8000_0000_0000` alias；它与非 canonical boundary 使用同组 PTE，
可验证该翻译路径和物理代码确实可执行，避免缺失 canonical 检查被无效 PTE、
X 权限或物理访问错误掩盖。alias 页起始会写 marker，偏移 `0x20` 才返回，
因此漏报 boundary fault 时能观察到错误执行。

X deny 版保留源用例的对比场景：该页有效、可读写，但不可执行。
它单独通过不能证明实现了 canonical 检查，因为缺少 X 权限也会产生 IPF。

## 原生模板的状态隔离与适用条件

每次插入使用独立标签和专用页表。代码静态放在 `.text.*` 中，运行时只写专用
`.data` 页表，以及由 `cfg.sp` 分配的 384B 保存帧和 marker，不向代码段写入。
因此可以与 `+restrict_store_to_data=1` 组合；保存帧要求原有栈指向有效可写内存。
流保存并恢复全部 GPR，以及 `mstatus`、`medeleg`、`satp`、`mtvec`、`mepc`、
`mcause`、`mtval` 七个 CSR。它临时安装私有 M-mode trap handler、配置 Sv48，
进入 S-mode 执行检查，再恢复外层 M-mode 程序。

启用时，生成器检查以下前提；不满足时会明确报错：

- SystemVerilog 后端，RV64、Sv48，target 声明支持 S-mode。
- `+boot_mode=m`、单 hart、非 `bare_program_mode`。
- `MSTATUS.MPRV=0`，不能同时开启随机 PMP 权限异常注入。
- 不能同时设置 `+no_directed_instr=1`。

NanHu target 必须在编译生成器时传
`--cmp_opts='+define+NANHU_V5_1_SV48'`；只设置运行时开关不会把默认 Sv39
target 改成 Sv48。PyFlow 尚未实现该功能，开启时会报错。

示例测试关闭普通随机 CSR、ECALL、EBREAK、DRET、WFI，避免周围随机指令额外
改写控制状态。若自行扩大随机候选，应继续保证插入点运行在预期的 M-mode，
且页表、栈、trap vector 和 PMP 配置满足上述条件。

## 生成和 Spike 检查

仓库提供 [hypertest_testlist.yaml](../yaml/hypertest_testlist.yaml)：

| 测试名 | 用途 |
| --- | --- |
| `hypertest_sv48_random_smoke` | 1000 条基础指令，hypertest ratio=4，普通随机定向池关闭 |
| `hypertest_sv48_mixed_restricted_smoke` | 1000 条基础指令，两种随机定向池均开启，`restrict_store_to_data=1` |
| `hypertest_sv48_canonical_explicit` | 固定运行 X 允许版，关闭额外 hypertest 随机插入 |
| `hypertest_sv48_canonical_x_deny_explicit` | 固定运行 X deny 版，关闭额外 hypertest 随机插入 |

上述四个原生 entry 均为一轮、M-mode boot，并使用
`-march=rv64imafdc_zicsr_zifencei -mabi=lp64d`。
另有 [hypertest_scoped_store_testlist.yaml](../yaml/hypertest_scoped_store_testlist.yaml)
的 `hypertest_scoped_generated_store_smoke`，专门验收范围限定开关与一个既有
Spike 验证的源用例组合；它不是全量 cosim-on 回归。
在 riscv-dv 仓库根目录运行下面命令；环境变量指向实际仓库、skill 和本地 Spike，
`SPIKE_PATH` 是包含 `spike` 的目录。每次运行使用新的 output 目录。

```bash
export RISCV_DV_HOME="$PWD"
export DV_WORKFLOW_SKILL_HOME="$HOME/.codex/skills/dv-workflow"
export SPIKE_PATH=/path/to/riscv-isa-sim/install/bin

python3 "$DV_WORKFLOW_SKILL_HOME/scripts/dv_preflight.py" \
  --repo "$RISCV_DV_HOME" \
  --target nanhu_v5_1 \
  --testlist yaml/hypertest_testlist.yaml \
  --test hypertest_sv48_random_smoke \
  --simulator vcs \
  --cmp-opts='+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss-timeout 120 \
  --iterations 1 --seed 101601 --output out/hypertest_sv48_101601

python3 run.py \
  --target nanhu_v5_1 \
  --testlist yaml/hypertest_testlist.yaml \
  --test hypertest_sv48_random_smoke \
  --simulator vcs \
  --cmp_opts='+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss_timeout 120 \
  --iterations 1 --seed 101601 --output out/hypertest_sv48_101601
```

运行另一个 entry 时，应在两条命令中同时替换 `--test`、seed 和 output，例如
mixed 使用 seed `101602`、两个显式版本分别使用 `101603` 和 `101604`。
也可以把两条命令中的 `--test` 都改成 `all`，一次验收四个 entry。

检查生成器日志的 `Selected random hypertest case:`，确认自动注入实际选择；
检查新生成的 `.S` 中专用 hypertest 标签、Sv48 设置、异常和 marker 判定，
确认测试命中。验收要求 generator 的 UVM warning/error/fatal 均为零、GNU
编译成功，且同一 ELF 实际完成 Spike 执行；具体执行结果以该次日志为准。

这些步骤不运行 LinkNan RTL。Spike 逐指令执行，无法验证前端是否实际按 32B
投机取指块取数、cache/TLB 的冷热状态或时序行为；用例通过也不证明这些 RTL
行为。后续 RTL/cosim 需使用同一次生成的 ELF/bin、seed、加载地址和 Sv48 配置，
分别报告执行结果。

## 全量 catalog 的选择与执行

全量入口在 `yaml/hypertest_catalog_testlist.yaml`，每项一轮，单独生成 ELF。
测试名含截短函数名和稳定 hash；通过 manifest 查名称，避免自行猜测长测试名：

```bash
python3 -c 'import json; m=json.load(open("src/hypertest/manifest.json")); print(next(c["catalog_test"] for c in m["cases"] if c["name"] == "test_sv48_mode"))'
```

例如 `test_sv48_mode` 的入口是
`hypertest_case_test_sv48_mode_c19f070fa402`。在仓库根目录运行：

```bash
python3 "$DV_WORKFLOW_SKILL_HOME/scripts/dv_preflight.py" \
  --repo "$PWD" --target nanhu_v5_1 \
  --testlist yaml/hypertest_catalog_testlist.yaml \
  --test hypertest_case_test_sv48_mode_c19f070fa402 \
  --simulator vcs --cmp-opts='+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss-timeout 120 \
  --seed 101605 --output out/hypertest_sv48_catalog_101605

python3 run.py --target nanhu_v5_1 \
  --testlist yaml/hypertest_catalog_testlist.yaml \
  --test hypertest_case_test_sv48_mode_c19f070fa402 \
  --simulator vcs --cmp_opts='+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss_timeout 120 \
  --seed 101605 --output out/hypertest_sv48_catalog_101605
```

随机混合和关闭开关的对照入口在 `yaml/hypertest_random_testlist.yaml`。
将上述两条命令的 testlist 改为该文件、test 改为 `all`，使用新的 output，
即可同时运行 `hypertest_random_pool_smoke` 和 `hypertest_random_pool_disabled`。
独立指定名字的例子：

```yaml
gen_opts: >
  +boot_mode=m
  +enable_random_hypertest=1
  +random_hypertest_ratio=4
  +hypertest_case_names=ai_micro_backend_issuequeue_long_alu_dependency_chain_corner,ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner
```

沿用随机池样例中的 `gcc_opts` 和控制状态限制。单个 ELF 不能混用原框架的
`page_tables.h` 静态核心和 `dynamic_page_tables.h` 动态核心；随机注入器会在
首次选中源用例后锁定该 ELF 的页表类别，手动混入时 builder 仍会报出冲突源码。
指定 `--test all` 会尝试全 catalog，包含
尚未适配运行条件的项目，不能把它当作默认全通过回归。

## C/ASM 桥接和验收含义

GNU 阶段读取生成 `.S` 中的 `HYPERTEST_CASE`，解析所选函数的 C/ASM 依赖闭包，
按最终 `-march/-mabi` 编译并缓存对象，链接原异常处理和 DV 桥接。不会额外链接
原仓库的 boot/main/注册主循环或平台 syscall。编译命令和框架机械适配记录在
`out/<run>/hypertest/*.build.json`。编译依赖 C 库头文件，但运行不链接 libc；
当前工具链提供 `lp64d` sysroot，示例统一使用该 ABI。

桥接使用独立 1 MiB 栈，保存外层所有 GPR、M/S 控制 CSR、32 个浮点寄存器、
32 个向量寄存器及 FCSR/VL/VTYPE/VSTART/VCSR；向量保存空间按实际 VLENB 分配。
每轮用对齐后的 RV64 宽 store 清零 C BSS，安装原异常处理，再执行原函数。
原布尔结果 false、`exit(0)`、
abort、失败 marker 均使程序失败。原始缺少 `TEST_END` 的测试只在派生副本中补
等价 `return test_status`，不删除断言，也不强行返回 true。

Spike 的 `.hypertest-validation.json` 记录实际执行的 begin/pass PC、原函数命中、
每个静态注入实例的命中/跳过，以及成功 C 区间中的 illegal trap PC/tval。
普通随机分支可能跳过整个块；跳过实例不计覆盖，重复执行一个实例不能补足
另一个实例。完整执行、部分执行和完全未到达分别记录，未返回或 tohost 失败
均报错。独立 catalog entry 关闭普通随机分支，方便逐项验收。

`+no_branch_jump=1` 只是保守验收配置，不是 CFI 扩展要求；基础 B/J/JALR
指令无需 CFI 扩展。大随机模板中的
`nanhu_big_random_hypertest_branch_mix_reviewed` 和
`nanhu_big_random_hypertest_branch_mix` 都用 `+no_branch_jump=0` 恢复普通
随机分支及自动候选中的 loop/JAL 流。前者是
`dv_big_random_hyp_dir.sh` 的默认入口，从已逐项通过 Spike 的六个候选抽样；
后者显式抽样完整 cosim-on 名单，可用
`TEST=nanhu_big_random_hypertest_branch_mix ./dv_big_random_hyp_dir.sh` 选择，
仍需逐项核实与 NanHu/Spike 环境的兼容性。前跳可能绕过整个 Hypertest 注入，
所以两个入口都设置 testlist 字段 `min_hypertest_hits: 1`：每个 ELF 的 Spike
日志必须至少证明一次注入完整执行，否则 runner 报错。此门槛不代表所有注入都
执行；需要逐实例命中时仍使用保守入口，并检查验证 JSON 的 skipped 数量。

负向用例允许在已经成功返回的 C 自检区间内出现原异常测试；区间外仍用 DV 的
严格 Spike 检查。通过含义为 `original_selfcheck_passed`：原断言接受了行为，
并不自动证明其内部每个负向操作都进行了足够严格的检查，也不证明中断来源、
PMA、时延或 RTL 微架构覆盖。native 区间不会豁免非法指令。

桥接不隔离锁定 PMP、任意 custom CSR、外部设备、reset、代码写入和所有可变
`.data`；这些用例保留原入口和审核提示，不能直接纳入默认随机池。当前 MSU
桥接也不初始化原框架的 H 私有状态，VS/VU 或 H 用例没有声明为已支持。
运行 H、外部中断、设备和平台专用用例前需补齐对应环境并单项验证，不能通过
关闭 ISS 检查或将失败改成成功来绕过。

## 维护清单和报告

`scripts/import_hypertest.py` 可重新盘点和更新完整快照，遇到不同内容的现有
快照会停止，避免静默覆盖。用 `--validation` 合并逐用例执行证据，用
`--compile-audit` 合并全量编译/链接诊断；报告始终区分对象、ELF、Spike 和 RTL。
`scripts/check_hypertest_compile.py --help` 提供全量静态诊断选项，其链接 fixture
不执行，不能代替上述完整验收链。桥接自测在 `yaml/hypertest_bridge_selftest.yaml`，
属于合成验证，不计入原有 4,636 个用例。
