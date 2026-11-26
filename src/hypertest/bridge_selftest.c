// SPDX-License-Identifier: Apache-2.0
// Synthetic adapter validation; this is not an imported upstream test case.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DV_HYP_SELFTEST_MAX_VLENB 8192U
#define DV_HYP_SELFTEST_REGISTERS 32U
#define DV_HYP_SELFTEST_STORAGE __attribute__((section(".dv_hypertest_selftest_data")))
#define DV_HYP_CSR_READ(csr) ({ \
    uint64_t value; \
    __asm__ volatile("csrr %0, " #csr : "=r"(value)); \
    value; \
})
#define DV_HYP_CSR_WRITE(csr, value) \
    __asm__ volatile("csrw " #csr ", %0" :: "r"((uint64_t)(value)) : "memory")

extern void dv_hypertest_fail(void) __attribute__((noreturn));
extern void dv_hyp_selftest_invoke(bool (*function)(void));
extern void dv_hyp_selftest_fill_regs(const uint64_t *fp, const unsigned char *vector);
extern void dv_hyp_selftest_save_regs(uint64_t *fp, unsigned char *vector);

#define DV_HYP_SELFTEST_CSRS(X) \
    X(mstatus) X(mtvec) X(mepc) X(mcause) X(mtval) X(mscratch) \
    X(medeleg) X(mideleg) X(mie) X(mip) X(mcounteren) X(menvcfg) \
    X(stvec) X(sepc) X(scause) X(stval) X(sscratch) X(satp) \
    X(scounteren) X(senvcfg) X(fcsr) X(vtype) X(vl) X(vstart) X(vcsr)

struct dv_hyp_selftest_control {
#define DV_HYP_SELFTEST_FIELD(csr) uint64_t csr;
    DV_HYP_SELFTEST_CSRS(DV_HYP_SELFTEST_FIELD)
#undef DV_HYP_SELFTEST_FIELD
};

static uint64_t fp_expected[DV_HYP_SELFTEST_REGISTERS] DV_HYP_SELFTEST_STORAGE;
static uint64_t fp_observed[DV_HYP_SELFTEST_REGISTERS] DV_HYP_SELFTEST_STORAGE;
static uint64_t fp_clobber[DV_HYP_SELFTEST_REGISTERS] DV_HYP_SELFTEST_STORAGE;
static unsigned char vector_expected[DV_HYP_SELFTEST_REGISTERS * DV_HYP_SELFTEST_MAX_VLENB]
    __attribute__((aligned(16))) DV_HYP_SELFTEST_STORAGE;
static unsigned char vector_observed[DV_HYP_SELFTEST_REGISTERS * DV_HYP_SELFTEST_MAX_VLENB]
    __attribute__((aligned(16))) DV_HYP_SELFTEST_STORAGE;
static unsigned char vector_clobber[DV_HYP_SELFTEST_REGISTERS * DV_HYP_SELFTEST_MAX_VLENB]
    __attribute__((aligned(16))) DV_HYP_SELFTEST_STORAGE;
static volatile unsigned clobber_calls DV_HYP_SELFTEST_STORAGE;

__asm__(
    ".section .text.dv_hypertest_selftest_helpers,\"ax\",@progbits\n"
    ".option push\n.option norvc\n.option norelax\n.balign 4\n"
    ".global dv_hyp_selftest_invoke\n"
    "dv_hyp_selftest_invoke:\n"
    "addi sp, sp, -32\n"
    "sd ra, 0(sp)\nsd s0, 8(sp)\nmv s0, sp\n"
    "lla sp, dv_hypertest_stack_top\n"
    "call dv_hypertest_call\n"
    "mv sp, s0\nld ra, 0(sp)\nld s0, 8(sp)\naddi sp, sp, 32\nret\n"
    ".global dv_hyp_selftest_fill_regs\n"
    "dv_hyp_selftest_fill_regs:\n"
    ".irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31\n"
    "fld f\\n, (8 * \\n)(a0)\n"
    ".endr\n"
    "csrw vstart, zero\ncsrr t1, vlenb\nmv t0, a1\n"
    ".irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31\n"
    "vl1re8.v v\\n, (t0)\nadd t0, t0, t1\n"
    ".endr\nret\n"
    ".global dv_hyp_selftest_save_regs\n"
    "dv_hyp_selftest_save_regs:\n"
    ".irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31\n"
    "fsd f\\n, (8 * \\n)(a0)\n"
    ".endr\n"
    "csrr t2, vstart\ncsrw vstart, zero\ncsrr t1, vlenb\nmv t0, a1\n"
    ".irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31\n"
    "vs1r.v v\\n, (t0)\nadd t0, t0, t1\n"
    ".endr\ncsrw vstart, t2\nret\n"
    ".balign 4\n.global dv_hyp_selftest_unexpected_trap\n"
    "dv_hyp_selftest_unexpected_trap:\nj dv_hypertest_fail\n"
    ".option pop\n"
    ".section .dv_hypertest_selftest_stack,\"aw\",@nobits\n"
    ".balign 16\n.global dv_hypertest_selftest_stack_bottom\n"
    "dv_hypertest_selftest_stack_bottom:\n.skip 32768\n"
    ".global dv_hypertest_selftest_stack_top\n"
    "dv_hypertest_selftest_stack_top:\n.previous\n"
);

extern void dv_hyp_selftest_unexpected_trap(void);

static void capture_control(struct dv_hyp_selftest_control *state)
{
#define DV_HYP_SELFTEST_CAPTURE(csr) state->csr = DV_HYP_CSR_READ(csr);
    DV_HYP_SELFTEST_CSRS(DV_HYP_SELFTEST_CAPTURE)
#undef DV_HYP_SELFTEST_CAPTURE
}

static bool same_control(const struct dv_hyp_selftest_control *left,
                         const struct dv_hyp_selftest_control *right)
{
#define DV_HYP_SELFTEST_COMPARE(csr) if (left->csr != right->csr) return false;
    DV_HYP_SELFTEST_CSRS(DV_HYP_SELFTEST_COMPARE)
#undef DV_HYP_SELFTEST_COMPARE
    return true;
}

static void seed_control(bool clobber)
{
    // MIE remains clear throughout the independent validation ELF. Distinct
    // WARL readbacks, rather than assumptions about supported CSR bit masks,
    // become the expected values below.
    uint64_t status = 0x6600ULL | (3ULL << 11);
    if (!clobber) {
        status |= (1ULL << 5) | (1ULL << 18) | (1ULL << 19);
    }
    DV_HYP_CSR_WRITE(mstatus, status);
    DV_HYP_CSR_WRITE(mtvec, (uintptr_t)dv_hyp_selftest_unexpected_trap);
    DV_HYP_CSR_WRITE(stvec, (uintptr_t)dv_hyp_selftest_unexpected_trap);
    DV_HYP_CSR_WRITE(mepc, clobber ? 0x80006000 : 0x80002000);
    DV_HYP_CSR_WRITE(sepc, clobber ? 0x80007000 : 0x80003000);
    DV_HYP_CSR_WRITE(mcause, clobber ? 7 : 13);
    DV_HYP_CSR_WRITE(scause, clobber ? 5 : 15);
    DV_HYP_CSR_WRITE(mtval, clobber ? 0x3210 : 0x1230);
    DV_HYP_CSR_WRITE(stval, clobber ? 0x7654 : 0x5678);
    DV_HYP_CSR_WRITE(mscratch, clobber ? 0x42 : 0x41);
    DV_HYP_CSR_WRITE(sscratch, clobber ? 0x82 : 0x81);
    DV_HYP_CSR_WRITE(medeleg, clobber ? 0 : (1ULL << 12));
    DV_HYP_CSR_WRITE(mideleg, clobber ? 0 : (1ULL << 1));
    DV_HYP_CSR_WRITE(mie, clobber ? 0 : (1ULL << 1));
    DV_HYP_CSR_WRITE(mip, clobber ? 0 : (1ULL << 1));
    DV_HYP_CSR_WRITE(mcounteren, clobber ? 0 : 7);
    DV_HYP_CSR_WRITE(scounteren, clobber ? 0 : 3);
    DV_HYP_CSR_WRITE(menvcfg, clobber ? 0 : (3ULL << 4));
    DV_HYP_CSR_WRITE(senvcfg, clobber ? 0 : (1ULL << 4));
    // BARE with a different legal/implementation-defined PPN field is safe in
    // M-mode and tests restoration when the implementation retains that field.
    DV_HYP_CSR_WRITE(satp, clobber ? 0 : 0x1234);
    __asm__ volatile("sfence.vma" ::: "memory");
    DV_HYP_CSR_WRITE(fcsr, clobber ? 0x70 : 0x05);
    if (clobber) {
        __asm__ volatile("li t0, 3\nvsetvli zero, t0, e16, m2, ta, ma" ::: "t0", "memory");
    } else {
        __asm__ volatile("li t0, 7\nvsetvli zero, t0, e8, m1, tu, mu" ::: "t0", "memory");
    }
    DV_HYP_CSR_WRITE(vcsr, clobber ? 0 : 5);
    DV_HYP_CSR_WRITE(vstart, clobber ? 1 : 2);
}

// This deliberately changes the state which bridge.S promises to restore.
// It is synthetic validation and must not be counted as a migrated hypertest.
bool dv_hypertest_selftest_clobber_case(void)
{
    DV_HYP_CSR_WRITE(mstatus, 0x6600ULL | (3ULL << 11));
    dv_hyp_selftest_fill_regs(fp_clobber, vector_clobber);
    seed_control(true);
    ++clobber_calls;
    return true;
}

bool dv_hypertest_bridge_selftest(void)
{
    const uint64_t required = (1ULL << ('F' - 'A')) |
                              (1ULL << ('D' - 'A')) |
                              (1ULL << ('V' - 'A'));
    if ((DV_HYP_CSR_READ(misa) & required) != required) {
        return false;
    }
    DV_HYP_CSR_WRITE(mstatus, 0x6600ULL | (3ULL << 11));
    const size_t vlenb = DV_HYP_CSR_READ(vlenb);
    if (vlenb == 0 || vlenb > DV_HYP_SELFTEST_MAX_VLENB) {
        return false;
    }
    clobber_calls = 0;
    for (unsigned round = 0; round < 2; ++round) {
        struct dv_hyp_selftest_control expected;
        struct dv_hyp_selftest_control observed;
        for (unsigned reg = 0; reg < DV_HYP_SELFTEST_REGISTERS; ++reg) {
            fp_expected[reg] = 0x1234567800000000ULL ^ ((uint64_t)round << 40) ^ reg;
            fp_clobber[reg] = ~fp_expected[reg];
        }
        const size_t bytes = DV_HYP_SELFTEST_REGISTERS * vlenb;
        for (size_t i = 0; i < bytes; ++i) {
            const size_t reg = i / vlenb;
            const size_t lane = i % vlenb;
            vector_expected[i] = (unsigned char)((reg * 7) ^ (lane * 17) ^
                                                  ((lane >> 8) * 13) ^ (round * 29));
            vector_clobber[i] = (unsigned char)~vector_expected[i];
        }
        dv_hyp_selftest_fill_regs(fp_expected, vector_expected);
        seed_control(false);
        // Test a saved Initial state first and a saved Clean state second;
        // neither equals the Dirty state needed while the bridge copies data.
        uint64_t status = DV_HYP_CSR_READ(mstatus) & ~0x6600ULL;
        status |= round == 0 ? 0x2200ULL : 0x4400ULL;
        DV_HYP_CSR_WRITE(mstatus, status);
        capture_control(&expected);
        dv_hyp_selftest_invoke(dv_hypertest_selftest_clobber_case);
        capture_control(&observed);
        if (!same_control(&expected, &observed)) {
            return false;
        }
        dv_hyp_selftest_save_regs(fp_observed, vector_observed);
        for (unsigned reg = 0; reg < DV_HYP_SELFTEST_REGISTERS; ++reg) {
            if (fp_observed[reg] != fp_expected[reg]) {
                return false;
            }
        }
        for (size_t i = 0; i < bytes; ++i) {
            if (vector_observed[i] != vector_expected[i]) {
                return false;
            }
        }
    }
    return clobber_calls == 2;
}
