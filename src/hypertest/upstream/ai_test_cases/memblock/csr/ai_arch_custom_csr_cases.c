#include <rvh_test.h>
#include <page_tables.h>
#include "../replay_retry/ai_micro_prefetch_common.h"
#include <stdint.h>

#ifndef CSR_SPFCTL
#define CSR_SPFCTL 0x5c1
#endif

#ifndef CSR_SMBLOCKCTL
#define CSR_SMBLOCKCTL 0x5c3
#endif

#ifndef CSR_SCEC
#define CSR_SCEC 0x5da
#endif

#ifndef CSR_SCEIC
#define CSR_SCEIC 0x5db
#endif

#ifndef CSR_MSTATEEN0
#define CSR_MSTATEEN0 0x30c
#endif

#ifndef CSR_MEMBLOCK_CUSTOM_UNUSED
#define CSR_MEMBLOCK_CUSTOM_UNUSED 0x5c2
#endif

#ifndef CSR_SSTATEEN0
#define CSR_SSTATEEN0 0x10c
#endif

#ifndef MSTATEEN_C
#define MSTATEEN_C (1ULL << 0)
#endif

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_ARCH_SPFCTL_ENABLE_MASK ( \
    (1ULL << 0) | \
    (1ULL << 1) | \
    (1ULL << 2) | \
    (1ULL << 3) | \
    (1ULL << 4) | \
    (1ULL << 5) | \
    (1ULL << 16) | \
    (1ULL << 17))
#define AI_ARCH_SPFCTL_L1I_PF_ENABLE         (1ULL << 0)
#define AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK (0xFULL << 6)
#define AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK    (0x3FULL << 10)
#define AI_ARCH_SPFCTL_L2_PF_ENABLE          (1ULL << 1)
#define AI_ARCH_SPFCTL_L1D_PF_ENABLE         (1ULL << 2)
#define AI_ARCH_SPFCTL_TRAIN_ON_HIT          (1ULL << 3)
#define AI_ARCH_SPFCTL_ENABLE_AGT            (1ULL << 4)
#define AI_ARCH_SPFCTL_ENABLE_PHT            (1ULL << 5)
#define AI_ARCH_SPFCTL_ENABLE_STRIDE         (1ULL << 16)
#define AI_ARCH_SPFCTL_L2_STORE_ONLY         (1ULL << 17)
#define AI_ARCH_SPFCTL_RESET_ACTIVE_THRESHOLD (12ULL << 6)
#define AI_ARCH_SPFCTL_RESET_ACTIVE_STRIDE    (30ULL << 10)
#define AI_ARCH_SPFCTL_RESET_VALUE ( \
    AI_ARCH_SPFCTL_L1I_PF_ENABLE | \
    AI_ARCH_SPFCTL_L2_PF_ENABLE | \
    AI_ARCH_SPFCTL_L1D_PF_ENABLE | \
    AI_ARCH_SPFCTL_ENABLE_AGT | \
    AI_ARCH_SPFCTL_ENABLE_PHT | \
    AI_ARCH_SPFCTL_RESET_ACTIVE_THRESHOLD | \
    AI_ARCH_SPFCTL_RESET_ACTIVE_STRIDE | \
    AI_ARCH_SPFCTL_ENABLE_STRIDE)

#define AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE (1ULL << 5)
#define AI_ARCH_SMBLOCKCTL_CACHE_ERROR_ENABLE   (1ULL << 6)
#define AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE   (1ULL << 8)
#define AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE   (1ULL << 9)
#define AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE (1ULL << 0)
#define AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK (0xFULL << 1)
#define AI_ARCH_SMBLOCKCTL_RESET_REPLAY_ESCAPE_TIME (12ULL << 1)
#define AI_ARCH_SMBLOCKCTL_RESET_VALUE ( \
    AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | \
    AI_ARCH_SMBLOCKCTL_RESET_REPLAY_ESCAPE_TIME)

#define AI_ARCH_CUSTOM_CSR_T0 5U
#define AI_ARCH_CUSTOM_CSR_T1 6U
#define AI_ARCH_CUSTOM_CSR_T2 7U
#define AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(csr) \
    (((uint32_t)(csr) << 20) | (2U << 12) | \
        (AI_ARCH_CUSTOM_CSR_T0 << 7) | 0x73U)
#define AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(csr) \
    (((uint32_t)(csr) << 20) | (AI_ARCH_CUSTOM_CSR_T1 << 15) | \
        (1U << 12) | (AI_ARCH_CUSTOM_CSR_T0 << 7) | 0x73U)
#define AI_ARCH_CUSTOM_CSR_CSRRW_T2_T1_INST(csr) \
    (((uint32_t)(csr) << 20) | (AI_ARCH_CUSTOM_CSR_T1 << 15) | \
        (1U << 12) | (AI_ARCH_CUSTOM_CSR_T2 << 7) | 0x73U)

#define AI_ARCH_SPFCTL_DEFINED_MASK ((1ULL << 18) - 1ULL)
#define AI_ARCH_SMBLOCKCTL_DEFINED_MASK ((1ULL << 5) - 1ULL)
#define AI_ARCH_SCEC_DEFINED_MASK (~0ULL)
#define AI_ARCH_SCEIC_DEFINED_MASK ((1ULL << 5) - 1ULL)
#define AI_ARCH_CUSTOM_CSR_SCEC_SEED \
    ((1ULL << 63) | (3ULL << 60) | 0x123456789abcULL)
#define AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE (3ULL << 2)
#define AI_ARCH_CUSTOM_CSR_SCEIC_RESET_VALUE (7ULL << 2)

// On current Xiangshan RTL, the replay_escape fields are the only smblockctl
// fields with confirmed downstream functional consumers. The soft/cache and
// misaligned-marked bits below are therefore covered only as WARL stability or
// no-regression probes, not as fixed readback or effect-enablement tests.

#define AI_ARCH_CUSTOM_DATA_PAGE VSRWX_GRWX
#define AI_ARCH_CUSTOM_WORD0_OFF 0x080U
#define AI_ARCH_CUSTOM_WORD1_OFF 0x088U
#define AI_ARCH_CUSTOM_WORD2_OFF 0x140U
#define AI_ARCH_CUSTOM_WORD3_OFF 0x180U
#define AI_ARCH_CUSTOM_CBO_BLOCK_OFF 0x240U
#define AI_ARCH_CUSTOM_CBO_BLOCK_WORDS 8U

static inline uintptr_t ai_arch_custom_vaddr(uintptr_t offset)
{
    return hs_page_base(AI_ARCH_CUSTOM_DATA_PAGE) + offset;
}

static inline uintptr_t ai_arch_custom_paddr(uintptr_t offset)
{
    return phys_page_base(AI_ARCH_CUSTOM_DATA_PAGE) + offset;
}

static inline uintptr_t ai_arch_page_vaddr(enum test_page page, uintptr_t offset)
{
    return hs_page_base(page) + offset;
}

static inline uintptr_t ai_arch_page_paddr(enum test_page page, uintptr_t offset)
{
    return phys_page_base(page) + offset;
}

static inline void ai_arch_custom_prefetch_reset_pbmt_env(void)
{
    ai_micro_prefetch_reset_pbmt_env();
}

static inline uintptr_t ai_arch_custom_prefetch_io_backing_paddr(void)
{
    return ai_micro_prefetch_io_backing_paddr();
}

static inline uintptr_t ai_arch_custom_prefetch_prepare_io_alias(void)
{
    return ai_micro_prefetch_prepare_io_alias();
}

static inline pte_t *ai_arch_custom_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_custom_prepare_hs_rw_page(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_arch_custom_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "ld %[value], 0(%[addr])\n\t"
        : [value] "+r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_arch_custom_prefetch_r_gpr_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "mv %[value], %[sentinel]\n\t"
        ".insn i 0x13, 0x6, x0, %[addr], 1\n\t"
        : [value] "+r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "memory");

    return value;
}

static inline uint64_t ai_arch_custom_prefetch_w_gpr_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "mv %[value], %[sentinel]\n\t"
        ".insn i 0x13, 0x6, x0, %[addr], 3\n\t"
        : [value] "+r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "memory");

    return value;
}

static inline uint64_t ai_arch_custom_prefetch_i_gpr_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "mv %[value], %[sentinel]\n\t"
        ".insn i 0x13, 0x6, x0, %[addr], 0\n\t"
        : [value] "+r"(value)
        : [addr] "r"(addr), [sentinel] "r"(sentinel)
        : "memory");

    return value;
}

static inline void ai_arch_custom_seed_four_words(
    uint64_t value0,
    uint64_t value1,
    uint64_t value2,
    uint64_t value3
)
{
    goto_priv(PRIV_M);
    write64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF), value0);
    write64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF), value1);
    write64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF), value2);
    write64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF), value3);
    ai_arch_custom_prepare_hs_rw_page();
}

static inline uint64_t ai_arch_read_spfctl(void)
{
    return CSRR(CSR_SPFCTL);
}

static inline void ai_arch_write_spfctl(uint64_t value)
{
    CSRW(CSR_SPFCTL, value);
}

static inline uint64_t ai_arch_read_smblockctl(void)
{
    return CSRR(CSR_SMBLOCKCTL);
}

static inline void ai_arch_write_smblockctl(uint64_t value)
{
    CSRW(CSR_SMBLOCKCTL, value);
}

static inline void ai_arch_custom_fence_i(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline void ai_arch_custom_fence_tso(void)
{
    asm volatile(".word 0x8330000f" ::: "memory");
}

static inline uint64_t ai_arch_custom_amoadd_d_aqrl(uintptr_t addr,
    uint64_t value)
{
    uint64_t old_value;

    asm volatile(
        "amoadd.d.aqrl %0, %2, 0(%1)\n\t"
        : "=r"(old_value)
        : "r"(addr), "r"(value)
        : "memory");

    return old_value;
}

static inline uint64_t ai_arch_read_scec(void)
{
    return CSRR(CSR_SCEC);
}

static inline void ai_arch_write_scec(uint64_t value)
{
    CSRW(CSR_SCEC, value);
}

static inline uint64_t ai_arch_read_sceic(void)
{
    return CSRR(CSR_SCEIC);
}

static inline void ai_arch_write_sceic(uint64_t value)
{
    CSRW(CSR_SCEIC, value);
}

static inline void ai_arch_custom_fixed_csrr_spfctl(void)
{
    asm volatile("csrr t0, 0x5c1" ::: "t0", "memory");
}

static inline void ai_arch_custom_fixed_csrw_spfctl(uint64_t value)
{
    asm volatile(
        "mv t1, %0\n\t"
        "csrrw t0, 0x5c1, t1\n\t"
        :: "r"(value)
        : "t0", "t1", "memory");
}

static inline void ai_arch_custom_fixed_csrr_smblockctl(void)
{
    asm volatile("csrr t0, 0x5c3" ::: "t0", "memory");
}

static inline void ai_arch_custom_fixed_csrw_smblockctl(uint64_t value)
{
    asm volatile(
        "mv t1, %0\n\t"
        "csrrw t0, 0x5c3, t1\n\t"
        :: "r"(value)
        : "t0", "t1", "memory");
}

static inline void ai_arch_custom_fixed_csrr_scec(void)
{
    asm volatile("csrr t0, 0x5da" ::: "t0", "memory");
}

static inline void ai_arch_custom_fixed_csrw_scec(uint64_t value)
{
    asm volatile(
        "mv t1, %0\n\t"
        "csrrw t0, 0x5da, t1\n\t"
        :: "r"(value)
        : "t0", "t1", "memory");
}

static inline void ai_arch_custom_fixed_csrr_sceic(void)
{
    asm volatile("csrr t0, 0x5db" ::: "t0", "memory");
}

static inline void ai_arch_custom_fixed_csrw_sceic(uint64_t value)
{
    asm volatile(
        "mv t1, %0\n\t"
        "csrrw t0, 0x5db, t1\n\t"
        :: "r"(value)
        : "t0", "t1", "memory");
}

static inline void ai_arch_custom_fixed_csrr_unused(void)
{
    asm volatile("csrr t0, 0x5c2" ::: "t0", "memory");
}

static inline void ai_arch_custom_fixed_csrw_unused(uint64_t value)
{
    asm volatile(
        "mv t1, %0\n\t"
        "csrrw t0, 0x5c2, t1\n\t"
        :: "r"(value)
        : "t0", "t1", "memory");
}

static inline bool ai_arch_custom_illegal_with_inst(uint32_t expected_inst)
{
    return excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (uint32_t)excpt.tval == expected_inst &&
        excpt.epc != 0;
}

static inline uint64_t ai_arch_custom_read_csr(unsigned csr)
{
    switch (csr) {
    case CSR_SPFCTL:
        return ai_arch_read_spfctl();
    case CSR_SMBLOCKCTL:
        return ai_arch_read_smblockctl();
    case CSR_SCEC:
        return ai_arch_read_scec();
    case CSR_SCEIC:
        return ai_arch_read_sceic();
    default:
        return 0ULL;
    }
}

static inline void ai_arch_custom_write_csr(unsigned csr, uint64_t value)
{
    switch (csr) {
    case CSR_SPFCTL:
        ai_arch_write_spfctl(value);
        break;
    case CSR_SMBLOCKCTL:
        ai_arch_write_smblockctl(value);
        break;
    case CSR_SCEC:
        ai_arch_write_scec(value);
        break;
    case CSR_SCEIC:
        ai_arch_write_sceic(value);
        break;
    default:
        break;
    }
}

static inline void ai_arch_custom_set_csr(unsigned csr, uint64_t value)
{
    switch (csr) {
    case CSR_SPFCTL:
        CSRS(CSR_SPFCTL, value);
        break;
    case CSR_SMBLOCKCTL:
        CSRS(CSR_SMBLOCKCTL, value);
        break;
    case CSR_SCEC:
        CSRS(CSR_SCEC, value);
        break;
    case CSR_SCEIC:
        CSRS(CSR_SCEIC, value);
        break;
    default:
        break;
    }
}

static inline void ai_arch_custom_clear_csr(unsigned csr, uint64_t value)
{
    switch (csr) {
    case CSR_SPFCTL:
        CSRC(CSR_SPFCTL, value);
        break;
    case CSR_SMBLOCKCTL:
        CSRC(CSR_SMBLOCKCTL, value);
        break;
    case CSR_SCEC:
        CSRC(CSR_SCEC, value);
        break;
    case CSR_SCEIC:
        CSRC(CSR_SCEIC, value);
        break;
    default:
        break;
    }
}

static inline uint64_t ai_arch_custom_csrrw_csr(unsigned csr, uint64_t value)
{
    uint64_t old_value = 0ULL;

    switch (csr) {
    case CSR_SPFCTL:
        asm volatile("csrrw %0, 0x5c1, %1"
            : "=r"(old_value) : "r"(value) : "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrrw %0, 0x5c3, %1"
            : "=r"(old_value) : "r"(value) : "memory");
        break;
    case CSR_SCEC:
        asm volatile("csrrw %0, 0x5da, %1"
            : "=r"(old_value) : "r"(value) : "memory");
        break;
    case CSR_SCEIC:
        asm volatile("csrrw %0, 0x5db, %1"
            : "=r"(old_value) : "r"(value) : "memory");
        break;
    default:
        break;
    }

    return old_value;
}

static inline void ai_arch_custom_csrrw_x0_csr(unsigned csr, uint64_t value)
{
    switch (csr) {
    case CSR_SPFCTL:
        asm volatile("csrrw x0, 0x5c1, %0" :: "r"(value) : "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrrw x0, 0x5c3, %0" :: "r"(value) : "memory");
        break;
    case CSR_SCEC:
        asm volatile("csrrw x0, 0x5da, %0" :: "r"(value) : "memory");
        break;
    case CSR_SCEIC:
        asm volatile("csrrw x0, 0x5db, %0" :: "r"(value) : "memory");
        break;
    default:
        break;
    }
}

static inline uint64_t ai_arch_custom_csrrs_csr(unsigned csr, uint64_t mask)
{
    uint64_t old_value = 0ULL;

    switch (csr) {
    case CSR_SPFCTL:
        asm volatile("csrrs %0, 0x5c1, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrrs %0, 0x5c3, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    case CSR_SCEC:
        asm volatile("csrrs %0, 0x5da, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    case CSR_SCEIC:
        asm volatile("csrrs %0, 0x5db, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    default:
        break;
    }

    return old_value;
}

static inline uint64_t ai_arch_custom_csrrc_csr(unsigned csr, uint64_t mask)
{
    uint64_t old_value = 0ULL;

    switch (csr) {
    case CSR_SPFCTL:
        asm volatile("csrrc %0, 0x5c1, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrrc %0, 0x5c3, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    case CSR_SCEC:
        asm volatile("csrrc %0, 0x5da, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    case CSR_SCEIC:
        asm volatile("csrrc %0, 0x5db, %1"
            : "=r"(old_value) : "r"(mask) : "memory");
        break;
    default:
        break;
    }

    return old_value;
}

static inline uint64_t ai_arch_custom_csrrs_x0_csr(unsigned csr)
{
    uint64_t old_value = 0ULL;

    switch (csr) {
    case CSR_SPFCTL:
        asm volatile("csrrs %0, 0x5c1, x0"
            : "=r"(old_value) :: "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrrs %0, 0x5c3, x0"
            : "=r"(old_value) :: "memory");
        break;
    case CSR_SCEC:
        asm volatile("csrrs %0, 0x5da, x0"
            : "=r"(old_value) :: "memory");
        break;
    case CSR_SCEIC:
        asm volatile("csrrs %0, 0x5db, x0"
            : "=r"(old_value) :: "memory");
        break;
    default:
        break;
    }

    return old_value;
}

static inline uint64_t ai_arch_custom_csrrc_x0_csr(unsigned csr)
{
    uint64_t old_value = 0ULL;

    switch (csr) {
    case CSR_SPFCTL:
        asm volatile("csrrc %0, 0x5c1, x0"
            : "=r"(old_value) :: "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrrc %0, 0x5c3, x0"
            : "=r"(old_value) :: "memory");
        break;
    case CSR_SCEC:
        asm volatile("csrrc %0, 0x5da, x0"
            : "=r"(old_value) :: "memory");
        break;
    case CSR_SCEIC:
        asm volatile("csrrc %0, 0x5db, x0"
            : "=r"(old_value) :: "memory");
        break;
    default:
        break;
    }

    return old_value;
}

static inline uint64_t ai_arch_custom_spfctl_csrrwi_imm3(void)
{
    uint64_t old_value;

    asm volatile("csrrwi %0, 0x5c1, 3"
        : "=r"(old_value) :: "memory");
    return old_value;
}

static inline uint64_t ai_arch_custom_spfctl_csrrwi_imm0(void)
{
    uint64_t old_value;

    asm volatile("csrrwi %0, 0x5c1, 0"
        : "=r"(old_value) :: "memory");
    return old_value;
}

static inline uint64_t ai_arch_custom_spfctl_csrrsi_imm6(void)
{
    uint64_t old_value;

    asm volatile("csrrsi %0, 0x5c1, 6"
        : "=r"(old_value) :: "memory");
    return old_value;
}

static inline uint64_t ai_arch_custom_spfctl_csrrci_imm2(void)
{
    uint64_t old_value;

    asm volatile("csrrci %0, 0x5c1, 2"
        : "=r"(old_value) :: "memory");
    return old_value;
}

static inline uint64_t ai_arch_custom_spfctl_csrrsi_imm0(void)
{
    uint64_t old_value;

    asm volatile("csrrsi %0, 0x5c1, 0"
        : "=r"(old_value) :: "memory");
    return old_value;
}

static inline uint64_t ai_arch_custom_spfctl_csrrci_imm0(void)
{
    uint64_t old_value;

    asm volatile("csrrci %0, 0x5c1, 0"
        : "=r"(old_value) :: "memory");
    return old_value;
}

static inline uint64_t ai_arch_custom_unused_csrrw_rd_probe(uint64_t value)
{
    uint64_t observed_rd;

    asm volatile(
        "li t2, 0x5a5a1234deadbeef\n\t"
        "mv t1, %1\n\t"
        "csrrw t2, 0x5c2, t1\n\t"
        "mv %0, t2\n\t"
        : "=r"(observed_rd)
        : "r"(value)
        : "t1", "t2", "memory");

    return observed_rd;
}

static inline void ai_arch_custom_fixed_csrr(unsigned csr)
{
    switch (csr) {
    case CSR_SPFCTL:
        ai_arch_custom_fixed_csrr_spfctl();
        break;
    case CSR_SMBLOCKCTL:
        ai_arch_custom_fixed_csrr_smblockctl();
        break;
    case CSR_SCEC:
        ai_arch_custom_fixed_csrr_scec();
        break;
    case CSR_SCEIC:
        ai_arch_custom_fixed_csrr_sceic();
        break;
    default:
        break;
    }
}

static inline void ai_arch_custom_fixed_csrw(unsigned csr, uint64_t value)
{
    switch (csr) {
    case CSR_SPFCTL:
        ai_arch_custom_fixed_csrw_spfctl(value);
        break;
    case CSR_SMBLOCKCTL:
        ai_arch_custom_fixed_csrw_smblockctl(value);
        break;
    case CSR_SCEC:
        ai_arch_custom_fixed_csrw_scec(value);
        break;
    case CSR_SCEIC:
        ai_arch_custom_fixed_csrw_sceic(value);
        break;
    default:
        break;
    }
}

static inline bool ai_arch_custom_probe_write_read(
    unsigned csr,
    uint64_t value,
    uint64_t mask,
    uint64_t *observed_out
)
{
    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(csr, value);
    if (excpt.triggered) {
        return false;
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_custom_read_csr(csr);
    if (excpt.triggered) {
        return false;
    }

    if (observed_out != 0) {
        *observed_out = observed;
    }

    return (observed & ~mask) == 0ULL;
}

static inline uintptr_t ai_arch_sms_region_offset(unsigned region_index, unsigned line_index)
{
    return 0x080U + (uintptr_t)region_index * 0x400U + (uintptr_t)line_index * 0x040U;
}

static inline uintptr_t ai_arch_sms_region_vaddr(
    const enum test_page *pages,
    unsigned global_region,
    unsigned regions_per_page,
    unsigned line_index
)
{
    unsigned page_index = global_region / regions_per_page;
    unsigned region_index = global_region % regions_per_page;
    return ai_arch_page_vaddr(pages[page_index], ai_arch_sms_region_offset(region_index, line_index));
}

static inline uintptr_t ai_arch_sms_region_paddr(
    const enum test_page *pages,
    unsigned global_region,
    unsigned regions_per_page,
    unsigned line_index
)
{
    unsigned page_index = global_region / regions_per_page;
    unsigned region_index = global_region % regions_per_page;
    return ai_arch_page_paddr(pages[page_index], ai_arch_sms_region_offset(region_index, line_index));
}

static inline uint64_t ai_arch_sms_region_value(uint64_t seed_base, unsigned global_region, unsigned line_index)
{
    return seed_base + ((uint64_t)global_region << 8) + (uint64_t)line_index;
}

static void ai_arch_sms_seed_region_stream(
    const enum test_page *pages,
    unsigned start_region,
    unsigned region_count,
    unsigned lines_per_region,
    unsigned regions_per_page,
    uint64_t seed_base
)
{
    for (unsigned region = 0; region < region_count; ++region) {
        unsigned global_region = start_region + region;
        for (unsigned line = 0; line < lines_per_region; ++line) {
            write64(
                ai_arch_sms_region_paddr(
                    pages,
                    global_region,
                    regions_per_page,
                    line
                ),
                ai_arch_sms_region_value(seed_base, global_region, line)
            );
        }
    }
}

static void ai_arch_sms_store_region_stream(
    const enum test_page *pages,
    unsigned start_region,
    unsigned region_count,
    unsigned lines_per_region,
    unsigned regions_per_page,
    uint64_t seed_base
)
{
    for (unsigned region = 0; region < region_count; ++region) {
        unsigned global_region = start_region + region;
        for (unsigned line = 0; line < lines_per_region; ++line) {
            write64(
                ai_arch_sms_region_vaddr(
                    pages,
                    global_region,
                    regions_per_page,
                    line
                ),
                ai_arch_sms_region_value(seed_base, global_region, line)
            );
        }
    }
}

__attribute__((noinline)) static bool ai_arch_sms_check_region_stream(
    const enum test_page *pages,
    unsigned start_region,
    unsigned region_count,
    unsigned lines_per_region,
    unsigned regions_per_page,
    uint64_t seed_base
)
{
    bool match = true;

    for (unsigned region = 0; region < region_count; ++region) {
        unsigned global_region = start_region + region;
        for (unsigned line = 0; line < lines_per_region; ++line) {
            uintptr_t vaddr = ai_arch_sms_region_vaddr(
                pages,
                global_region,
                regions_per_page,
                line
            );
            uint64_t expected = ai_arch_sms_region_value(seed_base, global_region, line);
            match = match && read64(vaddr) == expected;
        }
    }

    return match;
}

__attribute__((noinline)) static bool ai_arch_sms_check_region_stream_rounds(
    const enum test_page *pages,
    unsigned start_region,
    unsigned region_count,
    unsigned lines_per_region,
    unsigned regions_per_page,
    uint64_t seed_base,
    unsigned rounds
)
{
    bool match = true;

    for (unsigned round = 0; round < rounds; ++round) {
        match = match && ai_arch_sms_check_region_stream(
            pages,
            start_region,
            region_count,
            lines_per_region,
            regions_per_page,
            seed_base
        );
    }

    return match;
}

extern bool ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner(void);
extern bool ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner(void);
extern bool ai_micro_hs_prefetch_page_fault_then_valid_access_recovers_corner(void);
extern bool ai_micro_hs_prefetch_page_fault_then_valid_access_adjacent_preserved_corner(void);
extern bool ai_micro_hs_prefetch_repeated_page_fault_silent_corner(void);
extern bool ai_micro_hs_prefetch_access_fault_then_valid_access_recovers_corner(void);
extern bool ai_micro_hs_prefetch_access_fault_then_valid_access_adjacent_preserved_corner(void);
extern bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_adjacent_preserved_corner(void);
extern bool ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner(void);

static bool ai_arch_smblockctl_write_read_stable(uint64_t value, uint64_t *observed_out)
{
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(value);
    if (excpt.triggered) {
        return false;
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_smblockctl();
    if (excpt.triggered) {
        return false;
    }

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(observed);
    if (excpt.triggered) {
        return false;
    }

    TEST_SETUP_EXCEPT();
    uint64_t stable = ai_arch_read_smblockctl();
    if (excpt.triggered) {
        return false;
    }

    if (observed_out != 0) {
        *observed_out = observed;
    }

    return stable == observed;
}

bool ai_arch_smblockctl_non_replay_escape_warl_fields_probe_corner()
{
    TEST_START();

    const uint64_t non_replay_mask =
        AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE |
        AI_ARCH_SMBLOCKCTL_CACHE_ERROR_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE;
    const uint64_t replay_mask =
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK;
    const uint64_t patterns[] = {
        0ULL,
        AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE |
            AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE,
        AI_ARCH_SMBLOCKCTL_CACHE_ERROR_ENABLE |
            AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE,
        AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE |
            AI_ARCH_SMBLOCKCTL_CACHE_ERROR_ENABLE |
            AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE |
            AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE,
    };

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    bool smblockctl_readable = excpt.triggered == false;
    TEST_ASSERT("smblockctl is readable before probing non-replay_escape WARL fields",
        smblockctl_readable
    );

    if (smblockctl_readable) {
        for (unsigned i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
            uint64_t candidate = (original & ~non_replay_mask) | patterns[i];
            uint64_t observed = 0ULL;

            TEST_ASSERT("a non-replay_escape smblockctl WARL candidate write converges to a stable readback value",
                ai_arch_smblockctl_write_read_stable(candidate, &observed)
            );

            TEST_ASSERT("probing non-replay_escape WARL fields does not alter implemented replay_escape fields",
                (observed & replay_mask) == (original & replay_mask)
            );
        }

        TEST_ASSERT("restoring smblockctl after non-replay_escape WARL probing converges to a stable readback value",
            ai_arch_smblockctl_write_read_stable(original, 0)
        );
    }

    TEST_END();
}

static volatile uint64_t ai_arch_custom_handler_hits;
static volatile uint64_t ai_arch_custom_handler_cause;
static volatile uint64_t ai_arch_custom_handler_epc;
static volatile uint64_t ai_arch_custom_handler_spf_snapshot;
static volatile uint64_t ai_arch_custom_handler_spf_after_clear;
static volatile uint64_t ai_arch_custom_handler_smblock_snapshot;
static volatile uint64_t ai_arch_custom_handler_smblock_after_write;
static volatile uint64_t ai_arch_custom_handler_fell_through;

extern void ai_arch_custom_csr_snapshot_mtvec_entry(void);
extern void ai_arch_custom_csr_snapshot_ecall_site(void);
extern void ai_arch_custom_csr_snapshot_after_trap(void);

asm(
    ".balign 4\n"
    ".global ai_arch_custom_csr_snapshot_mtvec_entry\n"
    "ai_arch_custom_csr_snapshot_mtvec_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd x5, 0(sp)\n"
    "  sd x6, 8(sp)\n"
    "  sd x7, 16(sp)\n"
    "  la x5, ai_arch_custom_handler_hits\n"
    "  ld x6, 0(x5)\n"
    "  addi x6, x6, 1\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mcause\n"
    "  la x5, ai_arch_custom_handler_cause\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, mepc\n"
    "  la x5, ai_arch_custom_handler_epc\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, 0x5c1\n"
    "  la x5, ai_arch_custom_handler_spf_snapshot\n"
    "  sd x6, 0(x5)\n"
    "  csrw 0x5c1, x0\n"
    "  csrr x6, 0x5c1\n"
    "  la x5, ai_arch_custom_handler_spf_after_clear\n"
    "  sd x6, 0(x5)\n"
    "  csrr x6, 0x5c3\n"
    "  la x5, ai_arch_custom_handler_smblock_snapshot\n"
    "  sd x6, 0(x5)\n"
    "  li x7, 0x7\n"
    "  csrw 0x5c3, x7\n"
    "  csrr x6, 0x5c3\n"
    "  la x5, ai_arch_custom_handler_smblock_after_write\n"
    "  sd x6, 0(x5)\n"
    "  la x5, ai_arch_custom_csr_snapshot_after_trap\n"
    "  csrw mepc, x5\n"
    "  ld x5, 0(sp)\n"
    "  ld x6, 8(sp)\n"
    "  ld x7, 16(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
);

static inline void ai_arch_custom_handler_reset_state(void)
{
    ai_arch_custom_handler_hits = 0;
    ai_arch_custom_handler_cause = 0;
    ai_arch_custom_handler_epc = 0;
    ai_arch_custom_handler_spf_snapshot = 0;
    ai_arch_custom_handler_spf_after_clear = 0;
    ai_arch_custom_handler_smblock_snapshot = 0;
    ai_arch_custom_handler_smblock_after_write = 0;
    ai_arch_custom_handler_fell_through = 0;
}

bool ai_arch_memblock_custom_csr_privilege_wpri_warl_matrix_corner()
{
    TEST_START();

    static const unsigned custom_csrs[] = {
        CSR_SPFCTL,
        CSR_SMBLOCKCTL,
        CSR_SCEC,
        CSR_SCEIC,
    };
    static const uint64_t legal_values[] = {
        AI_ARCH_SPFCTL_ENABLE_MASK |
            AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK |
            AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
            AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK,
        AI_ARCH_CUSTOM_CSR_SCEC_SEED,
        AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE,
    };
    static const uint64_t defined_masks[] = {
        AI_ARCH_SPFCTL_DEFINED_MASK,
        AI_ARCH_SMBLOCKCTL_DEFINED_MASK,
        AI_ARCH_SCEC_DEFINED_MASK,
        AI_ARCH_SCEIC_DEFINED_MASK,
    };
    uint64_t original_values[sizeof(custom_csrs) / sizeof(custom_csrs[0])];
    uint64_t m_observed[sizeof(custom_csrs) / sizeof(custom_csrs[0])];
    uint64_t hs_observed[sizeof(custom_csrs) / sizeof(custom_csrs[0])];

    goto_priv(PRIV_M);
    const uint64_t original_mstateen0 = CSRR(CSR_MSTATEEN0);
    const uint64_t original_sstateen0 = CSRR(CSR_SSTATEEN0);
    const uint64_t original_mstatus = CSRR(CSR_MSTATUS);

    CSRS(CSR_MSTATEEN0, MSTATEEN_C);
    CSRC(CSR_SSTATEEN0, MSTATEEN_C);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_SETUP_EXCEPT();
        original_values[i] = ai_arch_custom_read_csr(custom_csrs[i]);
        TEST_ASSERT("M-mode can read each MemBlock custom CSR before the privilege matrix",
            excpt.triggered == false
        );
    }

    TEST_ASSERT("M-mode SPFctl WPRI high bits are masked while defined bits remain writable",
        ai_arch_custom_probe_write_read(
            CSR_SPFCTL,
            ~0ULL,
            AI_ARCH_SPFCTL_DEFINED_MASK,
            &m_observed[0]
        ) &&
        (m_observed[0] & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (m_observed[0] & AI_ARCH_SPFCTL_DEFINED_MASK) ==
            AI_ARCH_SPFCTL_DEFINED_MASK
    );

    TEST_ASSERT("M-mode SMBlockCtl WPRI high bits are masked while replay_escape fields remain writable",
        ai_arch_custom_probe_write_read(
            CSR_SMBLOCKCTL,
            ~0ULL,
            AI_ARCH_SMBLOCKCTL_DEFINED_MASK,
            &m_observed[1]
        ) &&
        (m_observed[1] & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL &&
        (m_observed[1] & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            AI_ARCH_SMBLOCKCTL_DEFINED_MASK
    );

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_ASSERT("M-mode CSRRW-style custom CSR write converges without trapping and does not expose WPRI high bits",
            ai_arch_custom_probe_write_read(
                custom_csrs[i],
                legal_values[i],
                defined_masks[i],
                &m_observed[i]
            )
        );
    }

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(custom_csrs[i], 0ULL);
        uint64_t old_value = ai_arch_custom_csrrw_csr(
            custom_csrs[i],
            legal_values[i]
        );
        uint64_t after_csrrw = ai_arch_custom_read_csr(custom_csrs[i]);
        TEST_ASSERT("M-mode CSRRW with rd returns the old custom CSR value and commits only defined fields",
            excpt.triggered == false &&
            (old_value & defined_masks[i]) == 0ULL &&
            (after_csrrw & ~defined_masks[i]) == 0ULL &&
            (after_csrrw & defined_masks[i]) ==
                (legal_values[i] & defined_masks[i])
        );
    }

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(custom_csrs[i], 0ULL);
        ai_arch_custom_csrrw_x0_csr(custom_csrs[i], legal_values[i]);
        uint64_t after_csrrw_x0 = ai_arch_custom_read_csr(custom_csrs[i]);
        TEST_ASSERT("M-mode CSRRW rd=x0 suppresses old-value writeback while still committing the requested custom CSR update",
            excpt.triggered == false &&
            (after_csrrw_x0 & ~defined_masks[i]) == 0ULL &&
            (after_csrrw_x0 & defined_masks[i]) ==
                (legal_values[i] & defined_masks[i])
        );
    }

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE);
    uint64_t spf_before_x0_read = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_rs1_x0_old = ai_arch_custom_csrrs_x0_csr(CSR_SPFCTL);
    uint64_t spf_after_rs1_x0 = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_rc_x0_old = ai_arch_custom_csrrc_x0_csr(CSR_SPFCTL);
    uint64_t spf_after_rc_x0 = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode CSRRS/CSRRC rs1=x0 read the custom CSR old value without modifying any SPFctl field",
        excpt.triggered == false &&
        spf_rs1_x0_old == spf_before_x0_read &&
        spf_rc_x0_old == spf_before_x0_read &&
        spf_after_rs1_x0 == spf_before_x0_read &&
        spf_after_rc_x0 == spf_before_x0_read
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    uint64_t spf_old_for_set = ai_arch_custom_csrrs_csr(
        CSR_SPFCTL,
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
            AI_ARCH_SPFCTL_ENABLE_AGT);
    uint64_t spf_after_rs_set = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_old_for_clear = ai_arch_custom_csrrc_csr(
        CSR_SPFCTL,
        AI_ARCH_SPFCTL_ENABLE_AGT);
    uint64_t spf_after_rc_clear = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode CSRRS/CSRRC rd returns the previous SPFctl value and set/clear only the requested mask bits",
        excpt.triggered == false &&
        spf_old_for_set == 0ULL &&
        (spf_after_rs_set & (AI_ARCH_SPFCTL_L1D_PF_ENABLE |
            AI_ARCH_SPFCTL_ENABLE_AGT)) ==
            (AI_ARCH_SPFCTL_L1D_PF_ENABLE |
                AI_ARCH_SPFCTL_ENABLE_AGT) &&
        spf_old_for_clear == spf_after_rs_set &&
        (spf_after_rc_clear & AI_ARCH_SPFCTL_L1D_PF_ENABLE) != 0ULL &&
        (spf_after_rc_clear & AI_ARCH_SPFCTL_ENABLE_AGT) == 0ULL &&
        (spf_after_rc_clear & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    uint64_t spf_old_iwrite = ai_arch_custom_spfctl_csrrwi_imm3();
    uint64_t spf_after_iwrite = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_old_iset = ai_arch_custom_spfctl_csrrsi_imm6();
    uint64_t spf_after_iset = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_old_iclear = ai_arch_custom_spfctl_csrrci_imm2();
    uint64_t spf_after_iclear = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode CSRRWI/CSRRSI/CSRRCI immediate forms are equivalent to register-form write/set/clear on low SPFctl bits",
        excpt.triggered == false &&
        spf_old_iwrite == 0ULL &&
        (spf_after_iwrite & 0x3ULL) == 0x3ULL &&
        spf_old_iset == spf_after_iwrite &&
        (spf_after_iset & 0x7ULL) == 0x7ULL &&
        spf_old_iclear == spf_after_iset &&
        (spf_after_iclear & 0x7ULL) == 0x5ULL &&
        (spf_after_iclear & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t spf_before_uimm0 = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_old_iwrite0 = ai_arch_custom_spfctl_csrrwi_imm0();
    uint64_t spf_after_iwrite0 = ai_arch_custom_read_csr(CSR_SPFCTL);
    ai_arch_custom_write_csr(CSR_SPFCTL, spf_before_uimm0);
    uint64_t spf_old_iset0 = ai_arch_custom_spfctl_csrrsi_imm0();
    uint64_t spf_after_iset0 = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t spf_old_iclear0 = ai_arch_custom_spfctl_csrrci_imm0();
    uint64_t spf_after_iclear0 = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode immediate uimm=0 forms return the old SPFctl value and do not modify it except CSRRWI's architectural zero write",
        excpt.triggered == false &&
        spf_old_iwrite0 == spf_before_uimm0 &&
        (spf_after_iwrite0 & AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        spf_old_iset0 == spf_before_uimm0 &&
        spf_after_iset0 == spf_before_uimm0 &&
        spf_old_iclear0 == spf_before_uimm0 &&
        spf_after_iclear0 == spf_before_uimm0
    );

    static const uint64_t convergence_patterns[] = {
        0ULL,
        ~0ULL,
        0xaaaaaaaaaaaaaaaaULL,
        0x5555555555555555ULL,
    };
    for (unsigned csr_i = 0; csr_i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); csr_i++) {
        for (unsigned pat_i = 0; pat_i < sizeof(convergence_patterns) / sizeof(convergence_patterns[0]); pat_i++) {
            uint64_t candidate = convergence_patterns[pat_i];
            uint64_t observed = 0ULL;

            if (custom_csrs[csr_i] == CSR_SCEIC) {
                candidate = (candidate & ((1ULL << 1) | (1ULL << 2))) |
                    AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE;
            }

            TEST_ASSERT("M-mode custom CSR all-zero/all-one/alternating writes converge to a stable legal readback image",
                ai_arch_custom_probe_write_read(
                    custom_csrs[csr_i],
                    candidate,
                    defined_masks[csr_i],
                    &observed
                ) &&
                (observed & ~defined_masks[csr_i]) == 0ULL
            );
        }
    }

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        if (defined_masks[i] == ~0ULL) {
            continue;
        }

        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(custom_csrs[i], legal_values[i]);
        ai_arch_custom_write_csr(custom_csrs[i],
            (legal_values[i] & defined_masks[i]) | ~defined_masks[i]);
        uint64_t after_reserved_only = ai_arch_custom_read_csr(custom_csrs[i]);
        TEST_ASSERT("M-mode custom CSR reserved/WPRI-only writes do not disturb already-programmed defined fields",
            excpt.triggered == false &&
            (after_reserved_only & ~defined_masks[i]) == 0ULL &&
            (after_reserved_only & defined_masks[i]) ==
                (legal_values[i] & defined_masks[i])
        );
    }

    TEST_SETUP_EXCEPT();
    ai_arch_write_sceic(0ULL);
    TEST_ASSERT("M-mode clears SCEIC after broad convergence/reserved-bit probing so no later access can consume an injection request",
        excpt.triggered == false &&
        ai_arch_read_sceic() == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_sceic(0ULL);
    TEST_ASSERT("M-mode clears SCEIC after the CSRRW all-CSR probe before any later explicit access can consume INJ_EN",
        excpt.triggered == false &&
        ai_arch_read_sceic() == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    ai_arch_custom_set_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE);
    uint64_t spf_after_set = ai_arch_custom_read_csr(CSR_SPFCTL);
    ai_arch_custom_clear_csr(CSR_SPFCTL, AI_ARCH_SPFCTL_ENABLE_AGT);
    uint64_t spf_after_clear = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode CSRRS/CSRRC update only the requested SPFctl defined bits and do not expose WPRI bits",
        excpt.triggered == false &&
        (spf_after_set & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (spf_after_set & AI_ARCH_SPFCTL_L1D_PF_ENABLE) != 0ULL &&
        (spf_after_set & AI_ARCH_SPFCTL_ENABLE_AGT) != 0ULL &&
        (spf_after_set & AI_ARCH_SPFCTL_ENABLE_STRIDE) != 0ULL &&
        (spf_after_clear & AI_ARCH_SPFCTL_ENABLE_AGT) == 0ULL &&
        (spf_after_clear & AI_ARCH_SPFCTL_L1D_PF_ENABLE) != 0ULL &&
        (spf_after_clear & AI_ARCH_SPFCTL_ENABLE_STRIDE) != 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL, 0ULL);
    ai_arch_custom_set_csr(CSR_SMBLOCKCTL,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
        (0x3ULL << 1));
    uint64_t smblock_after_set = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    ai_arch_custom_clear_csr(CSR_SMBLOCKCTL,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE);
    uint64_t smblock_after_clear = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    TEST_ASSERT("M-mode CSRRS/CSRRC update only the requested SMBlockCtl replay_escape fields and do not expose WPRI bits",
        excpt.triggered == false &&
        (smblock_after_set & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL &&
        (smblock_after_set & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE) != 0ULL &&
        (smblock_after_set & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) ==
            (0x3ULL << 1) &&
        (smblock_after_clear & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE) == 0ULL &&
        (smblock_after_clear & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) ==
            (0x3ULL << 1)
    );

    const uint64_t smblock_ret_patterns[] = {
        0x0ULL << 1,
        0x1ULL << 1,
        0x3ULL << 1,
        0x7ULL << 1,
        0xcULL << 1,
        0xfULL << 1,
    };
    for (unsigned i = 0; i < sizeof(smblock_ret_patterns) /
        sizeof(smblock_ret_patterns[0]); i++) {
        TEST_SETUP_EXCEPT();
        uint64_t candidate = AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
            smblock_ret_patterns[i];
        ai_arch_custom_write_csr(CSR_SMBLOCKCTL, candidate);
        uint64_t observed_once = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
        ai_arch_custom_write_csr(CSR_SMBLOCKCTL, observed_once);
        uint64_t observed_twice = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
        TEST_ASSERT("M-mode SMBlockCtl RET zero/small/intermediate/default/max patterns preserve ERE, converge, and remain repeat-stable",
            excpt.triggered == false &&
            observed_once == observed_twice &&
            (observed_once & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE) ==
                AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE &&
            (observed_once & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) ==
                smblock_ret_patterns[i] &&
            (observed_once & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL
        );
    }

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL, 0ULL);
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0xbULL << 1));
    uint64_t smblock_after_atomic_combo =
        ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    TEST_ASSERT("M-mode SMBlockCtl same-instruction ERE plus RET programming exposes the complete new image without half-updated fields",
        excpt.triggered == false &&
        (smblock_after_atomic_combo & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0xbULL << 1)) &&
        (smblock_after_atomic_combo & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x1ULL << 1));
    uint64_t smblock_interleave_a = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL, 0x7ULL << 1);
    uint64_t smblock_interleave_b = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    ai_arch_custom_set_csr(CSR_SMBLOCKCTL,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE);
    uint64_t smblock_interleave_c = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL, (~0ULL & ~1ULL));
    uint64_t smblock_interleave_d = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    TEST_ASSERT("M-mode SMBlockCtl interleaved ERE and RET writes always read back a complete masked configuration image",
        excpt.triggered == false &&
        (smblock_interleave_a & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x1ULL << 1)) &&
        (smblock_interleave_b & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (0x7ULL << 1) &&
        (smblock_interleave_c & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x7ULL << 1)) &&
        (smblock_interleave_d & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK &&
        (smblock_interleave_a & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL &&
        (smblock_interleave_b & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL &&
        (smblock_interleave_c & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL &&
        (smblock_interleave_d & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL
    );

    const uint64_t spf_i_l2_d_single[] = {
        AI_ARCH_SPFCTL_L1I_PF_ENABLE,
        AI_ARCH_SPFCTL_L2_PF_ENABLE,
        AI_ARCH_SPFCTL_L1D_PF_ENABLE,
    };
    for (unsigned i = 0; i < sizeof(spf_i_l2_d_single) / sizeof(spf_i_l2_d_single[0]); i++) {
        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(CSR_SPFCTL, spf_i_l2_d_single[i]);
        uint64_t observed = ai_arch_custom_read_csr(CSR_SPFCTL);
        TEST_ASSERT("M-mode SPFctl IPE/L2PE/DPE single-on programming reads back exactly the requested enable bit with other enable bits clear",
            excpt.triggered == false &&
            (observed & AI_ARCH_SPFCTL_ENABLE_MASK) == spf_i_l2_d_single[i] &&
            (observed & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
        );
    }

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_L1I_PF_ENABLE |
        AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L1D_PF_ENABLE);
    uint64_t spf_all_basic_on = ai_arch_custom_read_csr(CSR_SPFCTL);
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    uint64_t spf_all_basic_off = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode SPFctl IPE/L2PE/DPE all-on/all-off programming is stable and does not expose WPRI bits",
        excpt.triggered == false &&
        (spf_all_basic_on & (AI_ARCH_SPFCTL_L1I_PF_ENABLE |
            AI_ARCH_SPFCTL_L2_PF_ENABLE |
            AI_ARCH_SPFCTL_L1D_PF_ENABLE)) ==
            (AI_ARCH_SPFCTL_L1I_PF_ENABLE |
                AI_ARCH_SPFCTL_L2_PF_ENABLE |
                AI_ARCH_SPFCTL_L1D_PF_ENABLE) &&
        (spf_all_basic_off & AI_ARCH_SPFCTL_ENABLE_MASK) == 0ULL &&
        (spf_all_basic_on & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (spf_all_basic_off & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
    );

    const uint64_t spf_toggle_bits[] = {
        AI_ARCH_SPFCTL_L1I_PF_ENABLE,
        AI_ARCH_SPFCTL_L2_PF_ENABLE,
        AI_ARCH_SPFCTL_L1D_PF_ENABLE,
        AI_ARCH_SPFCTL_TRAIN_ON_HIT,
        AI_ARCH_SPFCTL_ENABLE_AGT,
        AI_ARCH_SPFCTL_ENABLE_PHT,
        AI_ARCH_SPFCTL_ENABLE_STRIDE,
        AI_ARCH_SPFCTL_L2_STORE_ONLY,
    };
    for (unsigned i = 0; i < sizeof(spf_toggle_bits) / sizeof(spf_toggle_bits[0]); i++) {
        const uint64_t guard_fields =
            (0x5ULL << 6) |
            (0x15ULL << 10) |
            AI_ARCH_SPFCTL_L2_PF_ENABLE |
            AI_ARCH_SPFCTL_ENABLE_AGT;
        const uint64_t masked_guard = guard_fields & ~spf_toggle_bits[i];
        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(CSR_SPFCTL, masked_guard);
        uint64_t toggle_before = ai_arch_custom_read_csr(CSR_SPFCTL);
        ai_arch_custom_set_csr(CSR_SPFCTL, spf_toggle_bits[i]);
        uint64_t toggle_set = ai_arch_custom_read_csr(CSR_SPFCTL);
        ai_arch_custom_clear_csr(CSR_SPFCTL, spf_toggle_bits[i]);
        uint64_t toggle_clear = ai_arch_custom_read_csr(CSR_SPFCTL);
        TEST_ASSERT("M-mode SPFctl individual enable/control bits toggle 0->1->0 while every non-target field remains isolated",
            excpt.triggered == false &&
            (toggle_before & spf_toggle_bits[i]) == 0ULL &&
            (toggle_set & spf_toggle_bits[i]) == spf_toggle_bits[i] &&
            (toggle_clear & spf_toggle_bits[i]) == 0ULL &&
            ((toggle_set ^ (toggle_before | spf_toggle_bits[i])) &
                ~spf_toggle_bits[i]) == 0ULL &&
            ((toggle_clear ^ toggle_before) & ~spf_toggle_bits[i]) == 0ULL &&
            (toggle_set & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
            (toggle_clear & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
        );
    }

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L2_STORE_ONLY);
    uint64_t spf_l2_store_only_set = ai_arch_custom_read_csr(CSR_SPFCTL);
    ai_arch_custom_clear_csr(CSR_SPFCTL, AI_ARCH_SPFCTL_L2_STORE_ONLY);
    uint64_t spf_l2_store_only_clear = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode SPFctl L2 store-only bit toggles independently while preserving L2PE",
        excpt.triggered == false &&
        (spf_l2_store_only_set & AI_ARCH_SPFCTL_L2_PF_ENABLE) != 0ULL &&
        (spf_l2_store_only_set & AI_ARCH_SPFCTL_L2_STORE_ONLY) != 0ULL &&
        (spf_l2_store_only_clear & AI_ARCH_SPFCTL_L2_PF_ENABLE) != 0ULL &&
        (spf_l2_store_only_clear & AI_ARCH_SPFCTL_L2_STORE_ONLY) == 0ULL &&
        (spf_l2_store_only_clear & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE |
        (0x1ULL << 6) |
        (0x09ULL << 10));
    uint64_t spf_train_combo_a = ai_arch_custom_read_csr(CSR_SPFCTL);
    ai_arch_custom_write_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        (0x6ULL << 6) |
        (0x12ULL << 10));
    uint64_t spf_train_combo_b = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode SPFctl AGT/PHT/train_on_hit/stride plus threshold/stride cross programming is isolated and repeat-stable",
        excpt.triggered == false &&
        (spf_train_combo_a & (AI_ARCH_SPFCTL_TRAIN_ON_HIT |
            AI_ARCH_SPFCTL_ENABLE_AGT |
            AI_ARCH_SPFCTL_ENABLE_PHT |
            AI_ARCH_SPFCTL_ENABLE_STRIDE |
            AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK |
            AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) ==
            (AI_ARCH_SPFCTL_TRAIN_ON_HIT |
                AI_ARCH_SPFCTL_ENABLE_AGT |
                AI_ARCH_SPFCTL_ENABLE_PHT |
                AI_ARCH_SPFCTL_ENABLE_STRIDE |
                (0x1ULL << 6) |
                (0x09ULL << 10)) &&
        (spf_train_combo_b & AI_ARCH_SPFCTL_ENABLE_AGT) == 0ULL &&
        (spf_train_combo_b & AI_ARCH_SPFCTL_ENABLE_PHT) != 0ULL &&
        (spf_train_combo_b & AI_ARCH_SPFCTL_ENABLE_STRIDE) == 0ULL &&
        (spf_train_combo_b & AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK) ==
            (0x6ULL << 6) &&
        (spf_train_combo_b & AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK) ==
            (0x12ULL << 10) &&
        (spf_train_combo_b & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
    );

    const uint64_t spf_threshold_stride_patterns[] = {
        0x0ULL,
        (0xfULL << 6) | (0x3fULL << 10),
        (0x3ULL << 6) | (0x08ULL << 10),
        (0xcULL << 6) | (0x1eULL << 10),
    };
    for (unsigned i = 0; i < sizeof(spf_threshold_stride_patterns) /
        sizeof(spf_threshold_stride_patterns[0]); i++) {
        const uint64_t seed = AI_ARCH_SPFCTL_L1I_PF_ENABLE |
            AI_ARCH_SPFCTL_L2_PF_ENABLE |
            AI_ARCH_SPFCTL_L1D_PF_ENABLE |
            AI_ARCH_SPFCTL_ENABLE_AGT |
            AI_ARCH_SPFCTL_ENABLE_PHT |
            AI_ARCH_SPFCTL_ENABLE_STRIDE;
        const uint64_t candidate = seed | spf_threshold_stride_patterns[i];
        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(CSR_SPFCTL, candidate);
        uint64_t observed_once = ai_arch_custom_read_csr(CSR_SPFCTL);
        ai_arch_custom_write_csr(CSR_SPFCTL, observed_once);
        uint64_t observed_twice = ai_arch_custom_read_csr(CSR_SPFCTL);
        TEST_ASSERT("M-mode SPFctl APT/APS min/max/intermediate/default patterns converge and remain repeat-stable without disturbing enable bits",
            excpt.triggered == false &&
            observed_once == observed_twice &&
            (observed_once & (AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK |
                AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) ==
                spf_threshold_stride_patterns[i] &&
            (observed_once & AI_ARCH_SPFCTL_ENABLE_MASK) ==
                (seed & AI_ARCH_SPFCTL_ENABLE_MASK) &&
            (observed_once & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
        );
    }

    TEST_SETUP_EXCEPT();
    const uint64_t spf_atomic_combo = AI_ARCH_SPFCTL_L1I_PF_ENABLE |
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_L2_STORE_ONLY |
        (0xbULL << 6) |
        (0x2aULL << 10);
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    ai_arch_custom_write_csr(CSR_SPFCTL, spf_atomic_combo);
    uint64_t spf_after_atomic_combo = ai_arch_custom_read_csr(CSR_SPFCTL);
    TEST_ASSERT("M-mode SPFctl same-instruction enable/threshold/stride programming exposes the complete new image without half-new half-old fields",
        excpt.triggered == false &&
        (spf_after_atomic_combo & AI_ARCH_SPFCTL_DEFINED_MASK) ==
            spf_atomic_combo &&
        (spf_after_atomic_combo & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL, 0ULL);
    ai_arch_custom_write_csr(CSR_SPFCTL,
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        (0x2ULL << 6));
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL,
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
        (0x4ULL << 1));
    uint64_t spf_after_cross_csr = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t smblock_after_cross_csr = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    ai_arch_custom_write_csr(CSR_SPFCTL, 0ULL);
    uint64_t smblock_after_spf_clear = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    TEST_ASSERT("M-mode cross-programming SPFctl and SMBlockCtl keeps each custom CSR field bank isolated",
        excpt.triggered == false &&
        (spf_after_cross_csr & (AI_ARCH_SPFCTL_L1D_PF_ENABLE |
            AI_ARCH_SPFCTL_TRAIN_ON_HIT |
            AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK)) ==
            (AI_ARCH_SPFCTL_L1D_PF_ENABLE |
                AI_ARCH_SPFCTL_TRAIN_ON_HIT |
                (0x2ULL << 6)) &&
        (smblock_after_cross_csr & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
                (0x4ULL << 1)) &&
        (smblock_after_spf_clear & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (smblock_after_cross_csr & AI_ARCH_SMBLOCKCTL_DEFINED_MASK)
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SCEC, 0ULL);
    ai_arch_custom_set_csr(CSR_SCEC, AI_ARCH_CUSTOM_CSR_SCEC_SEED);
    uint64_t scec_after_set = ai_arch_custom_read_csr(CSR_SCEC);
    ai_arch_custom_clear_csr(CSR_SCEC, 1ULL << 63);
    uint64_t scec_after_clear = ai_arch_custom_read_csr(CSR_SCEC);
    TEST_ASSERT("M-mode CSRRS/CSRRC update the SCEC EV bit without corrupting ES/EPA fields",
        excpt.triggered == false &&
        scec_after_set == AI_ARCH_CUSTOM_CSR_SCEC_SEED &&
        scec_after_clear == (AI_ARCH_CUSTOM_CSR_SCEC_SEED & ~(1ULL << 63))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SCEIC, 0ULL);
    ai_arch_custom_set_csr(CSR_SCEIC,
        AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE | (1ULL << 1));
    uint64_t sceic_after_set = ai_arch_custom_read_csr(CSR_SCEIC);
    ai_arch_custom_clear_csr(CSR_SCEIC, 1ULL << 1);
    uint64_t sceic_after_clear = ai_arch_custom_read_csr(CSR_SCEIC);
    TEST_ASSERT("M-mode CSRRS/CSRRC update the SCEIC source/bits fields without setting INJ_EN or exposing WPRI bits",
        excpt.triggered == false &&
        (sceic_after_set & ~AI_ARCH_SCEIC_DEFINED_MASK) == 0ULL &&
        (sceic_after_set & AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE) ==
            AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE &&
        (sceic_after_set & (1ULL << 1)) != 0ULL &&
        (sceic_after_set & 1ULL) == 0ULL &&
        (sceic_after_clear & AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE) ==
            AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE &&
        (sceic_after_clear & ((1ULL << 1) | 1ULL)) == 0ULL
    );

    TEST_ASSERT("M-mode SCEIC WPRI high bits are masked while all defined low fields remain writable with the non-consuming source encoding",
        ai_arch_custom_probe_write_read(
            CSR_SCEIC,
            ~0ULL,
            AI_ARCH_SCEIC_DEFINED_MASK,
            &m_observed[3]
        ) &&
        (m_observed[3] & ~AI_ARCH_SCEIC_DEFINED_MASK) == 0ULL &&
        (m_observed[3] & AI_ARCH_SCEIC_DEFINED_MASK) ==
            AI_ARCH_SCEIC_DEFINED_MASK
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_sceic(0ULL);
    TEST_ASSERT("M-mode clears SCEIC immediately after the safe WPRI/INJ_EN probe so no later access can consume an injection request",
        excpt.triggered == false &&
        ai_arch_read_sceic() == 0ULL
    );

    goto_priv(PRIV_HS);
    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_ASSERT("HS-mode legal custom CSR read/write converges without trapping under mstateen0.C=1",
            ai_arch_custom_probe_write_read(
                custom_csrs[i],
                legal_values[i],
                defined_masks[i],
                &hs_observed[i]
            )
        );
    }

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_MSTATUS, MSTATUS_TVM);
    TEST_ASSERT("setting mstatus.TVM in M-mode does not trap before S-mode custom CSR access",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_ASSERT("HS-mode SCEC/SCEIC custom CSR access is not blocked by mstatus.TVM",
        ai_arch_custom_probe_write_read(
            CSR_SCEC,
            AI_ARCH_CUSTOM_CSR_SCEC_SEED,
            AI_ARCH_SCEC_DEFINED_MASK,
            &hs_observed[2]
        ) &&
        ai_arch_custom_probe_write_read(
            CSR_SCEIC,
            AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE,
            AI_ARCH_SCEIC_DEFINED_MASK,
            &hs_observed[3]
        )
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("M-mode SCEC custom CSR access is not blocked by mstatus.TVM while the TVM gate is set",
        ai_arch_custom_probe_write_read(
            CSR_SCEC,
            AI_ARCH_CUSTOM_CSR_SCEC_SEED,
            AI_ARCH_SCEC_DEFINED_MASK,
            &m_observed[2]
        )
    );

    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATEEN0, MSTATEEN_C);
    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_SETUP_EXCEPT();
        ai_arch_custom_write_csr(custom_csrs[i], legal_values[i]);
        TEST_ASSERT("M-mode seeds a known custom CSR value before the HU illegal-access probe",
            excpt.triggered == false
        );
    }

    const uintptr_t no_r_vaddr = hs_page_base(VSX_GURWX);
    const uintptr_t no_r_paddr = phys_page_base(VSX_GURWX);
    const uint64_t no_r_seed = 0x6e6f726561646c64ULL;
    const uint64_t no_r_sentinel = 0x5152535455565758ULL;
    write64(no_r_paddr, no_r_seed);
    hspt_init();
    sfence_vma();

    CSRC(CSR_MSTATEEN0, MSTATEEN_C);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr(CSR_SPFCTL);
    TEST_ASSERT("HS-mode reading SPFctl with mstateen0.C=0 traps as a low-privilege custom CSR access and reports the CSRRS instruction in tval",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SPFCTL)) &&
        curr_priv == PRIV_HS
    );

    TEST_SETUP_EXCEPT();
    uint64_t no_r_after_spf_illegal = ai_arch_custom_ld_rd_probe(
        no_r_vaddr, no_r_sentinel);
    TEST_ASSERT("immediately after the illegal HS SPFctl read, a real no-R load is freshly rejudged as Load PF with its own VA and suppressed rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == no_r_vaddr &&
        no_r_after_spf_illegal == no_r_sentinel
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr(CSR_SMBLOCKCTL);
    TEST_ASSERT("HS-mode reading SMBlockCtl with mstateen0.C=0 traps before the custom CSR value can be observed",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SMBLOCKCTL)) &&
        curr_priv == PRIV_HS
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw(CSR_SPFCTL, 0ULL);
    TEST_ASSERT("HS-mode writing SPFctl with mstateen0.C=0 traps and cannot update the previously seeded SPFctl value",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SPFCTL))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw(CSR_SCEIC, AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE);
    TEST_ASSERT("HS-mode writing SCEIC with mstateen0.C=0 traps before partially updating MemBlock custom CSR state",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SCEIC))
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0, MSTATEEN_C);
    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr_unused();
    TEST_ASSERT("M-mode reading an unimplemented MemBlock custom CSR address traps with IllegalInstruction and reports the CSRRS instruction in tval",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_MEMBLOCK_CUSTOM_UNUSED))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw_unused(0x1fULL);
    TEST_ASSERT("M-mode writing an unimplemented MemBlock custom CSR address traps before producing any project CSR state side effect",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_MEMBLOCK_CUSTOM_UNUSED))
    );

    TEST_SETUP_EXCEPT();
    uint64_t fault_rd_after = ai_arch_custom_unused_csrrw_rd_probe(0x1fULL);
    TEST_ASSERT("faulting custom CSR instruction does not commit the destination GPR writeback",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T2_T1_INST(CSR_MEMBLOCK_CUSTOM_UNUSED)) &&
        fault_rd_after == 0x5a5a1234deadbeefULL
    );

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_SETUP_EXCEPT();
        uint64_t after_blocked = ai_arch_custom_read_csr(custom_csrs[i]);
        TEST_ASSERT("low-privilege or unimplemented custom CSR traps preserve the seeded MemBlock custom CSR state before the HU probes",
            excpt.triggered == false &&
            (after_blocked & defined_masks[i]) ==
                (legal_values[i] & defined_masks[i])
        );
    }

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr(CSR_SPFCTL);
    TEST_ASSERT("HU-mode reading SPFctl traps with IllegalInstruction, tval is the CSRRS encoding, and privilege remains HU",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SPFCTL)) &&
        curr_priv == PRIV_HU
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr(CSR_SMBLOCKCTL);
    TEST_ASSERT("HU-mode reading SMBlockCtl traps with IllegalInstruction and reports the CSRRS encoding",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SMBLOCKCTL))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw(CSR_SMBLOCKCTL, 0ULL);
    TEST_ASSERT("HU-mode writing SMBlockCtl traps with IllegalInstruction and tval is the CSRRW encoding",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SMBLOCKCTL))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr(CSR_SCEC);
    TEST_ASSERT("HU-mode reading SCEC traps with IllegalInstruction and tval is the CSRRS encoding",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SCEC))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw(CSR_SCEC, 0ULL);
    TEST_ASSERT("HU-mode writing SCEC to clear EV traps and cannot partially update the cache-error record",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SCEC))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr(CSR_SCEIC);
    TEST_ASSERT("HU-mode reading SCEIC traps with IllegalInstruction and tval is the CSRRS encoding",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SCEIC))
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw(CSR_SCEIC, 1ULL);
    TEST_ASSERT("HU-mode writing SCEIC.INJ_EN traps and cannot enable low-privilege cache-error injection",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SCEIC))
    );

    goto_priv_u_to_m_direct(PRIV_M);
    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); i++) {
        TEST_SETUP_EXCEPT();
        uint64_t after_illegal = ai_arch_custom_read_csr(custom_csrs[i]);
        TEST_ASSERT("HU-mode illegal custom CSR accesses preserve the previously seeded CSR state",
            excpt.triggered == false &&
            (after_illegal & defined_masks[i]) ==
                (legal_values[i] & defined_masks[i])
        );
    }

    TEST_ASSERT("after illegal custom CSR traps, a later legal M-mode read/write recovers without inheriting stale trap metadata",
        ai_arch_custom_probe_write_read(
            CSR_SPFCTL,
            AI_ARCH_SPFCTL_L1D_PF_ENABLE |
                AI_ARCH_SPFCTL_ENABLE_STRIDE,
            AI_ARCH_SPFCTL_DEFINED_MASK,
            &m_observed[0]
        ) &&
        ai_arch_custom_probe_write_read(
            CSR_SCEIC,
            AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE,
            AI_ARCH_SCEIC_DEFINED_MASK,
            &m_observed[3]
        )
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_sceic(0ULL);
    TEST_ASSERT("M-mode clears SCEIC after the legal recovery probe so the case leaves cache-error injection disabled",
        excpt.triggered == false &&
        ai_arch_read_sceic() == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_write_csr(CSR_SPFCTL, original_values[0]);
    ai_arch_custom_write_csr(CSR_SMBLOCKCTL, original_values[1]);
    ai_arch_custom_write_csr(CSR_SCEC, original_values[2]);
    ai_arch_custom_write_csr(CSR_SCEIC, original_values[3]);
    uint64_t restored_spfctl = ai_arch_custom_read_csr(CSR_SPFCTL);
    uint64_t restored_smblockctl = ai_arch_custom_read_csr(CSR_SMBLOCKCTL);
    uint64_t restored_scec = ai_arch_custom_read_csr(CSR_SCEC);
    uint64_t restored_sceic = ai_arch_custom_read_csr(CSR_SCEIC);
    CSRW(CSR_SSTATEEN0, original_sstateen0);
    CSRW(CSR_MSTATEEN0, original_mstateen0);
    CSRW(CSR_MSTATUS, original_mstatus);
    TEST_ASSERT("restoring MemBlock custom CSRs and state-enable controls after the privilege matrix returns every defined field to the saved image",
        excpt.triggered == false &&
        (restored_spfctl & AI_ARCH_SPFCTL_DEFINED_MASK) ==
            (original_values[0] & AI_ARCH_SPFCTL_DEFINED_MASK) &&
        (restored_smblockctl & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (original_values[1] & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) &&
        restored_scec == original_values[2] &&
        (restored_sceic & AI_ARCH_SCEIC_DEFINED_MASK) ==
            (original_values[3] & AI_ARCH_SCEIC_DEFINED_MASK)
    );

    TEST_END();
}

bool ai_arch_memblock_custom_csr_trap_handler_snapshot_restore_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t old_mtvec = CSRR(CSR_MTVEC);
    const uint64_t old_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t old_mie = CSRR(CSR_MIE);
    const uint64_t old_mip = CSRR(CSR_MIP);
    const uint64_t old_spfctl = ai_arch_read_spfctl();
    const uint64_t old_smblockctl = ai_arch_read_smblockctl();

    const uint64_t spf_seed =
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE |
        (0x5ULL << 6);
    const uint64_t smblock_seed =
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE |
        (0x4ULL << 1);

    ai_arch_custom_handler_reset_state();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIP, 0ULL);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(spf_seed);
    ai_arch_write_smblockctl(smblock_seed);
    CSRW(CSR_MTVEC, (uintptr_t)ai_arch_custom_csr_snapshot_mtvec_entry);
    TEST_ASSERT("M-mode seeds custom CSR state and redirects mtvec before the handler snapshot probe",
        excpt.triggered == false
    );

    asm volatile(
        ".global ai_arch_custom_csr_snapshot_ecall_site\n"
        "ai_arch_custom_csr_snapshot_ecall_site:\n"
        "ecall\n"
        "la t0, ai_arch_custom_handler_fell_through\n"
        "li t1, 0x1111\n"
        "sd t1, 0(t0)\n"
        ".global ai_arch_custom_csr_snapshot_after_trap\n"
        "ai_arch_custom_csr_snapshot_after_trap:\n"
        ::: "t0", "t1", "memory");

    TEST_ASSERT("custom trap handler first snapshots MemBlock custom CSR state, clears SPFctl, and updates only SMBlockCtl replay fields",
        ai_arch_custom_handler_hits == 1 &&
        ai_arch_custom_handler_cause == CAUSE_MACHINE_ECALL &&
        ai_arch_custom_handler_epc ==
            (uintptr_t)ai_arch_custom_csr_snapshot_ecall_site &&
        ai_arch_custom_handler_fell_through == 0 &&
        (ai_arch_custom_handler_spf_snapshot & AI_ARCH_SPFCTL_DEFINED_MASK) ==
            spf_seed &&
        (ai_arch_custom_handler_spf_after_clear & AI_ARCH_SPFCTL_DEFINED_MASK) ==
            0ULL &&
        (ai_arch_custom_handler_smblock_snapshot &
            AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == smblock_seed &&
        (ai_arch_custom_handler_smblock_after_write &
            AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x3ULL << 1))
    );

    TEST_SETUP_EXCEPT();
    uint64_t spf_after_handler = ai_arch_read_spfctl();
    uint64_t smblock_after_handler = ai_arch_read_smblockctl();
    ai_arch_write_spfctl(AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L2_STORE_ONLY);
    ai_arch_write_smblockctl(smblock_seed);
    uint64_t spf_after_explicit = ai_arch_read_spfctl();
    uint64_t smblock_after_explicit = ai_arch_read_smblockctl();
    TEST_ASSERT("after returning from the handler, explicit custom CSR accesses use the current architectural CSR image and do not inherit the handler's temporary state",
        excpt.triggered == false &&
        (spf_after_handler & AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (smblock_after_handler & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            (AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x3ULL << 1)) &&
        (spf_after_explicit & AI_ARCH_SPFCTL_DEFINED_MASK) ==
            (AI_ARCH_SPFCTL_L2_PF_ENABLE |
                AI_ARCH_SPFCTL_L2_STORE_ONLY) &&
        (smblock_after_explicit & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) ==
            smblock_seed
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(old_spfctl);
    ai_arch_write_smblockctl(old_smblockctl);
    CSRW(CSR_MTVEC, old_mtvec);
    CSRW(CSR_MIE, old_mie);
    CSRW(CSR_MIP, old_mip);
    CSRW(CSR_MSTATUS, old_mstatus);
    TEST_ASSERT("restoring trap-vector and custom CSR state after the handler snapshot probe does not trap",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_spfctl_smblockctl_reset_default_readback_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t spf_reset = ai_arch_read_spfctl();
    uint64_t smblock_reset = ai_arch_read_smblockctl();
    TEST_ASSERT("M-mode sees the NHV5.1AP SPFctl/SMBlockCtl reset defaults before any software rewrite",
        excpt.triggered == false &&
        (spf_reset & AI_ARCH_SPFCTL_DEFINED_MASK) == AI_ARCH_SPFCTL_RESET_VALUE &&
        (spf_reset & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (smblock_reset & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == AI_ARCH_SMBLOCKCTL_RESET_VALUE &&
        (smblock_reset & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(0ULL);
    ai_arch_write_smblockctl(0ULL);
    uint64_t spf_zero = ai_arch_read_spfctl();
    uint64_t smblock_zero = ai_arch_read_smblockctl();
    TEST_ASSERT("SPFctl and SMBlockCtl can be software-cleared without exposing WPRI high bits",
        excpt.triggered == false &&
        (spf_zero & AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (spf_zero & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (smblock_zero & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL &&
        (smblock_zero & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(AI_ARCH_SPFCTL_RESET_VALUE);
    ai_arch_write_smblockctl(AI_ARCH_SMBLOCKCTL_RESET_VALUE);
    uint64_t spf_restored = ai_arch_read_spfctl();
    uint64_t smblock_restored = ai_arch_read_smblockctl();
    TEST_ASSERT("rewriting the NHV5.1AP default field image restores a stable SPFctl/SMBlockCtl readback",
        excpt.triggered == false &&
        (spf_restored & AI_ARCH_SPFCTL_DEFINED_MASK) == AI_ARCH_SPFCTL_RESET_VALUE &&
        (spf_restored & ~AI_ARCH_SPFCTL_DEFINED_MASK) == 0ULL &&
        (smblock_restored & AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == AI_ARCH_SMBLOCKCTL_RESET_VALUE &&
        (smblock_restored & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) == 0ULL
    );

    TEST_END();
}

bool ai_arch_scec_sceic_baseline_field_privilege_matrix_corner()
{
    TEST_START();

    static const uint64_t scec_es_patterns[] = {
        0ULL, 1ULL, 2ULL, 3ULL, 4ULL, 5ULL, 6ULL, 7ULL,
    };
    static const uint64_t scec_epa_patterns[] = {
        0x0000000000000000ULL,
        0x0000000000001000ULL,
        0x0000abcdeffed000ULL,
        0x0ffffffffffff000ULL,
    };
    static const uint64_t sceic_defined_patterns[] = {
        0ULL,
        (1ULL << 2),
        (3ULL << 2) | (1ULL << 1),
        (6ULL << 2),
        ((7ULL << 2) | (1ULL << 1)),
        AI_ARCH_SCEIC_DEFINED_MASK,
    };
    const uintptr_t normal_vaddr =
        ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    const uintptr_t normal_paddr =
        ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    const uint64_t normal_seed = 0x5cec5cec12345678ULL;
    const uint64_t normal_store = 0x5ce1c0ffee55aa33ULL;
    const uint64_t sceic_non_dcache_source = (5ULL << 2) | 1ULL;

    goto_priv(PRIV_M);

    const uint64_t original_scec = CSRR(CSR_SCEC);
    const uint64_t original_sceic = CSRR(CSR_SCEIC);
    const uint64_t original_mstateen0 = CSRR(CSR_MSTATEEN0);
    const uint64_t original_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t original_scec_ev = (original_scec >> 63) & 0x1ULL;
    const uint64_t original_scec_es = (original_scec >> 60) & 0x7ULL;
    const uint64_t original_scec_epa =
        original_scec & ((1ULL << 60) - 1ULL);

    TEST_ASSERT("M-mode sees the SCEC/SCEIC reset baseline before any software rewrite",
        excpt.triggered == false &&
        original_scec_ev == 0ULL &&
        original_scec_es == 0x7ULL &&
        original_scec_epa == 0ULL &&
        (original_sceic & AI_ARCH_SCEIC_DEFINED_MASK) ==
            AI_ARCH_CUSTOM_CSR_SCEIC_RESET_VALUE
    );

    CSRS(CSR_MSTATEEN0, MSTATEEN_C);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(0ULL);
    ai_arch_write_sceic(0ULL);
    uint64_t m_scec_clear = ai_arch_read_scec();
    uint64_t m_sceic_clear = ai_arch_read_sceic();
    TEST_ASSERT("M-mode can establish a software-cleared SCEC/SCEIC baseline with no cache-error injection armed",
        excpt.triggered == false &&
        m_scec_clear == 0ULL &&
        m_sceic_clear == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(AI_ARCH_CUSTOM_CSR_SCEC_SEED);
    uint64_t m_scec_seeded = ai_arch_read_scec();
    ai_arch_write_scec(0ULL);
    uint64_t m_scec_recleared = ai_arch_read_scec();
    TEST_ASSERT("M-mode SCEC EV, ES, and EPA fields are software visible and can be returned to the cleared baseline",
        excpt.triggered == false &&
        m_scec_seeded == AI_ARCH_CUSTOM_CSR_SCEC_SEED &&
        m_scec_recleared == 0ULL
    );

    bool scec_source_matrix_ok = true;
    for (unsigned es_i = 0; es_i < sizeof(scec_es_patterns) /
        sizeof(scec_es_patterns[0]); es_i++) {
        for (unsigned epa_i = 0; epa_i < sizeof(scec_epa_patterns) /
            sizeof(scec_epa_patterns[0]); epa_i++) {
            uint64_t source = scec_es_patterns[es_i];
            uint64_t epa = scec_epa_patterns[epa_i];
            uint64_t ev0_value = (source << 60) | epa;
            uint64_t ev1_value = (1ULL << 63) | ev0_value;

            TEST_SETUP_EXCEPT();
            ai_arch_write_scec(ev0_value);
            uint64_t ev0_read = ai_arch_read_scec();
            ai_arch_write_scec(ev1_value);
            uint64_t ev1_read_first = ai_arch_read_scec();
            uint64_t ev1_read_second = ai_arch_read_scec();
            scec_source_matrix_ok = scec_source_matrix_ok &&
                excpt.triggered == false &&
                ev0_read == ev0_value &&
                ev1_read_first == ev1_value &&
                ev1_read_second == ev1_value;
        }
    }
    TEST_ASSERT("M-mode SCEC exposes every ES source encoding with EV=0/1 and EPA patterns, and repeated reads do not clear the record",
        scec_source_matrix_ok
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(AI_ARCH_CUSTOM_CSR_SCEC_SEED);
    uint64_t seeded_before_clear = ai_arch_read_scec();
    ai_arch_custom_clear_csr(CSR_SCEC, 1ULL << 63);
    uint64_t after_ev_clear_once = ai_arch_read_scec();
    ai_arch_custom_clear_csr(CSR_SCEC, 1ULL << 63);
    uint64_t after_ev_clear_twice = ai_arch_read_scec();
    TEST_ASSERT("M-mode CSRRC clears SCEC.EV while preserving ES/EPA, and repeating the clear is idempotent",
        excpt.triggered == false &&
        seeded_before_clear == AI_ARCH_CUSTOM_CSR_SCEC_SEED &&
        after_ev_clear_once ==
            (AI_ARCH_CUSTOM_CSR_SCEC_SEED & ~(1ULL << 63)) &&
        after_ev_clear_twice == after_ev_clear_once
    );

    const uint64_t sceic_wpri_probe =
        (~0ULL & ~1ULL) | AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE;
    TEST_SETUP_EXCEPT();
    ai_arch_write_sceic(sceic_wpri_probe);
    uint64_t m_sceic_probe = ai_arch_read_sceic();
    ai_arch_write_sceic(0ULL);
    uint64_t m_sceic_recleared = ai_arch_read_sceic();
    TEST_ASSERT("M-mode SCEIC source and bit controls read back through the defined low fields while reserved high bits stay hidden",
        excpt.triggered == false &&
        (m_sceic_probe & ~AI_ARCH_SCEIC_DEFINED_MASK) == 0ULL &&
        (m_sceic_probe & 1ULL) == 0ULL &&
        (m_sceic_probe & AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE) ==
            AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE &&
        (m_sceic_probe & (1ULL << 1)) != 0ULL &&
        m_sceic_recleared == 0ULL
    );

    bool sceic_defined_matrix_ok = true;
    for (unsigned pat_i = 0; pat_i < sizeof(sceic_defined_patterns) /
        sizeof(sceic_defined_patterns[0]); pat_i++) {
        uint64_t defined_value = sceic_defined_patterns[pat_i] &
            AI_ARCH_SCEIC_DEFINED_MASK;

        TEST_SETUP_EXCEPT();
        ai_arch_write_sceic(~0ULL);
        ai_arch_write_sceic(defined_value);
        uint64_t defined_read = ai_arch_read_sceic();
        sceic_defined_matrix_ok = sceic_defined_matrix_ok &&
            excpt.triggered == false &&
            (defined_read & ~AI_ARCH_SCEIC_DEFINED_MASK) == 0ULL &&
            (defined_read & AI_ARCH_SCEIC_DEFINED_MASK) == defined_value;
    }
    TEST_ASSERT("M-mode SCEIC INJ_EN/INJ_BITS/INJ_SOURCE combinations are atomically visible while reserved high bits remain zero",
        sceic_defined_matrix_ok
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_sceic(0ULL);
    ai_arch_write_scec(0ULL);
    write64(normal_paddr, normal_seed);
    hspt_init();
    sfence_vma();
    goto_priv(PRIV_HS);
    uint64_t normal_load = read64(normal_vaddr);
    write64(normal_vaddr, normal_store);
    uint64_t normal_reload = read64(normal_vaddr);
    prefetch_r(normal_vaddr);
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t no_inject_scec_after = ai_arch_read_scec();
    uint64_t no_inject_sceic_after = ai_arch_read_sceic();
    TEST_ASSERT("with SCEIC.INJ_EN clear, ordinary HS load/store/prefetch keep SCEC at the no-error image and leave no pending injection state",
        excpt.triggered == false &&
        normal_load == normal_seed &&
        normal_reload == normal_store &&
        read64(normal_paddr) == normal_store &&
        no_inject_scec_after == 0ULL &&
        no_inject_sceic_after == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(0ULL);
    ai_arch_write_sceic(sceic_non_dcache_source);
    goto_priv(PRIV_HS);
    uint64_t mismatch_load = read64(normal_vaddr);
    write64(normal_vaddr, normal_seed);
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t mismatch_scec_after = ai_arch_read_scec();
    uint64_t mismatch_sceic_after = ai_arch_read_sceic();
    ai_arch_write_sceic(0ULL);
    TEST_ASSERT("a pending SCEIC source that does not match the ordinary DCache data access is not consumed and does not create a false SCEC record",
        excpt.triggered == false &&
        mismatch_load == normal_store &&
        read64(normal_paddr) == normal_seed &&
        mismatch_scec_after == 0ULL &&
        mismatch_sceic_after == sceic_non_dcache_source &&
        ai_arch_read_sceic() == 0ULL
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(AI_ARCH_CUSTOM_CSR_SCEC_SEED);
    uint64_t hs_scec = ai_arch_read_scec();
    ai_arch_write_sceic(AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE | (1ULL << 1));
    uint64_t hs_sceic = ai_arch_read_sceic();
    TEST_ASSERT("HS-mode can access SCEC/SCEIC when mstateen0.C enables MemBlock custom CSR access",
        excpt.triggered == false &&
        hs_scec == AI_ARCH_CUSTOM_CSR_SCEC_SEED &&
        (hs_sceic & ~AI_ARCH_SCEIC_DEFINED_MASK) == 0ULL &&
        (hs_sceic & AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE) ==
            AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE &&
        (hs_sceic & (1ULL << 1)) != 0ULL &&
        (hs_sceic & 1ULL) == 0ULL
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_TVM);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(AI_ARCH_CUSTOM_CSR_SCEC_SEED ^ 0x55ULL);
    uint64_t hs_tvm_scec = ai_arch_read_scec();
    ai_arch_write_sceic(AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE);
    uint64_t hs_tvm_sceic = ai_arch_read_sceic();
    TEST_ASSERT("mstatus.TVM does not block HS-mode access to the SCEC/SCEIC custom CSRs",
        excpt.triggered == false &&
        hs_tvm_scec == (AI_ARCH_CUSTOM_CSR_SCEC_SEED ^ 0x55ULL) &&
        (hs_tvm_sceic & ~AI_ARCH_SCEIC_DEFINED_MASK) == 0ULL &&
        hs_tvm_sceic == AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    ai_arch_write_scec(AI_ARCH_CUSTOM_CSR_SCEC_SEED);
    ai_arch_write_sceic(AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE);
    CSRC(CSR_SSTATEEN0, MSTATEEN_C);
    CSRW(CSR_SATP, 0ULL);
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr_scec();
    TEST_ASSERT("HU-mode reading SCEC traps with IllegalInstruction and reports the trapped CSRRS instruction",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SCEC)) &&
        curr_priv == PRIV_HU
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw_scec(0ULL);
    TEST_ASSERT("HU-mode writing SCEC cannot clear the seeded cache-error record",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SCEC)) &&
        curr_priv == PRIV_HU
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrr_sceic();
    TEST_ASSERT("HU-mode reading SCEIC traps with IllegalInstruction and reports the trapped CSRRS instruction",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRS_T0_X0_INST(CSR_SCEIC)) &&
        curr_priv == PRIV_HU
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fixed_csrw_sceic(1ULL);
    TEST_ASSERT("HU-mode writing SCEIC.INJ_EN cannot arm low-privilege cache-error injection",
        ai_arch_custom_illegal_with_inst(
            AI_ARCH_CUSTOM_CSR_CSRRW_T0_T1_INST(CSR_SCEIC)) &&
        curr_priv == PRIV_HU
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t post_hu_scec = ai_arch_read_scec();
    uint64_t post_hu_sceic = ai_arch_read_sceic();
    TEST_ASSERT("HU-mode illegal SCEC/SCEIC accesses leave the seeded CSR state unchanged",
        excpt.triggered == false &&
        post_hu_scec == AI_ARCH_CUSTOM_CSR_SCEC_SEED &&
        post_hu_sceic == AI_ARCH_CUSTOM_CSR_SCEIC_SAFE_SOURCE
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_scec(original_scec);
    ai_arch_write_sceic(original_sceic);
    CSRW(CSR_MSTATEEN0, original_mstateen0);
    CSRW(CSR_MSTATUS, original_mstatus);
    TEST_ASSERT("restoring SCEC/SCEIC and privilege-control CSRs after the baseline field matrix does not trap",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_spfctl_disable_prefetch_paths()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable in M-mode before disabling the prefetch-path control bits",
        true
    );

    uint64_t disabled = original & ~AI_ARCH_SPFCTL_ENABLE_MASK;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("writing spfctl with the architecturally visible prefetch-path control bits cleared does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the disabled prefetch-path control bits",
        observed_disabled == disabled
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring the original spfctl value after disabling the prefetch-path control bits does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after restoring the prefetch-path control bits",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_threshold_stride_programmable_corner()
{
    TEST_START();

    const uint64_t new_threshold = 0x3ULL << 6;
    const uint64_t new_stride = 0x7ULL << 10;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable in M-mode before reprogramming the L1D active-threshold and active-stride fields",
        true
    );

    uint64_t reprogrammed = (original &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        new_threshold | new_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("writing spfctl with new L1D threshold/stride field values does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the programmed L1D threshold and stride fields while preserving the non-targeted control bits",
        observed == reprogrammed &&
        ((observed ^ original) &
            ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring the original spfctl value after the threshold/stride programming probe does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after restoring the L1D threshold and stride fields",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_disable_then_threshold_stride_preserves_disabled_paths_corner()
{
    TEST_START();

    const uint64_t new_threshold = 0x5ULL << 6;
    const uint64_t new_stride = 0x12ULL << 10;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before composing the disable-then-threshold/stride interaction corner",
        true
    );

    uint64_t disabled = original & ~AI_ARCH_SPFCTL_ENABLE_MASK;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("disabling all architecturally visible spfctl prefetch-path bits does not raise an exception before reprogramming threshold/stride",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects that the prefetch-path bits are disabled before reprogramming threshold/stride",
        observed_disabled == disabled
    );

    uint64_t reprogrammed = (disabled &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        new_threshold | new_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("reprogramming spfctl threshold/stride fields after disabling prefetch-path bits does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("reprogramming spfctl threshold/stride fields preserves the already-disabled prefetch-path bits",
        observed_reprogrammed == reprogrammed &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_ENABLE_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the disable-then-threshold/stride interaction corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the disable-then-threshold/stride interaction corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_smblockctl_soft_prefetch_bit_clear_no_regression_corner()
{
    TEST_START();

    const uint64_t initial0 = 0x1021324354657687ULL;
    const uint64_t initial1 = 0x8877665544332211ULL;
    const uint64_t stored1 = 0x0fedcba987654321ULL;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t paddr0 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t paddr1 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF);

    goto_priv(PRIV_M);
    write64(paddr0, initial0);
    write64(paddr1, initial1);
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before clearing the current no-regression soft_prefetch-marked CSR bit",
        true
    );

    uint64_t disabled = original & ~AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing the current no-regression smblockctl soft_prefetch-marked CSR bit does not raise an exception before the explicit-prefetch probe",
        true
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr0);
    TEST_ASSERT("after clearing the current no-regression smblockctl soft_prefetch-marked CSR bit, prefetch.r still retires silently on a valid HS address",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr1);
    TEST_ASSERT("after clearing the current no-regression smblockctl soft_prefetch-marked CSR bit, prefetch.w still retires silently on a valid HS address",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed0 = read64(vaddr0);
    TEST_ASSERT("after clearing the current no-regression soft_prefetch-marked CSR bit, a real HS load still reads the original payload correctly",
        excpt.triggered == false &&
        observed0 == initial0
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr1, stored1);
    TEST_ASSERT("after clearing the current no-regression soft_prefetch-marked CSR bit, a real HS store still completes successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the current no-regression soft_prefetch-marked CSR bit, the HS store still updates backing memory correctly",
        read64(paddr1) == stored1
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the soft_prefetch no-regression probe does not raise an exception",
        true
    );

    TEST_END();
}

bool ai_arch_smblockctl_misaligned_readback_programming_aligned_access_unaffected_corner()
{
    TEST_START();

    const uint64_t initial0 = 0x0123456789abcdefULL;
    const uint64_t stored1 = 0xfedcba9876543210ULL;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t paddr0 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t paddr1 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF);

    goto_priv(PRIV_M);
    write64(paddr0, initial0);
    write64(paddr1, 0ULL);
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before programming the current readback-only misaligned-marked CSR bits for the aligned-access no-regression corner",
        true
    );

    uint64_t enabled = original | AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(enabled);
    AI_ASSERT_NO_EXCEPT_AND("setting both current readback-only smblockctl misaligned-marked CSR bits does not raise an exception before the aligned-access no-regression corner",
        true
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed0 = read64(vaddr0);
    TEST_ASSERT("after programming the current readback-only smblockctl misaligned-marked CSR bits, an aligned HS load still returns the expected payload",
        excpt.triggered == false &&
        observed0 == initial0
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr1, stored1);
    TEST_ASSERT("after programming the current readback-only smblockctl misaligned-marked CSR bits, an aligned HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed1 = read64(vaddr1);
    TEST_ASSERT("after programming the current readback-only smblockctl misaligned-marked CSR bits, the aligned HS store remains architecturally visible",
        excpt.triggered == false &&
        observed1 == stored1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after programming the current readback-only smblockctl misaligned-marked CSR bits, backing memory matches the aligned HS store result",
        read64(paddr1) == stored1
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the aligned-access misaligned-readback no-regression corner does not raise an exception",
        true
    );

    TEST_END();
}

bool ai_arch_spfctl_disable_prefetch_paths_explicit_access_correct_corner()
{
    TEST_START();

    const uint64_t value0 = 0x1111222233334444ULL;
    const uint64_t value1 = 0x5555666677778888ULL;
    const uint64_t value2 = 0x9999aaaabbbbccccULL;
    const uint64_t stored3 = 0xddddeeeeffff0001ULL;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t vaddr2 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t vaddr3 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);
    uintptr_t paddr0 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t paddr1 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t paddr2 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t paddr3 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF);

    goto_priv(PRIV_M);
    write64(paddr0, value0);
    write64(paddr1, value1);
    write64(paddr2, value2);
    write64(paddr3, 0ULL);
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before clearing the hardware-prefetch enable bits for the explicit-access corner",
        true
    );

    uint64_t disabled = original & ~AI_ARCH_SPFCTL_ENABLE_MASK;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing spfctl hardware-prefetch enable bits does not raise an exception before the explicit-access corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the hardware-prefetch enable bits cleared before the explicit-access corner",
        observed_disabled == disabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed0_first = read64(vaddr0);
    uint64_t observed1_first = read64(vaddr1);
    uint64_t observed2_first = read64(vaddr2);
    uint64_t observed0_second = read64(vaddr0);
    uint64_t observed1_second = read64(vaddr1);
    TEST_ASSERT("after disabling spfctl prefetch paths, an explicit repeated HS load window still returns the expected payloads",
        excpt.triggered == false &&
        observed0_first == value0 &&
        observed1_first == value1 &&
        observed2_first == value2 &&
        observed0_second == value0 &&
        observed1_second == value1
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr3, stored3);
    TEST_ASSERT("after disabling spfctl prefetch paths, an explicit HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed3 = read64(vaddr3);
    TEST_ASSERT("after disabling spfctl prefetch paths, the explicit HS store remains readable through the same translated alias",
        excpt.triggered == false &&
        observed3 == stored3
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after disabling spfctl prefetch paths, the explicit HS store still updates backing memory correctly",
        read64(paddr3) == stored3
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the explicit-access corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the explicit-access corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_threshold_stride_stream_correct_corner()
{
    TEST_START();

    const uint64_t value0 = 0x0123012301230123ULL;
    const uint64_t value1 = 0x4567456745674567ULL;
    const uint64_t value2 = 0x89ab89ab89ab89abULL;
    const uint64_t stored3 = 0xcdefcdefcdefcdefULL;
    const uint64_t new_threshold = 0x2ULL << 6;
    const uint64_t new_stride = 0x5ULL << 10;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t vaddr2 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t vaddr3 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);
    uintptr_t paddr0 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t paddr1 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t paddr2 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t paddr3 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF);

    goto_priv(PRIV_M);
    write64(paddr0, value0);
    write64(paddr1, value1);
    write64(paddr2, value2);
    write64(paddr3, 0ULL);
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before reprogramming the active-threshold and stride fields for the explicit-stream corner",
        true
    );

    uint64_t reprogrammed = (original &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        new_threshold | new_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("writing new spfctl threshold and stride fields does not raise an exception before the explicit-stream corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the requested threshold and stride fields before the explicit-stream corner",
        observed_reprogrammed == reprogrammed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed0_first = read64(vaddr0);
    uint64_t observed1_first = read64(vaddr1);
    uint64_t observed2_first = read64(vaddr2);
    uint64_t observed0_second = read64(vaddr0);
    uint64_t observed1_second = read64(vaddr1);
    TEST_ASSERT("after reprogramming spfctl threshold and stride fields, an explicit repeated HS load window still returns the expected payloads",
        excpt.triggered == false &&
        observed0_first == value0 &&
        observed1_first == value1 &&
        observed2_first == value2 &&
        observed0_second == value0 &&
        observed1_second == value1
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr3, stored3);
    TEST_ASSERT("after reprogramming spfctl threshold and stride fields, an explicit HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed3 = read64(vaddr3);
    TEST_ASSERT("after reprogramming spfctl threshold and stride fields, the stored payload remains readable through the translated alias",
        excpt.triggered == false &&
        observed3 == stored3
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after reprogramming spfctl threshold and stride fields, backing memory still matches the explicit HS store result",
        read64(paddr3) == stored3
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the explicit-stream threshold/stride corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the explicit-stream threshold/stride corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_toggle_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable in M-mode before toggling replay_escape_enable",
        true
    );

    uint64_t toggled = original ^ AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(toggled);
    AI_ASSERT_NO_EXCEPT_AND("writing smblockctl with replay_escape_enable toggled does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_toggled = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl readback reflects the programmed replay_escape_enable bit",
        observed_toggled == toggled
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after toggling replay_escape_enable does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl returns to its original value after restoring replay_escape_enable",
        restored == original
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_threshold_programmable_corner()
{
    TEST_START();

    const uint64_t new_threshold = 0x5ULL << 1;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable in M-mode before reprogramming replay_escape_threshold",
        true
    );

    uint64_t reprogrammed = (original & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) |
        new_threshold;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("writing smblockctl with a new replay_escape_threshold does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl readback reflects the programmed replay_escape_threshold while preserving the non-targeted bits",
        observed == reprogrammed &&
        ((observed ^ original) & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the replay_escape_threshold probe does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl returns to its original value after restoring replay_escape_threshold",
        restored == original
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_disable_then_threshold_preserved_corner()
{
    TEST_START();

    const uint64_t new_threshold = 0x2ULL << 1;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before composing the replay_escape enable-threshold interaction corner",
        true
    );

    uint64_t disabled = original & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing replay_escape_enable does not raise an exception before reprogramming replay_escape_threshold",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl readback reflects replay_escape_enable cleared before reprogramming replay_escape_threshold",
        observed_disabled == disabled
    );

    uint64_t reprogrammed = (disabled & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) |
        new_threshold;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("reprogramming replay_escape_threshold after clearing replay_escape_enable does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("reprogramming replay_escape_threshold preserves replay_escape_enable cleared",
        observed_reprogrammed == reprogrammed &&
        (observed_reprogrammed & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the replay_escape enable-threshold interaction corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl returns to its original value after the replay_escape enable-threshold interaction corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_fault_metadata_recovery_corner()
{
    TEST_START();

    const uint64_t initial_load = 0x1029384756abcdefULL;
    const uint64_t initial_store = 0xfedcba6543210987ULL;
    const uint64_t repaired_store = 0x5566778899aabbccULL;
    const uint64_t misalign_seed0 = 0x0123456789abcdefULL;
    const uint64_t misalign_seed1 = 0xfedcba9876543210ULL;
    const uint64_t misalign_store_attempt = 0xa5a5a5a5a5a5a5a5ULL;
    const uintptr_t load_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    const uintptr_t load_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    const uintptr_t store_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    const uintptr_t store_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF);
    const uint64_t enabled_small =
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x1ULL << 1);
    const uint64_t disabled_small = 0x1ULL << 1;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before the replay_escape fault metadata recovery corner",
        true
    );
    uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    write64(load_paddr, initial_load);
    write64(store_paddr, initial_store);
    write64(load_paddr + sizeof(uint64_t), misalign_seed1);
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_M);
    pte_t *leaf = ai_arch_custom_hs_leaf_entry(AI_ARCH_CUSTOM_DATA_PAGE);
    pte_t original_leaf = *leaf;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) |
        enabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape enabled with a small threshold before the fault metadata corner does not raise an exception",
        true
    );

    *leaf = original_leaf & ~(PTE_R | PTE_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t blocked_load = read64(load_vaddr);
    (void)blocked_load;
    asm volatile("" ::: "memory");
    TEST_ASSERT("with replay_escape enabled, a no-R HS load still reports a Load PF with fresh tval at the faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr
    );

    goto_priv(PRIV_M);
    *leaf = original_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovered_load = read64(load_vaddr);
    TEST_ASSERT("after repairing the no-R leaf and fencing, the same replay_escape-enabled load re-translates and succeeds",
        excpt.triggered == false &&
        recovered_load == initial_load
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) |
        disabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape disabled with a small threshold before the store fault corner does not raise an exception",
        true
    );

    *leaf = original_leaf & ~PTE_W;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    write64(store_vaddr, repaired_store);
    asm volatile("" ::: "memory");
    TEST_ASSERT("with replay_escape disabled, a no-W HS store still reports a Store PF with fresh tval and does not commit data",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr
    );

    goto_priv(PRIV_M);
    uint64_t observed_fault_store = read64(store_paddr);
    TEST_ASSERT("the replay_escape-disabled no-W fault leaves backing memory unchanged before repair",
        observed_fault_store == initial_store,
        "observed=%016llx expected=%016llx attempted=%016llx",
        (unsigned long long)observed_fault_store,
        (unsigned long long)initial_store,
        (unsigned long long)repaired_store
    );

    *leaf = original_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    write64(store_vaddr, repaired_store);
    TEST_ASSERT("after repairing the no-W leaf and fencing, the same replay_escape-disabled store succeeds",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired store commits exactly once after the SMBlockCtl fault metadata recovery corner",
        read64(store_paddr) == repaired_store
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) |
        enabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape enabled before the PMP access-fault metadata probe does not raise an exception",
        true
    );

    ai_micro_prefetch_configure_pmp_window(
        load_paddr & PAGE_ADDR_MSK,
        (load_paddr & PAGE_ADDR_MSK) + PAGE_SIZE,
        AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t access_fault_load = read64(load_vaddr);
    (void)access_fault_load;
    asm volatile("" ::: "memory");
    TEST_ASSERT("with replay_escape enabled, a PMP-denied HS load still reports Load Access Fault with fresh tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == load_vaddr
    );

    ai_micro_prefetch_restore_pmp_full_access();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovered_access_fault_load = read64(load_vaddr);
    TEST_ASSERT("after restoring PMP read permission, the replay_escape-enabled access-fault load recovers normally",
        excpt.triggered == false &&
        recovered_access_fault_load == initial_load
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) |
        disabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape disabled before the PMP store access-fault metadata probe does not raise an exception",
        true
    );

    ai_micro_prefetch_configure_pmp_window(
        store_paddr & PAGE_ADDR_MSK,
        (store_paddr & PAGE_ADDR_MSK) + PAGE_SIZE,
        AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    write64(store_vaddr, repaired_store ^ 0x1111222233334444ULL);
    asm volatile("" ::: "memory");
    TEST_ASSERT("with replay_escape disabled, a PMP-denied HS store still reports Store Access Fault with fresh tval and no commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == store_vaddr
    );

    ai_micro_prefetch_restore_pmp_full_access();
    goto_priv(PRIV_M);
    TEST_ASSERT("the replay_escape-disabled store access fault leaves backing memory unchanged before later probes",
        read64(store_paddr) == repaired_store
    );

    write64(load_paddr, misalign_seed0);
    write64(load_paddr + sizeof(uint64_t), misalign_seed1);
    write64(store_paddr, initial_store);
    write64(store_paddr + sizeof(uint64_t), misalign_seed1);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    *ai_micro_prefetch_hs_leaf_entry(AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE) =
        (ai_micro_prefetch_alias_anchor(
            AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE,
            load_paddr & PAGE_ADDR_MSK) >> 2) |
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    sfence_vma();
    const uintptr_t nc_page_base =
        hs_page_base(AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE);
    const uintptr_t misalign_load_vaddr = nc_page_base +
        (load_paddr & (PAGE_SIZE - 1U)) + 1U;
    const uintptr_t misalign_store_vaddr = nc_page_base +
        (store_paddr & (PAGE_SIZE - 1U)) + 1U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t misaligned_load;
    asm volatile("ld %0, 0(%1)"
        : "=r"(misaligned_load)
        : "r"(misalign_load_vaddr)
        : "memory");
    (void)misaligned_load;
    asm volatile("" ::: "memory");
    TEST_ASSERT("with replay_escape disabled, an unaligned HS load reports fresh Load Address Misaligned metadata instead of replay_escape state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misalign_load_vaddr
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original & ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) |
        enabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape enabled before the misaligned store metadata probe does not raise an exception",
        true
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    asm volatile("sd %0, 0(%1)"
        :
        : "r"(misalign_store_attempt), "r"(misalign_store_vaddr)
        : "memory");
    asm volatile("" ::: "memory");
    TEST_ASSERT("with replay_escape enabled, an unaligned HS store reports fresh Store Address Misaligned metadata and does not modify the aligned backing word",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misalign_store_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the misaligned SMBlockCtl store metadata probe leaves the neighboring backing words unchanged",
        read64(store_paddr) == initial_store &&
        read64(store_paddr + sizeof(uint64_t)) == misalign_seed1
    );

    CSRW(CSR_MENVCFG, original_menvcfg);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the replay_escape fault metadata recovery corner does not raise an exception",
        true
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_amo_cbo_no_regression_corner()
{
    TEST_START();

    const uint64_t amo_initial = 0x0102030405060708ULL;
    const uint64_t amo_addend_enabled = 0x0001000100010001ULL;
    const uint64_t amo_addend_aqrl = 0x0000000000000010ULL;
    const uint64_t amo_addend_disabled = 0x00000000000000f0ULL;
    const uint64_t load_initial = 0x22446688aaccef01ULL;
    const uint64_t store_initial = 0x13579bdf2468ace0ULL;
    const uint64_t store_enabled = 0x55aa55aa55aa55aaULL;
    const uint64_t store_disabled = 0xaa55aa55aa55aa55ULL;
    const uintptr_t load_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    const uintptr_t load_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    const uintptr_t store_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    const uintptr_t store_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF);
    const uintptr_t amo_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    const uintptr_t amo_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF);
    const uintptr_t cbo_vaddr = ai_arch_page_vaddr(SWITCH1,
        AI_ARCH_CUSTOM_CBO_BLOCK_OFF);
    const uintptr_t cbo_paddr = ai_arch_page_paddr(SWITCH1,
        AI_ARCH_CUSTOM_CBO_BLOCK_OFF);
    const uint64_t enabled_small =
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | (0x1ULL << 1);
    const uint64_t disabled_small = 0x1ULL << 1;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original_smblockctl = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before the replay_escape AMO/CBO no-regression corner",
        true
    );
    uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    write64(load_paddr, load_initial);
    write64(store_paddr, store_initial);
    write64(amo_paddr, amo_initial);
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        write64(cbo_paddr + i * sizeof(uint64_t),
            0xa5a5000000000000ULL + i);
    }
    CSRS(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original_smblockctl &
        ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) | enabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape enabled before the AMO/CBO no-regression corner does not raise an exception",
        true
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_observed_enabled = read64(load_vaddr);
    TEST_ASSERT("with replay_escape enabled, a normal HS load still returns the exact architectural payload",
        excpt.triggered == false &&
        load_observed_enabled == load_initial
    );

    TEST_SETUP_EXCEPT();
    write64(store_vaddr, store_enabled);
    TEST_ASSERT("with replay_escape enabled, a normal HS store still retires without exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    TEST_ASSERT("with replay_escape enabled, FENCE iorw,iorw retires without changing explicit memory results",
        excpt.triggered == false &&
        read64(store_vaddr) == store_enabled
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fence_i();
    TEST_ASSERT("with replay_escape enabled, FENCE.I retires without changing explicit data-side load/store results",
        excpt.triggered == false &&
        read64(load_vaddr) == load_initial &&
        read64(store_vaddr) == store_enabled
    );

    TEST_SETUP_EXCEPT();
    ai_arch_custom_fence_tso();
    TEST_ASSERT("with replay_escape enabled, FENCE.TSO retires without changing explicit data-side load/store results",
        excpt.triggered == false &&
        read64(load_vaddr) == load_initial &&
        read64(store_vaddr) == store_enabled
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old_enabled = amoadd_d(amo_vaddr, amo_addend_enabled);
    TEST_ASSERT("with replay_escape enabled, HS AMO returns the old value and commits the add result exactly once",
        excpt.triggered == false &&
        amo_old_enabled == amo_initial &&
        read64(amo_vaddr) == amo_initial + amo_addend_enabled
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old_aqrl = ai_arch_custom_amoadd_d_aqrl(
        amo_vaddr, amo_addend_aqrl);
    TEST_ASSERT("with replay_escape enabled, HS amoadd.d.aqrl keeps old-value return, RMW update, and acquire-release form semantics",
        excpt.triggered == false &&
        amo_old_aqrl == amo_initial + amo_addend_enabled &&
        read64(amo_vaddr) == amo_initial + amo_addend_enabled +
            amo_addend_aqrl
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_vaddr);
    TEST_ASSERT("with replay_escape enabled, HS cbo.zero completes without an exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    bool zeroed_enabled = true;
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        zeroed_enabled = zeroed_enabled &&
            read64(cbo_vaddr + i * sizeof(uint64_t)) == 0ULL;
    }
    TEST_ASSERT("with replay_escape enabled, HS cbo.zero leaves the whole target block architecturally zero",
        excpt.triggered == false &&
        zeroed_enabled
    );

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        write64(cbo_paddr + i * sizeof(uint64_t),
            0x5a5a000000000000ULL + i);
    }
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original_smblockctl &
        ~AI_ARCH_SMBLOCKCTL_DEFINED_MASK) | disabled_small);
    AI_ASSERT_NO_EXCEPT_AND("programming replay_escape disabled before the AMO/CBO no-regression corner does not raise an exception",
        true
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load_observed_disabled = read64(load_vaddr);
    TEST_ASSERT("with replay_escape disabled, a normal HS load still returns the same architectural payload",
        excpt.triggered == false &&
        load_observed_disabled == load_initial
    );

    TEST_SETUP_EXCEPT();
    write64(store_vaddr, store_disabled);
    TEST_ASSERT("with replay_escape disabled, a normal HS store still retires and overwrites only the target word",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    ai_arch_custom_fence_i();
    ai_arch_custom_fence_tso();
    TEST_ASSERT("with replay_escape disabled, FENCE/FENCE.I/FENCE.TSO retire as ordering boundaries without corrupting visible memory results",
        excpt.triggered == false &&
        read64(load_vaddr) == load_initial &&
        read64(store_vaddr) == store_disabled
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old_disabled = amoadd_d(amo_vaddr, amo_addend_disabled);
    TEST_ASSERT("with replay_escape disabled, a second HS AMO sees the current value and commits the add result exactly once",
        excpt.triggered == false &&
        amo_old_disabled == amo_initial + amo_addend_enabled +
            amo_addend_aqrl &&
        read64(amo_vaddr) == amo_initial + amo_addend_enabled +
            amo_addend_aqrl + amo_addend_disabled
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_vaddr);
    TEST_ASSERT("with replay_escape disabled, HS cbo.zero still completes without an exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    bool zeroed_disabled = true;
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        zeroed_disabled = zeroed_disabled &&
            read64(cbo_vaddr + i * sizeof(uint64_t)) == 0ULL;
    }
    TEST_ASSERT("with replay_escape disabled, HS cbo.zero still leaves the whole target block architecturally zero",
        excpt.triggered == false &&
        zeroed_disabled
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both SMBlockCtl modes, backing memory reflects normal store and AMO results while CBO zero has already been checked through HS loads",
        read64(load_paddr) == load_initial &&
        read64(store_paddr) == store_disabled &&
        read64(amo_paddr) == amo_initial + amo_addend_enabled +
            amo_addend_aqrl + amo_addend_disabled
    );

    CSRW(CSR_MENVCFG, original_menvcfg);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original_smblockctl);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the replay_escape AMO/CBO no-regression corner does not raise an exception",
        true
    );
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_smblockctl_legacy_bits_amo_cbo_fault_no_regression_corner()
{
    TEST_START();

    const uint64_t amo_initial = 0x1122003344556677ULL;
    const uint64_t amo_addend = 0x0000000000000101ULL;
    const uintptr_t amo_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);
    const uintptr_t amo_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF);
    const uintptr_t cbo_vaddr = ai_arch_page_vaddr(SWITCH2,
        AI_ARCH_CUSTOM_CBO_BLOCK_OFF);
    const uintptr_t cbo_paddr = ai_arch_page_paddr(SWITCH2,
        AI_ARCH_CUSTOM_CBO_BLOCK_OFF);
    const uint64_t legacy_mask =
        AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE |
        AI_ARCH_SMBLOCKCTL_CACHE_ERROR_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE;
    const uint64_t legacy_pattern =
        AI_ARCH_SMBLOCKCTL_SOFT_PREFETCH_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_ST_ENABLE |
        AI_ARCH_SMBLOCKCTL_MISALIGN_LD_ENABLE;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original_smblockctl = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before the legacy-bit AMO/CBO/fault no-regression corner",
        true
    );
    uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    write64(amo_paddr, amo_initial);
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        write64(cbo_paddr + i * sizeof(uint64_t),
            0xcc33000000000000ULL + i);
    }
    CSRS(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();

    pte_t *cbo_leaf = ai_arch_custom_hs_leaf_entry(SWITCH2);
    pte_t original_cbo_leaf = *cbo_leaf;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl((original_smblockctl & ~legacy_mask) |
        legacy_pattern);
    AI_ASSERT_NO_EXCEPT_AND("writing the current legacy/non-replay_escape SMBlockCtl bit pattern does not raise an exception before AMO/CBO probes",
        true
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_old = amoadd_d(amo_vaddr, amo_addend);
    TEST_ASSERT("after programming legacy SMBlockCtl bits, HS AMO still returns the old value and commits exactly once",
        excpt.triggered == false &&
        amo_old == amo_initial &&
        read64(amo_vaddr) == amo_initial + amo_addend
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_vaddr);
    TEST_ASSERT("after programming legacy SMBlockCtl bits, HS cbo.zero still completes without an exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    bool zeroed = true;
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        zeroed = zeroed &&
            read64(cbo_vaddr + i * sizeof(uint64_t)) == 0ULL;
    }
    TEST_ASSERT("after programming legacy SMBlockCtl bits, HS loads observe the whole CBO zero block as zero",
        excpt.triggered == false &&
        zeroed
    );

    goto_priv(PRIV_M);
    *cbo_leaf = original_cbo_leaf & ~(PTE_R | PTE_W);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbo_vaddr);
    TEST_ASSERT("after programming legacy SMBlockCtl bits, a no-R CBO clean fault reports Store PF with the current target VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == cbo_vaddr
    );

    goto_priv(PRIV_M);
    *cbo_leaf = original_cbo_leaf;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(cbo_vaddr);
    TEST_ASSERT("after repairing the CBO leaf, legacy SMBlockCtl bits do not leave stale fault metadata on the re-executed CBO clean",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after legacy SMBlockCtl bit programming, backing memory reflects the AMO result and the CBO fault path did not corrupt memory",
        read64(amo_paddr) == amo_initial + amo_addend
    );

    CSRW(CSR_MENVCFG, original_menvcfg);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original_smblockctl);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the legacy-bit AMO/CBO/fault no-regression corner does not raise an exception",
        true
    );
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_spfctl_agt_pht_train_on_hit_cross_readback_corner()
{
    TEST_START();

    const uint64_t cross_mask = AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before toggling the AGT/PHT/train_on_hit cross bits",
        true
    );

    uint64_t reprogrammed = original ^ cross_mask;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("toggling the AGT/PHT/train_on_hit bits in spfctl does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the AGT/PHT/train_on_hit cross-bit programming while preserving the non-targeted bits",
        observed == reprogrammed &&
        ((observed ^ original) & ~cross_mask) == 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the AGT/PHT/train_on_hit cross-bit probe does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after restoring the AGT/PHT/train_on_hit cross bits",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_agt_disable_then_pht_threshold_stride_preserved_corner()
{
    TEST_START();

    const uint64_t new_threshold = 0x1ULL << 6;
    const uint64_t new_stride = 0x9ULL << 10;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before composing the AGT-disable plus PHT/threshold/stride interaction corner",
        true
    );

    uint64_t seeded = (original | AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE) & ~AI_ARCH_SPFCTL_ENABLE_AGT;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(seeded);
    AI_ASSERT_NO_EXCEPT_AND("clearing AGT while keeping PHT/train_on_hit/enable_stride seeded does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_seeded = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT disabled before reprogramming threshold/stride in the AGT-disable corner",
        observed_seeded == seeded
    );

    uint64_t reprogrammed = (seeded &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        new_threshold | new_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("reprogramming threshold/stride after disabling AGT does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("reprogramming threshold/stride preserves AGT disabled while keeping PHT/train_on_hit/enable_stride intact",
        observed_reprogrammed == reprogrammed &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_ENABLE_AGT) == 0ULL &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_ENABLE_PHT) != 0ULL &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_TRAIN_ON_HIT) != 0ULL &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_ENABLE_STRIDE) != 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the AGT-disable plus PHT/threshold/stride interaction corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the AGT-disable plus PHT/threshold/stride interaction corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_pht_disable_then_agt_threshold_stride_preserved_corner()
{
    TEST_START();

    const uint64_t new_threshold = 0x6ULL << 6;
    const uint64_t new_stride = 0xcULL << 10;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before composing the PHT-disable plus AGT/threshold/stride interaction corner",
        true
    );

    uint64_t seeded = (original | AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT) & ~AI_ARCH_SPFCTL_ENABLE_PHT;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(seeded);
    AI_ASSERT_NO_EXCEPT_AND("clearing PHT while keeping AGT/train_on_hit seeded does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_seeded = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects PHT disabled before reprogramming threshold/stride in the PHT-disable corner",
        observed_seeded == seeded
    );

    uint64_t reprogrammed = (seeded &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        new_threshold | new_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("reprogramming threshold/stride after disabling PHT does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("reprogramming threshold/stride preserves PHT disabled while keeping AGT/train_on_hit intact",
        observed_reprogrammed == reprogrammed &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_ENABLE_PHT) == 0ULL &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_ENABLE_AGT) != 0ULL &&
        (observed_reprogrammed & AI_ARCH_SPFCTL_TRAIN_ON_HIT) != 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the PHT-disable plus AGT/threshold/stride interaction corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the PHT-disable plus AGT/threshold/stride interaction corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_enable_stride_toggle_readback_no_regression_corner()
{
    TEST_START();

    const uint64_t value0 = 0x1011222233334444ULL;
    const uint64_t value1 = 0x5555666677778888ULL;
    const uint64_t value2 = 0x9999aaaabbbbccccULL;
    const uint64_t stored3 = 0xddddeeeeffff0001ULL;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t vaddr2 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t vaddr3 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);

    ai_arch_custom_seed_four_words(value0, value1, value2, 0ULL);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before toggling the enable_stride bit for the SMS no-regression corner",
        true
    );

    uint64_t reprogrammed = original ^ AI_ARCH_SPFCTL_ENABLE_STRIDE;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("toggling spfctl.enable_stride does not raise an exception before the explicit stream no-regression corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the toggled enable_stride bit before the explicit stream no-regression corner",
        observed_reprogrammed == reprogrammed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed0_first = read64(vaddr0);
    uint64_t observed1_first = read64(vaddr1);
    uint64_t observed2_first = read64(vaddr2);
    uint64_t observed0_second = read64(vaddr0);
    uint64_t observed1_second = read64(vaddr1);
    TEST_ASSERT("after toggling spfctl.enable_stride, an explicit repeated HS load window still returns the expected payloads",
        excpt.triggered == false &&
        observed0_first == value0 &&
        observed1_first == value1 &&
        observed2_first == value2 &&
        observed0_second == value0 &&
        observed1_second == value1
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr3, stored3);
    TEST_ASSERT("after toggling spfctl.enable_stride, an explicit HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed3 = read64(vaddr3);
    TEST_ASSERT("after toggling spfctl.enable_stride, the explicit HS store remains readable through the translated alias",
        excpt.triggered == false &&
        observed3 == stored3
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after toggling spfctl.enable_stride, backing memory still matches the explicit HS store result",
        read64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF)) == stored3
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the enable_stride no-regression corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the enable_stride no-regression corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_agt_pht_disable_explicit_prefetch_silent_corner()
{
    TEST_START();

    const uint64_t value0 = 0x0123456789abcdefULL;
    const uint64_t value1 = 0x1122334455667788ULL;
    const uint64_t value2 = 0x99aabbccddeeff00ULL;
    const uint64_t stored3 = 0x0f1e2d3c4b5a6978ULL;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t vaddr2 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t vaddr3 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);
    uintptr_t paddr0 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t paddr1 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t paddr2 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t paddr3 = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF);
    const uint64_t scause_seed = 0x6a6ULL;
    const uint64_t stval_seed = 0ULL;

    ai_arch_custom_seed_four_words(value0, value1, value2, 0ULL);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before disabling AGT/PHT for the explicit-prefetch silence corner",
        true
    );

    uint64_t disabled = original & ~(AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing the AGT/PHT bits in spfctl does not raise an exception before the explicit-prefetch silence corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT cleared before the explicit-prefetch silence corner",
        observed_disabled == disabled
    );

    goto_priv(PRIV_HS);
    CSRW(CSR_SCAUSE, scause_seed);
    CSRW(CSR_STVAL, stval_seed);

    TEST_SETUP_EXCEPT();
    uint64_t prefetch_r_rd = ai_arch_custom_prefetch_r_gpr_probe(
        vaddr0, 0xa1a2a3a4a5a6a7a8ULL);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, prefetch.r retires silently, preserves S trap CSRs, and cannot write a GPR destination",
        excpt.triggered == false &&
        prefetch_r_rd == 0xa1a2a3a4a5a6a7a8ULL &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t prefetch_w_rd = ai_arch_custom_prefetch_w_gpr_probe(
        vaddr1, 0xb1b2b3b4b5b6b7b8ULL);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, prefetch.w also retires silently without GPR, S trap CSR, or memory side effects",
        excpt.triggered == false &&
        prefetch_w_rd == 0xb1b2b3b4b5b6b7b8ULL &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, explicit prefetch hints do not modify target or adjacent backing words",
        read64(paddr0) == value0 &&
        read64(paddr1) == value1 &&
        read64(paddr2) == value2 &&
        read64(paddr3) == 0ULL
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed0 = read64(vaddr0);
    uint64_t observed1 = read64(vaddr1);
    uint64_t observed2 = read64(vaddr2);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, real HS loads still return the expected payloads",
        excpt.triggered == false &&
        observed0 == value0 &&
        observed1 == value1 &&
        observed2 == value2
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr3, stored3);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, a real HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed3 = read64(vaddr3);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, the stored payload remains readable through the translated alias",
        excpt.triggered == false &&
        observed3 == stored3
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with AGT/PHT disabled in spfctl, backing memory still matches the explicit HS store result",
        read64(paddr3) == stored3
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the AGT/PHT explicit-prefetch silence corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the AGT/PHT explicit-prefetch silence corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_train_on_hit_toggle_hit_stream_correct_corner()
{
    TEST_START();

    const uint64_t value0 = 0x2233445566778899ULL;
    const uint64_t value1 = 0xaabbccddeeff0011ULL;
    const uint64_t value2 = 0x13579bdf2468ace0ULL;
    const uint64_t stored3 = 0xf0e1d2c3b4a59687ULL;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t vaddr2 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t vaddr3 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);

    ai_arch_custom_seed_four_words(value0, value1, value2, 0ULL);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before toggling train_on_hit for the hot-hit stream corner",
        true
    );

    uint64_t reprogrammed = (original | AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT) ^
        AI_ARCH_SPFCTL_TRAIN_ON_HIT;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("toggling spfctl.train_on_hit while keeping AGT/PHT enabled does not raise an exception before the hot-hit stream corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_reprogrammed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the requested train_on_hit toggle before the hot-hit stream corner",
        observed_reprogrammed == reprogrammed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed0_first = read64(vaddr0);
    uint64_t observed1_first = read64(vaddr1);
    uint64_t observed0_second = read64(vaddr0);
    uint64_t observed1_second = read64(vaddr1);
    uint64_t observed2 = read64(vaddr2);
    TEST_ASSERT("after toggling spfctl.train_on_hit, a hot-hit HS load stream still returns the expected payloads",
        excpt.triggered == false &&
        observed0_first == value0 &&
        observed1_first == value1 &&
        observed0_second == value0 &&
        observed1_second == value1 &&
        observed2 == value2
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr3, stored3);
    TEST_ASSERT("after toggling spfctl.train_on_hit, a follow-on HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed3 = read64(vaddr3);
    TEST_ASSERT("after toggling spfctl.train_on_hit, the follow-on HS store remains architecturally visible",
        excpt.triggered == false &&
        observed3 == stored3
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after toggling spfctl.train_on_hit, backing memory still matches the hot-hit stream follow-on store result",
        read64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF)) == stored3
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the train_on_hit hot-hit stream corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the train_on_hit hot-hit stream corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_cross_reprogram_cbo_fence_i_fetch_no_regression_corner()
{
    TEST_START();

    const uintptr_t cbo_vaddr = ai_arch_page_vaddr(SWITCH1,
        AI_ARCH_CUSTOM_CBO_BLOCK_OFF);
    const uintptr_t cbo_paddr = ai_arch_page_paddr(SWITCH1,
        AI_ARCH_CUSTOM_CBO_BLOCK_OFF);
    const uintptr_t amo_vaddr = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    const uintptr_t amo_paddr = ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD2_OFF);
    const uint64_t amo_initial = 0x0102030405060708ULL;
    const uint64_t amo_addend_first = 0x0001000100010001ULL;
    const uint64_t amo_addend_second = 0x00000000000000f0ULL;
    const uint64_t first_threshold = 0x1ULL << 6;
    const uint64_t first_stride = 0x4ULL << 10;
    const uint64_t second_threshold = 0x8ULL << 6;
    const uint64_t second_stride = 0x11ULL << 10;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original_spfctl = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before the CBO/FENCE.I/fetch no-regression cross-reprogramming corner",
        true
    );
    uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    write64(amo_paddr, amo_initial);

    uint64_t first = original_spfctl |
        AI_ARCH_SPFCTL_L1I_PF_ENABLE |
        AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE;
    first &= ~(AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_L2_STORE_ONLY);
    first = (first &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK |
            AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        first_threshold | first_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(first);
    AI_ASSERT_NO_EXCEPT_AND("writing the first SPFctl cross configuration before the CBO probe does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("SPFctl readback reflects the first CBO/FENCE.I/fetch no-regression configuration",
        observed_first == first
    );

    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        write64(cbo_paddr + i * sizeof(uint64_t),
            0xfeed000000000000ULL + i);
    }
    CSRS(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBZFE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_old_first = amoadd_d(amo_vaddr, amo_addend_first);
    TEST_ASSERT("after the first SPFctl enable/threshold configuration, HS AMO returns the old value and commits the add result exactly once",
        excpt.triggered == false &&
        amo_old_first == amo_initial &&
        read64(amo_vaddr) == amo_initial + amo_addend_first
    );

    TEST_SETUP_EXCEPT();
    cbo_zero(cbo_vaddr);
    TEST_ASSERT("after the first SPFctl cross configuration, HS cbo.zero still completes without exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    bool zeroed_first = true;
    for (unsigned i = 0; i < AI_ARCH_CUSTOM_CBO_BLOCK_WORDS; i++) {
        zeroed_first = zeroed_first &&
            read64(cbo_vaddr + i * sizeof(uint64_t)) == 0ULL;
    }
    TEST_ASSERT("after the first SPFctl cross configuration, HS loads observe the full CBO zero block as zero",
        excpt.triggered == false &&
        zeroed_first
    );

    goto_priv(PRIV_M);
    uint64_t second = first | AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_L2_STORE_ONLY;
    second &= ~(AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE |
        AI_ARCH_SPFCTL_L1I_PF_ENABLE);
    second = (second &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK |
            AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        second_threshold | second_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(second);
    AI_ASSERT_NO_EXCEPT_AND("writing the second SPFctl cross configuration before the FENCE.I/fetch wrapper does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("SPFctl readback reflects the second CBO/FENCE.I/fetch no-regression configuration",
        observed_second == second
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t amo_old_second = amoadd_d(amo_vaddr, amo_addend_second);
    TEST_ASSERT("after disabling IPE/AGT/stride in the second SPFctl configuration, a second HS AMO still sees the current value and commits exactly once",
        excpt.triggered == false &&
        amo_old_second == amo_initial + amo_addend_first &&
        read64(amo_vaddr) == amo_initial + amo_addend_first +
            amo_addend_second
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both SPFctl configurations, backing memory reflects the full AMO old/new RMW sequence",
        read64(amo_paddr) == amo_initial + amo_addend_first +
            amo_addend_second
    );

    TEST_ASSERT("after two SPFctl cross configurations, the same-paddr alias rewrite plus FENCE.I fetch wrapper still returns its patched instruction result",
        ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_adjacent_preserved_corner()
    );

    TEST_ASSERT("after IPE was explicitly disabled in SPFctl, the same-paddr alias wrapper still reports a real no-X fetch fault with fresh metadata after FENCE.I",
        ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, original_menvcfg);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original_spfctl);
    AI_ASSERT_NO_EXCEPT_AND("restoring SPFctl after the CBO/FENCE.I/fetch no-regression cross-reprogramming corner does not raise an exception",
        true
    );
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_spfctl_pbmt_nc_io_prefetch_silent_no_side_effect_corner()
{
    TEST_START();

    static uint64_t nc_backing[] __attribute__((aligned(64))) = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
    };
    const uintptr_t nc_paddr = (uintptr_t)&nc_backing[0];
    const uintptr_t nc_adjacent_paddr = (uintptr_t)&nc_backing[1];
    const uintptr_t io_paddr =
        ai_arch_custom_prefetch_io_backing_paddr() + 0x100U;
    pte_t *nc_leaf = 0;
    pte_t nc_leaf_before = 0;
    pte_t nc_leaf_after_prefetch = 0;
    pte_t *io_leaf = 0;
    pte_t io_leaf_before = 0;
    pte_t io_leaf_after_prefetch = 0;
    const uint64_t nc_expected = 0x1021324354657687ULL;
    const uint64_t nc_adjacent = 0x89abcdef01234567ULL;
    const uint64_t nc_updated = 0xfedcba9876543210ULL;
    const uint64_t io_expected = 0x0badf00d12345678ULL;
    const uint64_t io_adjacent = 0x1122aabb3344ccddULL;
    const uint64_t io_updated = 0x5566778899aabbccULL;
    const uint64_t scause_seed = 0x5a5ULL;
    const uint64_t stval_seed = 0ULL;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original_spfctl = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before the PBMT NC/IO prefetch silent no-side-effect corner",
        true
    );

    uint64_t reprogrammed = original_spfctl |
        AI_ARCH_SPFCTL_L1I_PF_ENABLE |
        AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE;
    reprogrammed = (reprogrammed &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK |
            AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        (0x3ULL << 6) | (0x7ULL << 10);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("programming SPFctl before PBMT NC/IO software prefetch probes does not raise an exception",
        true
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    *ai_micro_prefetch_hs_leaf_entry(AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE) =
        (ai_micro_prefetch_alias_anchor(
            AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE,
            nc_paddr & PAGE_ADDR_MSK) >> 2) |
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    nc_leaf = ai_micro_prefetch_hs_leaf_entry(
        AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE);
    nc_leaf_before = *nc_leaf;
    sfence_vma();
    const uintptr_t nc_vaddr =
        hs_page_base(AI_MICRO_PREFETCH_PBMT_NC_ALIAS_PAGE) +
        (nc_paddr & (PAGE_SIZE - 1));
    const uintptr_t nc_adjacent_vaddr = nc_vaddr + sizeof(uint64_t);

    goto_priv(PRIV_HS);
    CSRW(CSR_SCAUSE, scause_seed);
    CSRW(CSR_STVAL, stval_seed);

    TEST_SETUP_EXCEPT();
    uint64_t prefetch_r_rd = ai_arch_custom_prefetch_r_gpr_probe(
        nc_vaddr, 0x1111222233334444ULL);
    TEST_ASSERT("with SPFctl reprogrammed, prefetch.r through a PBMT=NC alias stays silent, preserves S trap CSRs, and cannot write a GPR destination",
        excpt.triggered == false &&
        prefetch_r_rd == 0x1111222233334444ULL &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t prefetch_w_rd = ai_arch_custom_prefetch_w_gpr_probe(
        nc_vaddr, 0x5555666677778888ULL);
    TEST_ASSERT("with SPFctl reprogrammed, prefetch.w through the same PBMT=NC alias also stays silent without GPR or trap-CSR side effects",
        excpt.triggered == false &&
        prefetch_w_rd == 0x5555666677778888ULL &&
        CSRR(CSR_SCAUSE) == scause_seed &&
        CSRR(CSR_STVAL) == stval_seed
    );

    goto_priv(PRIV_M);
    nc_leaf_after_prefetch = *nc_leaf;
    TEST_ASSERT("PBMT=NC prefetch probes do not set or clear PTE A/D or PBMT bits by themselves",
        nc_leaf_after_prefetch == nc_leaf_before
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t nc_observed = read64(nc_vaddr);
    TEST_ASSERT("after silent PBMT=NC prefetch probes, the real load still reads the original payload",
        excpt.triggered == false &&
        nc_observed == nc_expected
    );

    TEST_SETUP_EXCEPT();
    write64(nc_vaddr, nc_updated);
    TEST_ASSERT("after silent PBMT=NC prefetch probes, a real store still updates the NC alias only when explicitly executed",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t nc_adjacent_observed = read64(nc_adjacent_vaddr);
    TEST_ASSERT("PBMT=NC prefetch probes and the later real store preserve the adjacent NC word",
        excpt.triggered == false &&
        nc_adjacent_observed == nc_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=NC prefetch probes do not create a hidden backing-memory writeback side effect",
        read64(nc_paddr) == nc_updated &&
        read64(nc_adjacent_paddr) == nc_adjacent
    );

    uintptr_t io_vaddr = ai_arch_custom_prefetch_prepare_io_alias() + 0x100U;
    uintptr_t io_adjacent_vaddr = io_vaddr + sizeof(uint64_t);
    write64(io_paddr, io_expected);
    write64(io_paddr + sizeof(uint64_t), io_adjacent);
    io_leaf = ai_micro_prefetch_hs_leaf_entry(AI_MICRO_PREFETCH_IO_ALIAS_PAGE);
    io_leaf_before = *io_leaf;

    goto_priv(PRIV_HS);
    CSRW(CSR_SCAUSE, scause_seed + 1ULL);
    CSRW(CSR_STVAL, stval_seed);

    TEST_SETUP_EXCEPT();
    uint64_t io_prefetch_rd = ai_arch_custom_prefetch_r_gpr_probe(
        io_vaddr, 0x9999aaaabbbbccccULL);
    TEST_ASSERT("with SPFctl reprogrammed, prefetch.r through a PBMT=IO/Device-style alias retires silently and leaves GPR and S trap CSR state unchanged",
        excpt.triggered == false &&
        io_prefetch_rd == 0x9999aaaabbbbccccULL &&
        CSRR(CSR_SCAUSE) == scause_seed + 1ULL &&
        CSRR(CSR_STVAL) == stval_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t io_prefetch_i_rd = ai_arch_custom_prefetch_i_gpr_probe(
        io_vaddr, 0xddddeeeeffff0001ULL);
    TEST_ASSERT("with SPFctl reprogrammed, prefetch.i through the PBMT=IO/Device-style alias is also silent and cannot create data-side state changes",
        excpt.triggered == false &&
        io_prefetch_i_rd == 0xddddeeeeffff0001ULL &&
        CSRR(CSR_SCAUSE) == scause_seed + 1ULL &&
        CSRR(CSR_STVAL) == stval_seed
    );

    goto_priv(PRIV_M);
    io_leaf_after_prefetch = *io_leaf;
    TEST_ASSERT("PBMT=IO/Device-style prefetch probes do not set or clear PTE A/D or PBMT bits by themselves",
        io_leaf_after_prefetch == io_leaf_before
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t io_observed = read64(io_vaddr);
    TEST_ASSERT("after silent PBMT=IO/Device-style prefetch probes, a real load still returns the target payload",
        excpt.triggered == false &&
        io_observed == io_expected
    );

    TEST_SETUP_EXCEPT();
    write64(io_vaddr, io_updated);
    TEST_ASSERT("after silent PBMT=IO/Device-style prefetch probes, an explicit store remains the only operation that changes the target payload",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t io_adjacent_observed = read64(io_adjacent_vaddr);
    TEST_ASSERT("PBMT=IO/Device-style prefetch probes and the later explicit store preserve the adjacent word",
        excpt.triggered == false &&
        io_adjacent_observed == io_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=IO/Device-style prefetch probes do not create hidden target or adjacent backing-memory side effects",
        read64(io_paddr) == io_updated &&
        read64(io_paddr + sizeof(uint64_t)) == io_adjacent
    );

    ai_arch_custom_prefetch_reset_pbmt_env();
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original_spfctl);
    AI_ASSERT_NO_EXCEPT_AND("restoring SPFctl after the PBMT NC/IO prefetch silent no-side-effect corner does not raise an exception",
        true
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_enable_small_threshold_ldld_replay_progress_corner()
{
    TEST_START();

    const uint64_t small_threshold = 0x1ULL << 1;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before enabling replay_escape with a small threshold for the replay-sensitive wrapper corner",
        true
    );

    uint64_t reprogrammed = (original & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) |
        AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE | small_threshold;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("enabling replay_escape and shrinking its threshold does not raise an exception before the ld-ld replay-sensitive wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl readback reflects replay_escape enabled with the requested small threshold before the ld-ld replay-sensitive wrapper corner",
        observed == reprogrammed
    );

    bool wrapped_passed = ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner();
    TEST_ASSERT("with replay_escape enabled and a small threshold, the ld-ld replay plus nearby writeback regression corner still completes without getting stuck",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the replay_escape-enabled replay-sensitive wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl returns to its original value after the replay_escape-enabled replay-sensitive wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_disable_small_threshold_dense_replay_no_regression_corner()
{
    TEST_START();

    const uint64_t small_threshold = 0x1ULL << 1;

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl is readable before disabling replay_escape with a small threshold for the dense replay wrapper corner",
        true
    );

    uint64_t reprogrammed = ((original & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE) &
        ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK) | small_threshold;

    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("clearing replay_escape while shrinking its threshold does not raise an exception before the dense replay wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl readback reflects replay_escape cleared with the requested small threshold before the dense replay wrapper corner",
        observed == reprogrammed &&
        (observed & AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE) == 0ULL
    );

    bool wrapped_passed = ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner();
    TEST_ASSERT("with replay_escape disabled and a small threshold, the dense replay/store-boundary regression corner still keeps its architecturally visible no-stuck result",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_smblockctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring smblockctl after the replay_escape-disabled dense replay wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_smblockctl();
    AI_ASSERT_NO_EXCEPT_AND("smblockctl returns to its original value after the replay_escape-disabled dense replay wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_agt_pht_disable_repeated_page_fault_prefetch_silent_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before disabling AGT/PHT for the repeated page-fault-class software-prefetch wrapper corner",
        true
    );

    uint64_t disabled = original & ~(AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing AGT/PHT in spfctl does not raise an exception before the repeated page-fault-class software-prefetch wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT cleared before the repeated page-fault-class software-prefetch wrapper corner",
        observed_disabled == disabled
    );

    bool wrapped_passed = ai_micro_hs_prefetch_repeated_page_fault_silent_corner();
    TEST_ASSERT("with AGT/PHT disabled in spfctl, repeated page-fault-class software prefetch probes still retire silently and keep the existing architectural result stable",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the repeated page-fault-class software-prefetch wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the repeated page-fault-class software-prefetch wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_train_on_hit_toggle_prefetch_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before toggling train_on_hit for the software-prefetch fault-then-recover wrapper corner",
        true
    );

    uint64_t reprogrammed = (original | AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT) ^
        AI_ARCH_SPFCTL_TRAIN_ON_HIT;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("toggling train_on_hit while seeding AGT/PHT enabled does not raise an exception before the software-prefetch fault-then-recover wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the requested train_on_hit toggle before the software-prefetch fault-then-recover wrapper corner",
        observed == reprogrammed
    );

    bool wrapped_passed = ai_micro_hs_prefetch_page_fault_then_valid_access_recovers_corner();
    TEST_ASSERT("after toggling train_on_hit, the software-prefetch page-fault-class path still recovers cleanly to a later valid access",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the software-prefetch fault-then-recover wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the software-prefetch fault-then-recover wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_train_on_hit_toggle_prefetch_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before toggling train_on_hit for the adjacent-preserved software-prefetch fault-then-recover wrapper corner",
        true
    );

    uint64_t reprogrammed = (original | AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT) ^
        AI_ARCH_SPFCTL_TRAIN_ON_HIT;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(reprogrammed);
    AI_ASSERT_NO_EXCEPT_AND("toggling train_on_hit while seeding AGT/PHT enabled does not raise an exception before the adjacent-preserved software-prefetch fault-then-recover wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the requested train_on_hit toggle before the adjacent-preserved software-prefetch fault-then-recover wrapper corner",
        observed == reprogrammed
    );

    bool wrapped_passed = ai_micro_hs_prefetch_page_fault_then_valid_access_adjacent_preserved_corner();
    TEST_ASSERT("after toggling train_on_hit, the software-prefetch page-fault-class path still recovers cleanly while preserving the recovery target's adjacent word",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the adjacent-preserved software-prefetch fault-then-recover wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the adjacent-preserved software-prefetch fault-then-recover wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_agt_pht_disable_prefetch_access_fault_then_valid_access_recovers_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before disabling AGT/PHT for the access-fault-class software-prefetch recovery wrapper corner",
        true
    );

    uint64_t disabled = original & ~(AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing AGT/PHT in spfctl does not raise an exception before the access-fault-class software-prefetch recovery wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT cleared before the access-fault-class software-prefetch recovery wrapper corner",
        observed_disabled == disabled
    );

    bool wrapped_passed = ai_micro_hs_prefetch_access_fault_then_valid_access_recovers_corner();
    TEST_ASSERT("with AGT/PHT disabled in spfctl, the software-prefetch access-fault-class path still recovers cleanly to a later valid access",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the access-fault-class software-prefetch recovery wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the access-fault-class software-prefetch recovery wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_agt_pht_disable_prefetch_access_fault_then_valid_access_adjacent_preserved_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before disabling AGT/PHT for the adjacent-preserved access-fault-class software-prefetch recovery wrapper corner",
        true
    );

    uint64_t disabled = original & ~(AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing AGT/PHT in spfctl does not raise an exception before the adjacent-preserved access-fault-class software-prefetch recovery wrapper corner",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_disabled = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT cleared before the adjacent-preserved access-fault-class software-prefetch recovery wrapper corner",
        observed_disabled == disabled
    );

    bool wrapped_passed = ai_micro_hs_prefetch_access_fault_then_valid_access_adjacent_preserved_corner();
    TEST_ASSERT("with AGT/PHT disabled in spfctl, the software-prefetch access-fault-class path still recovers cleanly while preserving the recovery target's adjacent word",
        wrapped_passed
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the adjacent-preserved access-fault-class software-prefetch recovery wrapper corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the adjacent-preserved access-fault-class software-prefetch recovery wrapper corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_cross_reprogram_twice_stream_stable_corner()
{
    TEST_START();

    const uint64_t value0 = 0x3141592653589793ULL;
    const uint64_t value1 = 0x2718281828459045ULL;
    const uint64_t value2 = 0x1123581321345589ULL;
    const uint64_t stored3 = 0x8877665544332211ULL;
    const uint64_t first_threshold = 0x2ULL << 6;
    const uint64_t first_stride = 0x8ULL << 10;
    const uint64_t second_threshold = 0x7ULL << 6;
    const uint64_t second_stride = 0x12ULL << 10;

    uintptr_t vaddr0 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD0_OFF);
    uintptr_t vaddr1 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD1_OFF);
    uintptr_t vaddr2 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD2_OFF);
    uintptr_t vaddr3 = ai_arch_custom_vaddr(AI_ARCH_CUSTOM_WORD3_OFF);

    ai_arch_custom_seed_four_words(value0, value1, value2, 0ULL);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before the two-step AGT/PHT/train_on_hit/threshold/stride cross reprogramming corner",
        true
    );

    uint64_t first = ((original | AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT | AI_ARCH_SPFCTL_ENABLE_STRIDE) &
        ~AI_ARCH_SPFCTL_ENABLE_PHT);
    first = (first &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        first_threshold | first_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(first);
    AI_ASSERT_NO_EXCEPT_AND("writing the first AGT/PHT/train_on_hit/threshold/stride cross configuration does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the first AGT/PHT/train_on_hit/threshold/stride cross configuration",
        observed_first == first
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_probe = read64(vaddr0);
    TEST_ASSERT("after the first cross reprogramming of spfctl, an explicit HS load still returns the expected payload",
        excpt.triggered == false &&
        first_probe == value0
    );

    goto_priv(PRIV_M);
    uint64_t second = ((first | AI_ARCH_SPFCTL_ENABLE_PHT) &
        ~(AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_STRIDE)) ^
        AI_ARCH_SPFCTL_TRAIN_ON_HIT;
    second = (second &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        second_threshold | second_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(second);
    AI_ASSERT_NO_EXCEPT_AND("writing the second AGT/PHT/train_on_hit/threshold/stride cross configuration does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the second AGT/PHT/train_on_hit/threshold/stride cross configuration",
        observed_second == second
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr0);
    TEST_ASSERT("after two cross reprogrammings of spfctl, prefetch.r still retires silently on a valid HS address",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr1);
    TEST_ASSERT("after two cross reprogrammings of spfctl, prefetch.w still retires silently on a valid HS address",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed0_first = read64(vaddr0);
    uint64_t observed1_first = read64(vaddr1);
    uint64_t observed0_second = read64(vaddr0);
    uint64_t observed2 = read64(vaddr2);
    TEST_ASSERT("after two cross reprogrammings of spfctl, the explicit hot-hit HS load stream still returns the expected payloads",
        excpt.triggered == false &&
        observed0_first == value0 &&
        observed1_first == value1 &&
        observed0_second == value0 &&
        observed2 == value2
    );

    TEST_SETUP_EXCEPT();
    write64(vaddr3, stored3);
    TEST_ASSERT("after two cross reprogrammings of spfctl, an explicit HS store still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed3 = read64(vaddr3);
    TEST_ASSERT("after two cross reprogrammings of spfctl, the explicit HS store remains architecturally visible",
        excpt.triggered == false &&
        observed3 == stored3
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after two cross reprogrammings of spfctl, backing memory still matches the explicit HS store result",
        read64(ai_arch_custom_paddr(AI_ARCH_CUSTOM_WORD3_OFF)) == stored3
    );

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the two-step cross reprogramming corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the two-step cross reprogramming corner completes",
        restored == original
    );

    TEST_END();
}

bool ai_arch_spfctl_sms_agt_pht_disable_then_reenable_stream_observable_corner()
{
    TEST_START();

    static const enum test_page stream_pages[] = {
        VSRWX_GRWX,
    };
    const unsigned page_count = sizeof(stream_pages) / sizeof(stream_pages[0]);
    const unsigned regions_per_page = 4;
    const unsigned lines_per_region = 14;
    const unsigned enabled_start_region = 0;
    const unsigned enabled_region_count = 1;
    const unsigned disabled_start_region = enabled_start_region + enabled_region_count;
    const unsigned disabled_region_count = 1;
    const unsigned reenabled_start_region = disabled_start_region + disabled_region_count;
    const unsigned reenabled_region_count = 1;
    const unsigned total_region_count = page_count * regions_per_page;
    const uint64_t enabled_threshold = 0xcULL << 6;
    const uint64_t enabled_stride = 0x1ULL << 10;
    const uint64_t enable_seed_base = 0x0102030405060708ULL;
    const uint64_t disable_seed_base = 0x4142434445464748ULL;
    const uint64_t reenable_seed_base = 0x8182838485868788ULL;
    uint64_t original = 0;
    uint64_t enabled = 0;
    uint64_t disabled = 0;
    uint64_t observed_enabled_cfg = 0;
    uint64_t observed_disabled_cfg = 0;
    uint64_t observed_reenabled_cfg = 0;
    bool warmup_ok = true;
    bool enabled_match = true;
    bool disabled_match = true;
    bool reenabled_match = true;

    TEST_ASSERT("the SMS AGT/PHT enable-disable-reenable observability corner reserves enough HS RW regions to cover the three single-region continuous-cacheline windows",
        reenabled_start_region + reenabled_region_count <= total_region_count
    );

    goto_priv(PRIV_M);
    ai_arch_sms_seed_region_stream(
        stream_pages,
        enabled_start_region,
        enabled_region_count,
        lines_per_region,
        regions_per_page,
        enable_seed_base
    );
    ai_arch_sms_seed_region_stream(
        stream_pages,
        disabled_start_region,
        disabled_region_count,
        lines_per_region,
        regions_per_page,
        disable_seed_base
    );
    ai_arch_sms_seed_region_stream(
        stream_pages,
        reenabled_start_region,
        reenabled_region_count,
        lines_per_region,
        regions_per_page,
        reenable_seed_base
    );
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_sms_store_region_stream(
        stream_pages,
        enabled_start_region,
        enabled_region_count,
        lines_per_region,
        regions_per_page,
        enable_seed_base
    );
    ai_arch_sms_store_region_stream(
        stream_pages,
        disabled_start_region,
        disabled_region_count,
        lines_per_region,
        regions_per_page,
        disable_seed_base
    );
    ai_arch_sms_store_region_stream(
        stream_pages,
        reenabled_start_region,
        reenabled_region_count,
        lines_per_region,
        regions_per_page,
        reenable_seed_base
    );
    asm volatile("fence rw, rw" ::: "memory");
    warmup_ok = excpt.triggered == false;
    TEST_ASSERT("a store-only HS cache warm-up across the three single-region continuous-cacheline windows completes without an exception so the later fixed-PC load phases can reuse the exact same 14-line spans",
        warmup_ok
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before the SMS AGT/PHT enable-disable-reenable observability corner",
        true
    );

    enabled = (original |
        AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT);
    enabled = (enabled &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        enabled_threshold | enabled_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(enabled);
    AI_ASSERT_NO_EXCEPT_AND("programming spfctl with AGT/PHT and train_on_hit enabled plus the default active-threshold semantics and a unit stride does not raise an exception before the first 14-cacheline stream",
        true
    );

    TEST_SETUP_EXCEPT();
    observed_enabled_cfg = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the enabled AGT/PHT/train_on_hit single-region 14-cacheline SMS configuration before the first stream",
        observed_enabled_cfg == enabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    enabled_match = ai_arch_sms_check_region_stream(
        stream_pages,
        enabled_start_region,
        enabled_region_count,
        lines_per_region,
        regions_per_page,
        enable_seed_base
    );
    TEST_ASSERT("with AGT/PHT and train_on_hit enabled, the first store-warmed fixed-PC HS load window continuously visits 14 cachelines inside one region so the stream crosses the default active-threshold-class SMS trigger depth while still returning the expected payloads",
        excpt.triggered == false &&
        enabled_match
    );

    goto_priv(PRIV_M);
    disabled = enabled & ~(AI_ARCH_SPFCTL_ENABLE_AGT | AI_ARCH_SPFCTL_ENABLE_PHT);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing the AGT/PHT bits in spfctl does not raise an exception before the second observable stream",
        true
    );

    TEST_SETUP_EXCEPT();
    observed_disabled_cfg = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT cleared before the second stream",
        observed_disabled_cfg == disabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    disabled_match = ai_arch_sms_check_region_stream(
        stream_pages,
        disabled_start_region,
        disabled_region_count,
        lines_per_region,
        regions_per_page,
        disable_seed_base
    );
    TEST_ASSERT("with AGT/PHT cleared while train_on_hit stays seeded, the second store-warmed fixed-PC HS load window still continuously visits 14 cachelines inside one region and returns the expected payloads for disabled-window comparison",
        excpt.triggered == false &&
        disabled_match
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(enabled);
    AI_ASSERT_NO_EXCEPT_AND("restoring the AGT/PHT-and-train_on_hit-enabled SMS configuration does not raise an exception before the third observable stream",
        true
    );

    TEST_SETUP_EXCEPT();
    observed_reenabled_cfg = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT/train_on_hit re-enabled before the third stream",
        observed_reenabled_cfg == enabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    reenabled_match = ai_arch_sms_check_region_stream(
        stream_pages,
        reenabled_start_region,
        reenabled_region_count,
        lines_per_region,
        regions_per_page,
        reenable_seed_base
    );
    TEST_ASSERT("after re-enabling AGT/PHT and train_on_hit, the third store-warmed fixed-PC HS load window again continuously visits 14 cachelines inside one region and returns the expected payloads while reopening the enabled-window comparison point",
        excpt.triggered == false &&
        reenabled_match
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the SMS AGT/PHT enable-disable-reenable observability corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the SMS AGT/PHT enable-disable-reenable observability corner completes",
        restored == original
    );

    TEST_END();
}

// Manual Xiangshan observability harness: first do a stable same-PC load training sweep
// that exceeds the 16-entry AGT capacity and repeatedly crosses the AGT active threshold,
// then observe fresh low-depth windows under all-on -> all-off -> all-on programming.
// Current RTL expectation:
// - The training sweep stably exercises AGT update/active-page conditions and feeds PHT
//   via AGT evictions from the same load PC.
// - AGT direct generation still stays silent because io.s2_pf_gen_req.valid is hardwired false.B.
// - stride generation stays unavailable because the wrapper pins sms.io_stride_en to false.B.
// - The fresh low-depth all-on windows should therefore expose the still-supported PHT path,
//   while the all-off window should mask all three routes.
bool ai_arch_spfctl_sms_agt_pht_stride_all_on_off_observable_corner()
{
    TEST_START();

    static const enum test_page stream_pages[] = {
        VSRWX_GRWX,
        VSRWX_GRW,
        VSRWX_GRX,
        VSRWX_GR,
        VSRWX_GX,
        VSRWX_GURWX,
    };
    const unsigned page_count = sizeof(stream_pages) / sizeof(stream_pages[0]);
    const unsigned regions_per_page = 4;
    const unsigned training_start_region = 0;
    const unsigned training_region_count = 18;
    const unsigned training_lines_per_region = 14;
    const unsigned training_rounds = 2;
    const unsigned observation_lines_per_region = 4;
    const unsigned observation_rounds = 2;
    const unsigned enabled_start_region = training_start_region + training_region_count;
    const unsigned enabled_region_count = 1;
    const unsigned disabled_start_region = enabled_start_region + enabled_region_count;
    const unsigned disabled_region_count = 1;
    const unsigned reenabled_start_region = disabled_start_region + disabled_region_count;
    const unsigned reenabled_region_count = 1;
    const unsigned total_region_count = page_count * regions_per_page;
    const uint64_t enabled_threshold = 0xcULL << 6;
    const uint64_t enabled_stride = 0x1ULL << 10;
    const uint64_t training_seed_base = 0x1112131415161718ULL;
    const uint64_t enable_seed_base = 0x5152535455565758ULL;
    const uint64_t disable_seed_base = 0x9192939495969798ULL;
    const uint64_t reenable_seed_base = 0xd1d2d3d4d5d6d7d8ULL;
    uint64_t original = 0;
    uint64_t all_enabled = 0;
    uint64_t all_disabled = 0;
    uint64_t observed_enabled_cfg = 0;
    uint64_t observed_disabled_cfg = 0;
    uint64_t observed_reenabled_cfg = 0;
    bool training_match = true;
    bool enabled_match = true;
    bool disabled_match = true;
    bool reenabled_match = true;

    TEST_ASSERT("the SMS AGT/PHT/enable_stride stable-training all-on-off-on observability corner reserves enough HS RW regions to cover the 18-region fixed-PC training sweep plus three fresh single-region observation windows",
        reenabled_start_region + reenabled_region_count <= total_region_count
    );

    goto_priv(PRIV_M);
    ai_arch_sms_seed_region_stream(
        stream_pages,
        training_start_region,
        training_region_count,
        training_lines_per_region,
        regions_per_page,
        training_seed_base
    );
    ai_arch_sms_seed_region_stream(
        stream_pages,
        enabled_start_region,
        enabled_region_count,
        observation_lines_per_region,
        regions_per_page,
        enable_seed_base
    );
    ai_arch_sms_seed_region_stream(
        stream_pages,
        disabled_start_region,
        disabled_region_count,
        observation_lines_per_region,
        regions_per_page,
        disable_seed_base
    );
    ai_arch_sms_seed_region_stream(
        stream_pages,
        reenabled_start_region,
        reenabled_region_count,
        observation_lines_per_region,
        regions_per_page,
        reenable_seed_base
    );
    ai_arch_custom_prepare_hs_rw_page();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    original = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl is readable before the SMS AGT/PHT/enable_stride all-on-off-on observability corner",
        true
    );

    all_enabled = (original |
        AI_ARCH_SPFCTL_L2_PF_ENABLE |
        AI_ARCH_SPFCTL_L1D_PF_ENABLE |
        AI_ARCH_SPFCTL_TRAIN_ON_HIT |
        AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE);
    all_enabled = (all_enabled &
        ~(AI_ARCH_SPFCTL_ACTIVE_THRESHOLD_MASK | AI_ARCH_SPFCTL_ACTIVE_STRIDE_MASK)) |
        enabled_threshold | enabled_stride;

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(all_enabled);
    AI_ASSERT_NO_EXCEPT_AND("programming spfctl with AGT/PHT/enable_stride all set plus train_on_hit and the default active-threshold semantics does not raise an exception before the first three-path observation window",
        true
    );

    TEST_SETUP_EXCEPT();
    observed_enabled_cfg = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the AGT/PHT/enable_stride all-enabled observation configuration before the first fixed-PC hot-hit stream",
        observed_enabled_cfg == all_enabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    training_match = ai_arch_sms_check_region_stream_rounds(
        stream_pages,
        training_start_region,
        training_region_count,
        training_lines_per_region,
        regions_per_page,
        training_seed_base,
        training_rounds
    );
    TEST_ASSERT("with AGT/PHT/enable_stride all programmed on, a same-PC 18-region x14-line x2-round HS load training sweep crosses the AGT active threshold on every region and exceeds the 16-entry AGT capacity, creating stable AGT-trigger and PHT-training pressure while still returning the expected payloads",
        excpt.triggered == false &&
        training_match
    );

    TEST_SETUP_EXCEPT();
    enabled_match = ai_arch_sms_check_region_stream_rounds(
        stream_pages,
        enabled_start_region,
        enabled_region_count,
        observation_lines_per_region,
        regions_per_page,
        enable_seed_base,
        observation_rounds
    );
    TEST_ASSERT("with AGT/PHT/enable_stride all programmed on, a fresh same-PC 4-line x2-round HS load observation window stays below the AGT active threshold while reusing the already-trained PHT PC, opening a stable all-on observation point where PHT may still issue while AGT direct generation and stride generation remain suppressed by current RTL",
        excpt.triggered == false &&
        enabled_match
    );

    goto_priv(PRIV_M);
    all_disabled = all_enabled & ~(AI_ARCH_SPFCTL_ENABLE_AGT |
        AI_ARCH_SPFCTL_ENABLE_PHT |
        AI_ARCH_SPFCTL_ENABLE_STRIDE);

    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(all_disabled);
    AI_ASSERT_NO_EXCEPT_AND("clearing the AGT/PHT/enable_stride bits together does not raise an exception before the second three-path observation window",
        true
    );

    TEST_SETUP_EXCEPT();
    observed_disabled_cfg = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects AGT/PHT/enable_stride all cleared before the second fixed-PC hot-hit stream",
        observed_disabled_cfg == all_disabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    disabled_match = ai_arch_sms_check_region_stream_rounds(
        stream_pages,
        disabled_start_region,
        disabled_region_count,
        observation_lines_per_region,
        regions_per_page,
        disable_seed_base,
        observation_rounds
    );
    TEST_ASSERT("with AGT/PHT/enable_stride all cleared, a fresh same-PC 4-line x2-round HS load observation window keeps the explicit data path stable while masking all three SMS generation routes for the all-off comparison point",
        excpt.triggered == false &&
        disabled_match
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(all_enabled);
    AI_ASSERT_NO_EXCEPT_AND("re-enabling AGT/PHT/enable_stride together does not raise an exception before the third three-path observation window",
        true
    );

    TEST_SETUP_EXCEPT();
    observed_reenabled_cfg = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl readback reflects the AGT/PHT/enable_stride all-enabled configuration again before the third fixed-PC hot-hit stream",
        observed_reenabled_cfg == all_enabled
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    reenabled_match = ai_arch_sms_check_region_stream_rounds(
        stream_pages,
        reenabled_start_region,
        reenabled_region_count,
        observation_lines_per_region,
        regions_per_page,
        reenable_seed_base,
        observation_rounds
    );
    TEST_ASSERT("after re-enabling AGT/PHT/enable_stride together, a second fresh same-PC 4-line x2-round HS load observation window again returns the expected payloads, reopening the stable all-on comparison point with independently chosen region state",
        excpt.triggered == false &&
        reenabled_match
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_write_spfctl(original);
    AI_ASSERT_NO_EXCEPT_AND("restoring spfctl after the SMS AGT/PHT/enable_stride all-on-off-on observability corner does not raise an exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ai_arch_read_spfctl();
    AI_ASSERT_NO_EXCEPT_AND("spfctl returns to its original value after the SMS AGT/PHT/enable_stride all-on-off-on observability corner completes",
        restored == original
    );

    TEST_END();
}
