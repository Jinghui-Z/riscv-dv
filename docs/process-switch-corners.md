# NanHu 进程切换 corner 场景与 DV 用例

本次补充面向单 hart、无 H 扩展的 RV64 Sv39/Sv48：两个 S-mode 任务交替
运行，使用相同代码/数据 VA、不同物理页，通过 ECALL 进入 M-mode 调度器。
这是原生汇编自检场景，直接生成于 DV，不依赖 Hypertest 源码或真实操作系统。

## 风险分析

| Corner | 需要观察的结果 | 软件必须遵守的顺序 |
| --- | --- | --- |
| A→B→A，同 VA、不同 PA | 取指来自当前任务的代码页；数据不会串到另一任务 | 正确切换根页表并恢复上下文 |
| 相同 ASID、更换页表根 | 旧 TLB/PTW 翻译不能污染新任务 | 写 SATP 后执行覆盖该 ASID 的 SFENCE，再访问 |
| 不同且未复用的 ASID | 两个地址空间都返回各自结果 | 映射不变时可省略额外 SFENCE；ASIDLEN=0 时退回刷新 |
| ASID=0 | 普通寄存器中的零值代表指定 ASID 0 | rs2=x0 代表所有 ASID，不能混淆两者 |
| 同 ASID 单页重映射 | 更新后访问新 PA | PTE store 后对 VA+ASID 定向 SFENCE |
| G=1 全局映射更新 | 取到更新后的全局映射 | SFENCE 的 rs2 必须为 x0 |
| 可写→只读 | 精确 store page fault，原数据不被修改 | PTE 更新后刷新，再发出 store |
| 可执行→不可执行 | 精确 instruction page fault | PTE 更新后刷新，再跳转目标 VA |
| 任务 GPR/CSR 恢复 | sp、tp、ra、保存寄存器、scratch/status 等属于各自任务 | 先保存现场，恢复 EPC/status 后返回 |
| FPU/Vector 状态 | 各任务 FPR/FCSR、VL/VTYPE/VSTART、向量寄存器隔离 | OS 必须显式保存/恢复；本次未加入该子场景 |
| LR/SC 跨切换 | reservation 不得导致另一个任务的错误写入 | 显式清 reservation；不能要求 xRET 一定保留它；本次未加入 |
| 真实 IRQ/NMI 抢占、S/U、跨 hart 迁移及 shootdown | 切换与异步事件、权限、跨核可见性共同正确 | 需要专门的调度/中断/多核 harness；本次未加入 |

当前 LinkNan 源码的关联位置（相对 `xscore/src/main/scala/xiangshan/`）：

- `backend/fu/NewCSR/NewCSR.scala` 的 `satpASIDChanged` 比较 ASID；
  `backend/fu/wrapper/CSR.scala` 将其连接为 `tlb.satp.changed`。
  不能据此假设写入相同 ASID 的新 PPN 会自动失效所有旧翻译。
- `cache/mmu/TLB.scala` 对 SFENCE 刷新翻译、在途请求及相关管线。
- `cache/mmu/TLBStorage.scala` 实现 rs1/rs2 刷新范围；
  ASID 定向 SFENCE 排除全局项。

已有 Hypertest 包含 SATP/ASID/SFENCE 独立测试，原生 DV 缺少将取指、数据、
任务上下文恢复组合起来的两个任务往返自检。本次只检查规范保证的最终结果，
不要求某个 TLB 项必须保留，也不要求缓存命中或固定 refill 次数。

## 实现和约束

- `src/riscv_process_switch_instr_lib.sv`：
  `riscv_process_switch_stream` 发出不可拆分的完整场景，含私有页表、
  静态代码页、数据页、局部 trap handler 和自检。
- 修改 PTE 后使用相应 SFENCE，A/D 预置 1，避免依赖 Svade 自动更新。
  不通过运行时 store 修改指令，兼容 `+restrict_store_to_data=1`。
- 注入前保存外围 GPR 和会修改的 CSR，完成后恢复；不触碰 FP/Vector
  寄存器，不把硬件 trap 自动保存等同于 OS 上下文保存。
- 两个任务自检的寄存器子集为 ra/sp/tp/s0/s1、sscratch/sepc/stvec 及
  SUM/MXR。ECALL 后保存下一条 PC，下一次切回恢复该 PC，并检查专用
  continuation 标记；不能靠重新从入口运行蒙混通过。sp 为上下文哨兵，
  任务 payload 不通过它访存；本次没有覆盖虚拟栈页重映射或完整 Linux 调度器。
- 固定 M boot、单 hart、MPRV=0；场景在主程序/普通子程序内注入，
  不在 kernel/debug 流内注入。禁止随机 boot、PMP 异常注入及 IRQ/NMI
  模板混用，不能在本用例运行期间由外部平台强行投递 NMI。
- 模板保留普通随机分支的版本可能跳过某些完整注入；runner 按真实提交
  PC 记录完成次数，不能把生成了场景当作已执行。

| 参数 | 含义 |
| --- | --- |
| `+directed_instr_0=riscv_process_switch_stream,N` | 用现有 directed-stream 接口插入场景；N 是每 1000 条基础指令的密度 |
| YAML `min_process_switch_hits` | Spike 必须完整执行的最少注入数；模板为 1 |

例如 `+instr_cnt=1000 +directed_instr_0=riscv_process_switch_stream,2`
会生成约两个完整场景；短于 1000 条的 main program 仍至少插入一次。
不需要单独的 `enable_random_process_switch`、`random_process_switch_ratio`
或 `process_switch_rounds` 开关。每次插入固定执行两个 A→B→A 往返，
并覆盖所有列出的关键 corner；随机的是插入位置和外围随机指令。
如需关闭场景，移除相应的 `+directed_instr_N` 条目。

可以同时使用 `+directed_instr_1=riscv_int_numeric_corner_stream,4`
等普通定向流。`+enable_random_directed_instr=1` 是原有的加权候选模式：
此时显式 `directed_instr_N` 的数字成为权重，总注入密度改由
`+random_directed_instr_ratio` 决定，因而不能单凭候选项保证进程切换
一定被选中。示例模板采用确定生成次数的普通模式，runner 另用
`min_process_switch_hits` 确认实际执行。基础 `instr_cnt` 不包含
场景自检、页表初始化和上下文保存指令。

## 生成命令

从 DV 仓库根目录执行；先通过同参数 preflight，再生成、编译、运行 Spike。
以下为固定 seed、Sv48 smoke；去掉两条命令的 Sv48 编译参数即可验证 Sv39。
输出目录必须全新。

```bash
python3 "$HOME/.codex/skills/dv-workflow/scripts/dv_preflight.py" \
  --repo "$PWD" --target nanhu_v5_1 \
  --testlist yaml/process_switch_testlist.yaml --test nanhu_process_switch_smoke \
  --simulator vcs --cmp-opts '+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss-timeout 120 \
  --iterations 1 --seed 20261124 --output out/process_switch_sv48_seed20261124
python3 run.py --target nanhu_v5_1 \
  --testlist yaml/process_switch_testlist.yaml --test nanhu_process_switch_smoke \
  --simulator vcs --cmp_opts '+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss_timeout 120 \
  --iterations 1 --seed 20261124 --output out/process_switch_sv48_seed20261124
```

随机混合版本选择 `nanhu_process_switch_branch_mix`；
大随机矩阵选择 `yaml/big_random_feature_matrix.yaml` 中的
`nanhu_big_random_process_switch`。不配置该 directed 条目的对照回归为
`nanhu_process_switch_disabled`。批量生成时将单 seed 改为
`--start_seed 20261124 --iterations 10 --batch_size 1`，preflight 和
`run.py` 保持一致。

## 验收证据

runner 的 `scripts/process_switch_validation.py` 根据 ELF 符号与 Spike
真实提交 PC 验证 begin/pass、场景到达及最终 tohost=1。进入场景却未完成、
进入失败出口、非成功 tohost 或完成数不足均判失败。报告输出为
`spike_sim/*.process-switch-validation.json`；开启分支后被跳过的注入
单独记录，不能算覆盖。

GNU/Spike 用来验证用例路径和自检；DUT 的 TLB/PTW/IFU、精确异常和上下文
实现是否正确还应将同一 `.bin` 送入 RTL+Difftest。裸 bin 入口/加载地址为
`0x80000000`，不要从文件后缀推断加载地址。

2026-11-24 当前 `+directed_instr_N` 配置的验证结果（均执行完整
generator → GNU → Spike，显式 `--iss spike`）：

| 配置 | seed / 用例 | 结果 |
| --- | --- | --- |
| Sv39 | 20261124；smoke + branch_mix | 2/2 通过，实际注入 1+2 次 |
| Sv48 | 20261124；smoke + branch_mix | 2/2 通过，实际注入 1+2 次 |
| 较高密度 | 20261125；Sv39，ratio=3/1000 | 1/1 通过，3 次注入均在 main，不落入 kernel |
| 大随机矩阵模板 | 20261126；nanhu_big_random_process_switch | 1/1 通过 |
| NanHu 无注入对照 | disabled，20261124 | 1/1 通过，汇编无进程切换注入 |
| 邻近 rv64gc | 无注入对照，20261124 | 1/1 通过，汇编无进程切换注入 |
| Python 运行证据/runner 回归 | 17 项新 validator 测试 + 4 项 runner 测试 | 21/21 通过 |

当前启用版本产物分别位于
`out/process_switch_directed_sv39_seed20261124/` 和
`out/process_switch_directed_sv48_seed20261124/`，密度 3 的额外回归位于
`out/process_switch_directed_ratio3_seed20261125/`，大随机矩阵产物位于
`out/process_switch_directed_big_random_seed20261126/`。每次实际完成的注入均到达所有
corner 标记并完成返回 PC、寄存器/CSR、数据及精确异常检查，正常 tohost=1。
对应 UVM warning/error/fatal 均为 0。无注入对照及 rv64gc 产物位于
`out/process_switch_directed_sv39_seed20261124/`、
`out/process_switch_directed_rv64gc_disabled_seed20261124/`。

本次未完成 RTL 回归；先前会话使用的 20261029 release 路径现已不存在。
以上结果不能替代当前 release 上的 DUT 检查。
