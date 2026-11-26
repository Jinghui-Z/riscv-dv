#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_RVC_FP_LOAD_FAULT_LEAF (PTE_V | PTE_AD | PTE_X)
#define AI_ARCH_RVC_FP_STORE_FAULT_LEAF (PTE_V | PTE_AD | PTE_R)
#define AI_ARCH_RVC_FP_LEGAL_LEAF (PTE_V | PTE_AD | PTE_RWX)
#define AI_ARCH_RVC_FP_LEGAL_LEAF_NO_AD (PTE_V | PTE_RWX)

typedef struct {
    uintptr_t pc;
    uintptr_t next_pc;
} ai_arch_rvc_fp_probe_pc_t;

static inline pte_t *ai_arch_rvc_fp_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uint64_t ai_arch_rvc_fp_hs_leaf_value(enum test_page page,
    uint64_t flags)
{
    return (phys_page_base(page) >> 2) | flags;
}

static inline void ai_arch_rvc_fp_prepare_hs_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    sfence_vma();
}

static inline void ai_arch_rvc_fp_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    *ai_arch_rvc_fp_hs_leaf_entry(page) =
        ai_arch_rvc_fp_hs_leaf_value(page, flags);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    sfence_vma();
}

static inline bool ai_arch_rvc_fp_hs_leaf_matches(enum test_page page,
    uint64_t flags)
{
    return *ai_arch_rvc_fp_hs_leaf_entry(page) ==
        ai_arch_rvc_fp_hs_leaf_value(page, flags);
}

static inline void ai_arch_rvc_fp_set_fs_clean(void)
{
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_SSTATUS, SSTATUS_FS_CLEAN);
}

static inline void ai_arch_rvc_fp_set_fs_dirty(void)
{
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
}

static inline uint64_t ai_arch_rvc_fp_fs_state(void)
{
    return CSRR(CSR_SSTATUS) & SSTATUS_FS_MSK;
}

static inline void ai_arch_rvc_fp_write_fcsr(uint64_t fflags, uint64_t frm)
{
    CSRW(CSR_FCSR, ((frm & 0x7ULL) << 5) | (fflags & 0x1fULL));
}

static inline uint64_t ai_arch_rvc_fp_read_fcsr(void)
{
    return CSRR(CSR_FCSR) & 0xffULL;
}

static inline void ai_arch_rvc_fp_seed_fs0(uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x fs0, %[bits]\n\t"
        ".option pop\n\t"
        : : [bits] "r"(bits) : "fs0");
}

static inline void ai_arch_rvc_fp_seed_fs1(uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x fs1, %[bits]\n\t"
        ".option pop\n\t"
        : : [bits] "r"(bits) : "fs1");
}

static inline void ai_arch_rvc_fp_seed_ft0(uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %[bits]\n\t"
        ".option pop\n\t"
        : : [bits] "r"(bits) : "ft0");
}

static inline uint64_t ai_arch_rvc_fp_read_fs0(void)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.x.d %[value], fs0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value) : : "memory");

    return value;
}

static inline uint64_t ai_arch_rvc_fp_read_fs1(void)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.x.d %[value], fs1\n\t"
        ".option pop\n\t"
        : [value] "=r"(value) : : "memory");

    return value;
}

static inline uint64_t ai_arch_rvc_fp_cfld_probe(uintptr_t base,
    ai_arch_rvc_fp_probe_pc_t *pc_info)
{
    uint64_t value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fld fs0, 16(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "fmv.x.d %[value], fs0\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [base] "r"(base), [pc_info] "r"(pc_info)
        : "t0", "t2", "t3", "t5", "fs0", "memory");

    return value;
}

static inline void ai_arch_rvc_fp_cfld_no_read_probe(uintptr_t base,
    ai_arch_rvc_fp_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fld fs0, 16(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [pc_info] "r"(pc_info)
        : "t0", "t2", "t3", "t5", "fs0", "memory");
}

static inline void ai_arch_rvc_fp_cfsd_probe(uintptr_t base,
    ai_arch_rvc_fp_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fsd fs1, 24(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [pc_info] "r"(pc_info)
        : "t0", "t2", "t3", "t5", "memory");
}

static inline void ai_arch_rvc_fp_sp_success_probe(uintptr_t sp_base,
    uint64_t *loaded_bits, uint64_t *fldsp_span, uint64_t *fsdsp_span)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t6, sp\n\t"
        "mv sp, %[sp_base]\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fldsp ft0, 24(sp)\n\t"
        "2:\n\t"
        "c.fsdsp fs1, 40(sp)\n\t"
        "3:\n\t"
        ".option norvc\n\t"
        "fmv.x.d t4, ft0\n\t"
        "sd t4, 0(%[loaded_bits])\n\t"
        "lla t0, 2b\n\t"
        "lla t1, 1b\n\t"
        "sub t0, t0, t1\n\t"
        "sd t0, 0(%[fldsp_span])\n\t"
        "lla t0, 3b\n\t"
        "lla t1, 2b\n\t"
        "sub t0, t0, t1\n\t"
        "sd t0, 0(%[fsdsp_span])\n\t"
        "mv sp, t6\n\t"
        ".option pop\n\t"
        :
        : [sp_base] "r"(sp_base),
          [loaded_bits] "r"(loaded_bits),
          [fldsp_span] "r"(fldsp_span),
          [fsdsp_span] "r"(fsdsp_span)
        : "t0", "t1", "t4", "t6", "ft0", "memory");
}

static inline void ai_arch_rvc_fp_fs_off_cfld_probe(uintptr_t base,
    ai_arch_rvc_fp_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fld fs0, 16(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [pc_info] "r"(pc_info)
        : "t0", "t2", "t3", "t5", "fs0", "memory");
}

static inline void ai_arch_rvc_fp_fs_off_cfsd_probe(uintptr_t base,
    ai_arch_rvc_fp_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fsd fs1, 24(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [pc_info] "r"(pc_info)
        : "t0", "t2", "t3", "t5", "memory");
}

bool ai_arch_rvc_fp_double_fault_repair_fs_dirty_corner()
{
    TEST_START();

    const enum test_page load_page = VSRWX_GRWX;
    const enum test_page store_page = VSRW_GRWX;
    const enum test_page sp_page = SCRATCHPAD;
    const uintptr_t load_base_vaddr = hs_page_base(load_page) + 0x180U;
    const uintptr_t load_base_paddr = phys_page_base(load_page) + 0x180U;
    const uintptr_t store_base_vaddr = hs_page_base(store_page) + 0x280U;
    const uintptr_t store_base_paddr = phys_page_base(store_page) + 0x280U;
    const uintptr_t sp_base_vaddr = hs_page_base(sp_page) + 0x300U;
    const uintptr_t sp_base_paddr = phys_page_base(sp_page) + 0x300U;
    const uint64_t load_guard0 = 0x0102030405060708ULL;
    const uint64_t load_pattern = 0x7ff8123456789abcULL;
    const uint64_t load_guard1 = 0xfedcba9876543210ULL;
    const uint64_t store_initial = 0x8877665544332211ULL;
    const uint64_t store_payload = 0x8000000000000000ULL;
    const uint64_t store_guard0 = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t store_guard1 = 0xf0e1d2c3b4a59687ULL;
    const uint64_t sp_load_pattern = 0x0000000000000001ULL;
    const uint64_t sp_store_payload = 0x400921fb54442d18ULL;
    const uint64_t sp_store_initial = 0x1122334455667788ULL;
    const uint64_t sp_guard0 = 0xaabbccddeeff0011ULL;
    const uint64_t sp_guard1 = 0x55aa33cc77ee11ffULL;
    const uint64_t fs0_sentinel = 0x7ff80000deadbabeULL;
    const uint64_t fs1_sentinel = 0x55aa33cc77ee11ffULL;
    const uint64_t initial_fflags = 0x12ULL;
    const uint64_t initial_frm = 0x1ULL;
    const uint64_t initial_fcsr =
        ((initial_frm & 0x7ULL) << 5) | initial_fflags;

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    write64(load_base_paddr + 0U, load_guard0);
    write64(load_base_paddr + 16U, load_pattern);
    write64(load_base_paddr + 24U, load_guard1);
    write64(store_base_paddr + 16U, store_guard0);
    write64(store_base_paddr + 24U, store_initial);
    write64(store_base_paddr + 32U, store_guard1);
    write64(sp_base_paddr + 16U, sp_guard0);
    write64(sp_base_paddr + 24U, sp_load_pattern);
    write64(sp_base_paddr + 40U, sp_store_initial);
    write64(sp_base_paddr + 48U, sp_guard1);

    ai_arch_rvc_fp_prepare_hs_translation();

    ai_arch_rvc_fp_probe_pc_t cfld_pc = {0};
    ai_arch_rvc_fp_set_hs_leaf(load_page, AI_ARCH_RVC_FP_LOAD_FAULT_LEAF);
    ai_arch_rvc_fp_set_fs_dirty();
    ai_arch_rvc_fp_write_fcsr(initial_fflags, initial_frm);
    ai_arch_rvc_fp_seed_fs0(fs0_sentinel);
    ai_arch_rvc_fp_set_fs_clean();

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fp_cfld_no_read_probe(load_base_vaddr, &cfld_pc);
    const uint64_t cfld_fault_fs = ai_arch_rvc_fp_fs_state();
    ai_arch_rvc_fp_set_fs_dirty();
    const uint64_t cfld_fault_value = ai_arch_rvc_fp_read_fs0();
    TEST_ASSERT("faulting C.FLD reports LPF at the scaled compressed EA, keeps frd/fflags stable, and does not dirty Clean FS",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.epc == cfld_pc.pc &&
        excpt.tval == load_base_vaddr + 16U &&
        cfld_pc.next_pc == cfld_pc.pc + 2U &&
        cfld_fault_value == fs0_sentinel &&
        cfld_fault_fs == SSTATUS_FS_CLEAN &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr &&
        ai_arch_rvc_fp_hs_leaf_matches(load_page,
            AI_ARCH_RVC_FP_LOAD_FAULT_LEAF)
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("faulting C.FLD leaves backing load bytes and adjacent guards unchanged",
        read64(load_base_paddr + 0U) == load_guard0 &&
        read64(load_base_paddr + 16U) == load_pattern &&
        read64(load_base_paddr + 24U) == load_guard1
    );

    ai_arch_rvc_fp_set_hs_leaf(load_page, AI_ARCH_RVC_FP_LEGAL_LEAF);
    ai_arch_rvc_fp_set_fs_dirty();
    ai_arch_rvc_fp_seed_fs0(fs0_sentinel);
    ai_arch_rvc_fp_set_fs_clean();
    TEST_SETUP_EXCEPT();
    uint64_t cfld_repair_value = ai_arch_rvc_fp_cfld_probe(load_base_vaddr,
        &cfld_pc);
    const uint64_t cfld_repair_fs = ai_arch_rvc_fp_fs_state();
    TEST_ASSERT("after repairing the leaf, the same C.FLD commits exact raw double bits and dirties FS",
        excpt.triggered == false &&
        cfld_repair_value == load_pattern &&
        cfld_pc.next_pc == cfld_pc.pc + 2U &&
        cfld_repair_fs == SSTATUS_FS_DIRTY &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr
    );

    ai_arch_rvc_fp_probe_pc_t cfsd_pc = {0};
    ai_arch_rvc_fp_set_hs_leaf(store_page, AI_ARCH_RVC_FP_STORE_FAULT_LEAF);
    ai_arch_rvc_fp_set_fs_dirty();
    ai_arch_rvc_fp_seed_fs1(store_payload);
    ai_arch_rvc_fp_set_fs_clean();

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fp_cfsd_probe(store_base_vaddr, &cfsd_pc);
    const uint64_t cfsd_fault_fs = ai_arch_rvc_fp_fs_state();
    TEST_ASSERT("faulting C.FSD reports SPF at the scaled compressed EA and does not dirty Clean FS",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == cfsd_pc.pc &&
        excpt.tval == store_base_vaddr + 24U &&
        cfsd_pc.next_pc == cfsd_pc.pc + 2U &&
        cfsd_fault_fs == SSTATUS_FS_CLEAN &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr &&
        ai_arch_rvc_fp_hs_leaf_matches(store_page,
            AI_ARCH_RVC_FP_STORE_FAULT_LEAF)
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("faulting C.FSD leaves target memory, guard bytes, fflags, and source FPR data unchanged",
        read64(store_base_paddr + 16U) == store_guard0 &&
        read64(store_base_paddr + 24U) == store_initial &&
        read64(store_base_paddr + 32U) == store_guard1 &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr &&
        ai_arch_rvc_fp_read_fs1() == store_payload
    );

    ai_arch_rvc_fp_set_hs_leaf(store_page, AI_ARCH_RVC_FP_LEGAL_LEAF);
    ai_arch_rvc_fp_set_fs_dirty();
    ai_arch_rvc_fp_seed_fs1(store_payload);
    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fp_cfsd_probe(store_base_vaddr, &cfsd_pc);
    TEST_ASSERT("after repairing the store leaf, C.FSD completes as a 16-bit instruction without stale SPF state",
        excpt.triggered == false &&
        cfsd_pc.next_pc == cfsd_pc.pc + 2U &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("repaired C.FSD writes exactly the source doubleword while preserving adjacent guard data",
        read64(store_base_paddr + 16U) == store_guard0 &&
        read64(store_base_paddr + 24U) == store_payload &&
        read64(store_base_paddr + 32U) == store_guard1
    );

    ai_arch_rvc_fp_set_hs_leaf(sp_page, AI_ARCH_RVC_FP_LEGAL_LEAF);
    ai_arch_rvc_fp_set_fs_dirty();
    ai_arch_rvc_fp_seed_ft0(fs0_sentinel);
    ai_arch_rvc_fp_seed_fs1(sp_store_payload);
    ai_arch_rvc_fp_set_fs_clean();
    uint64_t fldsp_bits = 0ULL;
    uint64_t fldsp_span = 0ULL;
    uint64_t fsdsp_span = 0ULL;
    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fp_sp_success_probe(sp_base_vaddr, &fldsp_bits,
        &fldsp_span, &fsdsp_span);
    const uint64_t sp_commit_fs = ai_arch_rvc_fp_fs_state();
    TEST_ASSERT("C.FLDSP/C.FSDSP use sp-based scaled offsets, retire as 16-bit instructions, preserve fcsr, and dirty FS through the load",
        excpt.triggered == false &&
        fldsp_bits == sp_load_pattern &&
        fldsp_span == 2U &&
        fsdsp_span == 2U &&
        sp_commit_fs == SSTATUS_FS_DIRTY &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("C.FLDSP/C.FSDSP update only the intended stack-relative doubleword and preserve surrounding stack image",
        read64(sp_base_paddr + 16U) == sp_guard0 &&
        read64(sp_base_paddr + 24U) == sp_load_pattern &&
        read64(sp_base_paddr + 40U) == sp_store_payload &&
        read64(sp_base_paddr + 48U) == sp_guard1
    );

    ai_arch_rvc_fp_probe_pc_t fs_off_load_pc = {0};
    ai_arch_rvc_fp_probe_pc_t fs_off_store_pc = {0};
    ai_arch_rvc_fp_set_hs_leaf(load_page, AI_ARCH_RVC_FP_LEGAL_LEAF_NO_AD);
    ai_arch_rvc_fp_set_hs_leaf(store_page, AI_ARCH_RVC_FP_LEGAL_LEAF_NO_AD);
    goto_priv(PRIV_HS);
    ai_arch_rvc_fp_set_fs_dirty();
    ai_arch_rvc_fp_write_fcsr(initial_fflags, initial_frm);
    ai_arch_rvc_fp_seed_fs0(fs0_sentinel);
    ai_arch_rvc_fp_seed_fs1(fs1_sentinel);
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fp_fs_off_cfld_probe(load_base_vaddr, &fs_off_load_pc);
    TEST_ASSERT("FS=Off C.FLD raises illegal instruction at the compressed halfword before any data translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION &&
        excpt.epc == fs_off_load_pc.pc &&
        fs_off_load_pc.next_pc == fs_off_load_pc.pc + 2U
    );

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fp_fs_off_cfsd_probe(store_base_vaddr, &fs_off_store_pc);
    TEST_ASSERT("FS=Off C.FSD raises illegal instruction at the compressed halfword before issuing a store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILLEGAL_INSTRUCTION &&
        excpt.epc == fs_off_store_pc.pc &&
        fs_off_store_pc.next_pc == fs_off_store_pc.pc + 2U
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    TEST_ASSERT("FS=Off compressed FP memory operations leave FPRs, memory image, fcsr, and PTE A/D bits unchanged",
        ai_arch_rvc_fp_read_fs0() == fs0_sentinel &&
        ai_arch_rvc_fp_read_fs1() == fs1_sentinel &&
        ai_arch_rvc_fp_read_fcsr() == initial_fcsr &&
        read64(load_base_paddr + 16U) == load_pattern &&
        read64(store_base_paddr + 24U) == store_payload &&
        *ai_arch_rvc_fp_hs_leaf_entry(load_page) ==
            ai_arch_rvc_fp_hs_leaf_value(load_page,
                AI_ARCH_RVC_FP_LEGAL_LEAF_NO_AD) &&
        *ai_arch_rvc_fp_hs_leaf_entry(store_page) ==
            ai_arch_rvc_fp_hs_leaf_value(store_page,
                AI_ARCH_RVC_FP_LEGAL_LEAF_NO_AD)
    );

    TEST_END();
}
