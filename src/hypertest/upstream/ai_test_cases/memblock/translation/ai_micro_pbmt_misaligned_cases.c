#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_PBMT_IO_MEM_BASE  0x30000000000ULL
#define AI_MICRO_PBMT_IO_MEM_LIMIT 0x40000000000ULL

static inline uintptr_t ai_micro_pbmt_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t ai_micro_pbmt_io_backing_paddr(void)
{
    return phys_page_base_offset(X, AI_MICRO_PBMT_IO_MEM_BASE);
}

static inline void ai_micro_reset_pbmt_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_reset_pbmt_pbmte_only_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_prepare_pbmt_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline void ai_micro_seed_pbmt_nc_alias_region(uintptr_t target_vaddr,
    uint64_t low, uint64_t high, uint64_t adjacent_low, uint64_t adjacent_high)
{
    sd(target_vaddr, low);
    sd(target_vaddr + sizeof(uint64_t), high);
    sd(target_vaddr + 16U, adjacent_low);
    sd(target_vaddr + 16U + sizeof(uint64_t), adjacent_high);
    fence_iorw_iorw();
}

static inline pte_t *ai_micro_pbmt_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_pbmt_set_hs_leaf(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    *ai_micro_pbmt_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uintptr_t ai_micro_pbmt_cross_page_addr(enum test_page low_page,
    uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline void ai_micro_pbmt_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_micro_pbmt_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_pbmt_masked_vle32_all_inactive_dump(
    const uint32_t *bad_base, const uint32_t *old_base, uint32_t *dump,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle32.v v6, (%[bad]), v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [bad] "r"(bad_base),
          [old] "r"(old_base),
          [dump] "r"(dump),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_pbmt_masked_vle32_mask_dump(
    const uint32_t *base, const uint32_t *old_base, const uint8_t *mask,
    uint32_t *dump, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vle32.v v6, (%[base]), v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [old] "r"(old_base),
          [mask] "r"(mask),
          [dump] "r"(dump),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_pbmt_masked_vle32_to_v6(
    const uint32_t *base, const uint8_t *mask, unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vle32.v v6, (%[base]), v0.t\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [mask] "r"(mask),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_pbmt_vlseg2e32_to_v6v7(const uint32_t *base,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vlseg2e32.v v6, (%[base])\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_pbmt_vsseg2e32_from_v6v7(uint32_t *base,
    unsigned vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vsseg2e32.v v6, (%[base])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [vl] "r"(vl)
        : "t0", "memory");
}

static inline void ai_micro_prepare_pbmt_mem_3t_4t_window(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    CSRW(CSR_PMAADDR0, AI_MICRO_PBMT_IO_MEM_BASE >> 2);
    CSRW(CSR_PMAADDR1, AI_MICRO_PBMT_IO_MEM_LIMIT >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline uintptr_t ai_micro_prepare_pbmt_io_alias(enum test_page page)
{
    ai_micro_prepare_pbmt_mem_3t_4t_window();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(page, AI_MICRO_PBMT_IO_MEM_BASE);
    sfence_vma();
    return hs_page_base(page);
}

bool ai_micro_hs_load_pbmt_nc_misaligned()
{
    TEST_START();

    goto_priv(PRIV_M);
    write64(ai_micro_pbmt_backing_paddr(), 0x1122334455667788ULL);

    uintptr_t vaddr = ai_micro_prepare_pbmt_alias(VSRWXPbmt1_GURWXPbmt0) + 7U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS misaligned load to a PBMT=NC page reports load misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_hs_load_pbmt_io_access_fault()
{
    TEST_START();

    goto_priv(PRIV_M);
    write64(ai_micro_pbmt_io_backing_paddr(), 0x8877665544332211ULL);

    uintptr_t vaddr = ai_micro_prepare_pbmt_io_alias(VSRWXPbmt2_GURWXPbmt0) + 7U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS misaligned load to a PBMT=IO page reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    ai_micro_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_nc_misaligned()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_backing_paddr();
    const uint64_t initial_value = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t vaddr = ai_micro_prepare_pbmt_alias(VSRWXPbmt1_GURWXPbmt0) + 7U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("an HS misaligned store to a PBMT=NC page reports store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=NC misaligned store leaves the shared backing bytes unchanged",
        read64(paddr) == initial_value
    );
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_hs_store_pbmt_io_access_fault()
{
    TEST_START();

    uintptr_t paddr = ai_micro_pbmt_io_backing_paddr();
    const uint64_t initial_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    uintptr_t vaddr = ai_micro_prepare_pbmt_io_alias(VSRWXPbmt2_GURWXPbmt0) + 7U;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("an HS misaligned store to a PBMT=IO page reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a PBMT=IO store access fault leaves the shared backing bytes unchanged",
        read64(paddr) == initial_value
    );
    ai_micro_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_vector_load_pbmt_nc_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x180U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x180U;
    const uintptr_t target_paddr = ai_micro_pbmt_backing_paddr() + 0x180U;
    const uintptr_t target_vaddr = ai_micro_prepare_pbmt_alias(
        VSRWXPbmt1_GURWXPbmt0) + 0x180U;
    const uintptr_t misaligned_vaddr = target_vaddr + 1U;
    const uintptr_t second_word_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    const uintptr_t adjacent_vaddr = target_vaddr + 16U;
    const uintptr_t scratch_paddr = phys_page_base(SWITCH2) + 0x200U;
    const uintptr_t scratch_vaddr = hs_page_base(SWITCH2) + 0x200U;
    const uint64_t source_low = 0x0123456789abcdefULL;
    const uint64_t source_high = 0xfedcba9876543210ULL;
    const uint64_t initial_low = 0x1122334455667788ULL;
    const uint64_t initial_high = 0x99aabbccddeeff00ULL;
    const uint64_t updated_low = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_low = 0x0fedcba987654321ULL;
    const uint64_t adjacent_high = 0x8877665544332211ULL;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    write64(source_paddr, source_low);
    write64(source_paddr + sizeof(uint64_t), source_high);
    write64(scratch_paddr, 0ULL);
    write64(scratch_paddr + sizeof(uint64_t), 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_seed_pbmt_nc_alias_region(target_vaddr, initial_low, initial_high,
        adjacent_low, adjacent_high);
    TEST_ASSERT("the PBMT=NC target alias can seed the target and adjacent words before the repeated vector-load fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)source_vaddr, 2);
    TEST_ASSERT("a normal vector e64 load can stage the reference payload before the repeated PBMT=NC misaligned vector-load fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first misaligned vector e64 load from a PBMT=NC alias reports load access fault with tval bound to the first faulting element address and vstart still at element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("repeating the same misaligned vector e64 load from a PBMT=NC alias keeps the same access-fault tval and element0 vstart metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)scratch_vaddr, 2);
    TEST_ASSERT("after the repeated PBMT=NC misaligned vector-load faults, storing the still-staged vector register into a normal scratch page still succeeds",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(target_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC misaligned vector-load faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=NC misaligned vector-load faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(target_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=NC alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=NC misaligned vector-load faults and the later scalar recovery store leave the high doubleword in the same target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC misaligned vector-load faults and the later scalar recovery store also leave the adjacent region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    fence_iorw_iorw();
    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=NC misaligned vector-load faults preserve the staged vector payload for the scratch copy and do not poison the later scalar same-alias path",
        read64(scratch_paddr) == source_low &&
        read64(scratch_paddr + sizeof(uint64_t)) == source_high &&
        read64(target_paddr) == updated_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_hs_vector_load_pbmt_io_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x240U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x240U;
    const uintptr_t target_paddr = ai_micro_pbmt_io_backing_paddr() + 0x200U;
    const uintptr_t target_vaddr = ai_micro_prepare_pbmt_io_alias(
        VSRWXPbmt2_GURWXPbmt0) + 0x200U;
    const uintptr_t misaligned_vaddr = target_vaddr + 1U;
    const uintptr_t second_word_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    const uintptr_t adjacent_vaddr = target_vaddr + 16U;
    const uintptr_t scratch_paddr = phys_page_base(SWITCH2) + 0x280U;
    const uintptr_t scratch_vaddr = hs_page_base(SWITCH2) + 0x280U;
    const uint64_t source_low = 0x89abcdef01234567ULL;
    const uint64_t source_high = 0x76543210fedcba98ULL;
    const uint64_t initial_low = 0x2233445566778899ULL;
    const uint64_t initial_high = 0xaabbccddeeff0011ULL;
    const uint64_t updated_low = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent_low = 0x02468ace13579bdfULL;
    const uint64_t adjacent_high = 0x1122334455667788ULL;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    write64(source_paddr, source_low);
    write64(source_paddr + sizeof(uint64_t), source_high);
    write64(target_paddr, initial_low);
    write64(target_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);
    write64(scratch_paddr, 0ULL);
    write64(scratch_paddr + sizeof(uint64_t), 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)source_vaddr, 2);
    TEST_ASSERT("a normal vector e64 load can stage the reference payload before the repeated PBMT=IO misaligned vector-load fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first misaligned vector e64 load from a PBMT=IO alias reports load access fault with tval bound to the first faulting element address and vstart still at element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("repeating the same misaligned vector e64 load from a PBMT=IO alias keeps the same access-fault tval and element0 vstart metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)scratch_vaddr, 2);
    TEST_ASSERT("after the repeated PBMT=IO misaligned vector-load faults, storing the still-staged vector register into a normal scratch page still succeeds",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(target_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO misaligned vector-load faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=IO misaligned vector-load faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(target_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=IO alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=IO misaligned vector-load faults and the later scalar recovery store leave the high doubleword in the same target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO misaligned vector-load faults and the later scalar recovery store also leave the adjacent region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO misaligned vector-load faults preserve the staged vector payload for the scratch copy and do not poison the later scalar same-alias path",
        read64(scratch_paddr) == source_low &&
        read64(scratch_paddr + sizeof(uint64_t)) == source_high &&
        read64(target_paddr) == updated_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_vector_store_pbmt_nc_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x300U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x300U;
    const uintptr_t target_paddr = ai_micro_pbmt_backing_paddr() + 0x280U;
    const uintptr_t target_vaddr = ai_micro_prepare_pbmt_alias(
        VSRWXPbmt1_GURWXPbmt0) + 0x280U;
    const uintptr_t misaligned_vaddr = target_vaddr + 1U;
    const uintptr_t second_word_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    const uintptr_t adjacent_vaddr = target_vaddr + 16U;
    const uint64_t source_low = 0x0102030405060708ULL;
    const uint64_t source_high = 0x1112131415161718ULL;
    const uint64_t initial_low = 0x33445566778899aaULL;
    const uint64_t initial_high = 0xbbccddee00112233ULL;
    const uint64_t updated_low = 0x99aabbccddeeff00ULL;
    const uint64_t adjacent_low = 0xfedcba9876543210ULL;
    const uint64_t adjacent_high = 0x02468ace13579bdfULL;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    write64(source_paddr, source_low);
    write64(source_paddr + sizeof(uint64_t), source_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_seed_pbmt_nc_alias_region(target_vaddr, initial_low, initial_high,
        adjacent_low, adjacent_high);
    TEST_ASSERT("the PBMT=NC target alias can seed the target and adjacent words before the repeated vector-store fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)source_vaddr, 2);
    TEST_ASSERT("a normal vector e64 load can stage the source payload before the repeated PBMT=NC misaligned vector-store fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first misaligned vector e64 store to a PBMT=NC alias reports store access fault with tval bound to the first faulting element address and vstart still at element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)source_vaddr, 2);
    TEST_ASSERT("the source payload can be reloaded into the vector register before the repeated PBMT=NC misaligned vector-store fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("repeating the same misaligned vector e64 store to a PBMT=NC alias keeps the same access-fault tval and element0 vstart metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(target_vaddr);
    TEST_ASSERT("after the repeated PBMT=NC misaligned vector-store faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=NC misaligned vector-store faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(target_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=NC alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=NC misaligned vector-store faults and the later scalar recovery store leave the high doubleword in the same target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=NC misaligned vector-store faults and the later scalar recovery store also leave the adjacent region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    fence_iorw_iorw();
    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=NC misaligned vector-store faults do not partially update memory and do not poison the later scalar same-alias path",
        read64(target_paddr) == updated_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_hs_vector_store_pbmt_io_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner()
{
    TEST_START();

    const uintptr_t source_paddr = phys_page_base(SWITCH1) + 0x340U;
    const uintptr_t source_vaddr = hs_page_base(SWITCH1) + 0x340U;
    const uintptr_t target_paddr = ai_micro_pbmt_io_backing_paddr() + 0x280U;
    const uintptr_t target_vaddr = ai_micro_prepare_pbmt_io_alias(
        VSRWXPbmt2_GURWXPbmt0) + 0x280U;
    const uintptr_t misaligned_vaddr = target_vaddr + 1U;
    const uintptr_t second_word_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = target_paddr + 16U;
    const uintptr_t adjacent_vaddr = target_vaddr + 16U;
    const uint64_t source_low = 0x2122232425262728ULL;
    const uint64_t source_high = 0x3132333435363738ULL;
    const uint64_t initial_low = 0x445566778899aabbULL;
    const uint64_t initial_high = 0xccddeeff00112233ULL;
    const uint64_t updated_low = 0x76543210fedcba98ULL;
    const uint64_t adjacent_low = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent_high = 0x8877665544332211ULL;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    write64(source_paddr, source_low);
    write64(source_paddr + sizeof(uint64_t), source_high);
    write64(target_paddr, initial_low);
    write64(target_paddr + sizeof(uint64_t), initial_high);
    write64(adjacent_paddr, adjacent_low);
    write64(adjacent_paddr + sizeof(uint64_t), adjacent_high);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)source_vaddr, 2);
    TEST_ASSERT("a normal vector e64 load can stage the source payload before the repeated PBMT=IO misaligned vector-store fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first misaligned vector e64 store to a PBMT=IO alias reports store access fault with tval bound to the first faulting element address and vstart still at element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)source_vaddr, 2);
    TEST_ASSERT("the source payload can be reloaded into the vector register before the repeated PBMT=IO misaligned vector-store fault probe",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)misaligned_vaddr, 2);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("repeating the same misaligned vector e64 store to a PBMT=IO alias keeps the same access-fault tval and element0 vstart metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        observed_vstart == 0ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_before = ld(target_vaddr);
    TEST_ASSERT("after the repeated PBMT=IO misaligned vector-store faults, a normal scalar load through the same alias still returns the original low doubleword",
        excpt.triggered == false &&
        observed_before == initial_low
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, updated_low);
    TEST_ASSERT("after the repeated PBMT=IO misaligned vector-store faults, a normal scalar store through the same alias still completes successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_after = ld(target_vaddr);
    TEST_ASSERT("after the scalar recovery store on the PBMT=IO alias, a later scalar load through the same alias observes the updated low doubleword immediately",
        excpt.triggered == false &&
        observed_after == updated_low
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_high = ld(second_word_vaddr);
    TEST_ASSERT("the repeated PBMT=IO misaligned vector-store faults and the later scalar recovery store leave the high doubleword in the same target region unchanged",
        excpt.triggered == false &&
        observed_high == initial_high
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("the repeated PBMT=IO misaligned vector-store faults and the later scalar recovery store also leave the adjacent region unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent_low
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated PBMT=IO misaligned vector-store faults do not partially update memory and do not poison the later scalar same-alias path",
        read64(target_paddr) == updated_low &&
        read64(target_paddr + sizeof(uint64_t)) == initial_high &&
        read64(adjacent_paddr) == adjacent_low &&
        read64(adjacent_paddr + sizeof(uint64_t)) == adjacent_high
    );
    ai_micro_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_ap33_vector_load_second_split_pbmt_nc_access_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x180U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x180U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x1c0U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x1c0U;
    const uint8_t old_vd_bytes[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    const uint8_t fault_src_bytes[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf
    };
    const uint8_t expected_scratch[16] = {
        0xa0, 0xa1, 0xa2, 0xa3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_micro_pbmt_set_hs_leaf(old_page, phys_page_base(old_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(scratch_page, phys_page_base(scratch_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, fault_src_bytes, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, fault_src_bytes + 7U, 9U);
    ai_micro_pbmt_write_bytes(old_paddr, old_vd_bytes, 16U);
    ai_micro_pbmt_write_bytes(scratch_paddr, old_vd_bytes, 16U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-33 preload seeds v6 before the vector load second-split PBMT=NC probe",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-33 vector load whose element1 second split enters PBMT=NC reports LAF rather than LAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("AP-33 dumping v6 after the PBMT=NC split load fault completes for destination footprint checking",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-33 split-load PBMT=NC fault updates only element0 and keeps the faulting and suffix elements old",
        ai_micro_pbmt_bytes_match(scratch_paddr, expected_scratch, 16U) &&
        ai_micro_pbmt_bytes_match(low_paddr, fault_src_bytes, 7U) &&
        ai_micro_pbmt_bytes_match(high_paddr, fault_src_bytes + 7U, 9U));
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_ap33_vector_store_second_split_pbmt_nc_access_fault_no_side_effect_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x240U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x240U;
    const uint8_t dest_initial[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t src_pattern[16] = {
        0xc0, 0xc1, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb,
        0xcc, 0xcd, 0xce, 0xcf
    };
    const uint8_t expected_after_fault[16] = {
        0xc0, 0xc1, 0xc2, 0xc3,
        0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_micro_pbmt_set_hs_leaf(src_page, phys_page_base(src_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, dest_initial, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, dest_initial + 7U, 9U);
    ai_micro_pbmt_write_bytes(src_paddr, src_pattern, 16U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-33 source vector load succeeds before the vector store second-split PBMT=NC probe",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-33 vector store whose element1 second split enters PBMT=NC reports SAF rather than SAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-33 split-store PBMT=NC fault commits only element0 and does not expose the faulting element low fragment",
        ai_micro_pbmt_bytes_match(low_paddr, expected_after_fault, 7U) &&
        ai_micro_pbmt_bytes_match(high_paddr, expected_after_fault + 7U, 9U) &&
        ai_micro_pbmt_bytes_match(src_paddr, src_pattern, 16U));
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_ap56_vector_load_second_split_pbmt_io_isvec_access_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr =
        phys_page_base_offset(high_page, AI_MICRO_PBMT_IO_MEM_BASE);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x300U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x300U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x340U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x340U;
    const uint8_t old_vd_bytes[16] = {
        0x60, 0x61, 0x62, 0x63,
        0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b,
        0x6c, 0x6d, 0x6e, 0x6f
    };
    const uint8_t fault_src_bytes[16] = {
        0xb0, 0xb1, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb,
        0xbc, 0xbd, 0xbe, 0xbf
    };
    const uint8_t expected_scratch[16] = {
        0xb0, 0xb1, 0xb2, 0xb3,
        0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b,
        0x6c, 0x6d, 0x6e, 0x6f
    };
    uint64_t observed_vstart = 0ULL;

    ai_micro_prepare_pbmt_mem_3t_4t_window();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, high_paddr,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt2);
    ai_micro_pbmt_set_hs_leaf(old_page, phys_page_base(old_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(scratch_page, phys_page_base(scratch_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, fault_src_bytes, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, fault_src_bytes + 7U, 9U);
    ai_micro_pbmt_write_bytes(old_paddr, old_vd_bytes, 16U);
    ai_micro_pbmt_write_bytes(scratch_paddr, old_vd_bytes, 16U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-56 preload seeds v6 before the vector load second-split PBMT=IO probe",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-56 vector load second split through LoadMisalignBuffer keeps isvec and reports LAF on PBMT=IO",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("AP-56 dumping v6 after the PBMT=IO split load fault completes for destination footprint checking",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-56 PBMT=IO split-load fault updates only element0 and leaves faulting and suffix elements old",
        ai_micro_pbmt_bytes_match(scratch_paddr, expected_scratch, 16U) &&
        ai_micro_pbmt_bytes_match(low_paddr, fault_src_bytes, 7U) &&
        ai_micro_pbmt_bytes_match(high_paddr, fault_src_bytes + 7U, 9U));
    ai_micro_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_ap56_masked_off_second_split_pbmt_io_no_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t masked_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr =
        phys_page_base_offset(high_page, AI_MICRO_PBMT_IO_MEM_BASE);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x380U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x380U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x3c0U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x3c0U;
    const uintptr_t mask_paddr = phys_page_base(scratch_page) + 0x3f0U;
    const uintptr_t mask_vaddr = hs_page_base(scratch_page) + 0x3f0U;
    const uint8_t old_vd_bytes[8] = {
        0x70, 0x71, 0x72, 0x73,
        0x74, 0x75, 0x76, 0x77
    };
    const uint8_t active_src_bytes[8] = {
        0xe0, 0xe1, 0xe2, 0xe3,
        0xe4, 0xe5, 0xe6, 0xe7
    };
    const uint8_t expected_dump[8] = {
        0xe0, 0xe1, 0xe2, 0xe3,
        0x74, 0x75, 0x76, 0x77
    };

    ai_micro_prepare_pbmt_mem_3t_4t_window();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, high_paddr,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt2);
    ai_micro_pbmt_set_hs_leaf(old_page, phys_page_base(old_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(scratch_page, phys_page_base(scratch_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, active_src_bytes, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, active_src_bytes + 7U, 1U);
    ai_micro_pbmt_write_bytes(old_paddr, old_vd_bytes, 8U);
    ai_micro_pbmt_write_bytes(scratch_paddr, old_vd_bytes, 8U);
    write8(mask_paddr, 0x01U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_pbmt_masked_vle32_all_inactive_dump((const uint32_t *)masked_vaddr,
        (const uint32_t *)old_vaddr, (uint32_t *)scratch_vaddr, 2U);
    TEST_ASSERT("AP-56 all-inactive mask suppresses the vector split load that would otherwise reach PBMT=IO",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    ai_micro_pbmt_masked_vle32_mask_dump((const uint32_t *)masked_vaddr,
        (const uint32_t *)old_vaddr, (const uint8_t *)mask_vaddr,
        (uint32_t *)scratch_vaddr, 2U);
    TEST_ASSERT("AP-56 mask with only element0 active lets the cacheable prefix load retire without consulting inactive element1 PBMT=IO split",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-56 masked PBMT=IO split probe updates only the active normal element and keeps the inactive split element old",
        ai_micro_pbmt_bytes_match(scratch_paddr, expected_dump, 8U) &&
        ai_micro_pbmt_bytes_match(low_paddr, active_src_bytes, 7U) &&
        read8(high_paddr) == active_src_bytes[7]);
    ai_micro_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_ap56_vector_load_second_split_pbmt_nc_fault_repair_success_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x400U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x400U;
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) + 0x440U;
    const uintptr_t scratch_paddr = phys_page_base(scratch_page) + 0x440U;
    const uint8_t old_vd_bytes[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t repaired_src_bytes[16] = {
        0x90, 0x91, 0x92, 0x93,
        0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b,
        0x9c, 0x9d, 0x9e, 0x9f
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_micro_pbmt_set_hs_leaf(old_page, phys_page_base(old_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(scratch_page, phys_page_base(scratch_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, repaired_src_bytes, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, repaired_src_bytes + 7U, 9U);
    ai_micro_pbmt_write_bytes(old_paddr, old_vd_bytes, 16U);
    ai_micro_pbmt_write_bytes(scratch_paddr, old_vd_bytes, 16U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)old_vaddr, 4);
    TEST_ASSERT("AP-56 repair probe preloads old v6 content before the first PBMT=NC split fault",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-56 first pass reports LAF on the vector load second split while high page is PBMT=NC",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL);

    goto_priv(PRIV_M);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, 4);
    TEST_ASSERT("AP-56 after repairing the high split to cacheable, the same vector load completes without stale MAB exception state",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)scratch_vaddr, 4);
    TEST_ASSERT("AP-56 repaired vector load result can be dumped after clearing the earlier fault",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-56 repaired second-split load returns the complete cross-page payload and not the stale old vector image",
        ai_micro_pbmt_bytes_match(scratch_paddr, repaired_src_bytes, 16U));
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_ap33_segment_load_second_field_pbmt_nc_access_fault_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uint8_t segment_src_bytes[16] = {
        0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b,
        0x1c, 0x1d, 0x1e, 0x1f
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_micro_pbmt_write_bytes(low_paddr, segment_src_bytes, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, segment_src_bytes + 7U, 9U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_pbmt_vlseg2e32_to_v6v7((const uint32_t *)fault_vaddr, 2U);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-33 segment load whose field1 split enters PBMT=NC reports LAF with vstart bound to architectural element0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 0ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-33 segment PBMT=NC fault probe preserves the original cross-page source bytes",
        ai_micro_pbmt_bytes_match(low_paddr, segment_src_bytes, 7U) &&
        ai_micro_pbmt_bytes_match(high_paddr, segment_src_bytes + 7U, 9U));
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_ap33_first_split_pbmt_nc_priority_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 3U);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 3U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x480U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x480U;
    const uintptr_t mask_paddr = phys_page_base(scratch_page) + 0x4c0U;
    const uintptr_t mask_vaddr = hs_page_base(scratch_page) + 0x4c0U;
    const uint8_t dest_bytes[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t src_bytes[16] = {
        0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb,
        0xdc, 0xdd, 0xde, 0xdf
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(src_page, phys_page_base(src_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(scratch_page, phys_page_base(scratch_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, dest_bytes, 3U);
    ai_micro_pbmt_write_bytes(high_paddr, dest_bytes + 3U, 13U);
    ai_micro_pbmt_write_bytes(src_paddr, src_bytes, 16U);
    write8(mask_paddr, 0x0fU);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, 4);
    TEST_ASSERT("AP-33 first-split priority probe stages store payload before the PBMT=NC split store",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vse32_from_v6((uint32_t *)fault_vaddr, 4);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-33 with the first active split in PBMT=NC reports SAF on the low split before consulting the cacheable high split",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-33 first-split PBMT=NC store fault leaves the low and high fragments unchanged",
        ai_micro_pbmt_bytes_match(low_paddr, dest_bytes, 3U) &&
        ai_micro_pbmt_bytes_match(high_paddr, dest_bytes + 3U, 13U));

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_pbmt_masked_vle32_to_v6((const uint32_t *)fault_vaddr,
        (const uint8_t *)mask_vaddr, 4U);
    TEST_ASSERT("AP-33 when all four elements are active, the low-split PBMT=NC element is observed before any later split priority candidate",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_vaddr);

    goto_priv(PRIV_M);
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}

bool ai_micro_ap33_segment_store_second_field_pbmt_nc_no_side_effect_manual_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t fault_vaddr = ai_micro_pbmt_cross_page_addr(low_page, 7U);
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x500U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x500U;
    const uint8_t dest_initial[16] = {
        0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    const uint8_t src_pattern[16] = {
        0xf0, 0xf1, 0xf2, 0xf3,
        0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb,
        0xfc, 0xfd, 0xfe, 0xff
    };
    const uint8_t expected_after_fault[16] = {
        0xf0, 0xf1, 0xf2, 0xf3,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_micro_pbmt_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1);
    ai_micro_pbmt_set_hs_leaf(src_page, phys_page_base(src_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_pbmt_write_bytes(low_paddr, dest_initial, 7U);
    ai_micro_pbmt_write_bytes(high_paddr, dest_initial + 7U, 9U);
    ai_micro_pbmt_write_bytes(src_paddr, src_pattern, 16U);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_pbmt_vlseg2e32_to_v6v7((const uint32_t *)src_vaddr, 2U);
    TEST_ASSERT("AP-33 segment store probe stages v6/v7 payload through a normal vlseg2e32 before the PBMT=NC store fault",
        excpt.triggered == false);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_pbmt_vsseg2e32_from_v6v7((uint32_t *)fault_vaddr, 2U);
    const bool store_faulted = excpt.triggered;
    const uint64_t store_cause = excpt.cause;
    const uint64_t store_tval = excpt.tval;
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-33 segment store whose field1 split enters PBMT=NC either reports the modeled SAF or completes on models that do not gate PBMT=NC as an access fault",
        (store_faulted == true &&
            store_cause == CAUSE_SAF &&
            store_tval == high_fault_vaddr &&
            observed_vstart == 0ULL) ||
        store_faulted == false,
        "triggered=%d cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        store_faulted, store_cause, store_tval, high_fault_vaddr,
        observed_vstart);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-33 segment store PBMT=NC fault keeps the prefix field0 store visible while suppressing the faulting field and suffix, or completes on models that do not gate PBMT=NC",
        ((store_faulted == true &&
            ai_micro_pbmt_bytes_match(low_paddr, expected_after_fault, 7U) &&
            ai_micro_pbmt_bytes_match(high_paddr,
                expected_after_fault + 7U, 9U)) ||
        (store_faulted == false &&
            ai_micro_pbmt_bytes_match(low_paddr, src_pattern, 7U) &&
            ai_micro_pbmt_bytes_match(high_paddr, src_pattern + 7U, 9U))) &&
        ai_micro_pbmt_bytes_match(src_paddr, src_pattern, 16U),
        "faulted=%d low0=0x%x low4=0x%x low6=0x%x high0=0x%x",
        store_faulted,
        (unsigned)read8(low_paddr),
        (unsigned)read8(low_paddr + 4U),
        (unsigned)read8(low_paddr + 6U),
        (unsigned)read8(high_paddr));
    ai_micro_reset_pbmt_pbmte_only_env();

    TEST_END();
}
