#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stddef.h>

static uint8_t ai_arch_rvc_sp_area[4096] __attribute__((aligned(64)));
static uint8_t ai_arch_rvc_mem[128] __attribute__((aligned(64)));

#define AI_ARCH_RVC_LOAD_FAULT_LEAF (PTE_V | PTE_AD | PTE_X)
#define AI_ARCH_RVC_STORE_FAULT_LEAF (PTE_V | PTE_AD | PTE_R)
#define AI_ARCH_RVC_LEGAL_LEAF (PTE_V | PTE_AD | PTE_RWX)

typedef struct {
    uintptr_t pc;
    uintptr_t next_pc;
} ai_arch_rvc_probe_pc_t;

static volatile uint64_t ai_arch_rvc_reserved_trap_hits;
static volatile uint64_t ai_arch_rvc_reserved_trap_cause;
static volatile uint64_t ai_arch_rvc_reserved_trap_epc;
static volatile uint64_t ai_arch_rvc_reserved_trap_tval;

extern void ai_arch_rvc_reserved_hs_trap_entry(void);

/* Keep the reserved-encoding trap path off the test sp page. */
asm(
    ".balign 4\n"
    ".global ai_arch_rvc_reserved_hs_trap_entry\n"
    "ai_arch_rvc_reserved_hs_trap_entry:\n"
    "  csrr t0, scause\n"
    "  la t1, ai_arch_rvc_reserved_trap_cause\n"
    "  sd t0, 0(t1)\n"
    "  csrr t0, sepc\n"
    "  la t1, ai_arch_rvc_reserved_trap_epc\n"
    "  sd t0, 0(t1)\n"
    "  csrr t0, stval\n"
    "  la t1, ai_arch_rvc_reserved_trap_tval\n"
    "  sd t0, 0(t1)\n"
    "  la t1, ai_arch_rvc_reserved_trap_hits\n"
    "  ld t2, 0(t1)\n"
    "  addi t2, t2, 1\n"
    "  sd t2, 0(t1)\n"
    "  csrr t0, sepc\n"
    "  addi t0, t0, 2\n"
    "  csrw sepc, t0\n"
    "  sret\n"
);

static inline pte_t *ai_arch_rvc_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline uint64_t ai_arch_rvc_hs_leaf_value(enum test_page page,
    uint64_t flags)
{
    return (phys_page_base(page) >> 2) | flags;
}

static inline void ai_arch_rvc_prepare_hs_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();
}

static inline void ai_arch_rvc_set_hs_leaf(enum test_page page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_arch_rvc_hs_leaf_entry(page) =
        ai_arch_rvc_hs_leaf_value(page, flags);
    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline bool ai_arch_rvc_hs_leaf_matches(enum test_page page,
    uint64_t flags)
{
    return *ai_arch_rvc_hs_leaf_entry(page) ==
        ai_arch_rvc_hs_leaf_value(page, flags);
}

static inline void ai_arch_rvc_clear(uint8_t *base, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        base[i] = 0;
    }
}

static inline void ai_arch_rvc_write32(uint8_t *base, size_t offset,
    uint32_t value)
{
    *(volatile uint32_t *)(base + offset) = value;
}

static inline uint32_t ai_arch_rvc_read32(uint8_t *base, size_t offset)
{
    return *(volatile uint32_t *)(base + offset);
}

static inline void ai_arch_rvc_write64(uint8_t *base, size_t offset,
    uint64_t value)
{
    *(volatile uint64_t *)(base + offset) = value;
}

static inline uint64_t ai_arch_rvc_read64(uint8_t *base, size_t offset)
{
    return *(volatile uint64_t *)(base + offset);
}

static inline uint64_t ai_arch_rvc_read_x0(void)
{
    uint64_t value;

    asm volatile("mv %0, x0\n\t" : "=r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_rvc_lwsp_probe(uintptr_t sp_base,
    uint64_t *span)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, sp\n\t"
        "mv sp, %[sp_base]\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.lwsp a0, 16(sp)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 2b\n\t"
        "lla t1, 1b\n\t"
        "sub t0, t0, t1\n\t"
        "mv %[value], a0\n\t"
        "mv %[span], t0\n\t"
        "mv sp, t2\n\t"
        ".option pop\n\t"
        : [value] "=r"(value), [span] "=r"(*span)
        : [sp_base] "r"(sp_base)
        : "a0", "t0", "t1", "t2", "memory");

    return value;
}

static inline uint64_t ai_arch_rvc_ldsp_probe(uintptr_t sp_base,
    uint64_t *span)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, sp\n\t"
        "mv sp, %[sp_base]\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.ldsp a0, 24(sp)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 2b\n\t"
        "lla t1, 1b\n\t"
        "sub t0, t0, t1\n\t"
        "mv %[value], a0\n\t"
        "mv %[span], t0\n\t"
        "mv sp, t2\n\t"
        ".option pop\n\t"
        : [value] "=r"(value), [span] "=r"(*span)
        : [sp_base] "r"(sp_base)
        : "a0", "t0", "t1", "t2", "memory");

    return value;
}

static inline void ai_arch_rvc_swsp_sdsp_probe(uintptr_t sp_base,
    uint64_t word_payload, uint64_t dword_payload, uint64_t *swsp_span,
    uint64_t *sdsp_span)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, sp\n\t"
        "mv sp, %[sp_base]\n\t"
        "mv a0, %[word_payload]\n\t"
        "mv a1, %[dword_payload]\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.swsp a0, 36(sp)\n\t"
        "2:\n\t"
        "c.sdsp a1, 48(sp)\n\t"
        "3:\n\t"
        ".option norvc\n\t"
        "lla t0, 2b\n\t"
        "lla t1, 1b\n\t"
        "sub t0, t0, t1\n\t"
        "mv %[swsp_span], t0\n\t"
        "lla t0, 3b\n\t"
        "lla t1, 2b\n\t"
        "sub t0, t0, t1\n\t"
        "mv %[sdsp_span], t0\n\t"
        "mv sp, t2\n\t"
        ".option pop\n\t"
        : [swsp_span] "=r"(*swsp_span), [sdsp_span] "=r"(*sdsp_span)
        : [sp_base] "r"(sp_base),
          [word_payload] "r"(word_payload),
          [dword_payload] "r"(dword_payload)
        : "a0", "a1", "t0", "t1", "t2", "memory");
}

static inline void ai_arch_rvc_compact_edge_probe(uintptr_t base,
    uint64_t result[6])
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t3, %[base]\n\t"
        "mv t4, %[result]\n\t"
        "mv t5, s0\n\t"
        "mv t6, s1\n\t"
        "mv s0, t3\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.lw s1, 4(s0)\n\t"
        "2:\n\t"
        "c.ld a5, 16(s0)\n\t"
        "3:\n\t"
        "c.sw a5, 28(s0)\n\t"
        "4:\n\t"
        "c.sd s1, 40(s0)\n\t"
        "5:\n\t"
        ".option norvc\n\t"
        "sd s1, 0(t4)\n\t"
        "sd a5, 8(t4)\n\t"
        "lla t0, 2b\n\t"
        "lla t1, 1b\n\t"
        "sub t0, t0, t1\n\t"
        "sd t0, 16(t4)\n\t"
        "lla t0, 3b\n\t"
        "lla t1, 2b\n\t"
        "sub t0, t0, t1\n\t"
        "sd t0, 24(t4)\n\t"
        "lla t0, 4b\n\t"
        "lla t1, 3b\n\t"
        "sub t0, t0, t1\n\t"
        "sd t0, 32(t4)\n\t"
        "lla t0, 5b\n\t"
        "lla t1, 4b\n\t"
        "sub t0, t0, t1\n\t"
        "sd t0, 40(t4)\n\t"
        "mv s0, t5\n\t"
        "mv s1, t6\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [result] "r"(result)
        : "a5", "t0", "t1", "t3", "t4", "t5", "t6", "memory");
}

static inline uint64_t ai_arch_rvc_fault_clw_probe(uintptr_t base,
    uint64_t sentinel, ai_arch_rvc_probe_pc_t *pc_info)
{
    uint64_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[sentinel]\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t4, %[value]\n\t"
        "mv t5, s0\n\t"
        "mv t6, s1\n\t"
        "mv s0, t2\n\t"
        "mv s1, t1\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.lw s1, 4(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "sd s1, 0(t4)\n\t"
        "mv s0, t5\n\t"
        "mv s1, t6\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [sentinel] "r"(sentinel),
          [pc_info] "r"(pc_info),
          [value] "r"(&value)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    return value;
}

static inline uint64_t ai_arch_rvc_fault_cld_probe(uintptr_t base,
    uint64_t sentinel, ai_arch_rvc_probe_pc_t *pc_info)
{
    uint64_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[sentinel]\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t4, %[value]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        "mv a5, t1\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.ld a5, 16(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "sd a5, 0(t4)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [sentinel] "r"(sentinel),
          [pc_info] "r"(pc_info),
          [value] "r"(&value)
        : "a5", "t0", "t1", "t2", "t3", "t4", "t5", "memory");

    return value;
}

static inline void ai_arch_rvc_fault_csw_probe(uintptr_t base,
    uint64_t payload, ai_arch_rvc_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[payload]\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        "mv a5, t1\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.sw a5, 28(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [payload] "r"(payload),
          [pc_info] "r"(pc_info)
        : "a5", "t0", "t1", "t2", "t3", "t5", "memory");
}

static inline void ai_arch_rvc_fault_csd_probe(uintptr_t base,
    uint64_t payload, ai_arch_rvc_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t1, %[payload]\n\t"
        "mv t2, %[base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "mv t5, s0\n\t"
        "mv t6, s1\n\t"
        "mv s0, t2\n\t"
        "mv s1, t1\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.sd s1, 40(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 8(t3)\n\t"
        "mv s0, t5\n\t"
        "mv s1, t6\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [payload] "r"(payload),
          [pc_info] "r"(pc_info)
        : "t0", "t1", "t2", "t3", "t5", "t6", "memory");
}

static inline uint8_t ai_arch_rvc_payload_byte(uint64_t value, unsigned index)
{
    return (uint8_t)(value >> (8U * index));
}

static inline void ai_arch_rvc_seed_page_edge(uintptr_t low_tail_paddr,
    uintptr_t high_head_paddr, uint8_t low_seed, uint8_t high_seed)
{
    for (unsigned i = 0; i < 8U; ++i) {
        write8(low_tail_paddr + i, (uint8_t)(low_seed + i));
    }
    for (unsigned i = 0; i < 16U; ++i) {
        write8(high_head_paddr + i, (uint8_t)(high_seed + i));
    }
}

static inline bool ai_arch_rvc_edge_prefix_unchanged(
    uintptr_t low_tail_paddr, uint8_t low_seed, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i) {
        if (read8(low_tail_paddr + i) != (uint8_t)(low_seed + i)) {
            return false;
        }
    }

    return true;
}

static inline bool ai_arch_rvc_csd_cross_image_matches(
    uintptr_t low_tail_paddr, uintptr_t high_head_paddr, uint8_t low_seed,
    uint8_t high_seed, uint64_t payload)
{
    for (unsigned i = 0; i < 4U; ++i) {
        if (read8(low_tail_paddr + i) != (uint8_t)(low_seed + i) ||
            read8(low_tail_paddr + 4U + i) !=
                ai_arch_rvc_payload_byte(payload, i) ||
            read8(high_head_paddr + i) !=
                ai_arch_rvc_payload_byte(payload, 4U + i) ||
            read8(high_head_paddr + 4U + i) !=
                (uint8_t)(high_seed + 4U + i)) {
            return false;
        }
    }

    for (unsigned i = 8U; i < 16U; ++i) {
        if (read8(high_head_paddr + i) != (uint8_t)(high_seed + i)) {
            return false;
        }
    }

    return true;
}

static inline bool ai_arch_rvc_csw_cross_image_matches(
    uintptr_t low_tail_paddr, uintptr_t high_head_paddr, uint8_t low_seed,
    uint8_t high_seed, uint64_t payload)
{
    for (unsigned i = 0; i < 6U; ++i) {
        if (read8(low_tail_paddr + i) != (uint8_t)(low_seed + i)) {
            return false;
        }
    }
    for (unsigned i = 0; i < 2U; ++i) {
        if (read8(low_tail_paddr + 6U + i) !=
                ai_arch_rvc_payload_byte(payload, i) ||
            read8(high_head_paddr + i) !=
                ai_arch_rvc_payload_byte(payload, 2U + i)) {
            return false;
        }
    }
    for (unsigned i = 2U; i < 16U; ++i) {
        if (read8(high_head_paddr + i) != (uint8_t)(high_seed + i)) {
            return false;
        }
    }

    return true;
}

static inline void ai_arch_rvc_csd_cross_marker_probe(uintptr_t store_vaddr,
    uint64_t payload, uintptr_t marker_vaddr, uint64_t marker_value,
    ai_arch_rvc_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[store_vaddr]\n\t"
        "mv t3, %[payload]\n\t"
        "mv t4, %[marker_vaddr]\n\t"
        "mv t5, %[marker_value]\n\t"
        "mv a6, %[pc_info]\n\t"
        "mv a4, s0\n\t"
        "mv a5, s1\n\t"
        "mv s0, t2\n\t"
        "mv s1, t3\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a6)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a6)\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.sd s1, 0(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "sd t5, 0(t4)\n\t"
        "mv s0, a4\n\t"
        "mv s1, a5\n\t"
        ".option pop\n\t"
        :
        : [store_vaddr] "r"(store_vaddr),
          [payload] "r"(payload),
          [marker_vaddr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [pc_info] "r"(pc_info)
        : "a4", "a5", "a6", "t0", "t2", "t3", "t4", "t5", "memory");
}

static inline void ai_arch_rvc_csw_cross_marker_probe(uintptr_t store_vaddr,
    uint64_t payload, uintptr_t marker_vaddr, uint64_t marker_value,
    ai_arch_rvc_probe_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[store_vaddr]\n\t"
        "mv t3, %[payload]\n\t"
        "mv t4, %[marker_vaddr]\n\t"
        "mv t5, %[marker_value]\n\t"
        "mv a6, %[pc_info]\n\t"
        "mv a4, s0\n\t"
        "mv a5, s1\n\t"
        "mv s0, t2\n\t"
        "mv s1, t3\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a6)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a6)\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.sw s1, 0(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "sd t5, 0(t4)\n\t"
        "mv s0, a4\n\t"
        "mv s1, a5\n\t"
        ".option pop\n\t"
        :
        : [store_vaddr] "r"(store_vaddr),
          [payload] "r"(payload),
          [marker_vaddr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [pc_info] "r"(pc_info)
        : "a4", "a5", "a6", "t0", "t2", "t3", "t4", "t5", "memory");
}

bool ai_arch_rvc_load_store_expansion_visible_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint8_t *const sp_base = ai_arch_rvc_sp_area + 2048;
    const uint64_t lwsp_expected = 0xffffffff80000023ULL;
    const uint64_t ldsp_expected = 0x8877665544332211ULL;

    ai_arch_rvc_clear(ai_arch_rvc_sp_area, sizeof(ai_arch_rvc_sp_area));
    ai_arch_rvc_write64(sp_base, 0, 0x0123456789abcdefULL);
    ai_arch_rvc_write32(sp_base, 16, 0x80000023U);
    ai_arch_rvc_write64(sp_base, 24, ldsp_expected);
    ai_arch_rvc_write64(sp_base, 32, 0x1122334455667788ULL);
    ai_arch_rvc_write64(sp_base, 40, 0x0102030405060708ULL);
    ai_arch_rvc_write64(sp_base, 48, 0xdeadbeefcafef00dULL);
    ai_arch_rvc_write64(sp_base, 56, 0xf0e1d2c3b4a59687ULL);

    uint64_t lwsp_span = 0;
    uint64_t ldsp_span = 0;
    TEST_SETUP_EXCEPT();
    uint64_t lwsp_value = ai_arch_rvc_lwsp_probe((uintptr_t)sp_base,
        &lwsp_span);
    uint64_t ldsp_value = ai_arch_rvc_ldsp_probe((uintptr_t)sp_base,
        &ldsp_span);
    TEST_ASSERT("C.LWSP/C.LDSP use sp plus scaled uimm, write the legal rd, preserve memory, and retire as 16-bit instructions",
        excpt.triggered == false &&
        lwsp_value == lwsp_expected &&
        ldsp_value == ldsp_expected &&
        lwsp_span == 2 &&
        ldsp_span == 2 &&
        ai_arch_rvc_read64(sp_base, 0) == 0x0123456789abcdefULL &&
        ai_arch_rvc_read32(sp_base, 16) == 0x80000023U &&
        ai_arch_rvc_read64(sp_base, 24) == ldsp_expected &&
        ai_arch_rvc_read_x0() == 0
    );

    const uint64_t swsp_payload = 0x00000000a1b2c3d4ULL;
    const uint64_t sdsp_payload = 0x13579bdf2468ace0ULL;
    uint64_t swsp_span = 0;
    uint64_t sdsp_span = 0;
    TEST_SETUP_EXCEPT();
    ai_arch_rvc_swsp_sdsp_probe((uintptr_t)sp_base, swsp_payload,
        sdsp_payload, &swsp_span, &sdsp_span);
    TEST_ASSERT("C.SWSP/C.SDSP use sp plus scaled uimm, consume rs2 data, and update only their target lanes",
        excpt.triggered == false &&
        swsp_span == 2 &&
        sdsp_span == 2 &&
        ai_arch_rvc_read64(sp_base, 32) == 0xa1b2c3d455667788ULL &&
        ai_arch_rvc_read64(sp_base, 40) == 0x0102030405060708ULL &&
        ai_arch_rvc_read64(sp_base, 48) == sdsp_payload &&
        ai_arch_rvc_read64(sp_base, 56) == 0xf0e1d2c3b4a59687ULL &&
        ai_arch_rvc_read_x0() == 0
    );

    ai_arch_rvc_clear(ai_arch_rvc_mem, sizeof(ai_arch_rvc_mem));
    ai_arch_rvc_write64(ai_arch_rvc_mem, 0, 0x0f1e2d3c4b5a6978ULL);
    ai_arch_rvc_write32(ai_arch_rvc_mem, 4, 0x80000031U);
    ai_arch_rvc_write64(ai_arch_rvc_mem, 16, 0x1122334487654321ULL);
    ai_arch_rvc_write64(ai_arch_rvc_mem, 24, 0xaabbccdd55667788ULL);
    ai_arch_rvc_write64(ai_arch_rvc_mem, 32, 0x0123456789abcdefULL);
    ai_arch_rvc_write64(ai_arch_rvc_mem, 40, 0xfeedfacecafebeefULL);
    ai_arch_rvc_write64(ai_arch_rvc_mem, 48, 0xfedcba9876543210ULL);

    uint64_t compact_result[6] = {0};
    TEST_SETUP_EXCEPT();
    ai_arch_rvc_compact_edge_probe((uintptr_t)ai_arch_rvc_mem,
        compact_result);
    TEST_ASSERT("C.LW/C.LD/C.SW/C.SD compact-register forms map x8-x15, scale uimm, and match expanded load/store side effects",
        excpt.triggered == false &&
        compact_result[0] == 0xffffffff80000031ULL &&
        compact_result[1] == 0x1122334487654321ULL &&
        compact_result[2] == 2 &&
        compact_result[3] == 2 &&
        compact_result[4] == 2 &&
        compact_result[5] == 2 &&
        ai_arch_rvc_read64(ai_arch_rvc_mem, 0) == 0x800000314b5a6978ULL &&
        ai_arch_rvc_read64(ai_arch_rvc_mem, 16) == 0x1122334487654321ULL &&
        ai_arch_rvc_read64(ai_arch_rvc_mem, 24) == 0x8765432155667788ULL &&
        ai_arch_rvc_read64(ai_arch_rvc_mem, 32) == 0x0123456789abcdefULL &&
        ai_arch_rvc_read64(ai_arch_rvc_mem, 40) == 0xffffffff80000031ULL &&
        ai_arch_rvc_read64(ai_arch_rvc_mem, 48) == 0xfedcba9876543210ULL &&
        ai_arch_rvc_read_x0() == 0
    );

    TEST_END();
}

bool ai_arch_rvc_fault_metadata_repair_visible_matrix_corner()
{
    TEST_START();

    const enum test_page load_page = VSRWX_GRWX;
    const enum test_page store_page = VSRW_GRWX;
    const uintptr_t load_base_vaddr = hs_page_base(load_page) + 0x180U;
    const uintptr_t load_base_paddr = phys_page_base(load_page) + 0x180U;
    const uintptr_t store_base_vaddr = hs_page_base(store_page) + 0x280U;
    const uintptr_t store_base_paddr = phys_page_base(store_page) + 0x280U;
    const uint64_t clw_sentinel = 0x123456789abcdef0ULL;
    const uint64_t cld_sentinel = 0x0badf00d55aa33ccULL;
    const uint32_t clw_word = 0x80000045U;
    const uint64_t clw_expected = 0xffffffff80000045ULL;
    const uint64_t cld_expected = 0x8877665544332211ULL;
    const uint64_t load_guard0 = 0x0102030405060708ULL;
    const uint64_t load_guard1 = 0xfedcba9876543210ULL;
    const uint64_t csw_initial = 0x1122334455667788ULL;
    const uint64_t csw_payload = 0x00000000a1b2c3d4ULL;
    const uint64_t csw_expected = 0xa1b2c3d455667788ULL;
    const uint64_t csd_initial = 0xdeadbeefcafef00dULL;
    const uint64_t csd_payload = 0x13579bdf2468ace0ULL;
    const uint64_t store_guard0 = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t store_guard1 = 0xf0e1d2c3b4a59687ULL;

    goto_priv(PRIV_M);
    write64(load_base_paddr, load_guard0);
    write32(load_base_paddr + 4U, clw_word);
    write64(load_base_paddr + 16U, cld_expected);
    write64(load_base_paddr + 24U, load_guard1);
    write64(store_base_paddr + 16U, store_guard0);
    write64(store_base_paddr + 24U, csw_initial);
    write64(store_base_paddr + 40U, csd_initial);
    write64(store_base_paddr + 48U, store_guard1);

    ai_arch_rvc_prepare_hs_translation();

    ai_arch_rvc_probe_pc_t clw_pc = {0};
    ai_arch_rvc_probe_pc_t cld_pc = {0};
    ai_arch_rvc_set_hs_leaf(load_page, AI_ARCH_RVC_LOAD_FAULT_LEAF);

    TEST_SETUP_EXCEPT();
    uint64_t clw_fault_value = ai_arch_rvc_fault_clw_probe(load_base_vaddr,
        clw_sentinel, &clw_pc);
    TEST_ASSERT("C.LW page fault records sepc at the compressed instruction start, stval at sp-expanded EA, and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.epc == clw_pc.pc &&
        excpt.tval == load_base_vaddr + 4U &&
        clw_pc.next_pc == clw_pc.pc + 2U &&
        clw_fault_value == clw_sentinel &&
        ai_arch_rvc_hs_leaf_matches(load_page, AI_ARCH_RVC_LOAD_FAULT_LEAF)
    );

    TEST_SETUP_EXCEPT();
    uint64_t cld_fault_value = ai_arch_rvc_fault_cld_probe(load_base_vaddr,
        cld_sentinel, &cld_pc);
    TEST_ASSERT("C.LD page fault records sepc at the compressed instruction start, stval at the scaled effective address, and keeps rd unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.epc == cld_pc.pc &&
        excpt.tval == load_base_vaddr + 16U &&
        cld_pc.next_pc == cld_pc.pc + 2U &&
        cld_fault_value == cld_sentinel &&
        ai_arch_rvc_hs_leaf_matches(load_page, AI_ARCH_RVC_LOAD_FAULT_LEAF)
    );

    ai_arch_rvc_set_hs_leaf(load_page, AI_ARCH_RVC_LEGAL_LEAF);
    TEST_SETUP_EXCEPT();
    uint64_t clw_repaired_value = ai_arch_rvc_fault_clw_probe(load_base_vaddr,
        clw_sentinel, &clw_pc);
    TEST_ASSERT("after repairing the load leaf, the same C.LW re-execution writes the sign-extended expanded load result",
        excpt.triggered == false &&
        clw_repaired_value == clw_expected &&
        clw_pc.next_pc == clw_pc.pc + 2U
    );

    TEST_SETUP_EXCEPT();
    uint64_t cld_repaired_value = ai_arch_rvc_fault_cld_probe(load_base_vaddr,
        cld_sentinel, &cld_pc);
    TEST_ASSERT("after repairing the load leaf, the same C.LD re-execution writes the expanded doubleword result",
        excpt.triggered == false &&
        cld_repaired_value == cld_expected &&
        cld_pc.next_pc == cld_pc.pc + 2U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("RVC load fault and repair leave backing load bytes and guard data unchanged",
        read64(load_base_paddr) == ((uint64_t)clw_word << 32 |
            (load_guard0 & 0xffffffffULL)) &&
        read64(load_base_paddr + 16U) == cld_expected &&
        read64(load_base_paddr + 24U) == load_guard1
    );

    ai_arch_rvc_probe_pc_t csw_pc = {0};
    ai_arch_rvc_probe_pc_t csd_pc = {0};
    ai_arch_rvc_set_hs_leaf(store_page, AI_ARCH_RVC_STORE_FAULT_LEAF);

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fault_csw_probe(store_base_vaddr, csw_payload, &csw_pc);
    TEST_ASSERT("C.SW page fault records sepc at the compressed instruction start and stval at the expanded store EA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == csw_pc.pc &&
        excpt.tval == store_base_vaddr + 28U &&
        csw_pc.next_pc == csw_pc.pc + 2U &&
        ai_arch_rvc_hs_leaf_matches(store_page, AI_ARCH_RVC_STORE_FAULT_LEAF)
    );

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fault_csd_probe(store_base_vaddr, csd_payload, &csd_pc);
    TEST_ASSERT("C.SD page fault records sepc at the compressed instruction start and stval at the scaled store effective address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == csd_pc.pc &&
        excpt.tval == store_base_vaddr + 40U &&
        csd_pc.next_pc == csd_pc.pc + 2U &&
        ai_arch_rvc_hs_leaf_matches(store_page, AI_ARCH_RVC_STORE_FAULT_LEAF)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("faulting C.SW/C.SD leave the target and guard backing bytes unchanged before repair",
        read64(store_base_paddr + 16U) == store_guard0 &&
        read64(store_base_paddr + 24U) == csw_initial &&
        read64(store_base_paddr + 40U) == csd_initial &&
        read64(store_base_paddr + 48U) == store_guard1
    );

    ai_arch_rvc_set_hs_leaf(store_page, AI_ARCH_RVC_LEGAL_LEAF);
    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fault_csw_probe(store_base_vaddr, csw_payload, &csw_pc);
    TEST_ASSERT("after repairing the store leaf, the same C.SW re-execution completes without an exception",
        excpt.triggered == false &&
        csw_pc.next_pc == csw_pc.pc + 2U
    );

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_fault_csd_probe(store_base_vaddr, csd_payload, &csd_pc);
    TEST_ASSERT("after repairing the store leaf, the same C.SD re-execution completes without an exception",
        excpt.triggered == false &&
        csd_pc.next_pc == csd_pc.pc + 2U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("RVC store repair writes exactly the expanded target lanes while preserving adjacent guard data",
        read64(store_base_paddr + 16U) == store_guard0 &&
        read64(store_base_paddr + 24U) == csw_expected &&
        read64(store_base_paddr + 40U) == csd_payload &&
        read64(store_base_paddr + 48U) == store_guard1 &&
        *ai_arch_rvc_hs_leaf_entry(store_page) ==
            ai_arch_rvc_hs_leaf_value(store_page, AI_ARCH_RVC_LEGAL_LEAF)
    );

    TEST_END();
}

bool ai_arch_compressed_store_mab_pc2_crosspage_owner_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page marker_page = VSRW_GRWX;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x100U;
    const uintptr_t high_head_vaddr = hs_page_base(high_page);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x100U;
    const uintptr_t csd_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t csw_vaddr = hs_page_base(low_page) + PAGE_SIZE - 2U;
    const uint8_t csd_low_seed = 0x20U;
    const uint8_t csd_high_seed = 0xa0U;
    const uint8_t csw_low_seed = 0x50U;
    const uint8_t csw_high_seed = 0xd0U;
    const uint64_t no_write_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t legal_leaf = AI_ARCH_RVC_LEGAL_LEAF;
    const uint64_t marker_initial = 0x1111222233334444ULL;
    const uint64_t csd_fault_marker = 0x0badf00d0000c5d0ULL;
    const uint64_t csd_success_marker = 0x12345678c5d0c5d0ULL;
    const uint64_t csw_fault_marker = 0x0badf00d0000c500ULL;
    const uint64_t csw_success_marker = 0x12345678c500c500ULL;
    const uint64_t csd_fault_payload = 0xf0e1d2c3b4a59687ULL;
    const uint64_t csd_success_payload = 0x8877665544332211ULL;
    const uint64_t csw_fault_payload = 0x00000000a1b2c3d4ULL;
    const uint64_t csw_success_payload = 0x000000005a6b7c8dULL;
    ai_arch_rvc_probe_pc_t pc = {0};

    goto_priv(PRIV_M);
    ai_arch_rvc_seed_page_edge(low_tail_paddr, high_head_paddr,
        csd_low_seed, csd_high_seed);
    write64(marker_paddr, marker_initial);

    ai_arch_rvc_prepare_hs_translation();
    ai_arch_rvc_set_hs_leaf(low_page, legal_leaf);
    ai_arch_rvc_set_hs_leaf(high_page, no_write_leaf);
    ai_arch_rvc_set_hs_leaf(marker_page, legal_leaf);

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_csd_cross_marker_probe(csd_vaddr, csd_fault_payload,
        marker_vaddr, csd_fault_marker, &pc);
    TEST_ASSERT("cross-page C.SD with a legal low fragment and no-write high fragment reports high-page SPF at the 16-bit store PC, then resumes at PC+2 marker",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == pc.pc &&
        excpt.tval == high_head_vaddr &&
        pc.next_pc == pc.pc + 2U &&
        read64(marker_vaddr) == csd_fault_marker,
        "cause=0x%llx epc=0x%llx pc=0x%llx tval=0x%llx high=0x%llx next=0x%llx marker=0x%llx",
        (unsigned long long)excpt.cause,
        (unsigned long long)excpt.epc,
        (unsigned long long)pc.pc,
        (unsigned long long)excpt.tval,
        (unsigned long long)high_head_vaddr,
        (unsigned long long)pc.next_pc,
        (unsigned long long)read64(marker_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("faulting cross-page C.SD preserves the high-page bytes and pre-store low-page guard prefix; the legal low fragment is intentionally not used as a default oracle",
        ai_arch_rvc_edge_prefix_unchanged(low_tail_paddr, csd_low_seed, 4U) &&
        ai_arch_rvc_edge_prefix_unchanged(high_head_paddr, csd_high_seed, 16U),
        "low_tail=0x%llx high0=0x%llx high8=0x%llx",
        (unsigned long long)read64(low_tail_paddr),
        (unsigned long long)read64(high_head_paddr),
        (unsigned long long)read64(high_head_paddr + 8U)
    );

    ai_arch_rvc_seed_page_edge(low_tail_paddr, high_head_paddr,
        csd_low_seed, csd_high_seed);
    write64(marker_paddr, marker_initial);
    ai_arch_rvc_set_hs_leaf(high_page, legal_leaf);

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_csd_cross_marker_probe(csd_vaddr, csd_success_payload,
        marker_vaddr, csd_success_marker, &pc);
    TEST_ASSERT("after high-page repair, the same cross-page C.SD retires as a 16-bit instruction and reaches the PC+2 marker without stale fault metadata",
        excpt.triggered == false &&
        pc.next_pc == pc.pc + 2U &&
        read64(marker_vaddr) == csd_success_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repaired cross-page C.SD writes exactly four payload bytes on each page and preserves both surrounding byte images",
        ai_arch_rvc_csd_cross_image_matches(low_tail_paddr, high_head_paddr,
            csd_low_seed, csd_high_seed, csd_success_payload),
        "low_tail=0x%llx high0=0x%llx high8=0x%llx",
        (unsigned long long)read64(low_tail_paddr),
        (unsigned long long)read64(high_head_paddr),
        (unsigned long long)read64(high_head_paddr + 8U)
    );

    ai_arch_rvc_seed_page_edge(low_tail_paddr, high_head_paddr,
        csw_low_seed, csw_high_seed);
    write64(marker_paddr, marker_initial);
    ai_arch_rvc_set_hs_leaf(high_page, no_write_leaf);

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_csw_cross_marker_probe(csw_vaddr, csw_fault_payload,
        marker_vaddr, csw_fault_marker, &pc);
    TEST_ASSERT("cross-page C.SW with high-page no-write reports SPF on the high-page VA while keeping compressed sepc and PC+2 resume identity",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.epc == pc.pc &&
        excpt.tval == high_head_vaddr &&
        pc.next_pc == pc.pc + 2U &&
        read64(marker_vaddr) == csw_fault_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("faulting cross-page C.SW preserves the high-page bytes and the low-page guard prefix before the legal low fragment",
        ai_arch_rvc_edge_prefix_unchanged(low_tail_paddr, csw_low_seed, 6U) &&
        ai_arch_rvc_edge_prefix_unchanged(high_head_paddr, csw_high_seed, 16U),
        "low_tail=0x%llx high0=0x%llx high8=0x%llx",
        (unsigned long long)read64(low_tail_paddr),
        (unsigned long long)read64(high_head_paddr),
        (unsigned long long)read64(high_head_paddr + 8U)
    );

    ai_arch_rvc_seed_page_edge(low_tail_paddr, high_head_paddr,
        csw_low_seed, csw_high_seed);
    write64(marker_paddr, marker_initial);
    ai_arch_rvc_set_hs_leaf(high_page, legal_leaf);

    TEST_SETUP_EXCEPT();
    ai_arch_rvc_csw_cross_marker_probe(csw_vaddr, csw_success_payload,
        marker_vaddr, csw_success_marker, &pc);
    TEST_ASSERT("after high-page repair, cross-page C.SW reaches the compressed PC+2 marker and completes without stale StoreMisalignBuffer fault state",
        excpt.triggered == false &&
        pc.next_pc == pc.pc + 2U &&
        read64(marker_vaddr) == csw_success_marker
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repaired cross-page C.SW writes exactly two payload bytes on each page and preserves all non-target guards",
        ai_arch_rvc_csw_cross_image_matches(low_tail_paddr, high_head_paddr,
            csw_low_seed, csw_high_seed, csw_success_payload),
        "low_tail=0x%llx high0=0x%llx high8=0x%llx",
        (unsigned long long)read64(low_tail_paddr),
        (unsigned long long)read64(high_head_paddr),
        (unsigned long long)read64(high_head_paddr + 8U)
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

static inline void ai_arch_rvc_reserved_lwsp_x0_probe(uintptr_t sp_base,
    ai_arch_rvc_probe_pc_t *pc_info, uint64_t *loaded_value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t6, sp\n\t"
        "mv sp, %[sp_base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(t3)\n\t"
        ".option rvc\n\t"
        "c.lwsp a0, 16(sp)\n\t"
        "1:\n\t"
        ".2byte 0x507e\n\t"
        "2:\n\t"
        "c.swsp a0, 32(sp)\n\t"
        ".option norvc\n\t"
        "mv t4, %[loaded_value]\n\t"
        "sd a0, 0(t4)\n\t"
        "mv sp, t6\n\t"
        ".option pop\n\t"
        :
        : [sp_base] "r"(sp_base),
          [pc_info] "r"(pc_info),
          [loaded_value] "r"(loaded_value)
        : "a0", "t0", "t3", "t4", "t6", "memory");
}

static inline void ai_arch_rvc_reserved_ldsp_x0_probe(uintptr_t sp_base,
    ai_arch_rvc_probe_pc_t *pc_info, uint64_t *loaded_value)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t6, sp\n\t"
        "mv sp, %[sp_base]\n\t"
        "mv t3, %[pc_info]\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(t3)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(t3)\n\t"
        ".option rvc\n\t"
        "c.ldsp a1, 24(sp)\n\t"
        "1:\n\t"
        ".2byte 0x707e\n\t"
        "2:\n\t"
        "c.sdsp a1, 40(sp)\n\t"
        ".option norvc\n\t"
        "mv t4, %[loaded_value]\n\t"
        "sd a1, 0(t4)\n\t"
        "mv sp, t6\n\t"
        ".option pop\n\t"
        :
        : [sp_base] "r"(sp_base),
          [pc_info] "r"(pc_info),
          [loaded_value] "r"(loaded_value)
        : "a1", "t0", "t3", "t4", "t6", "memory");
}

static inline void ai_arch_rvc_reserved_prepare_hs_trap(void)
{
    goto_priv(PRIV_M);
    ai_arch_rvc_reserved_trap_hits = 0;
    ai_arch_rvc_reserved_trap_cause = 0;
    ai_arch_rvc_reserved_trap_epc = 0;
    ai_arch_rvc_reserved_trap_tval = 0;
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_ILI);

    goto_priv(PRIV_HS);
    CSRW(CSR_STVEC, ai_arch_rvc_reserved_hs_trap_entry);
    sfence();
}

static inline void ai_arch_rvc_reserved_restore_trap(void)
{
    goto_priv(PRIV_M);
    reset_state();
}

bool ai_arch_rvc_reserved_encoding_illegal_isolation_matrix_corner()
{
    TEST_START();

    const enum test_page stack_page = VSUX_GUX;
    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t target_vaddr = hs_page_base(target_page);
    const uintptr_t target_paddr = phys_page_base(target_page);
    const uintptr_t stack_paddr = phys_page_base(stack_page);
    const uintptr_t lwsp_sp = target_vaddr - 128U;
    const uintptr_t ldsp_sp = target_vaddr - 384U;
    const uint64_t legal_leaf_no_ad = PTE_V | PTE_RWX;
    const uint32_t lwsp_legal_word = 0x80000071U;
    const uint64_t lwsp_legal_expected = 0xffffffff80000071ULL;
    const uint64_t lwsp_store_initial = 0xaabbccddeeff0011ULL;
    const uint64_t lwsp_store_expected = 0xaabbccdd80000071ULL;
    const uint64_t ldsp_legal_value = 0x8877665544332211ULL;
    const uint64_t ldsp_store_initial = 0x0123456789abcdefULL;
    const uint64_t target_guard0 = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t target_guard1 = 0xfeedfacecafebeefULL;
    const uint64_t target_guard2 = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(target_paddr + 112U, target_guard0);
    write64(target_paddr + 120U, target_guard1);
    write64(target_paddr + 128U, target_guard2);
    write32(stack_paddr + PAGE_SIZE - 112U, lwsp_legal_word);
    write64(stack_paddr + PAGE_SIZE - 96U, lwsp_store_initial);
    write64(stack_paddr + PAGE_SIZE - 360U, ldsp_legal_value);
    write64(stack_paddr + PAGE_SIZE - 344U, ldsp_store_initial);

    ai_arch_rvc_prepare_hs_translation();

    goto_priv(PRIV_M);
    *ai_arch_rvc_hs_leaf_entry(stack_page) =
        ai_arch_rvc_hs_leaf_value(stack_page, AI_ARCH_RVC_LEGAL_LEAF);
    *ai_arch_rvc_hs_leaf_entry(target_page) =
        ai_arch_rvc_hs_leaf_value(target_page, legal_leaf_no_ad);
    const uint64_t target_leaf_word =
        *ai_arch_rvc_hs_leaf_entry(target_page);
    fence_iorw_iorw();
    sfence_vma();
    TEST_ASSERT("the RVC reserved-encoding probe starts with a legal stack page and a valid target leaf whose A/D bits are clear",
        *ai_arch_rvc_hs_leaf_entry(stack_page) ==
            ai_arch_rvc_hs_leaf_value(stack_page, AI_ARCH_RVC_LEGAL_LEAF) &&
        (target_leaf_word & (PTE_V | PTE_RWX)) ==
            (PTE_V | PTE_RWX) &&
        (target_leaf_word & PTE_AD) == 0 &&
        read64(target_paddr + 112U) == target_guard0 &&
        read64(target_paddr + 120U) == target_guard1 &&
        read64(target_paddr + 128U) == target_guard2
    );

    ai_arch_rvc_reserved_prepare_hs_trap();

    ai_arch_rvc_probe_pc_t lwsp_pc = {0};
    uint64_t lwsp_loaded = 0;
    ai_arch_rvc_reserved_lwsp_x0_probe(lwsp_sp, &lwsp_pc, &lwsp_loaded);
    TEST_ASSERT("reserved C.LWSP rd=x0 raises illegal instruction at the compressed halfword and keeps the adjacent legal compressed load/store path intact",
        ai_arch_rvc_reserved_trap_hits == 1 &&
        ai_arch_rvc_reserved_trap_cause == CAUSE_ILI &&
        ai_arch_rvc_reserved_trap_epc == lwsp_pc.pc &&
        lwsp_pc.next_pc == lwsp_pc.pc + 2U &&
        lwsp_loaded == lwsp_legal_expected &&
        ai_arch_rvc_read_x0() == 0
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("reserved C.LWSP rd=x0 is rejected before any target-page data translation or memory side effect",
        *ai_arch_rvc_hs_leaf_entry(target_page) == target_leaf_word &&
        read64(target_paddr + 112U) == target_guard0 &&
        read64(target_paddr + 120U) == target_guard1 &&
        read64(target_paddr + 128U) == target_guard2
    );

    ai_arch_rvc_reserved_prepare_hs_trap();
    ai_arch_rvc_probe_pc_t ldsp_pc = {0};
    uint64_t ldsp_loaded = 0;
    ai_arch_rvc_reserved_ldsp_x0_probe(ldsp_sp, &ldsp_pc, &ldsp_loaded);
    TEST_ASSERT("reserved C.LDSP rd=x0 raises illegal instruction at the compressed halfword while the neighboring legal C.LDSP/C.SDSP still retire correctly",
        ai_arch_rvc_reserved_trap_hits == 1 &&
        ai_arch_rvc_reserved_trap_cause == CAUSE_ILI &&
        ai_arch_rvc_reserved_trap_epc == ldsp_pc.pc &&
        ldsp_pc.next_pc == ldsp_pc.pc + 2U &&
        ldsp_loaded == ldsp_legal_value &&
        ai_arch_rvc_read_x0() == 0
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("reserved C.LDSP rd=x0 leaves the target leaf A/D bits and backing bytes unchanged",
        *ai_arch_rvc_hs_leaf_entry(target_page) == target_leaf_word &&
        read64(target_paddr + 112U) == target_guard0 &&
        read64(target_paddr + 120U) == target_guard1 &&
        read64(target_paddr + 128U) == target_guard2
    );

    TEST_ASSERT("legal compressed instructions adjacent to the reserved encodings keep their expected register and memory effects",
        read64(stack_paddr + PAGE_SIZE - 96U) == lwsp_store_expected &&
        read64(stack_paddr + PAGE_SIZE - 344U) == ldsp_legal_value &&
        read64(target_paddr + 112U) == target_guard0 &&
        read64(target_paddr + 120U) == target_guard1 &&
        read64(target_paddr + 128U) == target_guard2 &&
        *ai_arch_rvc_hs_leaf_entry(target_page) == target_leaf_word
    );

    ai_arch_rvc_reserved_restore_trap();
    TEST_END();
}
