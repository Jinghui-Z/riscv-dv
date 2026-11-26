#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#ifndef CSR_SRNCTL
#define CSR_SRNCTL 0x5c4
#endif

#define AI_FCOV_SRNCTL_FUSION_ENABLE (1ULL << 0)
#define AI_FCOV_STRESS_ROUNDS 96U
#define AI_FCOV_READARB_CANCEL_ROUNDS 192U
#define AI_FCOV_READARB_ALIGNED_ROUNDS 128U
#define AI_FCOV_READARB_DELAYED_ROUNDS 96U
#define AI_FCOV_READARB_BJU3_DIV_ROUNDS 64U
#define AI_FCOV_IQ_CANCEL_WINDOWS 16U
#define AI_FCOV_IQ_CANCEL_SLOTS 4U
#define AI_FCOV_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_FCOV_MSTATUS_FS_DIRTY (3ULL << 13)
#define AI_FCOV_VECTOR_PAIR_ROUNDS 48U
#define AI_FCOV_VECTOR_PAIR_QWORDS 48U

static inline uintptr_t ai_fcov_paddr(uintptr_t offset)
{
    return phys_page_base(VSRWX_GRWX) + offset;
}

static inline uintptr_t ai_fcov_hs_vaddr(enum test_page page, uintptr_t offset)
{
    return hs_page_base(page) + offset;
}

static inline pte_t *ai_fcov_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_fcov_map_hs_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_fcov_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_fcov_set_hs_leaf_to_page(enum test_page alias_page,
    enum test_page base_page, uint64_t flags)
{
    *ai_fcov_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | flags;
}

static inline void ai_fcov_prepare_hs_tables(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();
}

static bool ai_fcov_check_u64_pattern(uintptr_t paddr, unsigned count,
    uint64_t seed)
{
    for (unsigned i = 0; i < count; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) !=
            (seed ^ ((uint64_t)i << 44))) {
            return false;
        }
    }

    return true;
}

static void ai_fcov_fill_u64_pattern(uintptr_t paddr, unsigned count,
    uint64_t seed)
{
    for (unsigned i = 0; i < count; ++i) {
        write64(paddr + i * sizeof(uint64_t), seed ^ ((uint64_t)i << 44));
    }
}

static inline void ai_fcov_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_FCOV_MSTATUS_VS_DIRTY);
    CSRS(CSR_MSTATUS, AI_FCOV_MSTATUS_FS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_fcov_read_srnctl(void)
{
    uint64_t value;
    asm volatile("csrr %0, 0x5c4" : "=r"(value) :: "memory");
    return value;
}

static inline void ai_fcov_write_srnctl(uint64_t value)
{
    asm volatile("csrw 0x5c4, %0" :: "rK"(value) : "memory");
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_decode_fusion_probe(uintptr_t branch_guard_paddr, uint64_t seed)
{
    uint64_t marker = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "li t1, 4\n\t"

        ".balign 16\n\t"
        "vsetvli t0, t1, e64, m1, ta, ma\n\t"
        "addi %[marker], %[marker], 1\n\t"
        "addi %[marker], %[marker], 2\n\t"
        "addi %[marker], %[marker], 3\n\t"

        ".balign 16\n\t"
        "addi %[marker], %[marker], 4\n\t"
        "vsetvli t0, t1, e32, m1, ta, ma\n\t"
        "addi %[marker], %[marker], 5\n\t"
        "addi %[marker], %[marker], 6\n\t"

        ".balign 16\n\t"
        "addi %[marker], %[marker], 7\n\t"
        "addi %[marker], %[marker], 8\n\t"
        "vsetvli t0, t1, e16, m1, ta, ma\n\t"
        "addi %[marker], %[marker], 9\n\t"

        ".balign 16\n\t"
        "addi %[marker], %[marker], 10\n\t"
        "addi %[marker], %[marker], 11\n\t"
        "addi %[marker], %[marker], 12\n\t"
        "vsetvli t0, t1, e8, m1, ta, ma\n\t"

        ".balign 16\n\t"
        "lui t2, 0x12345\n\t"
        "addi t2, t2, 0x678\n\t"
        "add %[marker], %[marker], t2\n\t"
        "addi %[marker], %[marker], 13\n\t"

        ".balign 16\n\t"
        "addi t3, zero, 0x123\n\t"
        "addi t4, zero, -7\n\t"
        "slliw t5, t4, 16\n\t"
        "sraiw t5, t5, 16\n\t"
        "add %[marker], %[marker], t5\n\t"

        "ld t6, 0(%[guard])\n\t"
        ".balign 16\n\t"
        "bnez t6, 1f\n\t"
        "addi %[marker], %[marker], 17\n\t"
        "j 2f\n\t"
        ".balign 16, 0\n\t"
        "1:\n\t"
        ".word 0x00000013\n\t"
        ".word 0x00000013\n\t"
        ".hword 0x0001\n\t"
        ".word 0x0000806b\n\t"
        "addi %[marker], %[marker], 19\n\t"
        "2:\n\t"
        ".option pop\n\t"
        : [marker] "+r"(marker)
        : [guard] "r"(branch_guard_paddr)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return marker;
}

__attribute__((naked, noinline, aligned(16), noreturn)) static void
ai_fcov_lane3_good_xstrap(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        ".word 0x00000013\n\t"
        ".word 0x00000013\n\t"
        ".word 0x0000006b\n\t"
        ".option pop\n\t");
    __builtin_unreachable();
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_iq_mou_dispatch_probe(uintptr_t amo0_paddr, uintptr_t amo1_paddr,
    uint64_t seed)
{
    uint64_t acc = seed;

    for (unsigned i = 0; i < AI_FCOV_STRESS_ROUNDS; ++i) {
        uint64_t add0 = 0x10ULL + i;
        uint64_t add1 = 0x80ULL + i;

        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            ".option norelax\n\t"
            ".balign 16\n\t"
            "csrr t0, cycle\n\t"
            "csrr t1, instret\n\t"
            "fence rw, rw\n\t"
            "fence.i\n\t"
            "add %[acc], %[acc], t0\n\t"
            "xor %[acc], %[acc], t1\n\t"
            ".balign 16\n\t"
            "amoadd.d.aqrl t2, %[add0], (%[addr0])\n\t"
            "amoadd.d.aqrl t3, %[add1], (%[addr1])\n\t"
            "amoswap.d t4, t2, (%[addr0])\n\t"
            "amoswap.d t5, t3, (%[addr1])\n\t"
            "add %[acc], %[acc], t2\n\t"
            "xor %[acc], %[acc], t3\n\t"
            ".option pop\n\t"
            : [acc] "+r"(acc)
            : [addr0] "r"(amo0_paddr),
              [addr1] "r"(amo1_paddr),
              [add0] "r"(add0),
              [add1] "r"(add1)
            : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
    }

    return acc;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_readarb_redirect_probe(uintptr_t vec_src_paddr,
    uintptr_t vec_dst_paddr, uintptr_t branch_guard_paddr, uint64_t seed)
{
    uint64_t acc = seed;
    uint64_t rounds = AI_FCOV_STRESS_ROUNDS;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "li t0, 4\n\t"
        "vsetvli t1, t0, e64, m1, ta, ma\n\t"
        "vle64.v v8, (%[src])\n\t"
        "vadd.vv v9, v8, v8\n\t"
        "vse64.v v9, (%[dst])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(vec_src_paddr), [dst] "r"(vec_dst_paddr)
        : "t0", "t1", "memory");

    for (unsigned i = 0; i < AI_FCOV_STRESS_ROUNDS; ++i) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            ".option norelax\n\t"
            ".balign 16\n\t"
            "add a0, %[acc], s0\n\t"
            "add a1, %[acc], s1\n\t"
            "add a2, %[acc], s2\n\t"
            "add a3, %[acc], s3\n\t"
            "beq a0, a1, 10f\n\t"
            "bne a2, a3, 11f\n\t"
            "10:\n\t"
            "addi %[acc], %[acc], 1\n\t"
            "11:\n\t"
            ".balign 16\n\t"
            "add a4, %[acc], s4\n\t"
            "add a5, %[acc], s5\n\t"
            "add a6, %[acc], s6\n\t"
            "add a7, %[acc], s7\n\t"
            "blt a4, a5, 12f\n\t"
            "bge a6, a7, 13f\n\t"
            "12:\n\t"
            "addi %[acc], %[acc], 3\n\t"
            "13:\n\t"
            ".option pop\n\t"
            : [acc] "+r"(acc)
            :
            : "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory");
    }

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "ld t2, 0(%[guard])\n\t"
        ".balign 16\n\t"
        "bnez t2, 1f\n\t"
        "addi %[acc], %[acc], 5\n\t"
        "j 2f\n\t"
        ".balign 16\n\t"
        "1:\n\t"
        "vle64.v v10, (%[src])\n\t"
        ".rept 24\n\t"
        "vadd.vv v11, v10, v10\n\t"
        "vadd.vv v12, v11, v10\n\t"
        "vadd.vv v13, v12, v11\n\t"
        "vadd.vv v14, v13, v12\n\t"
        ".endr\n\t"
        "vse64.v v12, (%[dst])\n\t"
        "addi %[acc], %[acc], 7\n\t"
        "2:\n\t"
        ".option pop\n\t"
        : [acc] "+r"(acc)
        : [guard] "r"(branch_guard_paddr),
          [src] "r"(vec_src_paddr),
          [dst] "r"(vec_dst_paddr)
        : "t2", "memory");

    return acc + rounds;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_readarb_cancelcounter_probe(uint64_t seed)
{
    uint64_t acc = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "li s8, 0x111\n\t"
        "li s9, 0x222\n\t"
        "li s10, 0x333\n\t"
        "li s11, 0x444\n\t"
        "li t0, 0x10\n\t"
        "li t1, 0x20\n\t"
        "li t2, 0x30\n\t"
        "li t3, 0x40\n\t"
        "li t4, 0x50\n\t"
        "li t5, 0x60\n\t"
        "li t6, 0x70\n\t"
        "li a0, 0x80\n\t"
        "li a1, 0x90\n\t"
        "li a2, 0xa0\n\t"
        "li a3, 0xb0\n\t"
        "li a4, 0xc0\n\t"
        "li a5, 0xd0\n\t"
        "li a6, 0xe0\n\t"
        "li a7, 0xf0\n\t"

        ".rept 64\n\t"
        "addi t0, t0, 1\n\t"
        "addi t1, t1, 2\n\t"
        "addi t2, t2, 3\n\t"
        "addi t3, t3, 4\n\t"
        "addi t4, t4, 5\n\t"
        "addi t5, t5, 6\n\t"
        "addi t6, t6, 7\n\t"
        "addi a0, a0, 8\n\t"
        "addi a1, a1, 9\n\t"
        "addi a2, a2, 10\n\t"
        "addi a3, a3, 11\n\t"
        "addi a4, a4, 12\n\t"
        "addi a5, a5, 13\n\t"
        "addi a6, a6, 14\n\t"
        "addi a7, a7, 15\n\t"
        ".endr\n\t"

        ".rept 256\n\t"
        ".balign 16\n\t"
        "add t0, s8, s9\n\t"
        "bne s8, s9, 1f\n\t"
        "1:\n\t"
        "add t1, s8, s10\n\t"
        "bne s10, s11, 1f\n\t"
        "1:\n\t"
        "add t2, s8, s11\n\t"
        "blt s8, s9, 1f\n\t"
        "1:\n\t"
        "add t3, s9, s10\n\t"
        "bge s11, s10, 1f\n\t"
        "1:\n\t"
        "add t4, s9, s11\n\t"
        "bltu s8, s9, 1f\n\t"
        "1:\n\t"
        "add t5, s10, s11\n\t"
        "bgeu s11, s8, 1f\n\t"
        "1:\n\t"
        "add t6, s11, s8\n\t"
        "add a0, s10, s9\n\t"
        "add a1, s8, s9\n\t"
        "add a2, s8, s10\n\t"
        "add a3, s8, s11\n\t"
        "add a4, s9, s10\n\t"
        "add a5, s9, s11\n\t"
        "add a6, s10, s11\n\t"
        "add a7, s11, s8\n\t"
        "add t0, s10, s9\n\t"
        ".endr\n\t"

        "divu a7, s11, s8\n\t"
        ".rept 48\n\t"
        "add a2, s8, s9\n\t"
        "add a3, s8, s10\n\t"
        "add a4, s8, s11\n\t"
        "add a5, s9, s10\n\t"
        "add a6, s9, s11\n\t"
        "add t6, s10, s11\n\t"
        "add t0, s11, s8\n\t"
        "add t1, s10, s9\n\t"
        ".endr\n\t"

        ".rept " STR(AI_FCOV_READARB_CANCEL_ROUNDS) "\n\t"
        ".balign 16\n\t"
        "add t0, s8, s9\n\t"
        "add t1, s10, s11\n\t"
        "add t2, s8, s10\n\t"
        "add t3, s9, s11\n\t"
        "bne s8, s9, 1f\n\t"
        "addi %[acc], %[acc], 0x21\n\t"
        "1:\n\t"
        "add t4, s8, s11\n\t"
        "add t5, s9, s10\n\t"
        "add t6, s10, s8\n\t"
        "blt s8, s9, 2f\n\t"
        "addi %[acc], %[acc], 0x22\n\t"
        "2:\n\t"
        "add a0, s11, s9\n\t"
        "add a1, s8, s10\n\t"
        "add a2, s9, s11\n\t"
        "bge s11, s10, 3f\n\t"
        "addi %[acc], %[acc], 0x23\n\t"
        "3:\n\t"
        "add a3, s8, s11\n\t"
        "add a4, s9, s10\n\t"
        "add a5, s10, s11\n\t"
        "bgeu s11, s8, 4f\n\t"
        "addi %[acc], %[acc], 0x24\n\t"
        "4:\n\t"
        "xor %[acc], %[acc], t0\n\t"
        "add %[acc], %[acc], t4\n\t"
        ".endr\n\t"
        ".option pop\n\t"
        : [acc] "+r"(acc)
        :
        : "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
          "t0", "t1", "t2", "t3", "t4", "t5", "t6",
          "s8", "s9", "s10", "s11", "memory");

    return acc;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_readarb_aligned_branch_probe(uint64_t seed)
{
    uint64_t acc = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "li s8, 0x111\n\t"
        "li s9, 0x222\n\t"
        "li s10, 0x333\n\t"
        "li s11, 0x444\n\t"

        ".rept " STR(AI_FCOV_READARB_ALIGNED_ROUNDS) "\n\t"
        ".balign 32\n\t"
        "add t0, s8, s9\n\t"
        "add t1, s9, s10\n\t"
        "add t2, s10, s11\n\t"
        "bne s8, s9, 1f\n\t"
        "1:\n\t"
        "bne s9, s10, 2f\n\t"
        "2:\n\t"
        "bne s10, s11, 3f\n\t"
        "3:\n\t"
        "xor %[acc], %[acc], t0\n\t"
        "add %[acc], %[acc], t1\n\t"
        ".endr\n\t"

        ".rept 128\n\t"
        ".balign 32\n\t"
        "add t3, s8, s10\n\t"
        "add t4, s9, s11\n\t"
        "add t5, s10, s8\n\t"
        "add t6, s11, s9\n\t"
        "bge s11, s10, 4f\n\t"
        "4:\n\t"
        "bltu s8, s9, 5f\n\t"
        "5:\n\t"
        "add %[acc], %[acc], t3\n\t"
        "xor %[acc], %[acc], t6\n\t"
        ".endr\n\t"
        ".option pop\n\t"
        : [acc] "+r"(acc)
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6",
          "s8", "s9", "s10", "s11", "memory");

    return acc;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_readarb_delayed_branch_probe(uint64_t seed)
{
    uint64_t acc = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "li s8, 0x111\n\t"
        "li s9, 0x222\n\t"
        "li s10, 0x333\n\t"
        "li s11, 0x444\n\t"

        ".rept " STR(AI_FCOV_READARB_DELAYED_ROUNDS) "\n\t"
        ".balign 32\n\t"
        "divu t0, s11, s8\n\t"
        "bne t0, s8, 1f\n\t"
        "1:\n\t"
        "bne t0, s9, 2f\n\t"
        "2:\n\t"
        "bne t0, s10, 3f\n\t"
        "3:\n\t"
        "bne t0, s11, 4f\n\t"
        "4:\n\t"
        ".rept 24\n\t"
        "add t1, s8, s9\n\t"
        "add t2, s9, s10\n\t"
        "add t3, s10, s11\n\t"
        "add t4, s11, s8\n\t"
        "add t5, s8, s10\n\t"
        "add t6, s9, s11\n\t"
        "add a0, s10, s8\n\t"
        "add a1, s11, s9\n\t"
        ".endr\n\t"
        "xor %[acc], %[acc], t1\n\t"
        "add %[acc], %[acc], t6\n\t"
        ".endr\n\t"
        ".option pop\n\t"
        : [acc] "+r"(acc)
        :
        : "a0", "a1", "t0", "t1", "t2", "t3", "t4", "t5", "t6",
          "s8", "s9", "s10", "s11", "memory");

    return acc;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_readarb_bju3_div_probe(uint64_t seed)
{
    uint64_t acc = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "li s8, 0x1357\n\t"
        "li s9, 0x2468\n\t"
        "li s10, 0x3579\n\t"
        "li s11, 0x468a\n\t"

        ".rept " STR(AI_FCOV_READARB_BJU3_DIV_ROUNDS) "\n\t"
        ".balign 32\n\t"
        "divu a2, s11, s8\n\t"
        "remu a3, s10, s9\n\t"
        ".rept 32\n\t"
        "add t0, s8, s9\n\t"
        "add t1, s9, s10\n\t"
        "add t2, s10, s11\n\t"
        "add t3, s11, s8\n\t"
        "add t4, s8, s10\n\t"
        "add t5, s9, s11\n\t"
        "add t6, s10, s8\n\t"
        "add a0, s11, s9\n\t"
        ".endr\n\t"
        "xor %[acc], %[acc], a2\n\t"
        "add %[acc], %[acc], a3\n\t"
        ".endr\n\t"
        ".option pop\n\t"
        : [acc] "+r"(acc)
        :
        : "a0", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6",
          "s8", "s9", "s10", "s11", "memory");

    return acc;
}

__attribute__((noinline, aligned(64))) static void
ai_fcov_iq_load_cancel_wakeup_probe(uintptr_t load_vaddr,
    uintptr_t slots_vaddr, uintptr_t marker_vaddr, uintptr_t obs_vaddr,
    uint64_t payload_mask, uint64_t side_marker)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "mv a1, %[load]\n\t"
        "mv a2, %[slots]\n\t"
        "mv a3, %[marker]\n\t"
        "mv a4, %[obs]\n\t"
        "mv a5, %[mask]\n\t"
        "mv a6, %[side]\n\t"
        "li s8, 0x111\n\t"
        "li s9, 0x222\n\t"
        "li s10, 0x333\n\t"
        "li s11, 0x444\n\t"
        "li t0, 0\n\t"

        ".balign 32\n\t"
        "ld t0, 0(a1)\n\t"
        "add t1, t0, s8\n\t"
        "add t2, s9, t0\n\t"
        "xor t3, t1, t2\n\t"
        "add a7, a7, t3\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 4\n\t"
        "10:\n\t"
        "add a7, a7, s8\n\t"
        "xor a7, a7, s9\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 10b\n\t"
        "add t1, t0, s8\n\t"
        "add t2, s9, t0\n\t"
        "add t3, t1, s10\n\t"
        "xor t4, s11, t2\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 8\n\t"
        "11:\n\t"
        "xor a7, a7, s10\n\t"
        "add a7, a7, s11\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 11b\n\t"
        "add t1, t0, s9\n\t"
        "xor t2, s10, t0\n\t"
        "add t3, t1, t2\n\t"
        "xor t4, t2, s8\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 16\n\t"
        "12:\n\t"
        "add a7, a7, s8\n\t"
        "xor a7, a7, s10\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 12b\n\t"
        "xor t1, t0, s11\n\t"
        "add t2, s8, t0\n\t"
        "add t3, t1, s9\n\t"
        "xor t4, s10, t2\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 24\n\t"
        "13:\n\t"
        "xor a7, a7, s9\n\t"
        "add a7, a7, s11\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 13b\n\t"
        "add t1, t0, s10\n\t"
        "xor t2, s11, t0\n\t"
        "add t3, t1, t2\n\t"
        "xor t4, t3, s8\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 32\n\t"
        "14:\n\t"
        "add a7, a7, s8\n\t"
        "xor a7, a7, s11\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 14b\n\t"
        "xor t1, t0, s8\n\t"
        "add t2, s9, t0\n\t"
        "add t3, t1, s10\n\t"
        "xor t4, t2, s11\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 48\n\t"
        "15:\n\t"
        "xor a7, a7, s8\n\t"
        "add a7, a7, s10\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 15b\n\t"
        "add t1, t0, s11\n\t"
        "xor t2, s8, t0\n\t"
        "add t3, t1, t2\n\t"
        "xor t4, t3, s9\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "li a0, 64\n\t"
        "16:\n\t"
        "add a7, a7, s9\n\t"
        "xor a7, a7, s10\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 16b\n\t"
        "xor t1, t0, s9\n\t"
        "add t2, s11, t0\n\t"
        "add t3, t1, s8\n\t"
        "xor t4, t2, s10\n\t"
        "add a7, a7, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "add t1, t0, s8\n\t"
        "add t2, t1, s9\n\t"
        "add t3, t1, s9\n\t"
        "xor t4, t1, s10\n\t"
        "add t5, t3, t0\n\t"
        "xor t6, t4, t0\n\t"
        "add a7, t5, t6\n\t"

        "ld t0, 0(a1)\n\t"
        "addi a7, a7, 1\n\t"
        "add t6, s8, s9\n\t"
        "add t1, t0, s10\n\t"
        "xor t2, t1, t6\n\t"
        "add t3, t2, t0\n\t"
        "xor t4, t3, s10\n\t"
        "add a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "addi a7, a7, 2\n\t"
        "add t6, s9, s10\n\t"
        "xor t6, t6, s11\n\t"
        "add t5, t0, s11\n\t"
        "xor t6, t5, t6\n\t"
        "add a7, a7, t6\n\t"

        "ld t0, 0(a1)\n\t"
        "addi a7, a7, 3\n\t"
        "add t6, s10, s11\n\t"
        "xor t6, t6, s8\n\t"
        "add t6, t6, s9\n\t"
        "add t1, t0, s8\n\t"
        "xor t2, t1, t6\n\t"
        "add t3, t2, s8\n\t"
        "xor t4, t3, t0\n\t"
        "add t5, t4, t3\n\t"
        "xor t6, t5, t4\n\t"
        "add a7, a7, t6\n\t"

        "ld t0, 0(a1)\n\t"
        "mul t1, t0, s8\n\t"
        "add t2, t1, s9\n\t"
        "xor t3, t1, s10\n\t"
        "add t4, t2, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "addi a7, a7, 5\n\t"
        "mul t1, t0, s9\n\t"
        "add t2, t1, s10\n\t"
        "xor t3, t1, s11\n\t"
        "add t4, t2, t3\n\t"
        "xor a7, a7, t4\n\t"

        "ld t0, 0(a1)\n\t"
        "addi a7, a7, 6\n\t"
        "add t6, s8, s11\n\t"
        "mul t1, t0, s10\n\t"
        "add t2, t1, t6\n\t"
        "xor t3, t1, s9\n\t"
        "add a7, a7, t2\n\t"

        "ld t0, 0(a1)\n\t"
        "addi a7, a7, 7\n\t"
        "add t6, s9, s11\n\t"
        "xor t6, t6, s8\n\t"
        "mul t1, t0, s11\n\t"
        "add t2, t1, t6\n\t"
        "xor t3, t1, s10\n\t"
        "add a7, a7, t3\n\t"

        ".rept 8\n\t"
        "add t1, t1, s8\n\t"
        "xor t2, t2, s9\n\t"
        "add t3, t3, s10\n\t"
        "xor t4, t4, s11\n\t"
        "add t5, t5, t1\n\t"
        "xor t6, t6, t2\n\t"
        "add a7, a7, t3\n\t"
        "xor s8, s8, t4\n\t"
        ".endr\n\t"

        "bnez t0, 1f\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "andi t1, t0, 24\n\t"
        "add t2, a2, t1\n\t"
        "xor t3, t0, a5\n\t"
        "sd t3, 0(t2)\n\t"
        "sd a6, 0(a3)\n\t"
        "sd t0, 0(a4)\n\t"
        "sd t1, 8(a4)\n\t"
        "2:\n\t"
        "sd t0, 16(a4)\n\t"
        "ld t4, 0(a3)\n\t"
        "sd t4, 24(a4)\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [load] "r"(load_vaddr),
          [slots] "r"(slots_vaddr),
          [marker] "r"(marker_vaddr),
          [obs] "r"(obs_vaddr),
          [mask] "r"(payload_mask),
          [side] "r"(side_marker)
        : "a0", "t0", "t1", "t2", "t3", "t4", "t5", "t6",
          "a1", "a2", "a3", "a4", "a5", "a6", "a7",
          "s8", "s9", "s10", "s11", "memory");
}

static bool ai_fcov_run_iq_load_cancel_regcache_window(uintptr_t offset,
    uint64_t load_value, uint64_t slot_seed, uint64_t marker_seed,
    uint64_t obs_seed)
{
    const enum test_page load_page = SWITCH1;
    const enum test_page side_page = SWITCH2;
    const uintptr_t load_vaddr = ai_fcov_hs_vaddr(load_page, offset);
    const uintptr_t load_paddr = phys_page_base(load_page) + offset;
    const uintptr_t slots_vaddr = ai_fcov_hs_vaddr(side_page, offset + 0x80U);
    const uintptr_t slots_paddr = phys_page_base(side_page) + offset + 0x80U;
    const uintptr_t marker_vaddr = ai_fcov_hs_vaddr(side_page, offset + 0xc0U);
    const uintptr_t marker_paddr = phys_page_base(side_page) + offset + 0xc0U;
    const uintptr_t obs_vaddr = ai_fcov_hs_vaddr(side_page, offset + 0xe0U);
    const uintptr_t obs_paddr = phys_page_base(side_page) + offset + 0xe0U;
    const uint64_t payload_mask = 0x99aa55aa33cc77ddULL;
    const uint64_t side_marker = 0xfedcba9876540000ULL + offset;
    const uint64_t expected_payload = load_value ^ payload_mask;

    goto_priv(PRIV_M);
    ai_fcov_fill_u64_pattern(slots_paddr, AI_FCOV_IQ_CANCEL_SLOTS,
        slot_seed);
    write64(marker_paddr, marker_seed);
    ai_fcov_fill_u64_pattern(obs_paddr, AI_FCOV_IQ_CANCEL_SLOTS, obs_seed);

    ai_fcov_prepare_hs_tables();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    ai_fcov_set_hs_leaf_to_page(load_page, load_page,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_fcov_set_hs_leaf_to_page(side_page, side_page,
        PTE_V | PTE_AD | PTE_RWX);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM);
    TEST_SETUP_EXCEPT();
    write64(load_vaddr, load_value);
    fence_iorw_iorw();
    if (excpt.triggered) {
        goto_priv(PRIV_M);
        ai_fcov_set_hs_leaf_to_page(load_page, load_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_fcov_set_hs_leaf_to_page(side_page, side_page,
            PTE_V | PTE_AD | PTE_RWX);
        CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
        fence_iorw_iorw();
        sfence_vma();
        return false;
    }

    TEST_SETUP_EXCEPT();
    ai_fcov_iq_load_cancel_wakeup_probe(load_vaddr, slots_vaddr,
        marker_vaddr, obs_vaddr, payload_mask, side_marker);

    const bool ok = !excpt.triggered &&
        read64(obs_vaddr) == load_value &&
        read64(obs_vaddr + 8U) == (load_value & 24ULL) &&
        read64(obs_vaddr + 16U) == load_value &&
        read64(obs_vaddr + 24U) == side_marker &&
        read64(marker_vaddr) == side_marker &&
        read64(slots_vaddr + (load_value & 24ULL)) == expected_payload;

    goto_priv(PRIV_M);
    ai_fcov_set_hs_leaf_to_page(load_page, load_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_fcov_set_hs_leaf_to_page(side_page, side_page,
        PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    fence_iorw_iorw();
    sfence_vma();

    return ok;
}

static bool ai_fcov_run_iq_load_cancel_regcache_matrix(void)
{
    for (unsigned i = 0; i < AI_FCOV_IQ_CANCEL_WINDOWS; ++i) {
        const uintptr_t offset = 0xa00U + (i * 0x80U);
        const uint64_t load = 0x6d5a4b3c2d1e0f18ULL + (i * 8ULL);
        const uint64_t slot_seed = 0x3141592600000100ULL + i;
        const uint64_t marker = 0x0badc0de00000000ULL + i;
        const uint64_t obs = 0x1234567800000000ULL + i;

        if (!ai_fcov_run_iq_load_cancel_regcache_window(
                offset, load, slot_seed, marker, obs)) {
            return false;
        }
    }

    return true;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_fcov_iq_pair_vector_mem_probe(uintptr_t src_paddr, uintptr_t dst_paddr,
    uintptr_t marker_paddr, uint64_t seed)
{
    uint64_t acc = seed;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".option norelax\n\t"
        "mv s8, %[src]\n\t"
        "mv s9, %[dst]\n\t"
        "li s10, 0x1357\n\t"
        "li s11, 0x2468\n\t"
        "li t0, 8\n\t"
        "li t1, 0x20\n\t"

        "vsetvli t2, t0, e32, m1, ta, ma\n\t"
        "csrr t6, vtype\n\t"
        "vle32.v v8, (s8)\n\t"
        "addi t3, s8, 0x80\n\t"
        "vle32.v v12, (t3)\n\t"
        "vmv.v.i v16, 1\n\t"
        "vmv.v.i v24, 3\n\t"

        "li a0, 96\n\t"
        ".balign 32\n\t"
        "7:\n\t"
        "beq s10, s11, 8f\n\t"
        "vsetvl t4, t0, t6\n\t"
        "xor %[acc], %[acc], t4\n\t"
        "addi a0, a0, -1\n\t"
        "bnez a0, 7b\n\t"
        "8:\n\t"

        ".rept 96\n\t"
        ".balign 32\n\t"
        "beq s10, s10, 1f\n\t"
        "1:\n\t"
        "vsetvli zero, t0, e32, m1, ta, ma\n\t"
        "beq s10, s10, 2f\n\t"
        "2:\n\t"
        "vsetvli zero, t0, e16, m1, ta, ma\n\t"
        "bne s10, s11, 3f\n\t"
        "3:\n\t"
        "vsetvli zero, t0, e8, m1, ta, ma\n\t"
        "beq s10, s10, 4f\n\t"
        "4:\n\t"
        "vsetvl t4, t0, t6\n\t"
        "beq s10, s10, 5f\n\t"
        "5:\n\t"
        "vsetvl t5, t0, t6\n\t"
        "bne s10, s11, 6f\n\t"
        "6:\n\t"
        "vsetvl t4, t0, t6\n\t"
        "xor %[acc], %[acc], t4\n\t"
        "add %[acc], %[acc], t5\n\t"
        ".endr\n\t"

        "vsetvli t2, t0, e32, m1, ta, ma\n\t"

        ".rept 96\n\t"
        ".balign 32\n\t"
        "vfadd.vv v16, v8, v12\n\t"
        "vsetvli zero, zero, e32, m1, ta, ma\n\t"
        "vfmacc.vv v16, v8, v12\n\t"
        "vrgather.vv v18, v8, v12\n\t"
        "vadd.vv v20, v8, v12\n\t"
        "vredsum.vs v22, v8, v12\n\t"
        "vfadd.vv v24, v8, v12\n\t"
        "vmacc.vv v26, v8, v12\n\t"
        "vfmacc.vv v16, v8, v12\n\t"
        "vfcvt.x.f.v v28, v16\n\t"
        "vadd.vv v20, v20, v8\n\t"
        "vfcvt.x.f.v v30, v24\n\t"
        ".endr\n\t"

        ".rept " STR(AI_FCOV_VECTOR_PAIR_ROUNDS) "\n\t"
        ".balign 32\n\t"
        "bne s10, s11, 1f\n\t"
        "addi %[acc], %[acc], 1\n\t"
        "1:\n\t"
        "vsetvli t4, t0, e32, m1, ta, ma\n\t"
        "vsetvli t5, t0, e16, m1, ta, ma\n\t"
        "vsetvli zero, zero, e32, m1, ta, ma\n\t"

        ".balign 32\n\t"
        "vsetvli zero, t0, e32, m1, ta, ma\n\t"
        "vfadd.vv v16, v8, v12\n\t"
        "vfmacc.vv v16, v8, v12\n\t"
        "vsext.vf2 v18, v8\n\t"
        "vredsum.vs v20, v8, v12\n\t"
        "vrgather.vi v22, v8, 1\n\t"
        "vmacc.vv v24, v8, v12\n\t"
        "vfcvt.x.f.v v26, v16\n\t"

        ".balign 32\n\t"
        "vsetvli zero, t0, e8, m1, ta, ma\n\t"
        "vle8.v v4, (s8)\n\t"
        "vse8.v v4, (s9)\n\t"
        "vlseg2e8.v v6, (s8)\n\t"
        "vsseg2e8.v v6, (s9)\n\t"
        "vle8.v v4, (s8)\n\t"
        "vlseg2e8.v v6, (s8)\n\t"
        "vsseg2e8.v v6, (s9)\n\t"
        "vse8.v v4, (s9)\n\t"

        ".balign 32\n\t"
        "vsetvli zero, t0, e32, m1, ta, ma\n\t"
        "vle32.v v4, (s8)\n\t"
        "vlseg2e32.v v6, (s8)\n\t"
        "vsseg2e32.v v6, (s9)\n\t"
        "vse32.v v4, (s9)\n\t"
        "vlseg2e32.v v6, (s8)\n\t"
        "vle32.v v4, (s8)\n\t"
        "vse32.v v4, (s9)\n\t"
        "vsseg2e32.v v6, (s9)\n\t"
        "xor %[acc], %[acc], t1\n\t"
        "addi t1, t1, 1\n\t"
        ".endr\n\t"

        "addi %[acc], %[acc], 0x5a\n\t"
        "sd %[acc], 0(%[marker])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [acc] "+r"(acc)
        : [src] "r"(src_paddr),
          [dst] "r"(dst_paddr),
          [marker] "r"(marker_paddr)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0",
          "s8", "s9", "s10", "s11", "memory");

    return acc;
}

bool ai_micro_backend_fcov_decode_fusion_vset_corner()
{
    TEST_START();

    const uintptr_t guard_paddr = ai_fcov_paddr(0x600U);
    const uint64_t seed = 0x1122334455667788ULL;

    ai_fcov_enable_vector_state();
    write64(guard_paddr, 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_decode_fusion_probe(guard_paddr, seed);
    TEST_ASSERT("decode/fusion fcov stimulus retires normally while placing vset, lui32 fusion, sexth fusion, and wrong-path XSTrap patterns in aligned decode lanes",
        excpt.triggered == false &&
        observed == seed + 108ULL + 0x12345678ULL - 7ULL,
        "observed=%lx expected=%lx", observed,
        seed + 108ULL + 0x12345678ULL - 7ULL);

    TEST_END();
}

bool ai_micro_backend_fcov_decode_lane3_xstrap_terminal_corner()
{
    TEST_START();

    const uintptr_t guard_paddr = ai_fcov_paddr(0x620U);
    const uint64_t seed = 0x0102030405060708ULL;

    ai_fcov_enable_vector_state();
    write64(guard_paddr, 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_decode_fusion_probe(guard_paddr, seed);
    TEST_ASSERT("pre-terminal decode/fusion stimulus completes before issuing the lane3 XSTrap fcov terminator",
        excpt.triggered == false &&
        observed == seed + 108ULL + 0x12345678ULL - 7ULL,
        "observed=%lx expected=%lx", observed,
        seed + 108ULL + 0x12345678ULL - 7ULL);

    INFO("ai_micro_backend_fcov_decode_lane3_xstrap_terminal_corner: issuing LinkNan lane3 XSTrap terminator");
    ai_fcov_lane3_good_xstrap();

    TEST_END();
}

bool ai_micro_backend_fcov_iq_mou_dispatch_pressure_corner()
{
    TEST_START();

    const uintptr_t amo0_paddr = ai_fcov_paddr(0x700U);
    const uintptr_t amo1_paddr = ai_fcov_paddr(0x708U);
    const uint64_t seed0 = 0x0102030405060708ULL;
    const uint64_t seed1 = 0x1112131415161718ULL;

    goto_priv(PRIV_M);
    write64(amo0_paddr, seed0);
    write64(amo1_paddr, seed1);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_iq_mou_dispatch_probe(
        amo0_paddr, amo1_paddr, 0x5a5a0000ULL);
    TEST_ASSERT("CSR/fence and paired AMO bursts retire without exception while stressing intIQ3 enq1 and memIQ0/4 MOU enq1 coverage bins",
        excpt.triggered == false &&
        observed != 0ULL &&
        read64(amo0_paddr) == seed0 &&
        read64(amo1_paddr) == seed1,
        "observed=%lx amo0=%lx amo1=%lx",
        observed, read64(amo0_paddr), read64(amo1_paddr));

    TEST_END();
}

bool ai_micro_backend_fcov_readarb_redirect_pressure_corner()
{
    TEST_START();

    const uintptr_t src_paddr = ai_fcov_paddr(0x800U);
    const uintptr_t dst_paddr = ai_fcov_paddr(0x840U);
    const uintptr_t guard_paddr = ai_fcov_paddr(0x880U);
    const uint64_t src0 = 0x0101010101010101ULL;
    const uint64_t src1 = 0x0202020202020202ULL;
    const uint64_t src2 = 0x0303030303030303ULL;
    const uint64_t src3 = 0x0404040404040404ULL;

    ai_fcov_enable_vector_state();
    write64(src_paddr, src0);
    write64(src_paddr + 8U, src1);
    write64(src_paddr + 16U, src2);
    write64(src_paddr + 24U, src3);
    write64(dst_paddr, 0ULL);
    write64(dst_paddr + 8U, 0ULL);
    write64(dst_paddr + 16U, 0ULL);
    write64(dst_paddr + 24U, 0ULL);
    write64(guard_paddr, 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_readarb_redirect_probe(
        src_paddr, dst_paddr, guard_paddr, 0x1234ULL);
    TEST_ASSERT("branch/ALU read-port pressure plus load-delayed predicted-taken vector wrong path retires normally for read-arb and vfIQ redirect-cancel coverage",
        excpt.triggered == false &&
        observed != 0ULL &&
        read64(dst_paddr) == (src0 << 1) &&
        read64(dst_paddr + 8U) == (src1 << 1) &&
        read64(dst_paddr + 16U) == 0ULL &&
        read64(dst_paddr + 24U) == 0ULL,
        "observed=%lx dst=%lx/%lx/%lx/%lx",
        observed,
        read64(dst_paddr),
        read64(dst_paddr + 8U),
        read64(dst_paddr + 16U),
        read64(dst_paddr + 24U));

    TEST_END();
}

bool ai_micro_backend_fcov_readarb_cancelcounter_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_readarb_cancelcounter_probe(0x31415926ULL);
    TEST_ASSERT("dense taken-branch and ALU bursts retire normally while stressing int RF read arbiter low-priority BJU cancel counters",
        excpt.triggered == false && observed != 0ULL,
        "observed=%lx", observed);

    TEST_END();
}

bool ai_micro_backend_fcov_readarb_aligned_branch_counter_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_readarb_aligned_branch_probe(0x27182818ULL);
    TEST_ASSERT("aligned 3-ALU/3-branch groups retire normally while creating sustained BJU0/BJU1/BJU2 read-arb starvation pressure",
        excpt.triggered == false && observed != 0ULL,
        "observed=%lx", observed);

    TEST_END();
}

bool ai_micro_backend_fcov_readarb_delayed_branch_counter_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_readarb_delayed_branch_probe(0x16180339ULL);
    TEST_ASSERT("div-delayed branch requests retire normally while independent ALU bursts keep high-priority int read ports busy for BJU cancel-counter fairness coverage",
        excpt.triggered == false && observed != 0ULL,
        "observed=%lx", observed);

    TEST_END();
}

bool ai_micro_backend_fcov_readarb_bju3_div_counter_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_readarb_bju3_div_probe(0x42424242ULL);
    TEST_ASSERT("BJU3 div/rem requests retire normally while independent ALU bursts keep arb0 high-priority ALU0 reads active for BJU3 cancel-counter fairness coverage",
        excpt.triggered == false && observed != 0ULL,
        "observed=%lx", observed);

    TEST_END();
}

bool ai_micro_backend_fcov_iq_load_cancel_regcache_wakeup_corner()
{
    TEST_START();

    TEST_ASSERT("PBMT=NC load replay windows retire while dependent ALU chains create IQ wakeup/regcache pressure for load-cancel RC status cross coverage",
        ai_fcov_run_iq_load_cancel_regcache_matrix());

    TEST_END();
}

bool ai_micro_backend_fcov_iq_pair_vector_mem_mix_corner()
{
    TEST_START();

    const uintptr_t src_paddr = ai_fcov_paddr(0xb00U);
    const uintptr_t dst_paddr = ai_fcov_paddr(0xd00U);
    const uintptr_t marker_paddr = ai_fcov_paddr(0xef0U);
    const uint64_t marker_seed = 0x0f0e0d0c0b0a0908ULL;

    goto_priv(PRIV_M);
    ai_fcov_enable_vector_state();
    ai_fcov_fill_u64_pattern(src_paddr, AI_FCOV_VECTOR_PAIR_QWORDS,
        0x3f80000040000000ULL);
    ai_fcov_fill_u64_pattern(dst_paddr, AI_FCOV_VECTOR_PAIR_QWORDS,
        0xaaaaaaaa55555555ULL);
    write64(marker_paddr, marker_seed);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_fcov_iq_pair_vector_mem_probe(src_paddr,
        dst_paddr, marker_paddr, 0x55aa3300ULL);
    TEST_ASSERT("dense branch/vset, vfIQ, vfcvt, and vector memory pairs retire normally while stressing backend fcov pair-cross bins",
        excpt.triggered == false &&
        observed != 0x55aa3300ULL &&
        read64(marker_paddr) == observed &&
        read64(dst_paddr) != 0xaaaaaaaa55555555ULL,
        "observed=%lx marker=%lx dst0=%lx",
        observed, read64(marker_paddr), read64(dst_paddr));

    TEST_END();
}

bool ai_micro_backend_fcov_fusion_disable_custom_csr_corner()
{
    TEST_START();

    const uintptr_t guard_paddr = ai_fcov_paddr(0x900U);
    uint64_t original = 0ULL;
    uint64_t observed_ctl = 0ULL;

    ai_fcov_enable_vector_state();
    write64(guard_paddr, 0ULL);

    TEST_SETUP_EXCEPT();
    original = ai_fcov_read_srnctl();
    TEST_ASSERT("LinkNan custom SRNCTL can be read before disabling fusion for fcov stimulus",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    ai_fcov_write_srnctl(original & ~AI_FCOV_SRNCTL_FUSION_ENABLE);
    observed_ctl = ai_fcov_read_srnctl();
    TEST_ASSERT("LinkNan custom SRNCTL fusion_enable bit can be cleared",
        excpt.triggered == false &&
        (observed_ctl & AI_FCOV_SRNCTL_FUSION_ENABLE) == 0ULL,
        "srnctl=%lx", observed_ctl);

    TEST_SETUP_EXCEPT();
    uint64_t marker = ai_fcov_decode_fusion_probe(guard_paddr, 0xabcdefULL);
    TEST_ASSERT("with SRNCTL fusion disabled, aligned fusion-candidate instruction pairs still retire as ordinary instructions",
        excpt.triggered == false &&
        marker == 0xabcdefULL + 108ULL + 0x12345678ULL - 7ULL,
        "marker=%lx", marker);

    TEST_SETUP_EXCEPT();
    ai_fcov_write_srnctl(original);
    TEST_ASSERT("LinkNan custom SRNCTL is restored after fusion-disable fcov stimulus",
        excpt.triggered == false);

    TEST_END();
}
