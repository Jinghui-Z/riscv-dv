# 限制随机 store 的地址范围

在 testlist 的 `gen_opts` 中增加：

```yaml
gen_opts: >
  +restrict_store_to_data=1
```

默认值为 `0`，保持原有生成行为。这个开关约束生成器产生的写地址，避免普通
随机用例意外成为自修改代码测试；它不会通过随机添加 `FENCE.I` 掩盖写代码。
开关可以与 `+enable_random_directed_instr=1` 一起使用，也作用于显式配置的
`+directed_instr_N`。

## 两种限制范围

| 选项 | 对 DV 自生成随机/定向写入 | 对导入的 Hypertest C/ASM |
| --- | --- | --- |
| `+restrict_store_to_data=1` | 使用本页的数据区边界约束 | 严格模式：未证明写地址受限的源流被拒绝；当前仅两个原生 Hypertest 流可用 |
| `+restrict_generated_store_to_data=1` | 复用同一组 DV 生成器约束 | 范围限定模式：保留原用例的 C/ASM 写入，包括有意修改代码的用例 |

两项都不设置时保持旧行为；两项同时设置时，严格选项仍会拒绝源 Hypertest，
不能用范围限定选项绕过严格检查。`+restrict_generated_store_to_data=1`
只保证生成器拥有的普通随机访存、受支持的 directed stream 和 DV 自身收尾写入
沿用本页的地址生成约束。它**不**保证运行时没有写可执行物理页：导入的源用例
可以修改 SATP/PTE、使用虚拟地址别名或直接写代码。链接器的
`.dv_store_data`/`.text` 分离也不能约束这些动态写地址。生成器不会替源用例
自动补 `FENCE.I`；有意自修改代码必须保留原用例的同步和自检，并分别验收
Spike 与 RTL/cosim。范围限定模式不是全局物理内存写保护。

## SystemVerilog 后端

标量访存继续使用 `mem_region`/`s_mem_region` 的标签作为基址，并按实际指令宽度
检查整个访问范围。压缩、整数和浮点访存都受约束。允许非对齐访问，但不会再
生成起始地址在数据区内、尾部越出数据区的访问。

AMO、LR/SC 使用 `amo_region`，完整访问必须落在共享数据区内。Vector 1.0
访存按 VL、EEW、NFIELDS、stride、index 和整寄存器访问大小计算范围，并在
访存前恢复对应的 VL/VTYPE。索引向量在访问前初始化，避免随机向量运算改变地址。
限制模式同时排除 `SEW > LMUL * ELEN` 的 fractional-LMUL 配置，避免恢复出
`vill` 状态。target、DUT 与 ISS 的 VLEN/ELEN 必须一致，尤其是整寄存器访问。

随机数据区生成到 `.dv_store_data.<label>` 输入段，每个区域按 4 KiB 对齐。
默认 `scripts/link.ld` 将这些段收集到独立的 `.dv_store_data` 输出段，提供
`__dv_store_data_start`/`__dv_store_data_end`，与程序代码分离。

原有 ECALL 收尾会把 32 个 GPR 写到 `_start`，这也是一个实际写代码的路径。
开启限制后，结果改存到 `.data` 中每个 hart 独立的 `<hart_prefix>register_dump`
缓冲区（PyFlow 标签为 `__dv_register_dump_<hart>`）。若 testbench 通过读取
`_start` 获取最终 GPR，需要改为读取 ELF 中的这个符号；关闭选项保持原约定。

受限制的 directed 类型包括 loop、JAL、数值边界、普通/单条/压力/hazard/
多页/shared-memory load-store、AMO/LRSC、CBO 和 Vector 1.0 memory stream。
显式列表和自动随机候选都检查实例的实际类型，并启用运行时寄存器保护。
`riscv_load_store_rand_addr_instr_stream`、legacy vector AMO 和未经检查的自定义
stream/工厂替换类型会报 `UVM_FATAL`，不会被静默放行。

`CBO.ZERO` 的写入范围由平台的块大小决定。启用它时还必须指定真实的块大小：

```yaml
gen_opts: >
  +restrict_store_to_data=1
  +enable_zicboz_extension=1
  +cbo_zero_block_size=64
  +directed_instr_0=riscv_cbo_instr_stream,1
```

这里的 `64` 只是示例，必须与 DUT/ISS 配置一致。大小必须是 2 的幂、不超过
4096，且所选数据区能容纳整个块；未指定时拒绝生成 `CBO.ZERO`。

## 为维持地址约束而收紧的行为

- 随机 CSR 指令限定访问当前特权级的 scratch CSR，防止随机 SATP、状态或
  trap-vector 写入破坏映射与控制流。显式的初始化、异常处理和向量状态设置保留。
- 任意 raw illegal 指令注入、指令地址故意错位、非法或旧版 Vector 模式不与
  此开关组合；无法证明这些指令解码后的写地址，生成器会报错。
- 数据区必须真实生成且具备写属性。`no_data_page` 与显式 memory stream 的
  冲突会报错。

严格开关没有关闭 store，也不需要关闭 `enable_unaligned_load_store`。若目标
是专门测试自修改代码、任意物理地址、CSR 改映射或自定义 stream，不能声称
满足严格限制；如仍需约束 DV 自生成的随机 store，可改用范围限定开关，并由
源用例及集成环境负责其写地址合法性和 `FENCE.I` 同步。

## PyFlow 后端

PyFlow 接受这两个 DV 写地址选项，支持其已验证的普通标量 load/store、
loop/JAL 和数值边界候选，使用相同的专用数据段和完整标量访问边界。尚未
验证的 AMO/LRSC、Vector 和其它 directed 类型拒绝使用此模式。PyFlow 尚未
实现源 Hypertest 桥接；范围限定选项不增加这一能力。NanHu target 仍使用
SystemVerilog 后端。

## 使用及验证

例如给现有 NanHu base smoke 增加运行参数：

```bash
python3 run.py --target nanhu_v5_1 --test nanhu_v5_1_base_smoke \
  --simulator vcs --seed 92001 --output out/restricted_example \
  --steps gen,gcc_compile,iss_sim --iss spike --iss_timeout 120 \
  --sim_opts '+restrict_store_to_data=1 +enable_random_directed_instr=1 +boot_mode=m'
```

运行前按 dv-workflow 对相同 target/test/seed/output 执行 preflight。
检查生成日志、ELF 段布局，以及该 ELF 的 Spike 写内存记录。生成器/GNU/Spike
通过不等于 RTL cosim 已通过。

范围限定模式的固定源用例 smoke 见
[hypertest_scoped_store_testlist.yaml](../yaml/hypertest_scoped_store_testlist.yaml)。
它显式设置 `+restrict_generated_store_to_data=1` 和
`+restrict_store_to_data=0`，并使用已在普通模式完成 Spike 验收的源用例。
从仓库根目录运行时，先用相同参数执行 preflight，再执行完整链路：

```bash
python3 "$DV_WORKFLOW_SKILL_HOME/scripts/dv_preflight.py" \
  --repo "$PWD" --target nanhu_v5_1 \
  --testlist yaml/hypertest_scoped_store_testlist.yaml \
  --test hypertest_scoped_generated_store_smoke --simulator vcs \
  --cmp-opts='+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss-timeout 120 \
  --seed 103001 --output out/hypertest_scoped_store_103001

python3 run.py --target nanhu_v5_1 \
  --testlist yaml/hypertest_scoped_store_testlist.yaml \
  --test hypertest_scoped_generated_store_smoke --simulator vcs \
  --cmp_opts='+define+NANHU_V5_1_SV48' \
  --steps gen,gcc_compile,iss_sim --iss spike --iss_timeout 120 \
  --seed 103001 --output out/hypertest_scoped_store_103001
```

每次复现使用新的 output；这里的 seed 仅为固定复现示例，不预先宣称这次运行
通过。smoke 证明两种生成器路径可组合；不能把它外推为完整 cosim-on 名单均可
在当前 NanHu/Spike/RTL 平台运行。

专门回归见 `yaml/restrict_store_to_data_testlist.yaml`。正常回归选择 `scalar`、
`auto`、`vector`、`cbo`、`smode` 后缀的条目；`off` 检查旧模式。四个 `reject_*`
条目预期生成失败，应单独作为诊断运行，不能用 `--test all` 当作成功回归。
`scalar` 会先验证微小/奇数长度区域的完整写边界，并确认旧模式确实可以触发尾部
越界，然后正常生成 ELF 并交给 Spike。

这是内置生成器与默认链接脚本的地址生成约束，不是运行时内存隔离机制。
栈、页表、tohost、签名和平台 MMIO 的显式写入仍由各自程序框架管理；自定义
页表、链接脚本、手写汇编、异常处理或生成器整体替换也需要独立审核。尤其不能
在平台上把专用数据区重新映射为程序代码的物理别名。

## 本次验证记录

NanHu 使用 seed `92001`，`out/restrict_store_verified_92001/` 中的 scalar、auto、
CBO、S-mode、Vector 五个用例均完成生成、GNU 编译和 Spike。UVM warning/error/fatal
均为零；标量边界单元检查覆盖 969 条实际生成的 store。五份 Spike commit log
共检查 9062 次写入，均落在可写 ELF section 内，与可执行 section 无重叠。
S-mode 检查使用内置页表的 low-VA → `0x80000000 + VA` 映射。

另外，`rv64gc` seed `92002` 的限制开启/关闭回归，以及 PyFlow `rv64imafdc`
seed `92021` 的显式流、自动流、关闭对照均完成 GNU/Spike；PyFlow RV32/RV64
各 7 项单元检查通过。四个 `reject_*` 生成诊断均出现预期的限制错误。
关闭模式对照中仍能观察到旧 ECALL 对代码入口的 32 次写入。

未运行 RTL cosim；Vector 回归沿用 NanHu smoke 的 segment 关闭设置。
