# NanHu 外部中断与 NMI 大随机模板

`yaml/big_random_feature_matrix.yaml` 中的
`nanhu_big_random_irq_nmi_forward_progress` 是封闭验证服务器的注入模板。
每个用例配置 1,000 条基础随机指令，默认脚本生成 700 个不同 seed 的用例。
定向指令流和初始化代码会增加最终汇编指令数；可通过
`ITERATIONS=10` 缩小批量，修改 YAML 的 `+instr_cnt` 增加单例长度。
模板保留普通分支、随机 directed stream、随机访存和 store 数据区限制。
为了使中断返回路径可控，它固定单 hart、M-mode boot 和 DIRECT `mtvec`；
关闭 Hypertest、随机 boot、随机 CSR、timer IRQ 和嵌套中断。
NanHu V5.1 target 未声明 PMP，故不打开 PMP 异常注入。

## Handler 行为

- `+enable_interrupt=1` 配置 `mie.MEIE`，`mret` 返回 M-mode 时启用
  `mstatus.MIE`；初始化末尾再次置位 MIE，确保进入主程序前可响应 MEI。
- `+enable_nanhu_nmi_handler=1` 先设置 `mtvec`，待 `mstatus/mie`、
  GPR、栈、可选 CSR 和初始化握手完成后才置位 `mnstatus.NMIE`。
  NanHu RTL 将 NMI_31/NMI_43 送到 DIRECT `mtvec`；NMI 入口清除 NMIE，
  共享入口据此区分 NMI 与普通 trap。
- M 外部中断用 `mcause` 识别，按 `mepc` 所指半字低两位判断
  16/32 位指令长度，写回 `mepc + 2/4`，恢复 GPR，执行 `mret`。
  其他普通中断不修改 EPC。
- NMI 同样按 `mnepc` 所指半字判断长度，写回 `mnepc + 2/4`，
  恢复 GPR，执行 `mnret`。本地 GNU 汇编器不认识 Smrnmi 的
  `mnret` 助记符，故输出 `.word 0x70200073`；该测试的 Spike ISA
  单独加 `_smrnmi`，不改变 GNU `-march`。

`+advance_async_epc=1` 是**显式压力策略**：每次中断刻意跳过一条指令。
这不是异步中断的无损架构恢复语义；正常恢复应保留 EPC。跳过分支、调用或
地址构造指令可能改变控制流，不能把此模板当成精确恢复用例。
旧版生成器在 `mstatus/mie` 写入前就开启 NMIE；启动阶段若发生 NMI，
EPC 前进可能跳过关键 CSR 写入或 `mret`，导致 MEI 无法响应。当前模板
已把 NMIE 延后到初始化末尾，并显式禁用随机 CSR 与 dummy CSR 写入。
需要无损恢复时，把该选项改为 0，并由服务器及时撤销中断源。
该模式只适合固定 M-mode 且代码地址可读取的场景；不支持在 S/U 虚拟地址上
通过 EPC 读取指令长度。

## 运行

先确认 `RISCV_GCC`、`RISCV_OBJCOPY`、`SPIKE_PATH`、VCS
和 Python/PyYAML 可用；`SPIKE_PATH` 指向包含 Spike 可执行文件的目录。
在仓库根目录执行：

```bash
ITERATIONS=10 START_SEED=20261105 ISS_TIMEOUT=600 ./dv_big_random_irq_nmi.sh
```

单例固定 seed 复现：

```bash
python3 "$HOME/.codex/skills/dv-workflow/scripts/dv_preflight.py" \
  --repo "$PWD" --target nanhu_v5_1 \
  --testlist yaml/big_random_feature_matrix.yaml \
  --test nanhu_big_random_irq_nmi_forward_progress --simulator vcs \
  --cmp-opts '+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss-timeout 600 \
  --iterations 1 --seed 20261105 --output out/irq_nmi_seed20261105
python3 run.py --target nanhu_v5_1 \
  --testlist yaml/big_random_feature_matrix.yaml \
  --test nanhu_big_random_irq_nmi_forward_progress --simulator vcs \
  --cmp_opts '+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss_timeout 600 \
  --iterations 1 --seed 20261105 --output out/irq_nmi_seed20261105
```

每次运行使用全新 `OUT`，脚本默认带时间戳。生成产物位于
`OUT/asm_test/`，实际 seed 位于 `OUT/seed.yaml`，generator 和
Spike 日志分别位于 `OUT/sim_*.log`、`OUT/spike_sim/`。
ELF 使用 `.o` 后缀；裸 `.bin` 应按 linker 入口 `0x80000000` 加载。

## 封闭服务器接入要求

1. 注入 M external interrupt（MEI，`mcause` 中断号 11），并在
   `mret` 前撤销电平型 MEI；否则 EPC 增加也挡不住重复进入中断。
   如依赖 PLIC claim/complete，先提供实际平台 context 地址再启用
   `+enable_plic_claim_complete`，不要猜测 MMIO 地址。
2. NMI_31/NMI_43 也应及时撤销，避免 `mnret` 后再次锁存；
   MEI/NMI 都应在初始化完成、进入 `main` 后才开始注入，在
   `test_done` 和退出路径前停止。NMIE 现于初始化末尾开启；
   应以到达 `main` 为注入起点，不能仅凭 `CORE_STATUS=INITIALIZED`
   就开始发一次性 NMI 脉冲，因为握手稍早于 NMIE 置位。
3. 记录实际投递次数、`mcause/mncause`、`mepc/mnepc` 修改前后、
   `mret/mnret` 和最终退出结果；co-sim 参考模型须同步相同中断事件
   和 EPC 策略。

`--iss spike` 验证无外部注入时的 ELF 执行，**不证明**服务器的
MEI/NMI 投递、handler 动态闭环或 RTL/co-sim 一致性。
