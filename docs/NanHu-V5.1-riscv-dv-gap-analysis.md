# NanHu V5.1 与 riscv-dv 架构支持差距

## 1. 审计口径

- 规格来源：`NanHu-V5.1规格.xlsx` 的工作表 `NanHu-V5.1 RVA23 feature`。
- NanHu V5.1 支持项：第 D 列 `NanHu-V5.1（AP）` 为 `YES` 的条目，共 64 项。
- riscv-dv 基线：提交 `b7a0b4b`（`master`）。
- “实现”分为三层：
  1. 指令生成：有独立指令组、合法性约束、汇编和二进制编码；
  2. 架构状态生成：有 CSR/PTE/异常或中断刺激模型；
  3. 合规验证：有结果、时序、PMA 或微架构行为的 checker。
- riscv-dv 是随机指令生成器。能够生成某条指令或某个 CSR 访问，不等于完成该扩展的架构合规验证。
- 工作簿没有给出 VLEN/ELEN、中断向量数量、hart 数和 SoC MMIO 地址；target 中的对应值按集成假设处理，不能视为由本表确认的规格。

## 2. 结论摘要

基线已经覆盖 RV64 IMAFDC、主要 B 子扩展、Zcb、虚拟内存基础机制、异常/中断框架、Debug ROM 和部分 profile 行为刺激。但基线的向量实现注明基于 V 0.8，且缺少 NanHu V5.1 所需的多组 ratified scalar、crypto、privileged 和 MMU 扩展。

当前工作树已补入以下生成器级能力（均需目标/plusarg 显式开启）：

- 指令扩展：Svinval、Zicond、Zimop、Zcmop、Zicbom、Zicbop、Zicboz、Zbkb、Zbkc、Zbkx、Zknd、Zkne、Zknh、Zksed、Zksh、ZKn/Zks 聚合组和 Zvbb。
- 特权状态：Smstateen、Ssstateen 的非 H 路径、Sstc、Sscofpmf 所需 CSR/字段，`SSTATUS.VS`、`VXSAT/VCSR`、零值 `MCONFIGPTR`、按 CSR `[9:8]` 的权限过滤，以及进入低特权级前的 FCSR/stateen 启动序列。
- MMU：Svpbmt 的 RV64 PTE `[62:61]` 生成和逐 leaf 独立 PBMT 随机；NanHu target 将 `menvcfg.ADUE` 建模为只读 0，以选择 Svade 而不是 Svadu，并提供 A-only/D-only 定向 fault 模式和真实 tval signature 观测路径。
- 向量生成：增加 V 1.0 汇编模式、三种 VSET 编码、fractional LMUL、indexed/data EMUL、whole-register/mask load-store、`vrgatherei16`、`vzext/vsext`、FP slide/reduction、RTZ conversion、`vsmul`、`vfrec7/vfrsqrt7`、`vfwadd/vfwsub` 和 Zvbb，并输出 V1.0 canonical reduction/mask 名称；同时修正 `.vi` 立即数、segment 合法性和地址跨度。生成器仍不等价于完整 Vector 1.0 合规 checker。
- 平台钩子：可配置的 ACLINT 清 MSIP/禁用 MTIMECMP 初始化，以及 PLIC M/S 独立 context 的 claim/complete 序列；testlist 中的地址只是禁用状态下的集成示例，不是已确认的 NanHu 地址图。
- Target：`target/nanhu_v5_1` 汇总 NanHu V5.1 的 ISA、CSR 和 smoke test 配置。

仍不能标记为“完整实现”的主要项目是 etrace、RAS/RERI、PMA/profile 属性、reservation-set 尺寸、cache-block 尺寸、取指原子性、LR/SC forward-progress、Zkt 恒定时延，以及完整的 Privileged 1.13、Debug transport/Trigger 和 Vector 1.0 执行语义/合规检查。

## 3. 规格矩阵

| 表格行 | NanHu V5.1 项目 | `b7a0b4b` 基线 | 当前工作树 | 判定 |
|---:|---|---|---|---|
| 3, 6-8, 11, 13-14 | A, B, C, D, F, I, M | 有对应指令类和 target | 保持 | 已有指令生成能力 |
| 64-66, 70-71 | Zba, Zbb, Zbc, Zbs, Zcb | 有独立指令组；Zcb 已在 HEAD 合入 | 保持 | 已有指令生成能力 |
| 68 | Zbkc | Zbc 已含 `clmul/clmulh` 编码，但也包含不属于 Zbkc 的 `clmulr` | 增加独立 RV32/RV64 Zbkc 组并严格限制为 `clmul/clmulh`；ZKn/Zks 只展开 Zbkc、不隐式开启 Zbc | 已补严格子集生成能力 |
| 93-94 | Zicsr, Zifencei | CSR 指令与 `fence.i` 已有 | 保持 | 已有指令生成能力 |
| 37 | Sscounterenw | `scounteren` 各计数器位为 WARL | 保持 | 可生成 CSR 写刺激，不能自行判定 DUT 可写性 |
| 43-45 | Sstvala, Sstvecd, Ssu64xl | 有 `stval/stvec`、异常处理和 RV64 UXL 设置 | 增加 tval signature handshake，以及 U-mode ECALL 强制委托到 S handler 的 STVAL smoke | 已补 U→S 委托和 STVAL 观测通路；ECALL 的 STVAL 通常为 0，尚未定向覆盖 Sstvala 所要求的 instruction/load/store faulting VA 语义；signature 地址仍为占位 |
| 47-48, 52 | Sv39, Sv48, Svbare | `SATP_MODE` 和页表生成器支持 | NanHu target 可在编译时选择三种模式；默认启用 Sv39 | 三种模式均通过生成和 GNU 编译；Bare 只验证模式选择，不包含页表/PBMT 语义 |
| 50 | Svade | 可生成 A/D 为 0 的页表异常 | NanHu target 将 `menvcfg.ADUE` 固定为生成器侧只读 0，避免启用 Svadu；分别提供 A-only 和 D-only leaf fault，关闭其他页表异常注入 | 已定向生成 A=0/D=1 与 A=1/D=0 叶 PTE；仍需 DUT checker 验证 page fault、ADUE 写后读回、禁止硬件更新 A/D 和软件恢复 |
| 9 | Debug | 有随机 Debug ROM、DRET、单步和 debug handshake | NanHu target 开启并补 CSR 字段模型，增加独立 Debug ROM smoke | 部分支持；已验证 Debug CSR 汇编生成，外部 debug transport 和执行语义依赖 testbench |
| 19-20 | Sdext, Sdtrig | CSR 地址枚举存在，Debug ROM 存在；无 trigger 语义模型 | 补 `dcsr/dpc/dscratch*` 和 trigger/context CSR 字段 | 部分支持；trigger match/action 仍需定向测试和 checker |
| 4 | ACLINT | 无地址化初始化模型 | 增加按 hart stride 清 MSIP、把 MTIMECMP 初始化为全 1 的钩子 | 部分支持；地址与控制器行为由 SoC/testbench 提供 |
| 15 | PLIC | 只有空的可覆写 hook 和 testbench handshake | 增加仅用于外部中断的 M/S 独立 claim/complete base、hart stride 和 legacy fallback | 部分支持；不是 PLIC 控制器模型 |
| 10 | etrace | 未找到 execution-trace 编码/控制模型 | 未补 | 未实现，且超出普通指令生成范围 |
| 18 | RAS/RERI | 未找到 RERI MMIO 寄存器或错误注入模型 | 未补 | 未实现，需平台模型与错误注入环境 |
| 30, 40 | Smstateen, Ssstateen | 缺 `mstateen*/sstateen*` | 补 M/S state-enable CSR 枚举、字段和 lower-mode 启动序列；U-mode smoke 实际设置 `mstateen0/sstateen0.FCSR` | M/S 状态生成已补；表格文字还要求 `hstateen0-3`，但 H 行(12)为 NO，当前按 H 未实现处理，访问违规语义仍由 DUT/checker 验证 |
| 33 | Ss1p13 | 页表代码仍标注 Privileged 1.10；模型不是完整 1.13 | 补本表直接依赖的 1.13 CSR/字段、`SSTATUS.VS`、V CSR、零值 `MCONFIGPTR` 和 CSR 地址权限分类 | 仅定向补齐，不能宣称完整 Privileged 1.13 合规 |
| 35 | Ssccptr | 能生成页表，但无 cacheability/coherence PMA 模型 | 未补 PMA checker | 部分刺激，profile 属性未实现 |
| 36 | Sscofpmf | 缺 `scountovf`、LCOFI 和 `mhpmevent` 过滤/OF 字段 | 已补相应 CSR、字段和中断 cause，并设置 MIE/SIE.LCOFIE 与 MIDELEG[13] | 已补状态生成；未制造真实计数器溢出，事件 ID、`scountovf` 和中断送达仍需平台/checker |
| 42 | Sstc | 缺 `stimecmp/h` 及 `menvcfg.STCE` 模型 | 已补 CSR，并在进入低特权级前设置 `menvcfg.STCE` 与 `mcounteren.TM` | 已补状态生成；timer delivery 和比较时序依赖环境/checker |
| 53 | Svinval | 只有 `sfence.vma` | 补三条指令及开关，并约束 `enable_sfence=1`、S-mode `mstatus.TVM=0` 以保证可达 | 已补指令生成能力 |
| 55 | Svpbmt | PTE `[62:61]` 被当作 reserved | 补 PBMT 枚举、约束、PTE 打包、逐有效 leaf 独立 PMA/NC/IO 随机，并在 `satp` 初始化前设置 `menvcfg.PBMTE`；invalid/non-leaf 固定 PMA | 已补 PTE 生成；内存类型语义依赖 DUT/checker |
| 57 | V | 实现文件明确写明基于 V 0.8 | 增加 V 1.0 语法模式、三种状态同步 VSET、fractional LMUL、indexed/data EMUL、whole-register/mask memory、gather/extend/FP slide/reduction/RTZ conversion、fixed-point multiply、reciprocal estimate 和 widening FP add/sub，并修正 canonical 名称、segment、地址跨度和 `.vi` 立即数 | 已补本次审计识别的生成缺口；通用生成器保留 segment，但 NanHu smoke 根据工作簿 H25 的已知死锁关闭 `zvlsseg`；执行语义和完整合规仍需外部 checker |
| 59 | Za64rs | 有合法 LR/SC 序列，未约束/检查 reservation-set 形状和 64B 上限 | 未补 checker | profile 属性未实现 |
| 67, 69 | Zbkb, Zbkx | 仅有旧 B 草案中的部分重叠指令 | 补 ratified 指令名、分组和编码 | 已补指令生成能力 |
| 72 | Zcmop | 无 | 补 `c.mop.*` | 已补指令生成能力 |
| 80 | Zic64b | 无 cache-block-size 架构参数/checker | 未补 | profile 属性未实现 |
| 81-83 | Zicbom, Zicbop, Zicboz | 无 CBO/prefetch 指令 | 补 clean/flush/inval/zero 和 prefetch I/R/W | 已补指令生成；cache/PMA 效果需 checker |
| 84 | Ziccamoa | 能生成 AMO，但不描述 cacheable/coherent PMA 区域 | 未补 PMA checker | 部分刺激，profile 属性未实现 |
| 86 | Ziccif | 有取指和 `fence.i`，无主存取指原子性定向检查 | 未补 checker | profile 属性未实现 |
| 87 | Zicclsm | 有 unaligned load/store 开关和覆盖 | NanHu target 开启 | 有刺激；PMA 范围与结果仍由 testbench 检查 |
| 88 | Ziccrse | 有受约束 LR/SC 序列 | 保持 | 有刺激；forward-progress 需要超时/checker |
| 91, 97 | Zicntr, Zihpm | 有 CSR 枚举和部分 machine counter 模型，user counter 字段不完整 | 补 user counter、`mcountinhibit` 和 event 字段 | 已补 CSR 生成；真实计数行为不在生成器内 |
| 92 | Zicond | 无 | 补 `czero.eqz/nez` | 已补指令生成能力 |
| 98 | Zimop | 无 | 补 `mop.r/mop.rr` | 已补指令生成能力 |
| 100-103 | ZKn, Zknd, Zkne, Zknh | 无 ratified scalar crypto 组/编码 | 补 AES、SHA-2 及聚合关系 | 已补指令生成能力 |
| 105-107 | Zks, Zksed, Zksh | 无 | 补 SM4、SM3 及聚合关系 | 已补指令生成能力 |
| 108 | Zkt | 无时延观测或恒定时延 checker | 未补 | 未实现；需要 cycle-accurate 监测 |
| 109 | Zvbb | 无 | 补 10 组 Zvbb 指令 | 已补指令生成；依赖 V 1.0 基础层继续审计 |

## 4. 代码证据

基线证据：

- `README.md` 仅声明 RV32/RV64 IMAFDC、特权模式、页表、异常/中断和 Debug ROM。
- HEAD 的 `riscv_instr_group_t` 只有基础 ISA、B 子扩展、Zcb 和 custom 组。
- HEAD 的 `riscv_vector_instr.sv` 注释明确写明基于 Vector spec v0.8。
- HEAD 的 `riscv_page_table_entry.sv` 注释明确写明 Privileged spec 1.10，RV64 PTE 高位全部作为 reserved。
- HEAD 的 `gen_plic_section()` 仅发送 testbench handshake；没有 ACLINT 地址化初始化。

当前工作树证据：

- `src/riscv_instr_pkg.sv`：新增指令组、指令名、CSR 和 LCOFI cause。
- `src/isa/riscv_scalar_ext_instr.sv`、`src/isa/rv_scalar_ext_instr.sv`：scalar、crypto、CBO/MOP 的约束、汇编和编码。
- `src/isa/riscv_svinval_instr.sv`：Svinval 三条指令。
- `src/isa/riscv_vector_set_instr.sv`：`vsetvli/vsetivli/vsetvl` 的约束、V 1.0 汇编和编码；普通随机流默认排除 VSET，防止实际向量状态与 `vector_cfg` 脱节。
- `src/riscv_instr_gen_config.sv`、`src/riscv_asm_program_gen.sv`：向量初始化先生成与 `vector_cfg` 一致的 `vsetvli`，在 AVL 可编码时生成 `vsetivli`，再用同一 `vtype` 生成 `vsetvl`，连续覆盖三种编码而不改变后续指令所依赖的状态。
- `src/riscv_vector_cfg.sv`、`src/isa/riscv_vector_instr.sv`、`src/isa/rv32v_instr.sv`、`src/riscv_load_store_instr_lib.sv`：修正 fractional LMUL 下的 VL 和寄存器组约束，区分 indexed operand EMUL 与 data EMUL；补 whole-register/mask memory、`vrgatherei16`、`vzext/vsext`、FP slide、`vfredmin`、RTZ conversion、`vsmul`、`vfrec7/vfrsqrt7` 和 `vfwadd/vfwsub`；按 V1.0 输出 `vfredusum/vfwredusum/vmandn/vmorn`，并修正 segment 的 NFIELDS/寄存器边界、load/store 地址跨度和 `.vi` 的 signed/uimm5 立即数生成。
- `src/riscv_directed_instr_lib.sv`：栈保护辅助分支显式从六条 B-type 指令中选择，避免 `vector_instr_only` 清空随机 BRANCH category 后把向量立即数指令误写成标签跳转。
- `src/isa/riscv_zvbb_instr.sv`：Zvbb 指令类。
- `src/riscv_privil_reg.sv`：state-enable、Sstc、Sscofpmf、debug/trigger、counter、`SSTATUS.VS`、`VCSR` 和零值 `MCONFIGPTR` 字段模型；NanHu target 下将 `menvcfg.ADUE` 作为零值保留字段生成。
- `src/riscv_privileged_common_seq.sv`：Svpbmt、Sstc、Sscofpmf、stateen/FCSR 和低特权级 CBO 的启动序列，包括 MIE/SIE.LCOFIE。
- `src/riscv_page_table_entry.sv`、`src/riscv_page_table_list.sv`、`src/riscv_page_table_exception_cfg.sv`：Svpbmt PTE、逐 leaf PBMT 和 Svade A-only/D-only 生成。
- `src/riscv_asm_program_gen.sv`：S-mode Vector 的 `SSTATUS.VS` 初始化、VCSR 初始化，以及 M/S trap handler 的 tval signature handshake。
- `src/riscv_instr_gen_config.sv`、`src/riscv_asm_program_gen.sv`：生成 LCOFI 的 MIDELEG[13] 委托状态；`src/riscv_instr_gen_config.sv`、`src/isa/riscv_instr.sv` 还按 CSR 地址 `[9:8]` 建立 M/S/U 权限过滤，并让显式 invalid-level smoke 在 S/U mode 中可达 CSR 指令。
- `src/riscv_debug_rom_gen.sv`、`src/isa/riscv_csr_instr.sv`：Debug CSR 只在 Debug ROM 中访问，普通 M/S/U CSR 随机流排除 `dcsr/dpc/dscratch*`。
- `target/nanhu_v5_1/`：NanHu V5.1 target，共 16 个 testlist 条目；14 个分层 smoke 默认启用，scalar encoding unit 和平台地址示例默认禁用。
- `run.py:788-998`：注册 `nanhu_v5_1` 预定义 target，使上述 testlist 可直接运行；`run.py:988-991` 明确拒绝尚未实现该 target 的 pyflow backend。

注意：当前没有 `HSTATEEN*` 枚举或 target CSR；这是有意保持 H 扩展关闭的结果。若 NanHu 后续要求同时实现 H，需增加 `0x60c-0x60f`（RV64）及 high-half CSR，并扩展 hypervisor target。

Target 当前使用 `VLEN=128`、`ELEN=64`、`SELEN=8`、`MAX_LMUL=8`、16 个中断向量和单 hart。工作簿没有给出这些数值，因此它们是待 NanHu 集成环境确认的参数，不是本次 Excel 审计得出的规格。

工作簿单元格 H25 的内容明确记录 V 扩展的 segment load 会因 LSQ 缩减发生死锁。通用 riscv-dv 生成器仍保留并修正了 segment 指令生成；NanHu Vector smoke 则保守设置 `+enable_zvlsseg=0`。当前开关同时关闭 segment load 和 store，所以该 smoke 不能作为任何 segment 指令的 RTL 或 ISS 覆盖证据。

## 5. 后续验证边界

1. 新增 scalar 指令已做 RV32/RV64 encoding unit 和 GNU binutils 2.44 golden 核对；Vector V1.0 新增 form 已由 GNU 2.44 汇编和 objdump 核对。普通 Vector arithmetic 类尚无 `convert2bin()`，因此没有逐 form 的 SystemVerilog golden encoding unit；以后修改编码、group alias 或 XLEN 约束时仍必须复跑 GNU 核验，不能只以 SystemVerilog 编译通过为准。
2. CSR 模型能够生成地址、字段和访问权限刺激，但 RV32 high-half、WARL/WPRI、`ADUE=0` 写后读回以及 state-enable 访问违规的 DUT 结果仍需 RTL/ISS checker。
3. 固定 seed 5101 的 A-only 用例生成 2054 个有效 leaf，全部为 A=0/D=1；seed 5201 的 D-only 用例同样生成 2054 个有效 leaf，全部为 A=1/D=0，其他权限异常为 0。这只证明刺激可达；仍需检查 DUT page fault、禁止硬件更新 A/D，以及软件修复后继续执行。
4. Svpbmt 已保证 invalid/non-leaf 为 PMA，并让每个有效 leaf 独立选择 PMA/NC/IO；仍需在 RTL 中检查内存类型语义。Bare 不含页表，不能作为 PBMT 语义覆盖。
5. 本次识别的 Vector 1.0 生成缺口已补，并通过语法/编码和多组 LMUL/SEW 回归；通用 CSV coverage 路径仍没有完整 RVV operand/form 采样，这些结果也不能证明向量执行结果、`vstart` restart、异常精确性或完整 Vector 1.0 合规。
6. 通用生成器的 segment 约束与 GNU 汇编通过，不代表 NanHu RTL 可执行；在工作簿记录的死锁解决前，NanHu target 保持禁用 segment。若未来只需禁用 load 而保留 store，需要拆分当前统一的 `enable_zvlsseg` 开关。
7. Sscofpmf smoke 只生成 LCOFI delegation/enable 状态，没有制造真实 counter overflow；Sstc 也没有 timer delivery 或比较时序 checker。这些行为必须由平台环境验证。
8. Debug ROM smoke 只证明 Debug CSR 访问能够生成和汇编；Debug transport、halt/resume、单步执行结果以及 trigger match/action 仍需 RTL testbench 和 checker。
9. ACLINT/PLIC 示例中的 `0x02000000`、`0x02004000`、M context `0x0c200004` 和 S context `0x0c201004` 是传统占位地址，hart stride 为 `0x2000`，且用例 `iterations: 0`。替换为真实 NanHu/SoC 地址图并验证控制器行为之前，不能标记为平台实现完成。
10. etrace、RAS/RERI、PMA/coherence/profile、forward-progress 和 Zkt 需要 RTL testbench、cycle checker 或平台模型，不能在 riscv-dv 内用一个 enable flag 代替。

已完成的生成器/汇编 smoke 验证：

- NanHu testlist 共 16 项；除 scalar encoding unit 和 platform interrupt example 外，14 个默认启用 smoke 均完成 VCS 生成与 GNU `gcc_compile`/binary conversion，日志为 `TEST PASSED`、`UVM_ERROR=0`、`UVM_FATAL=0`。
- 14 项覆盖 base、scalar、scalar crypto、严格 Zbkc、Svinval/Svpbmt、Svade D-only、STVAL delegation、CSR、invalid S-mode CSR、Sscofpmf、U-mode stateen、Vector/Zvbb、S-mode Vector 和 Debug ROM。严格 Zbkc 只生成 `clmul/clmulh`，Svinval 三条指令均可达。
- RV64/RV32 scalar encoding unit 均通过，并用 GNU binutils 2.44 对 ratified scalar/crypto 指令做逐条 golden 编码核验；RV32 聚合流未生成 RV64-only 指令或 `clmulr`。
- Vector 回归覆盖 e32/m2、e64/mf2、legacy 模式和定向 whole-register/mask memory；43 个新增 enum mnemonic 全部通过 GNU 2.44 语法核验。定向 seed 9101 还实际命中 `vsmul.vv/vx`、`vfrec7.v`、`vfrsqrt7.v`、`vfwadd/vfwsub` 的 `.vv/.vf/.wv/.wf`，以及 `vfredusum.vs/vfwredusum.vs/vmandn.mm/vmorn.mm`，GNU objdump 均按 V1.0 解码；whole-register 对齐/边界和 widening reduction operand 审计无错误。
- S-mode Vector smoke 实际设置 `SSTATUS.VS`；CSR smoke 覆盖 `VXSAT/VXRM/VCSR`。U-mode stateen smoke 实际设置 `mstateen0/sstateen0.FCSR`，invalid CSR smoke 实际命中 `0x7a*` trigger CSR。
- STVAL delegation 用例生成 MEDELEG bit 8、U-mode ECALL 和 S handler 对 `STVAL(0x143)` 的 signature write；64 位 signature 地址解析已核对为 `0x8ffffff0`，没有错误符号扩展。该用例不构成 Sstvala faulting-VA 语义覆盖。
- 固定 seed 的 Svade A-only/D-only 与 Svpbmt 回归均通过；A-only 的 2054 个 leaf 中 PBMT PMA/NC/IO 分布为 673/682/699，invalid/non-leaf 未使用 NC/IO。
- RV64 与 RV32 原有 target 的 VCS compile-only 回归通过；Sv39、Sv48 和 Bare 变体均完成 VCS 生成、GNU 汇编和 binary conversion。Bare 只证明模式选择与生成路径，不构成 PBMT 语义覆盖。
- 平台 interrupt example 仍为禁用模板，不计入 14 个默认启用 smoke，也没有真实 NanHu 地址上的验证结果。
- 这些结果只证明生成器、汇编语法和编码路径可运行；不替代 NanHu RTL、ISS、Debug transport 或平台级合规验证，不能据此宣称完整 Vector 1.0、Privileged 1.13 或 Debug 合规支持。
