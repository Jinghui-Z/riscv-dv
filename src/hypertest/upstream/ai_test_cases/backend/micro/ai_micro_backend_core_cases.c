#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#ifndef CSR_SBPCTL
#define CSR_SBPCTL 0x5c0
#endif

#ifndef CSR_SPFCTL
#define CSR_SPFCTL 0x5c1
#endif

#ifndef CSR_SMBLOCKCTL
#define CSR_SMBLOCKCTL 0x5c3
#endif

#ifndef CSR_SRNCTL
#define CSR_SRNCTL 0x5c4
#endif

#define AI_BACKEND_BRANCH_TRAINING_ROUNDS 64U
#define AI_BACKEND_CHAIN_ITERATIONS        48U
#define AI_BACKEND_TIGHT_BRANCH_COUNT      8U
#define AI_BACKEND_BURST_BRANCH_COUNT      12U
#define AI_BACKEND_SKEW_BRANCH_VARIANTS    4U
#define AI_BACKEND_SKEW_TRAINING_ROUNDS    6U
#define AI_BACKEND_P1C_DENSE_CFI_TRAIN_ROUNDS 8U
#define AI_BACKEND_P1C_DENSE_CFI_EPOCHS       4U
#define AI_BACKEND_P1C_DELAYED_TRAIN_ROUNDS   24U

#define AI_BACKEND_SBPCTL_DEFINED_MASK      0x7fULL
#define AI_BACKEND_SPFCTL_DEFINED_MASK      ((1ULL << 18) - 1ULL)
#define AI_BACKEND_SMBLOCKCTL_DEFINED_MASK  0xffULL
#define AI_BACKEND_SRNCTL_DEFINED_MASK      0xfULL
#define AI_BACKEND_SRNCTL_FUSION_ENABLE     (1ULL << 0)
#define AI_BACKEND_SRNCTL_WFI_ENABLE        (1ULL << 2)

static inline uintptr_t ai_backend_vaddr(uintptr_t offset)
{
    return hs_page_base(VSRWX_GRWX) + offset;
}

static inline uintptr_t ai_backend_paddr(uintptr_t offset)
{
    return phys_page_base(VSRWX_GRWX) + offset;
}

static inline void ai_backend_prepare_hs_cacheable_page(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline void ai_backend_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_backend_read_custom_csr(unsigned csr)
{
    uint64_t value = 0ULL;

    switch (csr) {
    case CSR_SBPCTL:
        asm volatile("csrr %0, 0x5c0" : "=r"(value) :: "memory");
        break;
    case CSR_SPFCTL:
        asm volatile("csrr %0, 0x5c1" : "=r"(value) :: "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrr %0, 0x5c3" : "=r"(value) :: "memory");
        break;
    case CSR_SRNCTL:
        asm volatile("csrr %0, 0x5c4" : "=r"(value) :: "memory");
        break;
    default:
        break;
    }

    return value;
}

static inline void ai_backend_write_custom_csr(unsigned csr, uint64_t value)
{
    switch (csr) {
    case CSR_SBPCTL:
        asm volatile("csrw 0x5c0, %0" :: "rK"(value) : "memory");
        break;
    case CSR_SPFCTL:
        asm volatile("csrw 0x5c1, %0" :: "rK"(value) : "memory");
        break;
    case CSR_SMBLOCKCTL:
        asm volatile("csrw 0x5c3, %0" :: "rK"(value) : "memory");
        break;
    case CSR_SRNCTL:
        asm volatile("csrw 0x5c4, %0" :: "rK"(value) : "memory");
        break;
    default:
        break;
    }
}

static inline bool ai_backend_custom_write_read_no_except(
    unsigned csr, uint64_t value, uint64_t *observed)
{
    TEST_SETUP_EXCEPT();
    ai_backend_write_custom_csr(csr, value);
    if (excpt.triggered) {
        return false;
    }

    TEST_SETUP_EXCEPT();
    *observed = ai_backend_read_custom_csr(csr);
    return excpt.triggered == false;
}

static inline pte_t *ai_backend_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_backend_map_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_backend_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

__attribute__((naked, noinline, aligned(4))) static void
ai_backend_branch_redirect_window_a(uintptr_t wrong_vaddr,
    uintptr_t right_vaddr, uintptr_t marker_vaddr, uint64_t skipped_seed,
    uintptr_t execute_wrong_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, a3\n"
        "beqz a4, 1f\n"
        "ld t0, 0(a0)\n"
        "1:\n"
        "sd t0, 0(a2)\n"
        "ld t1, 0(a1)\n"
        "sd t1, 8(a2)\n"
        "sd t0, 16(a2)\n"
        "fence rw, rw\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_backend_branch_redirect_window_b(uintptr_t wrong_vaddr,
    uintptr_t right_vaddr, uintptr_t marker_vaddr, uint64_t skipped_seed,
    uintptr_t execute_wrong_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, a3\n"
        "beqz a4, 1f\n"
        "ld t0, 0(a0)\n"
        "1:\n"
        "sd t0, 0(a2)\n"
        "ld t1, 0(a1)\n"
        "sd t1, 8(a2)\n"
        "sd t0, 16(a2)\n"
        "fence rw, rw\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "memory");
}

__attribute__((noinline, aligned(64))) static void
ai_backend_branch_redirect_cluster_a(uintptr_t wrong_vaddr,
    uintptr_t right_vaddr, uintptr_t marker_vaddr, uint64_t skipped_seed,
    uintptr_t execute_wrong_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 1f\n"
        "ld t0, 0(%[wrong])\n"
        "1:\n"
        "sd t0, 0(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 2f\n"
        "ld t0, 0(%[wrong])\n"
        "2:\n"
        "sd t0, 8(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 3f\n"
        "ld t0, 0(%[wrong])\n"
        "3:\n"
        "sd t0, 16(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 4f\n"
        "ld t0, 0(%[wrong])\n"
        "4:\n"
        "sd t0, 24(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 5f\n"
        "ld t0, 0(%[wrong])\n"
        "5:\n"
        "sd t0, 32(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 6f\n"
        "ld t0, 0(%[wrong])\n"
        "6:\n"
        "sd t0, 40(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 7f\n"
        "ld t0, 0(%[wrong])\n"
        "7:\n"
        "sd t0, 48(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 8f\n"
        "ld t0, 0(%[wrong])\n"
        "8:\n"
        "sd t0, 56(%[marker])\n"
        "ld t1, 0(%[right])\n"
        "sd t1, 64(%[marker])\n"
        ".option pop\n"
        :
        : [wrong] "r"(wrong_vaddr),
          [right] "r"(right_vaddr),
          [marker] "r"(marker_vaddr),
          [skipped] "r"(skipped_seed),
          [execute_wrong] "r"(execute_wrong_path)
        : "t0", "t1", "memory");
}

__attribute__((noinline, aligned(64))) static void
ai_backend_branch_redirect_cluster_b(uintptr_t wrong_vaddr,
    uintptr_t right_vaddr, uintptr_t marker_vaddr, uint64_t skipped_seed,
    uintptr_t execute_wrong_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 1f\n"
        "ld t0, 0(%[wrong])\n"
        "1:\n"
        "sd t0, 0(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 2f\n"
        "ld t0, 0(%[wrong])\n"
        "2:\n"
        "sd t0, 8(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 3f\n"
        "ld t0, 0(%[wrong])\n"
        "3:\n"
        "sd t0, 16(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 4f\n"
        "ld t0, 0(%[wrong])\n"
        "4:\n"
        "sd t0, 24(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 5f\n"
        "ld t0, 0(%[wrong])\n"
        "5:\n"
        "sd t0, 32(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 6f\n"
        "ld t0, 0(%[wrong])\n"
        "6:\n"
        "sd t0, 40(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 7f\n"
        "ld t0, 0(%[wrong])\n"
        "7:\n"
        "sd t0, 48(%[marker])\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 8f\n"
        "ld t0, 0(%[wrong])\n"
        "8:\n"
        "sd t0, 56(%[marker])\n"
        "ld t1, 0(%[right])\n"
        "sd t1, 64(%[marker])\n"
        ".option pop\n"
        :
        : [wrong] "r"(wrong_vaddr),
          [right] "r"(right_vaddr),
          [marker] "r"(marker_vaddr),
          [skipped] "r"(skipped_seed),
          [execute_wrong] "r"(execute_wrong_path)
        : "t0", "t1", "memory");
}

__attribute__((noinline, aligned(64))) static void
ai_backend_branch_redirect_burst_a(uintptr_t right_vaddr,
    uintptr_t marker_vaddr, uint64_t skipped_seed,
    uintptr_t execute_wrong_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 1f\n"
        "addi t0, t0, 1\n"
        "1:\n"
        "beqz %[execute_wrong], 2f\n"
        "addi t0, t0, 1\n"
        "2:\n"
        "beqz %[execute_wrong], 3f\n"
        "addi t0, t0, 1\n"
        "3:\n"
        "beqz %[execute_wrong], 4f\n"
        "addi t0, t0, 1\n"
        "4:\n"
        "beqz %[execute_wrong], 5f\n"
        "addi t0, t0, 1\n"
        "5:\n"
        "beqz %[execute_wrong], 6f\n"
        "addi t0, t0, 1\n"
        "6:\n"
        "beqz %[execute_wrong], 7f\n"
        "addi t0, t0, 1\n"
        "7:\n"
        "beqz %[execute_wrong], 8f\n"
        "addi t0, t0, 1\n"
        "8:\n"
        "beqz %[execute_wrong], 9f\n"
        "addi t0, t0, 1\n"
        "9:\n"
        "beqz %[execute_wrong], 10f\n"
        "addi t0, t0, 1\n"
        "10:\n"
        "beqz %[execute_wrong], 11f\n"
        "addi t0, t0, 1\n"
        "11:\n"
        "beqz %[execute_wrong], 12f\n"
        "addi t0, t0, 1\n"
        "12:\n"
        "sd t0, 0(%[marker])\n"
        "ld t1, 0(%[right])\n"
        "sd t1, 8(%[marker])\n"
        ".option pop\n"
        :
        : [right] "r"(right_vaddr),
          [marker] "r"(marker_vaddr),
          [skipped] "r"(skipped_seed),
          [execute_wrong] "r"(execute_wrong_path)
        : "t0", "t1", "memory");
}

__attribute__((noinline, aligned(64))) static void
ai_backend_branch_redirect_burst_b(uintptr_t right_vaddr,
    uintptr_t marker_vaddr, uint64_t skipped_seed,
    uintptr_t execute_wrong_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t0, %[skipped]\n"
        "beqz %[execute_wrong], 1f\n"
        "addi t0, t0, 1\n"
        "1:\n"
        "beqz %[execute_wrong], 2f\n"
        "addi t0, t0, 1\n"
        "2:\n"
        "beqz %[execute_wrong], 3f\n"
        "addi t0, t0, 1\n"
        "3:\n"
        "beqz %[execute_wrong], 4f\n"
        "addi t0, t0, 1\n"
        "4:\n"
        "beqz %[execute_wrong], 5f\n"
        "addi t0, t0, 1\n"
        "5:\n"
        "beqz %[execute_wrong], 6f\n"
        "addi t0, t0, 1\n"
        "6:\n"
        "beqz %[execute_wrong], 7f\n"
        "addi t0, t0, 1\n"
        "7:\n"
        "beqz %[execute_wrong], 8f\n"
        "addi t0, t0, 1\n"
        "8:\n"
        "beqz %[execute_wrong], 9f\n"
        "addi t0, t0, 1\n"
        "9:\n"
        "beqz %[execute_wrong], 10f\n"
        "addi t0, t0, 1\n"
        "10:\n"
        "beqz %[execute_wrong], 11f\n"
        "addi t0, t0, 1\n"
        "11:\n"
        "beqz %[execute_wrong], 12f\n"
        "addi t0, t0, 1\n"
        "12:\n"
        "sd t0, 0(%[marker])\n"
        "ld t1, 0(%[right])\n"
        "sd t1, 8(%[marker])\n"
        ".option pop\n"
        :
        : [right] "r"(right_vaddr),
          [marker] "r"(marker_vaddr),
          [skipped] "r"(skipped_seed),
          [execute_wrong] "r"(execute_wrong_path)
        : "t0", "t1", "memory");
}

#define AI_BACKEND_SKEW_PAD_0

#define AI_BACKEND_SKEW_PAD_2 \
        "addi t3, t3, 1\n" \
        "addi t4, t4, 2\n"

#define AI_BACKEND_SKEW_PAD_4 \
        AI_BACKEND_SKEW_PAD_2 \
        "addi t5, t5, 3\n" \
        "addi t3, t3, 4\n"

#define AI_BACKEND_SKEW_PAD_6 \
        AI_BACKEND_SKEW_PAD_4 \
        "addi t4, t4, 5\n" \
        "addi t5, t5, 6\n"

#define AI_BACKEND_SKEW_PAD_8 \
        AI_BACKEND_SKEW_PAD_6 \
        "addi t3, t3, 7\n" \
        "addi t4, t4, 8\n"

#define AI_BACKEND_SKEW_PAD_12 \
        AI_BACKEND_SKEW_PAD_8 \
        "addi t5, t5, 9\n" \
        "addi t3, t3, 10\n" \
        "addi t4, t4, 11\n" \
        "addi t5, t5, 12\n"

#define AI_BACKEND_SKEW_PAD_16 \
        AI_BACKEND_SKEW_PAD_12 \
        "addi t3, t3, 13\n" \
        "addi t4, t4, 14\n" \
        "addi t5, t5, 15\n" \
        "addi t3, t3, 16\n"

#define AI_BACKEND_SKEW_PAD_20 \
        AI_BACKEND_SKEW_PAD_16 \
        "addi t4, t4, 17\n" \
        "addi t5, t5, 18\n" \
        "addi t3, t3, 19\n" \
        "addi t4, t4, 20\n"

#define AI_BACKEND_SKEW_PAD_24 \
        AI_BACKEND_SKEW_PAD_20 \
        "addi t5, t5, 21\n" \
        "addi t3, t3, 22\n" \
        "addi t4, t4, 23\n" \
        "addi t5, t5, 24\n"

#define AI_BACKEND_SKEW_PAD_28 \
        AI_BACKEND_SKEW_PAD_24 \
        "addi t3, t3, 25\n" \
        "addi t4, t4, 26\n" \
        "addi t5, t5, 27\n" \
        "addi t3, t3, 28\n"

#define AI_BACKEND_SKEW_DELAY_2 \
        "mulhu t2, t0, t1\n" \
        "mul t2, t2, t1\n"

#define AI_BACKEND_SKEW_DELAY_3 \
        AI_BACKEND_SKEW_DELAY_2 \
        "mulhu t2, t2, t0\n"

#define AI_BACKEND_SKEW_DELAY_ALU_4 \
        "add t2, t2, t0\n" \
        "xor t2, t2, t1\n" \
        "add t2, t2, t0\n" \
        "xor t2, t2, t1\n"

#define AI_BACKEND_SKEW_DELAY_ALU_8 \
        AI_BACKEND_SKEW_DELAY_ALU_4 \
        AI_BACKEND_SKEW_DELAY_ALU_4

#define AI_BACKEND_SKEW_DELAY_ALU_12 \
        AI_BACKEND_SKEW_DELAY_ALU_8 \
        AI_BACKEND_SKEW_DELAY_ALU_4

#define AI_BACKEND_SKEW_DELAY_ALU_16 \
        AI_BACKEND_SKEW_DELAY_ALU_12 \
        AI_BACKEND_SKEW_DELAY_ALU_4

#define AI_BACKEND_SKEW_DELAY_3_ALU_4 \
        AI_BACKEND_SKEW_DELAY_3 \
        AI_BACKEND_SKEW_DELAY_ALU_4

#define AI_BACKEND_SKEW_DELAY_3_ALU_8 \
        AI_BACKEND_SKEW_DELAY_3 \
        AI_BACKEND_SKEW_DELAY_ALU_8

#define AI_BACKEND_SKEW_DELAY_3_ALU_12 \
        AI_BACKEND_SKEW_DELAY_3 \
        AI_BACKEND_SKEW_DELAY_ALU_12

#define AI_BACKEND_SKEW_DELAY_3_ALU_16 \
        AI_BACKEND_SKEW_DELAY_3 \
        AI_BACKEND_SKEW_DELAY_ALU_16

#define AI_BACKEND_SKEW_DELAY_4 \
        AI_BACKEND_SKEW_DELAY_3 \
        "mul t2, t2, t1\n"

#define AI_BACKEND_SKEW_DELAY_3_LD \
        AI_BACKEND_SKEW_DELAY_3 \
        "ld t2, 16(%[marker])\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_0

#define AI_BACKEND_SKEW_YOUNGER_DELAY_8 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_10 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_8 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_12 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_10 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_13 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_12 \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_16 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_12 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_14 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_12 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_15 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_14 \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_17 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_16 \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_18 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_16 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_20 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_16 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_24 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_20 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_22 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_20 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_26 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_24 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_28 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_24 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_32 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_28 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_36 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_32 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_40 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_36 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_SKEW_YOUNGER_DELAY_44 \
        AI_BACKEND_SKEW_YOUNGER_DELAY_40 \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n" \
        "addi t2, t2, 0\n"

#define AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(name, delay_seq, pad_seq, \
    younger_delay_seq) \
__attribute__((noinline, aligned(64))) static void \
name(uintptr_t marker_vaddr, uint64_t older_wrong, uint64_t younger_wrong, \
    uint64_t older_done, uint64_t younger_done, uintptr_t execute_wrong_path) \
{ \
    asm volatile( \
        ".option push\n" \
        ".option norvc\n" \
        "mv t6, %[execute_wrong]\n" \
        "li t0, 0x123456789abcdef\n" \
        "li t1, 0x1f1f1f1f\n" \
        delay_seq \
        "andi t2, t2, 0\n" \
        "add t2, t2, t6\n" \
        "beqz t2, 1f\n" \
        "sd %[older_wrong], 0(%[marker])\n" \
        "1:\n" \
        "addi t3, zero, 0\n" \
        "addi t4, zero, 0\n" \
        "addi t5, zero, 0\n" \
        pad_seq \
        "mv t2, t6\n" \
        younger_delay_seq \
        "beqz t2, 2f\n" \
        "sd %[younger_wrong], 8(%[marker])\n" \
        "2:\n" \
        "sd %[older_done], 16(%[marker])\n" \
        "sd %[younger_done], 24(%[marker])\n" \
        ".option pop\n" \
        : \
        : [marker] "r"(marker_vaddr), \
          [older_wrong] "r"(older_wrong), \
          [younger_wrong] "r"(younger_wrong), \
          [older_done] "r"(older_done), \
          [younger_done] "r"(younger_done), \
          [execute_wrong] "r"(execute_wrong_path) \
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory"); \
}

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m2_p12,
    AI_BACKEND_SKEW_DELAY_2,
    AI_BACKEND_SKEW_PAD_12,
    AI_BACKEND_SKEW_YOUNGER_DELAY_0)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3_p8,
    AI_BACKEND_SKEW_DELAY_3,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_0)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3_p16,
    AI_BACKEND_SKEW_DELAY_3,
    AI_BACKEND_SKEW_PAD_16,
    AI_BACKEND_SKEW_YOUNGER_DELAY_0)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_0)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y8,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_8)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y10,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_10)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y12,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_12)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y15,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_15)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y16,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y17,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_17)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y18,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_18)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3ld_p8_y10,
    AI_BACKEND_SKEW_DELAY_3_LD,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_10)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3ld_p8_y12,
    AI_BACKEND_SKEW_DELAY_3_LD,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_12)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3ld_p8_y14,
    AI_BACKEND_SKEW_DELAY_3_LD,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3ld_p8_y16,
    AI_BACKEND_SKEW_DELAY_3_LD,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a4_p8_y10,
    AI_BACKEND_SKEW_DELAY_3_ALU_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_10)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a4_p8_y12,
    AI_BACKEND_SKEW_DELAY_3_ALU_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_12)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a8_p8_y12,
    AI_BACKEND_SKEW_DELAY_3_ALU_8,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_12)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a8_p8_y14,
    AI_BACKEND_SKEW_DELAY_3_ALU_8,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a12_p8_y12,
    AI_BACKEND_SKEW_DELAY_3_ALU_12,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_12)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a12_p8_y14,
    AI_BACKEND_SKEW_DELAY_3_ALU_12,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a16_p8_y14,
    AI_BACKEND_SKEW_DELAY_3_ALU_16,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3a16_p8_y16,
    AI_BACKEND_SKEW_DELAY_3_ALU_16,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y20,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_20)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p0_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_0,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p2_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_2,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p4_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_4,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p6_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_6,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p0_y16,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_0,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p2_y16,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_2,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p4_y16,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_4,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p6_y16,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_6,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p12_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_12,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p16_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_16,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p20_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_20,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p24_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_24,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p28_y14,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_28,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3_p8_y12,
    AI_BACKEND_SKEW_DELAY_3,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_12)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3_p8_y14,
    AI_BACKEND_SKEW_DELAY_3,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_14)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3_p8_y16,
    AI_BACKEND_SKEW_DELAY_3,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_16)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m3_p8_y18,
    AI_BACKEND_SKEW_DELAY_3,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_18)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y22,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_22)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y24,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_24)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y26,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_26)

AI_BACKEND_DEFINE_SKEW_BRANCH_PAIR(
    ai_backend_skew_branch_redirect_pair_m4_p8_y28,
    AI_BACKEND_SKEW_DELAY_4,
    AI_BACKEND_SKEW_PAD_8,
    AI_BACKEND_SKEW_YOUNGER_DELAY_28)

__attribute__((naked, noinline, aligned(4))) static void
ai_backend_delayed_store_then_marked_two_loads(uintptr_t store_vaddr,
    uintptr_t older_load_vaddr, uintptr_t younger_load_vaddr,
    uintptr_t older_marker_vaddr, uintptr_t younger_marker_vaddr,
    uint64_t store_value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a5, 0(t2)\n"
        "ld t3, 0(a1)\n"
        "sd t3, 0(a3)\n"
        "ld t4, 0(a2)\n"
        "sd t4, 0(a4)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_backend_delayed_store_then_tight_two_loads(uintptr_t store_vaddr,
    uintptr_t older_load_vaddr, uintptr_t younger_load_vaddr,
    uintptr_t older_marker_vaddr, uintptr_t younger_marker_vaddr,
    uint64_t store_value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "jal zero, 1f\n"
        "1:\n"
        "sd a5, 0(t2)\n"
        "ld t3, 0(a1)\n"
        "ld t4, 0(a2)\n"
        "sd t3, 0(a3)\n"
        "sd t4, 0(a4)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

#define AI_BACKEND_STLD_LOAD_GAP_0

#define AI_BACKEND_STLD_LOAD_GAP_1 \
        "nop\n"

#define AI_BACKEND_STLD_LOAD_GAP_2 \
        AI_BACKEND_STLD_LOAD_GAP_1 \
        "nop\n"

#define AI_BACKEND_STLD_LOAD_GAP_3 \
        AI_BACKEND_STLD_LOAD_GAP_2 \
        "nop\n"

#define AI_BACKEND_DEFINE_TIGHT_STLD_GAP(name, gap_seq) \
__attribute__((naked, noinline, aligned(4))) static void \
name(uintptr_t store_vaddr, uintptr_t older_load_vaddr, \
    uintptr_t younger_load_vaddr, uintptr_t older_marker_vaddr, \
    uintptr_t younger_marker_vaddr, uint64_t store_value) \
{ \
    asm volatile( \
        ".option push\n" \
        ".option norvc\n" \
        "li t0, 0xfedcba9876543210\n" \
        "li t1, 7\n" \
        "divu t2, t0, t1\n" \
        "addi t2, t2, 1\n" \
        "divu t2, t0, t2\n" \
        "andi t2, t2, 0\n" \
        "add t2, t2, a0\n" \
        "jal zero, 1f\n" \
        "1:\n" \
        "sd a5, 0(t2)\n" \
        "ld t3, 0(a1)\n" \
        gap_seq \
        "ld t4, 0(a2)\n" \
        "sd t3, 0(a3)\n" \
        "sd t4, 0(a4)\n" \
        "ret\n" \
        ".option pop\n" \
        : \
        : \
        : "t0", "t1", "t2", "t3", "t4", "memory"); \
}

AI_BACKEND_DEFINE_TIGHT_STLD_GAP(
    ai_backend_delayed_store_then_tight_two_loads_gap1,
    AI_BACKEND_STLD_LOAD_GAP_1)

AI_BACKEND_DEFINE_TIGHT_STLD_GAP(
    ai_backend_delayed_store_then_tight_two_loads_gap2,
    AI_BACKEND_STLD_LOAD_GAP_2)

AI_BACKEND_DEFINE_TIGHT_STLD_GAP(
    ai_backend_delayed_store_then_tight_two_loads_gap3,
    AI_BACKEND_STLD_LOAD_GAP_3)

#define AI_BACKEND_DENSE_NT_BRANCH_1 \
        "bnez t6, 1f\n" \
        "nop\n" \
        "1:\n"

#define AI_BACKEND_DENSE_NT_BRANCH_2 \
        AI_BACKEND_DENSE_NT_BRANCH_1 \
        AI_BACKEND_DENSE_NT_BRANCH_1

#define AI_BACKEND_DENSE_NT_BRANCH_4 \
        AI_BACKEND_DENSE_NT_BRANCH_2 \
        AI_BACKEND_DENSE_NT_BRANCH_2

#define AI_BACKEND_DENSE_NT_BRANCH_8 \
        AI_BACKEND_DENSE_NT_BRANCH_4 \
        AI_BACKEND_DENSE_NT_BRANCH_4

#define AI_BACKEND_DENSE_NT_BRANCH_16 \
        AI_BACKEND_DENSE_NT_BRANCH_8 \
        AI_BACKEND_DENSE_NT_BRANCH_8

#define AI_BACKEND_DENSE_NT_BRANCH_32 \
        AI_BACKEND_DENSE_NT_BRANCH_16 \
        AI_BACKEND_DENSE_NT_BRANCH_16

#define AI_BACKEND_DENSE_NT_BRANCH_64 \
        AI_BACKEND_DENSE_NT_BRANCH_32 \
        AI_BACKEND_DENSE_NT_BRANCH_32

#define AI_BACKEND_P1C_MUL_DELAY_2 \
        "mul t2, t2, t0\n" \
        "mul t2, t2, t0\n"

#define AI_BACKEND_P1C_MUL_DELAY_4 \
        AI_BACKEND_P1C_MUL_DELAY_2 \
        AI_BACKEND_P1C_MUL_DELAY_2

#define AI_BACKEND_P1C_MUL_DELAY_6 \
        AI_BACKEND_P1C_MUL_DELAY_4 \
        AI_BACKEND_P1C_MUL_DELAY_2

#define AI_BACKEND_P1C_MUL_DELAY_8 \
        AI_BACKEND_P1C_MUL_DELAY_4 \
        AI_BACKEND_P1C_MUL_DELAY_4

#define AI_BACKEND_P1C_MUL_DELAY_10 \
        AI_BACKEND_P1C_MUL_DELAY_8 \
        AI_BACKEND_P1C_MUL_DELAY_2

/* Train the older branch as taken, then make it not-taken so its predicted
 * taken path feeds a short run of dense not-taken branches. */
#define AI_BACKEND_DEFINE_DELAYED_WRONGPATH_DENSE(name, delay_seq) \
__attribute__((naked, noinline, aligned(64))) static void \
name(uintptr_t marker_vaddr, uint64_t wrong_marker, uint64_t done_marker, \
    uintptr_t execute_taken_path) \
{ \
    asm volatile( \
        ".option push\n" \
        ".option norvc\n" \
        "mv t6, a3\n" \
        "li t0, 3\n" \
        "addi t2, t6, 0\n" \
        delay_seq \
        "bnez t2, 1f\n" \
        "j 2f\n" \
        "1:\n" \
        AI_BACKEND_DENSE_NT_BRANCH_8 \
        "sd a1, 0(a0)\n" \
        "2:\n" \
        "sd a2, 8(a0)\n" \
        "ret\n" \
        ".option pop\n" \
        : \
        : \
        : "t0", "t2", "t6", "memory"); \
}

AI_BACKEND_DEFINE_DELAYED_WRONGPATH_DENSE(
    ai_backend_p1c_delayed_wrongpath_dense_d2,
    AI_BACKEND_P1C_MUL_DELAY_2)

AI_BACKEND_DEFINE_DELAYED_WRONGPATH_DENSE(
    ai_backend_p1c_delayed_wrongpath_dense_d4,
    AI_BACKEND_P1C_MUL_DELAY_4)

AI_BACKEND_DEFINE_DELAYED_WRONGPATH_DENSE(
    ai_backend_p1c_delayed_wrongpath_dense_d6,
    AI_BACKEND_P1C_MUL_DELAY_6)

AI_BACKEND_DEFINE_DELAYED_WRONGPATH_DENSE(
    ai_backend_p1c_delayed_wrongpath_dense_d8,
    AI_BACKEND_P1C_MUL_DELAY_8)

AI_BACKEND_DEFINE_DELAYED_WRONGPATH_DENSE(
    ai_backend_p1c_delayed_wrongpath_dense_d10,
    AI_BACKEND_P1C_MUL_DELAY_10)

__attribute__((naked, noinline, aligned(64))) static void
ai_backend_p1c_dense_cfi_snapshot_trigger(uintptr_t marker_vaddr,
    uint64_t wrong_marker, uint64_t done_marker,
    uintptr_t execute_taken_path)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "mv t6, a3\n"
        AI_BACKEND_DENSE_NT_BRANCH_64
        "sd a1, 0(a0)\n"
        "sd a2, 8(a0)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t6", "memory");
}

static bool ai_backend_run_p1c_dense_cfi_snapshot_trigger(void)
{
    const uintptr_t marker_paddr = ai_backend_paddr(0xe80U);
    const uintptr_t guard_paddr = ai_backend_paddr(0xea0U);
    const uintptr_t marker_vaddr = ai_backend_vaddr(0xe80U);
    const uint64_t marker_seed = 0x7031635f73656564ULL;
    const uint64_t wrong_train = 0x7031635f74726e77ULL;
    const uint64_t wrong_final = 0x7031635f666e7772ULL;
    const uint64_t done_train = 0x7031635f74726e64ULL;
    const uint64_t done_final = 0x7031635f666e646eULL;
    const uint64_t guard_seed = 0x7031635f67756172ULL;

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + 8U, marker_seed);
    write64(marker_paddr + 16U, marker_seed);
    write64(guard_paddr, guard_seed);
    ai_backend_prepare_hs_cacheable_page();

    for (unsigned epoch = 0; epoch < AI_BACKEND_P1C_DENSE_CFI_EPOCHS;
        ++epoch) {
        TEST_SETUP_EXCEPT();
        for (unsigned round = 0;
            round < AI_BACKEND_P1C_DENSE_CFI_TRAIN_ROUNDS; ++round) {
            ai_backend_p1c_dense_cfi_snapshot_trigger(marker_vaddr,
                wrong_train, done_train, 1U);
        }
        if (excpt.triggered ||
            ld(marker_vaddr) != wrong_train ||
            ld(marker_vaddr + 8U) != done_train) {
            goto_priv(PRIV_M);
            return false;
        }

        goto_priv(PRIV_M);
        write64(marker_paddr, marker_seed);
        write64(marker_paddr + 8U, marker_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_backend_p1c_dense_cfi_snapshot_trigger(marker_vaddr, wrong_final,
            done_final, 0U);
        if (excpt.triggered ||
            ld(marker_vaddr) != wrong_final ||
            ld(marker_vaddr + 8U) != done_final) {
            goto_priv(PRIV_M);
            return false;
        }
    }

    goto_priv(PRIV_M);
    return read64(marker_paddr) == wrong_final &&
        read64(marker_paddr + 8U) == done_final &&
        read64(marker_paddr + 16U) == marker_seed &&
        read64(guard_paddr) == guard_seed;
}

static bool ai_backend_run_p1c_delayed_wrongpath_dense_variant(
    void (*probe)(uintptr_t, uint64_t, uint64_t, uintptr_t), uintptr_t offset,
    uint64_t salt)
{
    const uintptr_t marker_paddr = ai_backend_paddr(offset);
    const uintptr_t guard_paddr = ai_backend_paddr(offset + 0x20U);
    const uintptr_t marker_vaddr = ai_backend_vaddr(offset);
    const uint64_t marker_seed = 0x7031635f64736564ULL ^ salt;
    const uint64_t wrong_train = 0x7031635f6474726eULL ^ salt;
    const uint64_t done_train = 0x7031635f64746f6eULL ^ salt;
    const uint64_t done_final = 0x7031635f64666f6eULL ^ salt;
    const uint64_t guard_seed = 0x7031635f64677564ULL ^ salt;

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + 8U, marker_seed);
    write64(guard_paddr, guard_seed);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < AI_BACKEND_P1C_DELAYED_TRAIN_ROUNDS;
        ++round) {
        probe(marker_vaddr, wrong_train, done_train, 1U);
    }
    if (excpt.triggered ||
        ld(marker_vaddr) != wrong_train ||
        ld(marker_vaddr + 8U) != done_train) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + 8U, marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    probe(marker_vaddr, wrong_train, done_final, 0U);
    if (excpt.triggered ||
        ld(marker_vaddr) != marker_seed ||
        ld(marker_vaddr + 8U) != done_final) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    return read64(marker_paddr) == marker_seed &&
        read64(marker_paddr + 8U) == done_final &&
        read64(guard_paddr) == guard_seed;
}

static bool ai_backend_run_p1c_delayed_wrongpath_dense_variants(void)
{
#define AI_BACKEND_RUN_P1C_DELAYED_VARIANT(fn, off, salt) \
    do { \
        if (!ai_backend_run_p1c_delayed_wrongpath_dense_variant( \
                (fn), (off), (salt))) { \
            return false; \
        } \
    } while (0)

    AI_BACKEND_RUN_P1C_DELAYED_VARIANT(
        ai_backend_p1c_delayed_wrongpath_dense_d2,
        0xe00U, 0x0101010101010101ULL);
    AI_BACKEND_RUN_P1C_DELAYED_VARIANT(
        ai_backend_p1c_delayed_wrongpath_dense_d4,
        0xe40U, 0x0202020202020202ULL);

#undef AI_BACKEND_RUN_P1C_DELAYED_VARIANT

    return true;
}

static bool ai_backend_run_branch_redirect_window_pair(
    void (*first_window)(uintptr_t, uintptr_t, uintptr_t, uint64_t, uintptr_t),
    void (*second_window)(uintptr_t, uintptr_t, uintptr_t, uint64_t, uintptr_t))
{
    const uintptr_t wrong_paddr = ai_backend_paddr(0x100U);
    const uintptr_t right_paddr = ai_backend_paddr(0x140U);
    const uintptr_t marker0_paddr = ai_backend_paddr(0x180U);
    const uintptr_t marker1_paddr = ai_backend_paddr(0x1a0U);
    const uintptr_t guard_paddr = ai_backend_paddr(0x1d0U);
    const uintptr_t wrong_vaddr = ai_backend_vaddr(0x100U);
    const uintptr_t right_vaddr = ai_backend_vaddr(0x140U);
    const uintptr_t marker0_vaddr = ai_backend_vaddr(0x180U);
    const uintptr_t marker1_vaddr = ai_backend_vaddr(0x1a0U);
    const uint64_t wrong_train = 0x77726f6e675f7430ULL;
    const uint64_t wrong_final = 0x77726f6e675f6631ULL;
    const uint64_t right_payload = 0x72696768745f7030ULL;
    const uint64_t skipped0 = 0x736b69705f303030ULL;
    const uint64_t skipped1 = 0x736b69705f313131ULL;
    const uint64_t marker_seed = 0x6d61726b65725f30ULL;
    const uint64_t guard_seed = 0x67756172645f7031ULL;

    goto_priv(PRIV_M);
    write64(wrong_paddr, wrong_train);
    write64(right_paddr, right_payload);
    write64(marker0_paddr, marker_seed);
    write64(marker0_paddr + 8U, marker_seed);
    write64(marker0_paddr + 16U, marker_seed);
    write64(marker1_paddr, marker_seed);
    write64(marker1_paddr + 8U, marker_seed);
    write64(marker1_paddr + 16U, marker_seed);
    write64(guard_paddr, guard_seed);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < AI_BACKEND_BRANCH_TRAINING_ROUNDS;
         ++round) {
        first_window(wrong_vaddr, right_vaddr, marker0_vaddr, skipped0, 1U);
        second_window(wrong_vaddr, right_vaddr, marker1_vaddr, skipped1, 1U);
    }
    if (excpt.triggered ||
        ld(marker0_vaddr) != wrong_train ||
        ld(marker1_vaddr) != wrong_train) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    write64(wrong_paddr, wrong_final);
    write64(marker0_paddr, marker_seed);
    write64(marker0_paddr + 8U, marker_seed);
    write64(marker0_paddr + 16U, marker_seed);
    write64(marker1_paddr, marker_seed);
    write64(marker1_paddr + 8U, marker_seed);
    write64(marker1_paddr + 16U, marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    first_window(wrong_vaddr, right_vaddr, marker0_vaddr, skipped0, 0U);
    second_window(wrong_vaddr, right_vaddr, marker1_vaddr, skipped1, 0U);
    if (excpt.triggered ||
        ld(marker0_vaddr) != skipped0 ||
        ld(marker0_vaddr + 8U) != right_payload ||
        ld(marker0_vaddr + 16U) != skipped0 ||
        ld(marker1_vaddr) != skipped1 ||
        ld(marker1_vaddr + 8U) != right_payload ||
        ld(marker1_vaddr + 16U) != skipped1) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    return read64(wrong_paddr) == wrong_final &&
        read64(right_paddr) == right_payload &&
        read64(guard_paddr) == guard_seed;
}

static bool ai_backend_run_tight_branch_redirect_cluster_pair(
    void (*first_cluster)(uintptr_t, uintptr_t, uintptr_t, uint64_t, uintptr_t),
    void (*second_cluster)(uintptr_t, uintptr_t, uintptr_t, uint64_t, uintptr_t))
{
    const uintptr_t wrong_paddr = ai_backend_paddr(0x500U);
    const uintptr_t right_paddr = ai_backend_paddr(0x540U);
    const uintptr_t marker0_paddr = ai_backend_paddr(0x600U);
    const uintptr_t marker1_paddr = ai_backend_paddr(0x680U);
    const uintptr_t guard_paddr = ai_backend_paddr(0x700U);
    const uintptr_t wrong_vaddr = ai_backend_vaddr(0x500U);
    const uintptr_t right_vaddr = ai_backend_vaddr(0x540U);
    const uintptr_t marker0_vaddr = ai_backend_vaddr(0x600U);
    const uintptr_t marker1_vaddr = ai_backend_vaddr(0x680U);
    const uint64_t wrong_train = 0x636c757374725f30ULL;
    const uint64_t wrong_final = 0x636c757374725f31ULL;
    const uint64_t right_payload = 0x72656469725f7031ULL;
    const uint64_t skipped0 = 0x736b69705f633030ULL;
    const uint64_t skipped1 = 0x736b69705f633131ULL;
    const uint64_t marker_seed = 0x6d61726b635f3030ULL;
    const uint64_t guard_seed = 0x6775617264636c75ULL;

    goto_priv(PRIV_M);
    write64(wrong_paddr, wrong_train);
    write64(right_paddr, right_payload);
    for (unsigned idx = 0; idx <= AI_BACKEND_TIGHT_BRANCH_COUNT; ++idx) {
        write64(marker0_paddr + idx * sizeof(uint64_t), marker_seed);
        write64(marker1_paddr + idx * sizeof(uint64_t), marker_seed);
    }
    write64(guard_paddr, guard_seed);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < AI_BACKEND_BRANCH_TRAINING_ROUNDS;
         ++round) {
        first_cluster(wrong_vaddr, right_vaddr, marker0_vaddr, skipped0, 1U);
        second_cluster(wrong_vaddr, right_vaddr, marker1_vaddr, skipped1, 1U);
    }
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        return false;
    }
    for (unsigned idx = 0; idx < AI_BACKEND_TIGHT_BRANCH_COUNT; ++idx) {
        if (ld(marker0_vaddr + idx * sizeof(uint64_t)) != wrong_train ||
            ld(marker1_vaddr + idx * sizeof(uint64_t)) != wrong_train) {
            goto_priv(PRIV_M);
            return false;
        }
    }

    goto_priv(PRIV_M);
    write64(wrong_paddr, wrong_final);
    for (unsigned idx = 0; idx <= AI_BACKEND_TIGHT_BRANCH_COUNT; ++idx) {
        write64(marker0_paddr + idx * sizeof(uint64_t), marker_seed);
        write64(marker1_paddr + idx * sizeof(uint64_t), marker_seed);
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    first_cluster(wrong_vaddr, right_vaddr, marker0_vaddr, skipped0, 0U);
    second_cluster(wrong_vaddr, right_vaddr, marker1_vaddr, skipped1, 0U);
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        return false;
    }
    for (unsigned idx = 0; idx < AI_BACKEND_TIGHT_BRANCH_COUNT; ++idx) {
        if (ld(marker0_vaddr + idx * sizeof(uint64_t)) != skipped0 ||
            ld(marker1_vaddr + idx * sizeof(uint64_t)) != skipped1) {
            goto_priv(PRIV_M);
            return false;
        }
    }
    if (ld(marker0_vaddr + AI_BACKEND_TIGHT_BRANCH_COUNT *
            sizeof(uint64_t)) != right_payload ||
        ld(marker1_vaddr + AI_BACKEND_TIGHT_BRANCH_COUNT *
            sizeof(uint64_t)) != right_payload) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    return read64(wrong_paddr) == wrong_final &&
        read64(right_paddr) == right_payload &&
        read64(guard_paddr) == guard_seed;
}

static bool ai_backend_run_tight_branch_redirect_burst_pair(
    void (*first_burst)(uintptr_t, uintptr_t, uint64_t, uintptr_t),
    void (*second_burst)(uintptr_t, uintptr_t, uint64_t, uintptr_t))
{
    const uintptr_t right_paddr = ai_backend_paddr(0x780U);
    const uintptr_t marker0_paddr = ai_backend_paddr(0x7a0U);
    const uintptr_t marker1_paddr = ai_backend_paddr(0x7c0U);
    const uintptr_t guard_paddr = ai_backend_paddr(0x7e0U);
    const uintptr_t right_vaddr = ai_backend_vaddr(0x780U);
    const uintptr_t marker0_vaddr = ai_backend_vaddr(0x7a0U);
    const uintptr_t marker1_vaddr = ai_backend_vaddr(0x7c0U);
    const uint64_t right_payload = 0x62757273745f7031ULL;
    const uint64_t skipped0 = 0x1000ULL;
    const uint64_t skipped1 = 0x2000ULL;
    const uint64_t marker_seed = 0x6d61726b62757273ULL;
    const uint64_t guard_seed = 0x6775617264627572ULL;

    goto_priv(PRIV_M);
    write64(right_paddr, right_payload);
    write64(marker0_paddr, marker_seed);
    write64(marker0_paddr + 8U, marker_seed);
    write64(marker1_paddr, marker_seed);
    write64(marker1_paddr + 8U, marker_seed);
    write64(guard_paddr, guard_seed);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < AI_BACKEND_BRANCH_TRAINING_ROUNDS;
         ++round) {
        first_burst(right_vaddr, marker0_vaddr, skipped0, 1U);
        second_burst(right_vaddr, marker1_vaddr, skipped1, 1U);
    }
    if (excpt.triggered ||
        ld(marker0_vaddr) != skipped0 + AI_BACKEND_BURST_BRANCH_COUNT ||
        ld(marker1_vaddr) != skipped1 + AI_BACKEND_BURST_BRANCH_COUNT) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    write64(marker0_paddr, marker_seed);
    write64(marker0_paddr + 8U, marker_seed);
    write64(marker1_paddr, marker_seed);
    write64(marker1_paddr + 8U, marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    first_burst(right_vaddr, marker0_vaddr, skipped0, 0U);
    second_burst(right_vaddr, marker1_vaddr, skipped1, 0U);
    if (excpt.triggered ||
        ld(marker0_vaddr) != skipped0 ||
        ld(marker0_vaddr + 8U) != right_payload ||
        ld(marker1_vaddr) != skipped1 ||
        ld(marker1_vaddr + 8U) != right_payload) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    return read64(right_paddr) == right_payload &&
        read64(guard_paddr) == guard_seed;
}

static bool ai_backend_run_skew_branch_redirect_pair(
    void (*branch_pair)(uintptr_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uintptr_t),
    uintptr_t offset, uint64_t salt)
{
    const uintptr_t marker_paddr = ai_backend_paddr(offset);
    const uintptr_t guard_paddr = ai_backend_paddr(offset + 0x30U);
    const uintptr_t marker_vaddr = ai_backend_vaddr(offset);
    const uint64_t marker_seed = 0x736b65775f6d6b72ULL ^ salt;
    const uint64_t older_wrong = 0x6f6c645f77726f6eULL ^ salt;
    const uint64_t younger_wrong = 0x796e675f77726f6eULL ^ salt;
    const uint64_t older_done = 0x6f6c645f646f6e65ULL ^ salt;
    const uint64_t younger_done = 0x796e675f646f6e65ULL ^ salt;
    const uint64_t guard_seed = 0x6775617264736b77ULL ^ salt;

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + 8U, marker_seed);
    write64(marker_paddr + 16U, marker_seed);
    write64(marker_paddr + 24U, marker_seed);
    write64(guard_paddr, guard_seed);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < AI_BACKEND_SKEW_TRAINING_ROUNDS;
         ++round) {
        branch_pair(marker_vaddr, older_wrong, younger_wrong, older_done,
            younger_done, 1U);
    }
    if (excpt.triggered ||
        ld(marker_vaddr) != older_wrong ||
        ld(marker_vaddr + 8U) != younger_wrong ||
        ld(marker_vaddr + 16U) != older_done ||
        ld(marker_vaddr + 24U) != younger_done) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + 8U, marker_seed);
    write64(marker_paddr + 16U, marker_seed);
    write64(marker_paddr + 24U, marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    branch_pair(marker_vaddr, older_wrong, younger_wrong, older_done,
        younger_done, 0U);
    if (excpt.triggered ||
        ld(marker_vaddr) != marker_seed ||
        ld(marker_vaddr + 8U) != marker_seed ||
        ld(marker_vaddr + 16U) != older_done ||
        ld(marker_vaddr + 24U) != younger_done) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    return read64(marker_paddr) == marker_seed &&
        read64(marker_paddr + 8U) == marker_seed &&
        read64(marker_paddr + 16U) == older_done &&
        read64(marker_paddr + 24U) == younger_done &&
        read64(guard_paddr) == guard_seed;
}

static bool ai_backend_run_skew_branch_redirect_variants(void)
{
#define AI_BACKEND_RUN_SKEW_VARIANT(fn, off, salt) \
    do { \
        if (!ai_backend_run_skew_branch_redirect_pair((fn), (off), (salt))) { \
            return false; \
        } \
    } while (0)

    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m2_p12,
        0x500U, 0x1111111111111111ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m3_p8,
        0x540U, 0x1212121212121212ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m3_p16,
        0x580U, 0x1313131313131313ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m3_p8_y12,
        0x5c0U, 0x1414141414141414ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m3_p8_y14,
        0x600U, 0x1515151515151515ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m3_p8_y16,
        0x640U, 0x1616161616161616ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m3_p8_y18,
        0x680U, 0x1717171717171717ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y10,
        0x880U, 0x0101010101010101ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y12,
        0x8c0U, 0x0202020202020202ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y14,
        0x900U, 0x0303030303030303ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y16,
        0x940U, 0x0404040404040404ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y18,
        0x980U, 0x0505050505050505ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y20,
        0x9c0U, 0x0606060606060606ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y22,
        0xb40U, 0x0707070707070707ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y24,
        0xb80U, 0x0808080808080808ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y26,
        0xbc0U, 0x0909090909090909ULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p8_y28,
        0xc00U, 0x0a0a0a0a0a0a0a0aULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p12_y14,
        0xc40U, 0x0b0b0b0b0b0b0b0bULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p16_y14,
        0xc80U, 0x0c0c0c0c0c0c0c0cULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p20_y14,
        0xcc0U, 0x0d0d0d0d0d0d0d0dULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p24_y14,
        0xd00U, 0x0e0e0e0e0e0e0e0eULL);
    AI_BACKEND_RUN_SKEW_VARIANT(
        ai_backend_skew_branch_redirect_pair_m4_p28_y14,
        0xd40U, 0x0f0f0f0f0f0f0f0fULL);

#undef AI_BACKEND_RUN_SKEW_VARIANT

    return true;
}

typedef void (*ai_backend_stld_sequence_t)(uintptr_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uint64_t);

static bool ai_backend_run_stld_violation_window_with(
    ai_backend_stld_sequence_t stld_sequence, uintptr_t offset,
    uint64_t initial_value, uint64_t store_value, uint64_t adjacent_value,
    uint64_t marker_seed)
{
    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page older_load_page = SWITCH2;
    const enum test_page younger_load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + offset;
    const uintptr_t store_vaddr = hs_page_base(store_page) + offset;
    const uintptr_t older_load_vaddr = hs_page_base(older_load_page) + offset;
    const uintptr_t younger_load_vaddr = hs_page_base(younger_load_page) + offset;
    const uintptr_t adjacent_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t older_marker_vaddr = hs_page_base(base_page) + offset + 0x80U;
    const uintptr_t younger_marker_vaddr = hs_page_base(base_page) + offset + 0xc0U;
    const uintptr_t paddr = phys_page_base(base_page) + offset;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t older_marker_paddr = phys_page_base(base_page) + offset + 0x80U;
    const uintptr_t younger_marker_paddr = phys_page_base(base_page) + offset + 0xc0U;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    write64(older_marker_paddr, marker_seed);
    write64(younger_marker_paddr, marker_seed ^ 0xffffULL);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_backend_map_alias_to_base(store_page, base_page);
    ai_backend_map_alias_to_base(older_load_page, base_page);
    ai_backend_map_alias_to_base(younger_load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(base_vaddr);
    uint64_t warm_adjacent = ld(adjacent_vaddr);
    if (excpt.triggered ||
        warm != initial_value ||
        warm_adjacent != adjacent_value) {
        goto_priv(PRIV_M);
        return false;
    }

    TEST_SETUP_EXCEPT();
    stld_sequence(store_vaddr,
        older_load_vaddr, younger_load_vaddr, older_marker_vaddr,
        younger_marker_vaddr, store_value);
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        return false;
    }

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    if (excpt.triggered ||
        ld(older_marker_vaddr) != store_value ||
        ld(younger_marker_vaddr) != store_value ||
        ld(base_vaddr) != store_value ||
        ld(adjacent_vaddr) != adjacent_value) {
        goto_priv(PRIV_M);
        return false;
    }

    goto_priv(PRIV_M);
    return read64(paddr) == store_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(older_marker_paddr) == store_value &&
        read64(younger_marker_paddr) == store_value;
}

static bool ai_backend_run_stld_violation_window(uintptr_t offset,
    uint64_t initial_value, uint64_t store_value, uint64_t adjacent_value,
    uint64_t marker_seed)
{
    return ai_backend_run_stld_violation_window_with(
        ai_backend_delayed_store_then_marked_two_loads, offset,
        initial_value, store_value, adjacent_value, marker_seed);
}

static bool ai_backend_run_tight_stld_violation_window(uintptr_t offset,
    uint64_t initial_value, uint64_t store_value, uint64_t adjacent_value,
    uint64_t marker_seed)
{
    return ai_backend_run_stld_violation_window_with(
        ai_backend_delayed_store_then_tight_two_loads, offset,
        initial_value, store_value, adjacent_value, marker_seed);
}

static bool ai_backend_run_p1c_stld_violation_variants(void)
{
    return ai_backend_run_stld_violation_window_with(
            ai_backend_delayed_store_then_tight_two_loads,
            0xa40U,
            0x1020304050607080ULL,
            0x8877665544332211ULL,
            0x99aabbccddeeff00ULL,
            0x13579bdf2468ace0ULL) &&
        ai_backend_run_stld_violation_window_with(
            ai_backend_delayed_store_then_tight_two_loads_gap1,
            0xb40U,
            0x1122334455667788ULL,
            0x0102030405060708ULL,
            0x8877665544332211ULL,
            0x2468ace013579bdfULL) &&
        ai_backend_run_stld_violation_window_with(
            ai_backend_delayed_store_then_tight_two_loads_gap2,
            0xc40U,
            0xa0b0c0d0e0f00123ULL,
            0xfeedfacecafebeefULL,
            0x0123456789abcdefULL,
            0x3141592653589793ULL) &&
        ai_backend_run_stld_violation_window_with(
            ai_backend_delayed_store_then_tight_two_loads_gap3,
            0xd40U,
            0x0f1e2d3c4b5a6978ULL,
            0x7766554433221100ULL,
            0xf0e1d2c3b4a59687ULL,
            0x2718281828459045ULL);
}

static bool ai_backend_run_p1c_branch_redirect_variants(void)
{
    return ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a4_p8_y10,
            0x880U,
            0x0101010101010101ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a4_p8_y12,
            0x8c0U,
            0x0202020202020202ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a8_p8_y12,
            0x900U,
            0x0303030303030303ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a8_p8_y14,
            0x940U,
            0x0404040404040404ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a12_p8_y12,
            0x980U,
            0x0505050505050505ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a12_p8_y14,
            0x9c0U,
            0x0606060606060606ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a16_p8_y14,
            0xb40U,
            0x0707070707070707ULL) &&
        ai_backend_run_skew_branch_redirect_pair(
            ai_backend_skew_branch_redirect_pair_m3a16_p8_y16,
            0xb80U,
            0x0808080808080808ULL);
}

static __attribute__((noinline)) uint64_t
ai_backend_long_alu_chain_probe(uint64_t seed)
{
    uint64_t x = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[x]\n\t"
        "li t1, 0\n\t"
        "1:\n\t"
        "addi t1, t1, 1\n\t"
        "add t0, t0, t1\n\t"
        "slli t0, t0, 1\n\t"
        "srli t0, t0, 1\n\t"
        "xori t0, t0, 0x55\n\t"
        "xori t0, t0, 0x55\n\t"
        "li t2, %c[rounds]\n\t"
        "bltu t1, t2, 1b\n\t"
        "mv %[x], t0\n\t"
        ".option pop\n\t"
        : [x] "+r"(x)
        : [rounds] "i"(AI_BACKEND_CHAIN_ITERATIONS)
        : "t0", "t1", "t2", "memory");

    return x;
}

static uint64_t ai_backend_long_alu_chain_expected(uint64_t seed)
{
    uint64_t x = seed;

    for (unsigned i = 1; i <= AI_BACKEND_CHAIN_ITERATIONS; ++i) {
        x += i;
    }

    return x;
}

static __attribute__((noinline)) uint64_t
ai_backend_branch_chain_probe(uint64_t seed, uintptr_t marker_vaddr)
{
    uint64_t x = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[x]\n\t"
        "mv t3, %[marker]\n\t"
        "li t1, 0\n\t"
        "1:\n\t"
        "addi t1, t1, 1\n\t"
        "andi t2, t1, 1\n\t"
        "beqz t2, 2f\n\t"
        "add t0, t0, t1\n\t"
        "j 3f\n\t"
        "2:\n\t"
        "add t0, t0, t1\n\t"
        "addi t0, t0, 3\n\t"
        "addi t0, t0, -3\n\t"
        "3:\n\t"
        "sd t0, 0(t3)\n\t"
        "li t4, %c[rounds]\n\t"
        "bltu t1, t4, 1b\n\t"
        "mv %[x], t0\n\t"
        ".option pop\n\t"
        : [x] "+r"(x)
        : [marker] "r"(marker_vaddr),
          [rounds] "i"(AI_BACKEND_CHAIN_ITERATIONS)
        : "t0", "t1", "t2", "t3", "t4", "memory");

    return x;
}

static __attribute__((noinline)) uint64_t
ai_backend_multi_source_network_probe(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t out = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[a]\n\t"
        "mv t1, %[b]\n\t"
        "mv t2, %[c]\n\t"
        "add t3, t0, t1\n\t"
        "xor t4, t3, t2\n\t"
        "sub t5, t4, t0\n\t"
        "add t6, t5, t1\n\t"
        "xor t3, t6, t4\n\t"
        "add t3, t3, t2\n\t"
        "sub t3, t3, t1\n\t"
        "xor %[out], t3, t0\n\t"
        ".option pop\n\t"
        : [out] "=r"(out)
        : [a] "r"(a), [b] "r"(b), [c] "r"(c)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return out;
}

static uint64_t ai_backend_multi_source_network_expected(
    uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t t3 = a + b;
    uint64_t t4 = t3 ^ c;
    uint64_t t5 = t4 - a;
    uint64_t t6 = t5 + b;

    t3 = t6 ^ t4;
    t3 += c;
    t3 -= b;
    return t3 ^ a;
}

static __attribute__((noinline)) void ai_backend_vector_integer_operand_probe(
    uintptr_t src0,
    uintptr_t src1,
    uintptr_t dest,
    uint64_t scalar_addend,
    uint64_t *scalar_out)
{
    uint64_t scalar = scalar_addend;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t1, 2\n\t"
        "vsetvli t0, t1, e64, m1, ta, ma\n\t"
        "vle64.v v6, (%[src0])\n\t"
        "ld t2, 0(%[src1])\n\t"
        "add %[scalar], %[scalar], t2\n\t"
        "vadd.vx v6, v6, %[scalar]\n\t"
        "vse64.v v6, (%[dest])\n\t"
        ".option pop\n\t"
        : [scalar] "+r"(scalar)
        : [src0] "r"(src0),
          [src1] "r"(src1),
          [dest] "r"(dest)
        : "t0", "t1", "t2", "memory");

    *scalar_out = scalar;
}

bool ai_micro_backend_nested_redirect_same_snapshot_marker_integrity_corner()
{
    TEST_START();

    TEST_ASSERT("tight branch-mispredict redirect cluster flushes wrong loads and preserves survivor markers for same-snapshot nested redirect waveform closure",
        ai_backend_run_tight_branch_redirect_cluster_pair(
            ai_backend_branch_redirect_cluster_a,
            ai_backend_branch_redirect_cluster_a)
    );

    TEST_END();
}

bool ai_micro_backend_nested_redirect_different_snapshot_marker_integrity_corner()
{
    TEST_START();

    TEST_ASSERT("two independent tight branch-mispredict redirect clusters preserve marker state for different-snapshot nested redirect waveform closure",
        ai_backend_run_tight_branch_redirect_cluster_pair(
            ai_backend_branch_redirect_cluster_a,
            ai_backend_branch_redirect_cluster_b)
    );

    TEST_END();
}

bool ai_micro_backend_nested_redirect_outer_snapshot_inner_no_snapshot_corner()
{
    TEST_START();

    TEST_ASSERT("tight branch-mispredict redirect windows keep snapshot and no-snapshot survivor markers precise for outer-snapshot inner-no-snapshot nested redirect waveform closure",
        ai_backend_run_p1c_delayed_wrongpath_dense_variants()
    );

    TEST_END();
}

bool ai_micro_backend_nested_redirect_no_snapshot_double_redirect_corner()
{
    TEST_START();

    TEST_ASSERT("branch redirect cluster and back-to-back store-load violation rollback windows retire with fresh payloads and no stale rollback leakage",
        ai_backend_run_tight_branch_redirect_cluster_pair(
            ai_backend_branch_redirect_cluster_a,
            ai_backend_branch_redirect_cluster_b) &&
        ai_backend_run_stld_violation_window(
            0x300U,
            0x0123456789abcdefULL,
            0xa1b2c3d4e5f60718ULL,
            0x55aa33cc77ee11ffULL,
            0x1111222233334444ULL) &&
        ai_backend_run_stld_violation_window(
            0x400U,
            0xfedcba9876543210ULL,
            0x5566778899aabbccULL,
            0x0badf00dfacefeedULL,
            0x88889999aaaabbbbULL)
    );

    TEST_END();
}

bool ai_micro_backend_issuequeue_long_alu_dependency_chain_corner()
{
    TEST_START();

    const uint64_t seed = 0x1234500000000000ULL;
    const uint64_t expected = ai_backend_long_alu_chain_expected(seed);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_backend_long_alu_chain_probe(seed);
    TEST_ASSERT("IssueQueue long ALU dependency chain wakes and executes every dependent producer in order",
        excpt.triggered == false &&
        observed == expected,
        "observed=%lx expected=%lx", observed, expected);

    TEST_END();
}

bool ai_micro_backend_issuequeue_long_dependency_chain_with_branches_corner()
{
    TEST_START();

    const uint64_t seed = 0x2234500000000000ULL;
    const uint64_t expected = ai_backend_long_alu_chain_expected(seed);
    const uintptr_t marker_paddr = ai_backend_paddr(0x180U);
    const uintptr_t marker_vaddr = ai_backend_vaddr(0x180U);

    goto_priv(PRIV_M);
    write64(marker_paddr, 0ULL);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_backend_branch_chain_probe(seed, marker_vaddr);
    TEST_ASSERT("IssueQueue long dependency chain remains correct while interleaved branches redirect within the chain",
        excpt.triggered == false &&
        observed == expected &&
        ld(marker_vaddr) == expected,
        "observed=%lx expected=%lx marker=%lx",
        observed, expected, ld(marker_vaddr));

    TEST_END();
}

bool ai_micro_backend_issuequeue_interleaved_multi_source_dependency_network_corner()
{
    TEST_START();

    const uint64_t a = 0x0123456789abcdefULL;
    const uint64_t b = 0x1111222233334444ULL;
    const uint64_t c = 0xf0e0d0c0b0a09080ULL;
    const uint64_t expected = ai_backend_multi_source_network_expected(a, b, c);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_backend_multi_source_network_probe(a, b, c);
    TEST_ASSERT("IssueQueue interleaved multi-source dependency network forwards the right producer values to all consumers",
        excpt.triggered == false &&
        observed == expected,
        "observed=%lx expected=%lx", observed, expected);

    TEST_END();
}

bool ai_micro_backend_vector_integer_operand_readiness_mixed_source_corner()
{
    TEST_START();

    const uintptr_t src_paddr = ai_backend_paddr(0x200U);
    const uintptr_t scalar_paddr = ai_backend_paddr(0x240U);
    const uintptr_t dest_paddr = ai_backend_paddr(0x280U);
    const uintptr_t src_vaddr = ai_backend_vaddr(0x200U);
    const uintptr_t scalar_vaddr = ai_backend_vaddr(0x240U);
    const uintptr_t dest_vaddr = ai_backend_vaddr(0x280U);
    const uint64_t src0 = 0x0102030405060708ULL;
    const uint64_t src1 = 0x1112131415161718ULL;
    const uint64_t scalar_mem = 0x20ULL;
    const uint64_t scalar_addend = 0x11ULL;
    uint64_t scalar_observed = 0ULL;

    ai_backend_enable_vector_state();
    write64(src_paddr, src0);
    write64(src_paddr + 8U, src1);
    write64(scalar_paddr, scalar_mem);
    write64(dest_paddr, 0ULL);
    write64(dest_paddr + 8U, 0ULL);
    ai_backend_prepare_hs_cacheable_page();

    TEST_SETUP_EXCEPT();
    ai_backend_vector_integer_operand_probe(
        src_vaddr, scalar_vaddr, dest_vaddr, scalar_addend, &scalar_observed);
    TEST_ASSERT("one vector instruction with integer and vector operands waits for both source classes before producing the final vector image",
        excpt.triggered == false &&
        scalar_observed == (scalar_addend + scalar_mem) &&
        ld(dest_vaddr) == src0 + scalar_observed &&
        ld(dest_vaddr + 8U) == src1 + scalar_observed,
        "scalar=%lx dest0=%lx dest1=%lx",
        scalar_observed, ld(dest_vaddr), ld(dest_vaddr + 8U));

    TEST_END();
}

bool ai_arch_backend_custom_csr_privilege_programming_matrix_corner()
{
    TEST_START();

    static const unsigned csrs[] = {
        CSR_SBPCTL,
        CSR_SPFCTL,
        CSR_SMBLOCKCTL,
        CSR_SRNCTL,
    };
    static const uint64_t masks[] = {
        AI_BACKEND_SBPCTL_DEFINED_MASK,
        AI_BACKEND_SPFCTL_DEFINED_MASK,
        AI_BACKEND_SMBLOCKCTL_DEFINED_MASK,
        AI_BACKEND_SRNCTL_DEFINED_MASK,
    };
    static const uint64_t values[] = {
        AI_BACKEND_SBPCTL_DEFINED_MASK,
        AI_BACKEND_SPFCTL_DEFINED_MASK,
        AI_BACKEND_SMBLOCKCTL_DEFINED_MASK,
        AI_BACKEND_SRNCTL_FUSION_ENABLE | AI_BACKEND_SRNCTL_WFI_ENABLE,
    };
    uint64_t original[sizeof(csrs) / sizeof(csrs[0])];
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(csrs) / sizeof(csrs[0]); ++i) {
        TEST_SETUP_EXCEPT();
        original[i] = ai_backend_read_custom_csr(csrs[i]);
        TEST_ASSERT("M-mode can read each backend custom CSR before programming/permission checks",
            excpt.triggered == false
        );
    }

    for (unsigned i = 0; i < sizeof(csrs) / sizeof(csrs[0]); ++i) {
        TEST_ASSERT("M-mode backend custom CSR target bits can be enabled and read back",
            ai_backend_custom_write_read_no_except(csrs[i], values[i], &observed) &&
            (observed & masks[i]) == (values[i] & masks[i]),
            "csr=%x observed=%lx expected=%lx",
            csrs[i], observed, values[i] & masks[i]);

        TEST_ASSERT("M-mode backend custom CSR target bits can be disabled and read back",
            ai_backend_custom_write_read_no_except(csrs[i], 0ULL, &observed) &&
            (observed & masks[i]) == 0ULL,
            "csr=%x observed=%lx", csrs[i], observed);
    }

    TEST_ASSERT("M-mode srnctl exposes fusion and wfi enable bits as independently programmable controls",
        ai_backend_custom_write_read_no_except(
            CSR_SRNCTL, AI_BACKEND_SRNCTL_FUSION_ENABLE, &observed) &&
        (observed & AI_BACKEND_SRNCTL_FUSION_ENABLE) != 0ULL &&
        (observed & AI_BACKEND_SRNCTL_WFI_ENABLE) == 0ULL
    );
    TEST_ASSERT("M-mode srnctl can set wfi_enable while preserving fusion_enable in the same CSR image",
        ai_backend_custom_write_read_no_except(
            CSR_SRNCTL,
            AI_BACKEND_SRNCTL_FUSION_ENABLE | AI_BACKEND_SRNCTL_WFI_ENABLE,
            &observed) &&
        (observed & (AI_BACKEND_SRNCTL_FUSION_ENABLE |
            AI_BACKEND_SRNCTL_WFI_ENABLE)) ==
            (AI_BACKEND_SRNCTL_FUSION_ENABLE | AI_BACKEND_SRNCTL_WFI_ENABLE)
    );

    goto_priv(PRIV_HS);
    for (unsigned i = 0; i < sizeof(csrs) / sizeof(csrs[0]); ++i) {
        TEST_ASSERT("S/HS-mode backend custom CSR read/write is permitted and converges",
            ai_backend_custom_write_read_no_except(csrs[i], values[i], &observed) &&
            (observed & masks[i]) == (values[i] & masks[i]),
            "csr=%x observed=%lx expected=%lx",
            csrs[i], observed, values[i] & masks[i]);
    }

    goto_priv(PRIV_HU);
    for (unsigned i = 0; i < sizeof(csrs) / sizeof(csrs[0]); ++i) {
        TEST_SETUP_EXCEPT();
        (void)ai_backend_read_custom_csr(csrs[i]);
        TEST_ASSERT("U/HU-mode backend custom CSR read raises illegal instruction",
            excpt.triggered == true &&
            excpt.cause == CAUSE_ILI,
            "csr=%x cause=%lx", csrs[i], excpt.cause);
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(csrs) / sizeof(csrs[0]); ++i) {
        TEST_SETUP_EXCEPT();
        ai_backend_write_custom_csr(csrs[i], original[i]);
        TEST_ASSERT("restoring backend custom CSR state after permission matrix does not trap",
            excpt.triggered == false
        );
    }

    TEST_END();
}
