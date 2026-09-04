# riscv-dv 随机生成增强功能说明

本文说明本仓库新增的三项 opt-in 功能：随机 directed instruction stream、随机
boot mode，以及 PMP 随机权限异常注入。三项功能默认均关闭，因此不带新选项的
现有 testlist 保持原有行为。

## 1. 选项总览

| 功能 | `gen_opts` 主选项 | 别名 | 默认值 |
| --- | --- | --- | --- |
| 随机 directed stream | `+enable_random_directed_instr=1` | `+random_directed_instr=1` | 关闭 |
| directed stream 总密度 | `+random_directed_instr_ratio=N` | 无 | `4/1000` |
| 随机 boot mode | `+enable_random_boot_mode=1` | `+random_boot_mode=1` | 关闭 |
| PMP 权限异常注入 | `+enable_random_pmp_exception=1` | `+pmp_exception_inject=1` | 关闭 |

`random_directed_instr_ratio` 的范围是 `0..1000`。它表示每 1000 条基础指令
插入多少个 directed stream，而不是单个 stream 的选择概率。

## 2. 随机 directed instruction stream

### 2.1 不配置候选时

只需要在 testlist 中加入以下选项，不需要再写任何 `directed_instr_N` 或
`stream_name_N`：

```yaml
gen_opts: >
  +enable_random_directed_instr=1
  +random_directed_instr_ratio=20
```

生成器会从当前后端、ISA、扩展开关和数据区配置共同支持的全部自动候选中等权
随机选择。每个 main program、sub-program 和 kernel program 都会按自身指令数
计算插入数量；main program 至少插入一个。日志中的
`Random directed instruction candidates:` 会列出本次调用的实际候选集合。

SystemVerilog 后端的自动候选如下：

| 生效条件 | 自动候选 |
| --- | --- |
| 始终可用 | `riscv_int_numeric_corner_stream` |
| 未设置 `no_branch_jump` | `riscv_loop_instr`、`riscv_jal_instr` |
| 未设置 `no_load_store`，且存在当前级别数据页 | `riscv_single_load_store_instr_stream`、`riscv_load_store_stress_instr_stream`、`riscv_load_store_rand_instr_stream`、`riscv_hazard_instr_stream`、`riscv_load_store_hazard_instr_stream`、`riscv_mem_region_stress_test` |
| 上述 load/store 条件，且至少两个当前级别数据页 | `riscv_multi_page_load_store_instr_stream` |
| 上述 load/store 条件，且开启 Zicbom 或 Zicboz | `riscv_cbo_instr_stream` |
| 上述 load/store 条件，且开启 Vector | `riscv_vector_load_store_instr_stream` |
| 上述 load/store 条件，开启旧版 Vector、关闭 Vector 1.0，且支持 A | `riscv_vector_amo_instr_stream` |
| 未设置 `no_load_store`，AMO 数据区存在，且 `RV32A` 生效 | `riscv_load_store_shared_mem_stream`、`riscv_lr_sc_instr_stream`、`riscv_amo_instr_stream` |

DSim 因动态数组随机化限制不会自动加入 `riscv_loop_instr`。kernel program 使用
`s_mem_region` 判断数据页，其余程序使用 `mem_region`。所有自动候选的默认权重
均为 1。

PyFlow 后端会从其当前能安全构造和随机化的全部候选中选择：

- `riscv_int_numeric_corner_stream`
- `riscv_jal_instr`
- `riscv_load_store_rand_instr_stream`
- `riscv_load_store_hazard_instr_stream`
- `riscv_load_store_stress_instr_stream`
- `riscv_single_load_store_instr_stream`
- `riscv_loop_instr`

当 `no_data_page=1` 或 `no_load_store=1` 时，PyFlow 会过滤四个 load/store
候选；`no_branch_jump=1` 会过滤 loop/JAL。某个候选若因目标配置仍无法生成有效
非空指令序列，PyFlow 会记录 warning、移除该候选，并继续完成其余插入。

### 2.2 显式候选和权重

提供 `directed_instr_N` 后，该列表就是随机选择的白名单，不再自动追加其他
stream；逗号后的 ratio 作为选择权重：

```yaml
gen_opts: >
  +enable_random_directed_instr=1
  +random_directed_instr_ratio=20
  +directed_instr_0=riscv_loop_instr,10
  +directed_instr_1=riscv_jal_instr,2
```

这个例子每次插入时选择 loop 的权重为 10，选择 JAL 的权重为 2。关闭随机模式
时，`directed_instr_N` 仍沿用原 riscv-dv 语义：ratio 决定各 stream 的确定性
插入数量。

`+no_directed_instr=1` 的优先级最高，会同时关闭普通和随机 directed stream。
权重为 0 的显式候选不参与选择。

### 2.3 自动候选边界

“全部支持”指能够在当前调用上下文中独立构造、随机化并嵌入现有程序的 stream，
不是把所有派生类名无条件交给 UVM factory：

- `riscv_directed_instr_stream`、`riscv_mem_access_stream`、load/store 和 AMO base
  class 是抽象/基础实现，不生成可独立执行的完整序列。
- `riscv_jump_instr` 需要调用者提供目标 label；push/pop stack 类由专用流程配对
  使用，不能单独随机插入。
- `riscv_load_store_rand_addr_instr_stream` 依赖 SoC 的物理内存窗口，自动使用会把
  普通用例变成平台地址测试；需要时仍可通过显式候选启用。
- CBO、Vector、AMO 和共享内存 stream 只有在所需 ISA、功能开关、数据区及链接
  symbol 均存在时才进入自动候选。

随机注入路径会要求 loop/JAL 等相关 stream 保留生成器的 SP、TP、RA、scratch、
PMP 及调用栈寄存器，避免 directed stream 破坏外层程序的返回和结束流程。

## 3. 随机 boot mode

```yaml
gen_opts: >
  +enable_random_boot_mode=1
```

每个生成 iteration 会从 target 的 `supported_privileged_mode` 中随机选择 boot
mode。两种 mode 时沿用 6:4 权重，三种 mode 时沿用 4:3:3 权重；其他数量等权。
选择结果会重新生成当前特权级不可访问的 CSR 列表。

优先级如下：

1. 显式 `+boot_mode=m|s|u` 最高，并检查 target 是否支持该 mode。
2. 未显式指定且 `+enable_random_boot_mode=1` 时随机选择。
3. 开关关闭时保持确定性 M-mode；若 target 不支持 M-mode，则使用其声明的第一种
   mode。

SV 和 PyFlow 均支持此开关。PyFlow 尚未实现页表生成，因此其 S/U-mode 当前使用
物理地址路径；这不影响 SystemVerilog 后端。`run.py` 为声明 M/S/U 的 RV64
预定义 target 向 Spike 配置 `--priv=msu`，避免随机选到的 lower privilege mode
被 ISS 参数错误拒绝。

## 4. PMP 随机权限异常注入

```yaml
gen_opts: >
  +enable_random_pmp_exception=1
  +boot_mode=s
  +no_delegation=1
```

该功能仅在 SystemVerilog 后端实现。生成器随机选择 load access fault 或 store/AMO
access fault，在第一块用户数据区上构造一个缺失相应权限的 PMP entry，并生成一次
必然命中该区域的访问。M-mode trap handler 根据 fault 类型修复 PMP 权限，执行
`mret` 后重试同一条访问，再进入正常 main program。

开启后如果原配置少于三个 PMP region，生成器会提升到三个，以覆盖代码区、故障
数据区和尾部允许区。RV64 的 PMP config CSR 地址按偶数 CSR 编号映射，不访问保留
的 `pmpcfg1/pmpcfg3`。地址翻译开启时，handler 会把 `mtval`/`mepc` 对应的虚拟地址
转换回 PMP 使用的物理地址。

当前限制会在生成阶段强制检查：

- target 必须设置 `support_pmp=1`，且启用正常 PMP exception handler。
- 必须使用 S-mode 或 U-mode boot、单 hart、非 bare program，并生成用户数据页。
- 必须设置 `+no_delegation=1`；当前没有 delegated S-mode PMP 修复 handler。
- 不能设置 `+suppress_pmp_setup=1`。
- ePMP target 必须使用 legacy `MSECCFG(MML=0, MMWP=0, RLB=1)`。
- NanHu V5.1 规格未声明 PMP/ePMP，因此 `nanhu_v5_1` target 的
  `support_pmp=0`，不能启用该功能。

## 5. 源码改动位置

| 范围 | 文件 | 说明 |
| --- | --- | --- |
| SV 配置 | `src/riscv_instr_gen_config.sv` | 解析随机 directed/boot 开关、维护默认 M-mode 和显式 boot 优先级 |
| SV directed | `src/riscv_asm_program_gen.sv`、`src/riscv_instr_stream.sv`、`src/riscv_loop_instr.sv`、`src/riscv_directed_instr_lib.sv` | 自动候选过滤、加权选择、插入、运行时寄存器保护 |
| SV PMP | `src/riscv_pmp_cfg.sv`、`src/riscv_asm_program_gen.sv`、`src/riscv_instr_pkg.sv` | fault layout、访问与恢复、RV64 PMP CSR 映射、trap stack 对齐 |
| PyFlow 配置 | `pygen/pygen_src/riscv_instr_gen_config.py` | 随机 directed/boot 参数、每 iteration boot 选择、CSR 权限集刷新 |
| PyFlow directed | `pygen/pygen_src/riscv_asm_program_gen.py`、`pygen/pygen_src/riscv_utils.py`、`pygen/pygen_src/riscv_instr_stream.py`、`pygen/pygen_src/riscv_loop_instr.py`、`pygen/pygen_src/riscv_directed_instr_lib.py`、`pygen/pygen_src/riscv_load_store_instr_lib.py` | factory/安全候选、随机插入、失败候选降级、loop/JAL 保护、逐地址 load/store 候选过滤 |
| PyFlow 稳定性 | `pygen/pygen_src/isa/riscv_instr.py`、`riscv_callstack_gen.py`、`riscv_reg.py`、`riscv_privil_reg.py`、`riscv_privileged_common_seq.py` | 指令模板深拷贝、CSR、callstack、特权寄存器和页表未实现路径修复 |
| Runner/target | `run.py`、`pygen/pygen_src/target/rv64imafdc/riscv_core_setting.py`、`target/rv32imafdc/testlist.yaml`、`target/rv64gc/testlist.yaml`、`target/rv64imafdc/testlist.yaml` | Python 解释器一致性、ISS privilege、PyFlow 页表能力声明、原随机特权用例显式开启新开关 |

## 6. 生成与 Spike 验收

完整验收必须使用全新的 output 目录，并显式执行 generator、GNU 编译和 Spike：

```bash
cd /nfs/home/zhongjinghui/tool/ISG/riscv-dv

python3 /nfs/home/zhongjinghui/.codex/skills/dv-workflow/scripts/dv_preflight.py \
  --repo "$PWD" \
  --target nanhu_v5_1 \
  --test <test-name> \
  --steps gen,gcc_compile,iss_sim \
  --iss spike \
  --iss-timeout 120 \
  --simulator vcs \
  --seed <seed> \
  --output out/<new-output>

python3 run.py \
  --target nanhu_v5_1 \
  --test <test-name> \
  --simulator vcs \
  --steps gen,gcc_compile,iss_sim \
  --iss spike \
  --iss_timeout 120 \
  --seed <seed> \
  --output out/<new-output>
```

用于验证“无显式候选”的 testlist 项只能包含
`+enable_random_directed_instr=1` 和可选的 `+random_directed_instr_ratio=N`，不能
包含 `directed_instr_N`/`stream_name_N`。验收时检查：

- generator log 有 `TEST PASSED`，且 UVM warning/error/fatal 均为 0；
- 候选日志不再只有 `riscv_loop_instr`，汇编中存在实际选中的 directed stream；
- `.S`、ELF 和 `.bin` 均为本次新生成且非空；
- 对应 Spike log 非空、自然退出，且 strict 检查通过。

Spike 通过只证明该 ELF 在参考 ISS 上完成预检查，不代表 NanHu RTL 已通过相同
用例。PMP 功能也必须在声明 `support_pmp=1` 的 target 上验证，不能用 NanHu
target 绕过能力检查。

## 7. 本次验证记录

以下结果使用 2026-09-04 工作树和指定 Spike 完成，output 目录均为新建：

| 路径/seed | 覆盖内容 | 结果 |
| --- | --- | --- |
| `out/verify_auto_directed_final_90403` / `90403` | NanHu SV，无显式候选；基础配置列出 13 类，CBO+Vector 配置列出 15 类并实际选中 CBO/Vector | 两项均 `TEST PASSED`，UVM W/E/F 为 0/0/0；两份 ELF/bin 非空；Spike strict 通过 |
| `out/verify_auto_directed_smode_90407` / `90407` | NanHu S-mode SV，无显式候选；main 和三个 kernel program 均使用自动候选 | `TEST PASSED`，UVM W/E/F 为 0/0/0；GNU 和 Spike strict 通过 |
| `out/verify_pyflow_auto_directed_90406` / `90404` | RV64IMAFDC PyFlow，无显式候选；日志列出全部 7 类安全候选 | `TEST GENERATION DONE`；GNU 和 Spike 通过；无 access/illegal fault |
| `out/verify_random_boot_full_246801_final` / `246801` | NanHu 随机 boot mode | 本次选择 U-mode；生成、GNU、Spike strict 通过 |
| `out/final_pmp_load_7317` / `7317` | PMP load permission fault | 恰好一次 load access fault，handler 修复后 Spike strict 通过 |
| `out/final_pmp_store_7312` / `7312` | PMP store permission fault | 恰好一次 store access fault，handler 修复后 Spike strict 通过 |
| `out/final_pmp_legacy_7340` / `7340` | RV64 16-region legacy PMP CSR 映射 | 只访问合法的 `pmpcfg0/pmpcfg2`，生成、GNU、Spike strict 通过 |

PyFlow 自动候选验收期间另外捕获并修复了两项原有问题：load/store 的合法指令表
曾跨地址累积，可能把负 offset 配给 `c.lwsp`；全局指令模板曾使用浅拷贝，后续
stream 会回写早先指令的 base/destination register。当前分别改为逐地址重建候选表
和深拷贝模板，同 seed 已完成全链路复验。
