# NanHu V5.1 用例生成使用说明

本文说明如何使用本仓库的 SystemVerilog/UVM 生成器生成 NanHu V5.1
随机汇编用例，并将其编译为 ELF 和裸二进制。NanHu target 的配置入口为：

- [`target/nanhu_v5_1/riscv_core_setting.sv`](../target/nanhu_v5_1/riscv_core_setting.sv)：架构、CSR、特权级和地址翻译模式配置。
- [`target/nanhu_v5_1/testlist.yaml`](../target/nanhu_v5_1/testlist.yaml)：NanHu 用例清单及每项生成参数。
- [`docs/NanHu-V5.1-riscv-dv-gap-analysis.md`](NanHu-V5.1-riscv-dv-gap-analysis.md)：规格覆盖范围、实现边界和已知风险。

文中的命令均从 riscv-dv 仓库根目录执行。

## 1. 运行范围

当前 `nanhu_v5_1` 是预定义的 SystemVerilog target，默认配置为 RV64、
`lp64d`、M/S/U 特权级和 Sv39。它不支持 `--simulator pyflow`。本文使用
VCS；其他 SystemVerilog/UVM 模拟器需要在实际环境中另行验证。

推荐将生成和编译明确限制为：

```bash
--steps gen,gcc_compile
```

这两个阶段分别完成 UVM 随机程序生成和 GNU 汇编/链接，不会运行 NanHu
RTL。testlist 中的 `rtl_test: core_base_test` 只是下游 testbench 的集成元数据，
`run.py` 当前不会消费该字段。

## 2. 环境准备

### 2.1 Python 和 VCS

```bash
cd /nfs/home/zhongjinghui/tool/ISG/riscv-dv
pip3 install -r requirements.txt

python3 run.py --help
command -v vcs
```

VCS 环境需要支持 SystemVerilog 和 UVM 1.2，并配置好许可证。当前机器已验证的
VCS 路径为 `/nfs/tools/synopsys/vcs/U-2023.03/bin/vcs`。

### 2.2 RISC-V GNU 工具链

`gcc_compile` 阶段通过以下环境变量定位编译器和 objcopy：

```bash
export RISCV_GCC=/nfs/share/opt/riscv/bin/riscv64-unknown-linux-gnu-gcc
export RISCV_OBJCOPY=/nfs/share/opt/riscv/bin/riscv64-unknown-linux-gnu-objcopy

"${RISCV_GCC}" --version
"${RISCV_OBJCOPY}" --version
```

使用其他工具链时，应确保其支持 testlist 中的 Vector 1.0、bitmanip、crypto、
Svinval 和 Svpbmt 等 `-march` 扩展字符串。

## 3. 快速开始

### 3.1 生成并编译单个用例

下面的命令生成一个 Vector 1.0/Zvbb 用例，并产生 `.S`、`.o` 和 `.bin`：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_vector_zvbb_smoke \
  --simulator vcs \
  --steps gen,gcc_compile \
  --seed 9101 \
  --output out/nanhu_vector
```

### 3.2 只生成汇编

只需要 `.S` 文件时，可跳过 GNU 编译：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_base_smoke \
  --steps gen \
  --seed 9102 \
  --output out/nanhu_base_asm
```

### 3.3 运行默认回归

`--test all` 会选择 testlist 中 14 个 `iterations: 1` 的默认用例；两个
`iterations: 0` 的可选项不会运行。

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test all \
  --simulator vcs \
  --steps gen,gcc_compile \
  --start_seed 9200 \
  --output out/nanhu_all
```

每项默认生成一次，因此成功时 `out/nanhu_all/asm_test/` 下应有 14 个
`.S`、14 个 `.o` 和 14 个 `.bin`。

### 3.4 一次选择多个用例

`--test` 接受逗号分隔的名称：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_scalar_extensions_smoke,nanhu_v5_1_scalar_crypto_smoke \
  --iterations 5 \
  --steps gen,gcc_compile \
  --start_seed 9400 \
  --output out/nanhu_scalar_regression
```

`--iterations N` 会覆盖已启用条目的默认次数，但不能启用原本为
`iterations: 0` 的条目，相关操作见第 7 节。

## 4. Seed 管理与重现

每次生成会把实际使用的 seed 映射写入输出目录的 `seed.yaml`。

固定 seed 重现单项用例：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_vector_smode_smoke \
  --steps gen,gcc_compile \
  --seed 9200 \
  --output out/nanhu_replay_one
```

重放一次已有回归：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test all \
  --steps gen,gcc_compile \
  --seed_yaml out/nanhu_all/seed.yaml \
  --output out/nanhu_replay_all
```

使用 `--seed_yaml` 时应保持原回归的 test 选择、`--iterations` 和
`--batch_size`。runner 只从该文件恢复 seed，不会恢复生成次数或 batch 划分；
例如重放一个 `--iterations 5` 的回归时，重放命令也必须显式带上
`--iterations 5`。`--seed`、`--start_seed` 和 `--seed_yaml` 应只使用其中一个；
`--seed` 固定为单次迭代，不能与大于 1 的 `--iterations` 同用。

## 5. Sv39、Sv48 和 Bare 模式

地址翻译模式是生成器编译期配置。默认使用 Sv39；建议不同模式使用不同输出
目录，避免复用上一种模式的 VCS 产物。

### 5.1 Sv48

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_svinval_svpbmt_smoke \
  --cmp_opts="+define+NANHU_V5_1_SV48" \
  --steps gen,gcc_compile \
  --seed 5201 \
  --output out/nanhu_sv48
```

### 5.2 Bare

Bare 模式应选择不依赖页表的用例，例如 base smoke：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_base_smoke \
  --cmp_opts="+define+NANHU_V5_1_BARE" \
  --steps gen,gcc_compile \
  --seed 5202 \
  --output out/nanhu_bare
```

Bare 结果只证明模式选择和生成路径可用，不能作为 Svpbmt、Svinval 或 Svade
语义验证结果。

## 6. 默认用例清单

| 用例 | 默认 | 主要用途 |
| --- | --- | --- |
| `nanhu_v5_1_base_smoke` | 开 | RV64 IMAFDC、特权、浮点、非对齐访存；唯一未设置 `no_iss` 的项目。 |
| `nanhu_v5_1_scalar_extensions_smoke` | 开 | Zba/Zbb/Zbc/Zbkc/Zbs、Zcb、Zicond、Zimop/Zcmop 和 Zicbo*。 |
| `nanhu_v5_1_scalar_crypto_smoke` | 开 | Zbkb/Zbkc/Zbkx、AES/SHA、SM3/SM4 及 Zkn/Zks 组合。 |
| `nanhu_v5_1_zbkc_subset_smoke` | 开 | 严格 Zbkc 子集；通过不启用 Zbc 捕获错误的 `clmulr` 生成。 |
| `nanhu_v5_1_svinval_svpbmt_smoke` | 开 | S-mode、Svinval、Svpbmt、A=0 Svade fault 和 tval 观测。 |
| `nanhu_v5_1_svade_dirty_smoke` | 开 | 定向生成 A=1、D=0 的 Svade dirty-bit page fault。 |
| `nanhu_v5_1_stval_delegation_smoke` | 开 | U-mode 异常委托到 S-mode，并通过 signature 输出 STVAL。 |
| `nanhu_v5_1_csr_smoke` | 开 | base、stateen、Sstc、Sscofpmf、trigger、FP 和 Vector CSR。 |
| `nanhu_v5_1_invalid_csr_smode_smoke` | 开 | S-mode 非法级别 CSR 访问，包括 trigger CSR。 |
| `nanhu_v5_1_sscofpmf_state_smoke` | 开 | LCOFI 委托和 M/S 中断使能状态；事件选择仍为平台相关。 |
| `nanhu_v5_1_stateen_umode_smoke` | 开 | U-mode FP 以及 `mstateen0/sstateen0.FCSR` 启动路径。 |
| `nanhu_v5_1_vector_zvbb_smoke` | 开 | M-mode Vector 1.0、Zvbb、FP64、widening/narrowing 和向量访存。 |
| `nanhu_v5_1_vector_smode_smoke` | 开 | S-mode Vector 1.0/Zvbb 和 `sstatus.VS` 初始化。 |
| `nanhu_v5_1_debug_rom_smoke` | 开 | Debug ROM、DCSR/DPC/scratch、debug ebreak 和 single-step 生成。 |
| `nanhu_v5_1_scalar_encoding_unit` | 关 | UVM 内部 scalar/crypto 编码与扩展分组断言；不生成汇编。 |
| `nanhu_v5_1_platform_interrupt_example` | 关 | ACLINT 初始化和 PLIC claim/complete 的地址集成模板。 |

除 `nanhu_v5_1_base_smoke` 外，其余 15 项均设置了 `no_iss: 1`。因此即使
使用默认的 `--steps all`，这些项目也不会进入 ISS；NanHu 日常生成回归应显式
使用 `--steps gen,gcc_compile`。

不要对 NanHu 全量 testlist 使用 `--iss <iss1>,<iss2> --steps all`：当前
`iss_sim` 会跳过 `no_iss` 项，但双 ISS 的 `iss_cmp` 仍会尝试读取这些项目的
日志。确需 ISS 比对时，应使用只包含 ISS 已支持项目的独立 testlist。

## 7. 运行两个默认关闭的项目

当前 runner 会先过滤 `iterations: 0` 的项目，再应用命令行次数覆盖。因此，
下面的写法**不能**启用关闭项：

```bash
# 当前实现会报 Cannot find ...
python3 run.py --target nanhu_v5_1 \
  --test nanhu_v5_1_scalar_encoding_unit --iterations 1
```

正确做法是复制一份 testlist，在副本中将目标项的 `iterations` 改为 `1`，然后
通过 `--testlist` 指定该副本：

```bash
mkdir -p out/nanhu_config
cp target/nanhu_v5_1/testlist.yaml out/nanhu_config/testlist.yaml
```

### 7.1 Scalar encoding unit

将副本中的 `nanhu_v5_1_scalar_encoding_unit` 改为 `iterations: 1` 后运行：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --testlist out/nanhu_config/testlist.yaml \
  --test nanhu_v5_1_scalar_encoding_unit \
  --steps gen \
  --seed 9300 \
  --output out/nanhu_scalar_encoding
```

该项目只执行 UVM 内部编码断言，不生成 `.S`、`.o` 或 `.bin`，所以不能添加
`gcc_compile`。

### 7.2 平台中断模板

在启用 `nanhu_v5_1_platform_interrupt_example` 前，必须同时在 testlist 副本中：

1. 将其 `iterations` 改为 `1`。
2. 将 `aclint_msip_addr`、`aclint_mtimecmp_addr`、
   `plic_m_claim_complete_addr` 和 `plic_s_claim_complete_addr` 替换为真实
   NanHu/SoC MMIO 地址。
3. 根据集成确认各 stride。当前 target 的 `NUM_HARTS=1`，只修改 stride 不会
   使生成器产生多 hart 程序。
4. 由 testbench/平台配置 PLIC priority、enable 和 threshold，并注入实际外部
   中断。模板只在 external interrupt handler 中执行 claim/complete，不负责
   配置或产生中断。

ACLINT 初始化会清零 MSIP，并把 MTIMECMP 写为全 1，即初始关闭软件和定时器
触发；后续 IRQ 触发与投递同样需要 testbench/平台配合。

完成地址配置后运行：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --testlist out/nanhu_config/testlist.yaml \
  --test nanhu_v5_1_platform_interrupt_example \
  --steps gen,gcc_compile \
  --seed 9301 \
  --output out/nanhu_platform_interrupt
```

仓库中的 `0x02000000`、`0x02004000`、`0x0c200004` 和 `0x0c201004` 只是传统
占位地址，不能直接作为 NanHu 平台验证结果。

## 8. 输出文件与结果检查

典型输出如下：

| 路径 | 内容 |
| --- | --- |
| `compile.log` | 生成器的 VCS 编译日志。 |
| `sim_<test>_<batch>.log` | UVM 生成日志。 |
| `seed.yaml` | test/batch 到随机 seed 的映射。 |
| `asm_test/<test>_<idx>.S` | 生成的汇编程序。 |
| `asm_test/<test>_<idx>.o` | 链接后的 ELF，虽然后缀为 `.o`。 |
| `asm_test/<test>_<idx>.bin` | 供下游 memory loader 使用的裸二进制；文件本身不携带加载地址。 |
| `vcs_simv`、`vcs_simv.csrc/` | VCS 可执行文件和编译目录。 |

默认 [`scripts/link.ld`](../scripts/link.ld) 将 `.text` 放在 `0x80000000`，入口
符号为 `_start`。下游加载 `.bin` 时必须以 `0x80000000` 为基址，并让复位/启动
流程进入 `_start`；若 NanHu 地址图不匹配，应同步调整链接脚本和 testbench
loader，不能只把无地址信息的 `.bin` 放入任意 RAM。

可用以下命令检查生成日志和产物数量：

```bash
rg -n "TEST PASSED|UVM_ERROR|UVM_FATAL" out/nanhu_all/sim_*.log
find out/nanhu_all/asm_test -maxdepth 1 -name '*.bin' -print
find out/nanhu_all/asm_test -maxdepth 1 -name '*.bin' | wc -l
```

成功的生成日志应包含 `TEST PASSED`，UVM summary 中 `UVM_ERROR` 和
`UVM_FATAL` 均应为 0。默认回归的 `.bin` 数量应为 14。

`run.py` 当前会保留已有输出目录内容，`--noclean` 的默认值也是 true。为避免
旧日志或旧二进制混入结果，普通回归应使用新的输出目录；只有明确复用已编译
生成器时才使用同一目录。

## 9. 生成器编译复用

大量单项回归可以先只编译一次生成器，再在同一输出目录中只运行仿真：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --steps gen \
  --co \
  --output out/nanhu_cached

python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_base_smoke \
  --steps gen \
  --so \
  --seed 9500 \
  --output out/nanhu_cached
```

第二条命令只生成 `.S`。如需 ELF 和 binary，再运行：

```bash
python3 run.py \
  --target nanhu_v5_1 \
  --test nanhu_v5_1_base_smoke \
  --steps gcc_compile \
  --seed 9500 \
  --output out/nanhu_cached
```

Sv39、Sv48、Bare 或其他 `--cmp_opts` 不同的配置不能共用同一个已编译
`vcs_simv`。

## 10. 接入 NanHu RTL 前的必要配置

生成器和 GCC 通过不等于 NanHu RTL 执行通过。接入真实 testbench 前至少需要
确认以下事项：

1. 默认链接脚本从 `0x80000000` 布局程序。确认该地址及完整链接映像位于可执行
   RAM 中，testbench 以同一基址加载 binary，并使复位/boot PC 最终进入
   `_start`；否则应先定制链接布局。
2. `nanhu_v5_1_svinval_svpbmt_smoke`、`nanhu_v5_1_svade_dirty_smoke` 和
   `nanhu_v5_1_stval_delegation_smoke` 使用的 `signature_addr` 分别为
   `0x8ffffff0`、`0x8fffffe0` 和 `0x8fffffd0`。这些只是生成器 scratch
   占位地址，必须在 testlist 副本中换成 SoC 内真实可写地址。
3. `enable_tval_check` 只生成 MTVAL/STVAL 的 signature 观测路径，不会在
   riscv-dv runner 中自动判断 DUT 行为。
4. target 当前固定 `VLEN=128`、`ELEN=64` 和 `MAX_LMUL=8`，但规格工作簿没有
   确认这些数值。运行 Vector binary 前必须与实际 NanHu RTL 参数核对，否则
   生成器的 VL/SEW/LMUL 和寄存器组合法性假设可能不成立。
5. Vector smoke 设置 `enable_zvlsseg=0`，因为规格表记录 segment load 存在
   NanHu LSQ 死锁风险；当前 smoke 不能作为 segment load/store 覆盖证据。
6. Debug smoke 只生成 Debug ROM 和相关 CSR 序列，外部 debug transport、
   halt/resume 和执行语义仍需 testbench 驱动及检查。
7. 除 base smoke 外的项目没有 ISS 比对；scalar/crypto、特权、MMU 和 Vector
   扩展的执行正确性必须由 NanHu RTL 环境、参考模型或 checker 验证。

因此，日志中的 `TEST PASSED` 仅表示 UVM 生成阶段通过；`.bin` 生成表示 GNU
汇编、链接和 objcopy 通过，二者都不能单独作为 NanHu 架构合规结论。
