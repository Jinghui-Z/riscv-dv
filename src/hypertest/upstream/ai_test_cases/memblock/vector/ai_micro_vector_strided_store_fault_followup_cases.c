#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_VEC_STRIDED_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_VEC_STRIDED_PMP_TOR_NO_W 0x0dULL

#define AI_MICRO_VEC_STRIDED_VL 33U
#define AI_MICRO_VEC_STRIDED_ACTIVE_VL 32U
#define AI_MICRO_VEC_STRIDED_FAULT_IDX 31U
#define AI_MICRO_VEC_STRIDED_SUFFIX_IDX 32U
#define AI_MICRO_VEC_STRIDED_TARGET_OFFSET 0x120U
#define AI_MICRO_VEC_STRIDED_SOURCE_OFFSET 0x200U
#define AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET 0x280U
#define AI_MICRO_VEC_STRIDED_LMUL_M2 1
#define AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL 0x5aU
#define AI_MICRO_VEC_STRIDED_PRESSURE_LINE_COUNT 64U
#define AI_MICRO_VEC_STRIDED_FILL_STRIDE 0x40U
#define AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT 16U

#define AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(status_ptr, test, cond, ...) { \
    if(LOG_LEVEL >= LOG_DETAIL){ \
        size_t line_size = 140; \
        size_t size = strlen(test); \
        printf(CBLU "\t%-140.*s" CDFLT, line_size, test); \
        for(int i = line_size; i < size; i += line_size) \
            printf(CBLU "\n\t%-140.*s" CDFLT, line_size, &test[i]); \
        printf("%s" CDFLT, (cond) ? CGRN "PASSED" : CRED "FAILED"); \
        if(!(cond)) { printf("\n\t("); printf(""__VA_ARGS__); printf(")"); } \
        printf("\n"); \
    } \
    *(status_ptr) = *(status_ptr) && (cond); \
}

static volatile uintptr_t ai_micro_vec_strided_trap_probe_addr = 0;
static volatile uint64_t ai_micro_vec_strided_trap_probe_value = 0;
static volatile uint64_t ai_micro_vec_strided_trap_probe_fired = 0;
static volatile uintptr_t ai_micro_vec_strided_handler_pressure_base_addr = 0;
static volatile uint64_t ai_micro_vec_strided_handler_pressure_fired = 0;
static volatile uint64_t
    ai_micro_vec_strided_handler_pressure_words[
        AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT]
    __attribute__((aligned(16)));
static volatile uint64_t ai_micro_vec_strided_trap_cause = 0;
static volatile uint64_t ai_micro_vec_strided_trap_tval = 0;
static volatile uint64_t ai_micro_vec_strided_trap_epc = 0;
static volatile uint64_t ai_micro_vec_strided_trap_vstart = 0;
static uint64_t ai_micro_vec_strided_trap_scratch[8] __attribute__((aligned(16)));

extern void ai_micro_vec_strided_trap_entry_byte_load_handler(void);

#define AI_MICRO_VEC_STRIDED_HANDLER_PRESSURE_ASM \
"    la t1, ai_micro_vec_strided_handler_pressure_base_addr\n" \
"    ld t1, 0(t1)\n" \
"    beqz t1, 9f\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 0(t2)\n" \
"    sd t2, 0(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 8(t2)\n" \
"    sd t2, 64(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 16(t2)\n" \
"    sd t2, 128(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 24(t2)\n" \
"    sd t2, 192(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 32(t2)\n" \
"    sd t2, 256(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 40(t2)\n" \
"    sd t2, 320(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 48(t2)\n" \
"    sd t2, 384(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 56(t2)\n" \
"    sd t2, 448(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 64(t2)\n" \
"    sd t2, 512(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 72(t2)\n" \
"    sd t2, 576(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 80(t2)\n" \
"    sd t2, 640(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 88(t2)\n" \
"    sd t2, 704(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 96(t2)\n" \
"    sd t2, 768(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 104(t2)\n" \
"    sd t2, 832(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 112(t2)\n" \
"    sd t2, 896(t1)\n" \
"    la t2, ai_micro_vec_strided_handler_pressure_words\n" \
"    ld t2, 120(t2)\n" \
"    sd t2, 960(t1)\n" \
"    la t1, ai_micro_vec_strided_handler_pressure_fired\n" \
"    li t2, 1\n" \
"    sd t2, 0(t1)\n" \
"9:\n"

asm(
".balign 0x4\n"
".global ai_micro_vec_strided_trap_entry_byte_load_handler\n"
"ai_micro_vec_strided_trap_entry_byte_load_handler:\n"
"    csrrw t0, sscratch, t0\n"
"    sd t1, 0(t0)\n"
"    sd t2, 8(t0)\n"
"    la t1, ai_micro_vec_strided_trap_probe_addr\n"
"    ld t1, 0(t1)\n"
"    lbu t2, 0(t1)\n"
"    la t1, ai_micro_vec_strided_trap_probe_value\n"
"    sd t2, 0(t1)\n"
"    la t1, ai_micro_vec_strided_trap_probe_fired\n"
"    li t2, 1\n"
"    sd t2, 0(t1)\n"
AI_MICRO_VEC_STRIDED_HANDLER_PRESSURE_ASM
"    csrr t2, vstart\n"
"    la t1, ai_micro_vec_strided_trap_vstart\n"
"    sd t2, 0(t1)\n"
"    csrr t2, scause\n"
"    la t1, ai_micro_vec_strided_trap_cause\n"
"    sd t2, 0(t1)\n"
"    csrr t2, stval\n"
"    la t1, ai_micro_vec_strided_trap_tval\n"
"    sd t2, 0(t1)\n"
"    csrr t2, sepc\n"
"    la t1, ai_micro_vec_strided_trap_epc\n"
"    sd t2, 0(t1)\n"
"    addi t2, t2, 4\n"
"    csrw sepc, t2\n"
"    ld t1, 0(t0)\n"
"    ld t2, 8(t0)\n"
"    csrrw t0, sscratch, t0\n"
"    sret\n"
);

static inline void ai_micro_vec_strided_reset_trap_probe_state(uintptr_t probe_addr)
{
    __sync_synchronize();
    ai_micro_vec_strided_trap_probe_addr = probe_addr;
    ai_micro_vec_strided_trap_probe_value = 0;
    ai_micro_vec_strided_trap_probe_fired = 0;
    ai_micro_vec_strided_handler_pressure_base_addr = 0;
    ai_micro_vec_strided_handler_pressure_fired = 0;
    ai_micro_vec_strided_trap_cause = 0;
    ai_micro_vec_strided_trap_tval = 0;
    ai_micro_vec_strided_trap_epc = 0;
    ai_micro_vec_strided_trap_vstart = 0;
    __sync_synchronize();
}

static inline void ai_micro_vec_strided_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_VEC_STRIDED_PMP_TOR_RWX);
}

static inline void ai_micro_vec_strided_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_VEC_STRIDED_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_VEC_STRIDED_PMP_TOR_RWX << 16));
}

static inline pte_t *ai_micro_vec_strided_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_vec_strided_set_hs_leaf(enum test_page page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_micro_vec_strided_hs_leaf_entry(page) = (phys_page_base(page) >> 2) | flags;
}

static inline enum test_page ai_micro_vec_strided_target_page(unsigned index)
{
    return (enum test_page)((int)VSRWX_GURWX + (int)index);
}

static inline uintptr_t ai_micro_vec_strided_target_vaddr(unsigned index)
{
    return hs_page_base(ai_micro_vec_strided_target_page(index)) +
        AI_MICRO_VEC_STRIDED_TARGET_OFFSET;
}

static inline uintptr_t ai_micro_vec_strided_target_phys(unsigned index)
{
    return phys_page_base(ai_micro_vec_strided_target_page(index)) +
        AI_MICRO_VEC_STRIDED_TARGET_OFFSET;
}

static inline void ai_micro_vec_strided_write_bytes(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline void ai_micro_vec_strided_fill_bytes(uintptr_t addr, uint8_t value,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, value);
    }
}

static inline void ai_micro_vec_strided_zero_bytes(uintptr_t addr, unsigned len)
{
    ai_micro_vec_strided_fill_bytes(addr, 0U, len);
}

static inline bool ai_micro_vec_strided_bytes_match(uintptr_t addr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline int ai_micro_vec_strided_first_bytes_mismatch(uintptr_t addr,
    const uint8_t *expected, unsigned len, uint8_t *actual)
{
    for (unsigned i = 0; i < len; ++i) {
        uint8_t observed = (uint8_t)read8(addr + i);
        if (observed != expected[i]) {
            if (actual != NULL) {
                *actual = observed;
            }
            return (int)i;
        }
    }
    return -1;
}

static inline bool ai_micro_vec_strided_target_window_matches(const uint8_t *expected,
    unsigned vl)
{
    for (unsigned i = 0; i < vl; ++i) {
        if (read8(ai_micro_vec_strided_target_phys(i)) != expected[i]) {
            return false;
        }
    }
    return true;
}

static inline int ai_micro_vec_strided_first_target_mismatch(const uint8_t *expected,
    unsigned vl, uint8_t *actual)
{
    for (unsigned i = 0; i < vl; ++i) {
        uint8_t observed = (uint8_t)read8(ai_micro_vec_strided_target_phys(i));
        if (observed != expected[i]) {
            if (actual != NULL) {
                *actual = observed;
            }
            return (int)i;
        }
    }
    return -1;
}

static inline void ai_micro_vec_strided_fill_patterns(uint8_t *old_image,
    uint8_t *new_image, uint8_t *expected_after_fault, unsigned vl, unsigned fault_idx)
{
    for (unsigned i = 0; i < vl; ++i) {
        old_image[i] = (uint8_t)(0x30U + i);
        new_image[i] = (uint8_t)(0xa0U + i);
        expected_after_fault[i] = (i < fault_idx) ? new_image[i] : old_image[i];
    }
}

static inline void ai_micro_vec_strided_prepare_environment(const uint8_t *old_image,
    const uint8_t *new_image, unsigned vl, unsigned fault_idx)
{
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uintptr_t deny_start = phys_page_base(ai_micro_vec_strided_target_page(fault_idx));
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    goto_priv(PRIV_M);
    ai_micro_vec_strided_restore_pmp_full_access();
    for (unsigned i = 0; i < vl; ++i) {
        write8(ai_micro_vec_strided_target_phys(i), old_image[i]);
    }
    ai_micro_vec_strided_write_bytes(source_phys, new_image, vl);
    ai_micro_vec_strided_zero_bytes(observer_phys, vl);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < vl; ++i) {
        ai_micro_vec_strided_set_hs_leaf(ai_micro_vec_strided_target_page(i), leaf_flags);
    }
    ai_micro_vec_strided_set_hs_leaf(source_page, leaf_flags);
    ai_micro_vec_strided_set_hs_leaf(observer_page, leaf_flags);
    ai_micro_vec_strided_configure_pmp_window(deny_start, deny_end,
        AI_MICRO_VEC_STRIDED_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline void ai_micro_vec_strided_prepare_all_rw_environment(
    const uint8_t *old_image, const uint8_t *new_image, unsigned vl)
{
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;

    goto_priv(PRIV_M);
    ai_micro_vec_strided_restore_pmp_full_access();
    for (unsigned i = 0; i < vl; ++i) {
        write8(ai_micro_vec_strided_target_phys(i), old_image[i]);
    }
    ai_micro_vec_strided_write_bytes(source_phys, new_image, vl);
    ai_micro_vec_strided_zero_bytes(observer_phys, vl);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < vl; ++i) {
        ai_micro_vec_strided_set_hs_leaf(ai_micro_vec_strided_target_page(i), leaf_flags);
    }
    ai_micro_vec_strided_set_hs_leaf(source_page, leaf_flags);
    ai_micro_vec_strided_set_hs_leaf(observer_page, leaf_flags);
    ai_micro_vec_strided_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline void ai_micro_vec_strided_prepare_leaf_write_deny_environment(
    const uint8_t *old_image, const uint8_t *new_image, unsigned vl, unsigned fault_idx)
{
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t fault_leaf_flags = PTE_V | PTE_AD | PTE_R;

    goto_priv(PRIV_M);
    ai_micro_vec_strided_restore_pmp_full_access();
    for (unsigned i = 0; i < vl; ++i) {
        write8(ai_micro_vec_strided_target_phys(i), old_image[i]);
    }
    ai_micro_vec_strided_write_bytes(source_phys, new_image, vl);
    ai_micro_vec_strided_zero_bytes(observer_phys, vl);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < vl; ++i) {
        ai_micro_vec_strided_set_hs_leaf(ai_micro_vec_strided_target_page(i), leaf_flags);
    }
    ai_micro_vec_strided_set_hs_leaf(ai_micro_vec_strided_target_page(fault_idx),
        fault_leaf_flags);
    ai_micro_vec_strided_set_hs_leaf(source_page, leaf_flags);
    ai_micro_vec_strided_set_hs_leaf(observer_page, leaf_flags);
    ai_micro_vec_strided_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    sfence_vma();
}

static inline void ai_micro_vec_strided_cleanup_environment(void)
{
    ai_micro_vec_strided_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_micro_vec_strided_dense_store_pattern(uint64_t seed,
    uintptr_t line_index)
{
    return seed ^ ((0x9e3779b97f4a7c15ULL * (line_index + 1U)) +
        (line_index << 7));
}

static inline void ai_micro_vec_strided_dense_store_tail_refresh(
    uintptr_t page_base_vaddr, uintptr_t first_line_index,
    uintptr_t line_count, uint64_t seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        uintptr_t line_index = first_line_index + i;
        *(volatile uint64_t *)(page_base_vaddr +
            line_index * AI_MICRO_VEC_STRIDED_FILL_STRIDE) =
            ai_micro_vec_strided_dense_store_pattern(seed, line_index);
    }
}

static inline void ai_micro_vec_strided_dense_store_pressure_window(
    uintptr_t page_base_vaddr, uintptr_t line_count,
    uint64_t first_seed, uint64_t second_seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        *(volatile uint64_t *)(page_base_vaddr + i *
            AI_MICRO_VEC_STRIDED_FILL_STRIDE) =
            ai_micro_vec_strided_dense_store_pattern(first_seed, i);
    }

    for (uintptr_t i = 0; i < line_count; ++i) {
        *(volatile uint64_t *)(page_base_vaddr + i *
            AI_MICRO_VEC_STRIDED_FILL_STRIDE) =
            ai_micro_vec_strided_dense_store_pattern(second_seed, i);
    }

    if (line_count >= AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT) {
        uintptr_t first_tail_line = line_count -
            AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT;

        ai_micro_vec_strided_dense_store_tail_refresh(page_base_vaddr,
            first_tail_line, AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT,
            second_seed);

        ai_micro_vec_strided_handler_pressure_base_addr = page_base_vaddr +
            first_tail_line * AI_MICRO_VEC_STRIDED_FILL_STRIDE;
        ai_micro_vec_strided_handler_pressure_fired = 0;
        for (uintptr_t i = 0; i <
            AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT; ++i) {
            ai_micro_vec_strided_handler_pressure_words[i] =
                ai_micro_vec_strided_dense_store_pattern(second_seed,
                    first_tail_line + i);
        }
        __sync_synchronize();
    }
}

static inline bool ai_micro_vec_strided_dense_store_window_match(
    uintptr_t page_base_paddr, uintptr_t line_count, uint64_t seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        uint64_t expected = ai_micro_vec_strided_dense_store_pattern(seed, i);
        uint64_t observed = read64(page_base_paddr +
            i * AI_MICRO_VEC_STRIDED_FILL_STRIDE);
        if (observed != expected) {
            return false;
        }
    }
    return true;
}

static inline bool ai_micro_vec_strided_vaddr_window_matches(
    const uint8_t *expected, unsigned vl)
{
    for (unsigned i = 0; i < vl; ++i) {
        if (lbu(ai_micro_vec_strided_target_vaddr(i)) != expected[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_vec_vle8_to_v8_m2(const uint8_t *src, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), AI_MICRO_VEC_STRIDED_LMUL_M2);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vle8.v v8, (%0)\n\t"
        ".option pop\n\t"
        :
        : "r"(src), "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vec_vsse8_from_v8_m2(uint8_t *dst, intptr_t stride, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), AI_MICRO_VEC_STRIDED_LMUL_M2);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %2, %3\n\t"
        "vsse8.v v8, (%0), %1\n\t"
        ".option pop\n\t"
        :
        : "r"(dst), "r"(stride), "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vec_vlse8_to_v10_m2(const uint8_t *src, intptr_t stride,
    unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), AI_MICRO_VEC_STRIDED_LMUL_M2);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %2, %3\n\t"
        "vlse8.v v10, (%0), %1\n\t"
        ".option pop\n\t"
        :
        : "r"(src), "r"(stride), "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vec_vse8_from_v10_m2(uint8_t *dst, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), AI_MICRO_VEC_STRIDED_LMUL_M2);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vse8.v v10, (%0)\n\t"
        ".option pop\n\t"
        : "+r"(dst)
        : "r"(vl), "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_same_parameter_strided_load_sees_committed_prefix_and_old_suffix_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_preflight[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_preflight_load_vl = 0;
    uint64_t observed_preflight_store_vl = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_observer_load_vl = 0;
    uint64_t observed_observer_store_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault, requested_vl,
        fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_preflight[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        expected_observer_after_fault[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_preflight[i] = old_image[i];
            expected_observer_after_fault[i] = expected_after_fault[i];
        }
    }
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl, fault_idx);

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_preflight_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("with requested AVL=33 on e8,m2, the preflight same-parameter strided load resolves to active vl=32 and touches only elements0-31 before the late31 fault episode",
        excpt.triggered == false &&
        observed_preflight_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u vlenb=%lu",
        excpt.triggered, observed_preflight_load_vl, active_vl, CSRR(CSR_VLENB)
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_preflight_store_vl = CSRR(CSR_VL);
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_preflight, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("the preflight observer scratch receives old bytes for active elements0-31 while leaving inactive idx32 at the sentinel, proving same-parameter e8,m2 observation does not touch idx+1",
        excpt.triggered == false &&
        observed_preflight_store_vl == active_vl &&
        observer_mismatch_idx < 0,
        "triggered=%d observed_vl=%lu mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        excpt.triggered, observed_preflight_store_vl, observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_preflight[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_preflight[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one), expected_observer_preflight[idx_plus_one]
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31-of-33 strided store sees the original first-byte image before the fault episode",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source unit-stride vector load for requested AVL=33 also resolves to active vl=32 before the late31 fault probe",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the requested-33 e8,m2 strided store faults on active element31, which is the late active slot in the current 32-element execution shape",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart, observed_fault_vl,
        fault_vaddr, fault_idx, active_vl
    );

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_observer_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the late31 fault, the younger same-parameter observer load still runs with active vl=32 and can only inspect elements0-31",
        excpt.triggered == false &&
        observed_observer_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_observer_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_observer_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("the younger same-parameter observer vector store also retires at active vl=32 without introducing a second exception",
        excpt.triggered == false &&
        observed_observer_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_observer_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("the fault episode commits active prefix elements0-30, keeps active fault element31 old, and also leaves inactive idx32 old in memory",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_fault, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("the younger same-parameter observer sees committed-prefix plus old fault element31 on active slots0-31, while its inactive idx32 scratch byte remains untouched at the sentinel",
        observer_mismatch_idx < 0,
        "observer_mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_fault[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_fault[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one), expected_observer_after_fault[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31 fault and same-parameter observer sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_scalar_probes_see_committed_prefix_and_old_suffix_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t prefix_probe = 0;
    uint8_t fault_probe = 0;
    uint8_t suffix_probe = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault, requested_vl,
        fault_idx);
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl, fault_idx);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector setup for the late31 scalar-probe case resolves requested AVL=33 into active vl=32",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    TEST_SETUP_EXCEPT();
    (void)lbu(target_base_vaddr);
    TEST_ASSERT("the older scalar load in front of the late31-of-33 strided store plus scalar-probe sequence retires without exception",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the late31 scalar-probe case faults on active element31 while the requested AVL=33 execution shape still has active vl=32",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart, observed_fault_vl,
        fault_vaddr, fault_idx, active_vl
    );

    TEST_SETUP_EXCEPT();
    prefix_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx - 1U));
    TEST_ASSERT("the scalar control probe on element30 sees the committed prefix byte after the late31-of-33 store fault",
        excpt.triggered == false &&
        prefix_probe == new_image[fault_idx - 1U]
    );

    TEST_SETUP_EXCEPT();
    fault_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx));
    TEST_ASSERT("the scalar probe on the denied fault element31 still sees the old byte after the late31-of-33 store fault",
        excpt.triggered == false &&
        fault_probe == old_image[fault_idx]
    );

    TEST_SETUP_EXCEPT();
    suffix_probe = lbu(ai_micro_vec_strided_target_vaddr(idx_plus_one));
    TEST_ASSERT("the scalar probe on idx+1 page32, which is outside the active vl=32 window, also still sees the old byte after the late31 store fault",
        excpt.triggered == false &&
        suffix_probe == old_image[idx_plus_one]
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("the late31 fault leaves active elements0-30 committed, active fault element31 old, and inactive idx32 old in memory, matching the scalar probes",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31 store fault and scalar follow-up probes",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_req33_e8m2_active32_same_parameter_observer_leaves_idx32_untouched_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t source_vaddr = hs_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_preflight[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_store[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t inactive_idx32_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint64_t observed_preflight_load_vl = 0;
    uint64_t observed_preflight_store_vl = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_store_vl = 0;
    uint64_t observed_post_store_load_vl = 0;
    uint64_t observed_post_store_observer_vl = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_observer_after_store,
        requested_vl, active_vl);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_preflight[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        expected_observer_after_store[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_preflight[i] = old_image[i];
            expected_observer_after_store[i] = new_image[i];
        }
    }
    ai_micro_vec_strided_prepare_all_rw_environment(old_image, new_image, requested_vl);

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_preflight_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("the no-fault boundary guard resolves requested AVL=33 on e8,m2 into active vl=32 before any store runs",
        excpt.triggered == false &&
        observed_preflight_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_preflight_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_preflight_store_vl = CSRR(CSR_VL);
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_preflight, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("before the no-fault store, the same-parameter observer copies old bytes for active elements0-31 and leaves idx32 untouched at the sentinel",
        excpt.triggered == false &&
        observed_preflight_store_vl == active_vl &&
        observer_mismatch_idx < 0,
        "triggered=%d observed_vl=%lu mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        excpt.triggered, observed_preflight_store_vl, observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_preflight[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + active_vl - 1U),
        expected_observer_preflight[active_vl - 1U],
        (unsigned)read8(observer_phys + idx_plus_one),
        expected_observer_preflight[idx_plus_one]
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the no-fault guard store sees the original first-byte image before the same-parameter vector store runs",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source unit-stride vector load for the no-fault guard also resolves to active vl=32",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("the no-fault same-parameter vector store retires without exception and still only executes 32 active elements",
        excpt.triggered == false &&
        observed_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(new_image, active_vl,
        &target_mismatch_actual);
    inactive_idx32_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("after the no-fault same-parameter store, active elements0-31 become the new image while inactive idx32 remains the old image because it never enters the active vl=32 window",
        target_mismatch_idx < 0 &&
        inactive_idx32_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? new_image[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(active_vl - 1U)),
        new_image[active_vl - 1U], inactive_idx32_value, old_image[idx_plus_one]
    );
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_post_store_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the no-fault store, the same-parameter observer load still resolves to active vl=32",
        excpt.triggered == false &&
        observed_post_store_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_post_store_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_post_store_observer_vl = CSRR(CSR_VL);
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_store, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("after the no-fault store, the same-parameter observer sees new bytes for active elements0-31 while still leaving idx32 untouched at the sentinel",
        excpt.triggered == false &&
        observed_post_store_observer_vl == active_vl &&
        observer_mismatch_idx < 0,
        "triggered=%d observed_vl=%lu mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        excpt.triggered, observed_post_store_observer_vl, observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_store[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + active_vl - 1U),
        expected_observer_after_store[active_vl - 1U],
        (unsigned)read8(observer_phys + idx_plus_one),
        expected_observer_after_store[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the no-fault guard sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_repair_exact_replay_same_parameter_strided_load_sees_full_new_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_replay[AI_MICRO_VEC_STRIDED_VL];
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_pre_repair_load_vl = 0;
    uint64_t observed_pre_repair_store_vl = 0;
    uint64_t observed_replay_source_vl = 0;
    uint64_t observed_replay_store_vl = 0;
    uint64_t observed_post_replay_load_vl = 0;
    uint64_t observed_post_replay_store_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault, requested_vl,
        fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_after_fault[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        expected_observer_after_replay[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_after_fault[i] = expected_after_fault[i];
            expected_observer_after_replay[i] = new_image[i];
        }
    }
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl, fault_idx);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the first source staging step for the late31 repair case resolves requested AVL=33 into active vl=32",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first late31 repair episode faults on active element31 while the requested-33 e8,m2 execution shape remains 32 active elements",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart, observed_fault_vl,
        fault_vaddr, fault_idx, active_vl
    );

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_pre_repair_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("the immediate pre-repair same-parameter observer load also runs with active vl=32 after the first late31 fault",
        excpt.triggered == false &&
        observed_pre_repair_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_pre_repair_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_pre_repair_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("the pre-repair observer vector store retires at active vl=32 without introducing a second exception",
        excpt.triggered == false &&
        observed_pre_repair_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_pre_repair_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_fault, requested_vl, &observer_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("before repairing PMP, active slots0-31 still show committed-prefix plus old fault element31, and inactive idx32 stays old in both memory and observer scratch",
        target_mismatch_idx < 0 &&
        observer_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d observer_mismatch_idx=%d mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x obs31=0x%x exp31_obs=0x%x obs32=0x%x exp32_obs=0x%x",
        target_mismatch_idx, observer_mismatch_idx,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one],
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_fault[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one),
        expected_observer_after_fault[idx_plus_one]
    );

    ai_micro_vec_strided_restore_pmp_full_access();
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_replay_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("after repairing PMP, the source payload can be restaged and still resolves to active vl=32 for the exact replay",
        excpt.triggered == false &&
        observed_replay_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_replay_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_replay_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after restoring write permission on page31, the exact replay retires without exception and still runs with active vl=32",
        excpt.triggered == false &&
        observed_replay_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_replay_store_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_post_replay_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the repaired exact replay, the same-parameter observer load retires without exception at active vl=32",
        excpt.triggered == false &&
        observed_post_replay_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_post_replay_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_post_replay_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the repaired exact replay, the observer vector store writes back active slots0-31 without introducing a second exception",
        excpt.triggered == false &&
        observed_post_replay_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_post_replay_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(new_image, active_vl,
        &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_replay, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("after repairing PMP and replaying the identical store, active elements0-31 become the staged new image while inactive idx32 still stays old because it never enters the active vl=32 window",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "first_target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? new_image[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)), new_image[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    TEST_ASSERT("after the repaired replay, the same-parameter observer returns the full active new image on slots0-31 while leaving inactive idx32 scratch untouched at the sentinel",
        observer_mismatch_idx < 0,
        "first_observer_mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_replay[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_replay[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one),
        expected_observer_after_replay[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the fault episode, repair step, and exact replay observer sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_preflight[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_preflight_load_vl = 0;
    uint64_t observed_preflight_store_vl = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_observer_load_vl = 0;
    uint64_t observed_observer_store_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault, requested_vl,
        fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_preflight[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        expected_observer_after_fault[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_preflight[i] = old_image[i];
            expected_observer_after_fault[i] = expected_after_fault[i];
        }
    }
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_preflight_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("with requested AVL=33 on e8,m2, the leaf-no-write preflight same-parameter strided load resolves to active vl=32 before the late31 store page fault episode",
        excpt.triggered == false &&
        observed_preflight_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_preflight_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_preflight_store_vl = CSRR(CSR_VL);
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_preflight, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("before the leaf-no-write store fault probe, the observer scratch receives old bytes on active elements0-31 and leaves idx32 untouched at the sentinel",
        excpt.triggered == false &&
        observed_preflight_store_vl == active_vl &&
        observer_mismatch_idx < 0,
        "triggered=%d observed_vl=%lu mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        excpt.triggered, observed_preflight_store_vl, observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_preflight[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_preflight[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one), expected_observer_preflight[idx_plus_one]
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the leaf-no-write late31 store fault sees the original first-byte image",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source unit-stride vector load for the leaf-no-write case also resolves requested AVL=33 into active vl=32",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the requested-33 e8,m2 strided store raises store page fault on active element31 when the HS leaf removes write permission on only that page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart, observed_fault_vl,
        fault_vaddr, fault_idx, active_vl
    );

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_observer_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the leaf-no-write late31 store page fault, the younger same-parameter observer load still runs with active vl=32",
        excpt.triggered == false &&
        observed_observer_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_observer_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_observer_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the leaf-no-write late31 store page fault, the observer vector store retires at active vl=32 without a second exception",
        excpt.triggered == false &&
        observed_observer_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_observer_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("the leaf-no-write fault episode commits active prefix elements0-30, keeps active fault element31 old, and also leaves inactive idx32 old in memory",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_fault, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("after the leaf-no-write late31 store page fault, the younger same-parameter observer sees committed-prefix plus old fault element31 on active slots0-31 while leaving idx32 untouched at the sentinel",
        observer_mismatch_idx < 0,
        "observer_mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_fault[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_fault[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one), expected_observer_after_fault[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the leaf-no-write late31 store fault and same-parameter observer sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_vector_observer_and_scalar_probes_both_see_old_fault_and_idx32_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) + AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) + AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t prefix_probe = 0;
    uint8_t fault_probe = 0;
    uint8_t idx_plus_one_probe = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_vector_observer_load_vl = 0;
    uint64_t observed_vector_observer_store_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault, requested_vl,
        fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_after_fault[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_after_fault[i] = expected_after_fault[i];
        }
    }
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl, fault_idx);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31-of-33 PMP st store fault plus double-observer sequence sees the original first-byte image",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source unit-stride vector load for the double-observer case resolves requested AVL=33 into active vl=32 before the fault probe",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the double-observer case first raises store access fault on active element31 when PMP denies only that page's write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart, observed_fault_vl,
        fault_vaddr, fault_idx, active_vl
    );

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE, requested_vl);
    observed_vector_observer_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the late31-of-33 PMP st fault, the first younger same-parameter vector observer load runs with active vl=32",
        excpt.triggered == false &&
        observed_vector_observer_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_vector_observer_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_vector_observer_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the late31-of-33 PMP st fault, the first younger vector observer store retires at active vl=32 without a second exception",
        excpt.triggered == false &&
        observed_vector_observer_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_vector_observer_store_vl, active_vl
    );

    TEST_SETUP_EXCEPT();
    prefix_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx - 1U));
    TEST_ASSERT("after the vector observer has already run, the scalar control probe on element30 still sees the committed prefix byte",
        excpt.triggered == false &&
        prefix_probe == new_image[fault_idx - 1U]
    );

    TEST_SETUP_EXCEPT();
    fault_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx));
    TEST_ASSERT("after the vector observer has already run, the scalar probe on the denied fault element31 still sees the old byte",
        excpt.triggered == false &&
        fault_probe == old_image[fault_idx]
    );

    TEST_SETUP_EXCEPT();
    idx_plus_one_probe = lbu(ai_micro_vec_strided_target_vaddr(idx_plus_one));
    TEST_ASSERT("after the vector observer has already run, the scalar probe on extra page32 still sees the old byte",
        excpt.triggered == false &&
        idx_plus_one_probe == old_image[idx_plus_one]
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("the late31-of-33 PMP st fault leaves active prefix elements0-30 committed, active fault element31 old, and inactive idx32 old in memory even after both younger observers have run",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_fault, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("the first younger same-parameter vector observer sees committed-prefix plus old fault element31 on active slots0-31 while leaving idx32 untouched at the sentinel, and the later scalar probes agree with that image",
        observer_mismatch_idx < 0,
        "observer_mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x prefix_probe=0x%x fault_probe=0x%x idx32_probe=0x%x",
        observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_fault[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_fault[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one), expected_observer_after_fault[idx_plus_one],
        prefix_probe, fault_probe, idx_plus_one_probe
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31-of-33 PMP st fault and the vector-plus-scalar double-observer sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

// The trap-entry same-parameter strided observer already proved it can miss the
// SQ-live window, so these late strided cases switch to the earliest possible
// scalar byte probe at handler head to tighten coverage around redirect-time
// forward.
bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned probe_idx = fault_idx;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl,
        fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31-of-33 PMP st trap-entry fault-byte probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late31-of-33 PMP st trap-entry fault-byte probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late31-of-33 PMP st trap-entry fault-byte probe case reports a precise store access fault on element31 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SAF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late31-of-33 PMP st trap-entry fault-byte probe case records vstart=31 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the late31 fault element still reads the old byte instead of a ghost-forwarded new payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late31-of-33 PMP st trap-entry fault-byte probe case still leaves active prefix elements0-30 committed while keeping active fault element31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31-of-33 PMP st trap-entry fault-byte probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned probe_idx = fault_idx;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31-of-33 HS leaf no-write trap-entry fault-byte probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late31-of-33 HS leaf no-write trap-entry fault-byte probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late31-of-33 HS leaf no-write trap-entry fault-byte probe case reports a precise store page fault on element31 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SPF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late31-of-33 HS leaf no-write trap-entry fault-byte probe case records vstart=31 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the late31 HS leaf no-write fault element still reads the old byte instead of a ghost-forwarded new payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late31-of-33 HS leaf no-write trap-entry fault-byte probe case still leaves active prefix elements0-30 committed while keeping active fault element31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31-of-33 HS leaf no-write trap-entry fault-byte probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned probe_idx = fault_idx;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl,
        fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 PMP st trap-entry fault-byte probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late29-of-33 PMP st trap-entry fault-byte probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late29-of-33 PMP st trap-entry fault-byte probe case reports a precise store access fault on element29 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SAF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 PMP st trap-entry fault-byte probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the late29 fault element still reads the old byte instead of a ghost-forwarded new payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late29-of-33 PMP st trap-entry fault-byte probe case still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29-of-33 PMP st trap-entry fault-byte probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_next_active_suffix_byte_probe_sees_old_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned probe_idx = fault_idx + 1U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl,
        fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 PMP st trap-entry next-active-suffix probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late29-of-33 PMP st trap-entry next-active-suffix probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late29-of-33 PMP st trap-entry next-active-suffix probe case reports a precise store access fault on element29 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SAF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 PMP st trap-entry next-active-suffix probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the immediate active suffix element30 still reads the old byte instead of a ghost-forwarded new payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late29-of-33 PMP st trap-entry next-active-suffix probe case still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29-of-33 PMP st trap-entry next-active-suffix probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_last_active_suffix_byte_probe_sees_old_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned probe_idx = last_active_idx;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl,
        fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 PMP st trap-entry last-active-suffix probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late29-of-33 PMP st trap-entry last-active-suffix probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late29-of-33 PMP st trap-entry last-active-suffix probe case reports a precise store access fault on element29 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SAF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 PMP st trap-entry last-active-suffix probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the last active suffix element31 still reads the old byte instead of a ghost-forwarded active-suffix payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late29-of-33 PMP st trap-entry last-active-suffix probe case still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29-of-33 PMP st trap-entry last-active-suffix probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned probe_idx = fault_idx;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 HS leaf no-write trap-entry fault-byte probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late29-of-33 HS leaf no-write trap-entry fault-byte probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry fault-byte probe case reports a precise store page fault on element29 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SPF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry fault-byte probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the late29 HS leaf no-write fault element still reads the old byte instead of a ghost-forwarded new payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry fault-byte probe case still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29-of-33 HS leaf no-write trap-entry fault-byte probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_next_active_suffix_byte_probe_sees_old_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned probe_idx = fault_idx + 1U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 HS leaf no-write trap-entry next-active-suffix probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late29-of-33 HS leaf no-write trap-entry next-active-suffix probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry next-active-suffix probe case reports a precise store page fault on element29 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SPF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry next-active-suffix probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the immediate active suffix element30 in the HS leaf no-write case still reads the old byte instead of a ghost-forwarded new payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry next-active-suffix probe case still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29-of-33 HS leaf no-write trap-entry next-active-suffix probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_last_active_suffix_byte_probe_sees_old_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned probe_idx = last_active_idx;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late29-of-33 HS leaf no-write trap-entry last-active-suffix probe case still sees the original first-byte image before the vector store faults",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector load for the late29-of-33 HS leaf no-write trap-entry last-active-suffix probe case resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);

    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry last-active-suffix probe case reports a precise store page fault on element29 before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == CAUSE_SPF &&
        ai_micro_vec_strided_trap_tval == fault_vaddr
    );
    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry last-active-suffix probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    TEST_ASSERT("the trap-entry handler's earliest byte probe on the last active suffix element31 in the HS leaf no-write case still reads the old byte instead of a ghost-forwarded active-suffix payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx]
    );
    TEST_ASSERT("the late29-of-33 HS leaf no-write trap-entry last-active-suffix probe case still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late29-of-33 HS leaf no-write trap-entry last-active-suffix probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_scalar_probes_see_committed_prefix_and_old_suffix_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t prefix_probe = 0;
    uint8_t fault_probe = 0;
    uint8_t suffix_probe = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source vector setup for the late31 leaf-no-write scalar-probe case resolves requested AVL=33 into active vl=32",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    TEST_SETUP_EXCEPT();
    (void)lbu(target_base_vaddr);
    TEST_ASSERT("the older scalar load in front of the late31 leaf-no-write scalar-probe sequence retires without exception",
        excpt.triggered == false
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the late31 leaf-no-write scalar-probe case faults on active element31 while the requested AVL=33 execution shape still has active vl=32",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart,
        observed_fault_vl, fault_vaddr, fault_idx, active_vl
    );

    TEST_SETUP_EXCEPT();
    prefix_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx - 1U));
    TEST_ASSERT("the scalar control probe on element30 sees the committed prefix byte after the late31 leaf-no-write store fault",
        excpt.triggered == false &&
        prefix_probe == new_image[fault_idx - 1U]
    );

    TEST_SETUP_EXCEPT();
    fault_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx));
    TEST_ASSERT("the scalar probe on the denied fault element31 still sees the old byte after the late31 leaf-no-write store fault",
        excpt.triggered == false &&
        fault_probe == old_image[fault_idx]
    );

    TEST_SETUP_EXCEPT();
    suffix_probe = lbu(ai_micro_vec_strided_target_vaddr(idx_plus_one));
    TEST_ASSERT("the scalar probe on idx32, which is outside the active vl=32 window, also still sees the old byte after the late31 leaf-no-write store fault",
        excpt.triggered == false &&
        suffix_probe == old_image[idx_plus_one]
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("the late31 leaf-no-write fault leaves active elements0-30 committed, active fault element31 old, and inactive idx32 old in memory, matching the scalar probes",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31 leaf-no-write store fault and scalar follow-up probes",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_repair_exact_replay_same_parameter_strided_load_sees_full_new_image_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_replay[AI_MICRO_VEC_STRIDED_VL];
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_pre_repair_load_vl = 0;
    uint64_t observed_pre_repair_store_vl = 0;
    uint64_t observed_replay_source_vl = 0;
    uint64_t observed_replay_store_vl = 0;
    uint64_t observed_post_replay_load_vl = 0;
    uint64_t observed_post_replay_store_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_after_fault[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        expected_observer_after_replay[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_after_fault[i] = expected_after_fault[i];
            expected_observer_after_replay[i] = new_image[i];
        }
    }
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the first source staging step for the late31 leaf-no-write repair case resolves requested AVL=33 into active vl=32",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the first late31 leaf-no-write repair episode faults on active element31 while the requested-33 e8,m2 execution shape remains 32 active elements",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart,
        observed_fault_vl, fault_vaddr, fault_idx, active_vl
    );

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_pre_repair_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("the immediate pre-repair same-parameter observer load also runs with active vl=32 after the first late31 leaf-no-write fault",
        excpt.triggered == false &&
        observed_pre_repair_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_pre_repair_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_pre_repair_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("the pre-repair observer vector store retires at active vl=32 without introducing a second exception in the late31 leaf-no-write repair case",
        excpt.triggered == false &&
        observed_pre_repair_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_pre_repair_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_fault, requested_vl, &observer_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("before repairing the leaf permission, active slots0-31 still show committed-prefix plus old fault element31, and inactive idx32 stays old in both memory and observer scratch",
        target_mismatch_idx < 0 &&
        observer_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d observer_mismatch_idx=%d mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x obs31=0x%x exp31_obs=0x%x obs32=0x%x exp32_obs=0x%x",
        target_mismatch_idx, observer_mismatch_idx,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one],
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_fault[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one),
        expected_observer_after_fault[idx_plus_one]
    );

    ai_micro_vec_strided_set_hs_leaf(ai_micro_vec_strided_target_page(fault_idx),
        leaf_flags);
    ai_micro_vec_strided_restore_pmp_full_access();
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_replay_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("after repairing the leaf permission, the source payload can be restaged and still resolves to active vl=32 for the exact replay",
        excpt.triggered == false &&
        observed_replay_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_replay_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_replay_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after restoring write permission on page31 in the leaf-no-write repair case, the exact replay retires without exception and still runs with active vl=32",
        excpt.triggered == false &&
        observed_replay_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_replay_store_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_post_replay_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the repaired exact replay in the leaf-no-write case, the same-parameter observer load retires without exception at active vl=32",
        excpt.triggered == false &&
        observed_post_replay_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_post_replay_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_post_replay_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the repaired exact replay in the leaf-no-write case, the observer vector store writes back active slots0-31 without introducing a second exception",
        excpt.triggered == false &&
        observed_post_replay_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_post_replay_store_vl, active_vl
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(new_image, active_vl,
        &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_replay, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("after repairing the leaf permission and replaying the identical store, active elements0-31 become the staged new image while inactive idx32 still stays old because it never enters the active vl=32 window",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "first_target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? new_image[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        new_image[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    TEST_ASSERT("after the repaired leaf replay, the same-parameter observer returns the full active new image on slots0-31 while leaving inactive idx32 scratch untouched at the sentinel",
        observer_mismatch_idx < 0,
        "first_observer_mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x",
        observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_replay[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_replay[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one),
        expected_observer_after_replay[idx_plus_one]
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the leaf-no-write fault episode, repair step, and exact replay observer sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_vector_observer_and_scalar_probes_both_see_old_fault_and_idx32_corner()
{
    TEST_START();

    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = AI_MICRO_VEC_STRIDED_FAULT_IDX;
    const unsigned idx_plus_one = AI_MICRO_VEC_STRIDED_SUFFIX_IDX;
    const enum test_page source_page = SWITCH1;
    const enum test_page observer_page = SWITCH2;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t observer_vaddr = hs_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    const uintptr_t observer_phys = phys_page_base(observer_page) +
        AI_MICRO_VEC_STRIDED_OBSERVER_OFFSET;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_observer_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t prefix_probe = 0;
    uint8_t fault_probe = 0;
    uint8_t idx_plus_one_probe = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t observer_mismatch_actual = 0;
    uint8_t idx_plus_one_value = 0;
    uint64_t observed_source_vl = 0;
    uint64_t observed_fault_vl = 0;
    uint64_t observed_vector_observer_load_vl = 0;
    uint64_t observed_vector_observer_store_vl = 0;
    uint64_t observed_vstart = 0;
    int target_mismatch_idx = -1;
    int observer_mismatch_idx = -1;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    for (unsigned i = 0; i < requested_vl; ++i) {
        expected_observer_after_fault[i] = AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL;
        if (i < active_vl) {
            expected_observer_after_fault[i] = expected_after_fault[i];
        }
    }
    ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
        requested_vl, fault_idx);

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    TEST_ASSERT("the older scalar load in front of the late31 leaf-no-write store fault plus double-observer sequence sees the original first-byte image",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    TEST_ASSERT("the source unit-stride vector load for the leaf-no-write double-observer case resolves requested AVL=33 into active vl=32 before the fault probe",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_fault_vl = CSRR(CSR_VL);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("the leaf-no-write double-observer case first raises store page fault on active element31 when the leaf clears only that page's write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == fault_idx &&
        observed_fault_vl == active_vl,
        "triggered=%d cause=0x%lx tval=0x%lx vstart=%lu observed_vl=%lu expected_tval=0x%lx expected_vstart=%u expected_active_vl=%u",
        excpt.triggered, excpt.cause, excpt.tval, observed_vstart,
        observed_fault_vl, fault_vaddr, fault_idx, active_vl
    );

    goto_priv(PRIV_M);
    ai_micro_vec_strided_fill_bytes(observer_phys, AI_MICRO_VEC_STRIDED_OBSERVER_SENTINEL,
        requested_vl);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vlse8_to_v10_m2((const uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    observed_vector_observer_load_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the late31 leaf-no-write fault, the first younger same-parameter vector observer load runs with active vl=32",
        excpt.triggered == false &&
        observed_vector_observer_load_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_vector_observer_load_vl, active_vl
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vse8_from_v10_m2((uint8_t *)observer_vaddr, requested_vl);
    observed_vector_observer_store_vl = CSRR(CSR_VL);
    TEST_ASSERT("after the late31 leaf-no-write fault, the first younger vector observer store retires at active vl=32 without a second exception",
        excpt.triggered == false &&
        observed_vector_observer_store_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_vector_observer_store_vl, active_vl
    );

    TEST_SETUP_EXCEPT();
    prefix_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx - 1U));
    TEST_ASSERT("after the vector observer has already run in the leaf-no-write case, the scalar control probe on element30 still sees the committed prefix byte",
        excpt.triggered == false &&
        prefix_probe == new_image[fault_idx - 1U]
    );

    TEST_SETUP_EXCEPT();
    fault_probe = lbu(ai_micro_vec_strided_target_vaddr(fault_idx));
    TEST_ASSERT("after the vector observer has already run in the leaf-no-write case, the scalar probe on the denied fault element31 still sees the old byte",
        excpt.triggered == false &&
        fault_probe == old_image[fault_idx]
    );

    TEST_SETUP_EXCEPT();
    idx_plus_one_probe = lbu(ai_micro_vec_strided_target_vaddr(idx_plus_one));
    TEST_ASSERT("after the vector observer has already run in the leaf-no-write case, the scalar probe on extra page32 still sees the old byte",
        excpt.triggered == false &&
        idx_plus_one_probe == old_image[idx_plus_one]
    );

    goto_priv(PRIV_M);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(expected_after_fault,
        active_vl, &target_mismatch_actual);
    idx_plus_one_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(idx_plus_one));
    TEST_ASSERT("the late31 leaf-no-write fault leaves active prefix elements0-30 committed, active fault element31 old, and inactive idx32 old in memory even after both younger observers have run",
        target_mismatch_idx < 0 &&
        idx_plus_one_value == old_image[idx_plus_one],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        idx_plus_one_value, old_image[idx_plus_one]
    );
    observer_mismatch_idx = ai_micro_vec_strided_first_bytes_mismatch(observer_phys,
        expected_observer_after_fault, requested_vl, &observer_mismatch_actual);
    TEST_ASSERT("the first younger same-parameter vector observer sees committed-prefix plus old fault element31 on active slots0-31 while leaving idx32 untouched at the sentinel in the leaf-no-write case, and the later scalar probes agree with that image",
        observer_mismatch_idx < 0,
        "observer_mismatch_idx=%d actual=0x%x expected=0x%x obs31=0x%x exp31=0x%x obs32=0x%x exp32=0x%x prefix_probe=0x%x fault_probe=0x%x idx32_probe=0x%x",
        observer_mismatch_idx,
        observer_mismatch_idx >= 0 ? observer_mismatch_actual : 0U,
        observer_mismatch_idx >= 0 ? expected_observer_after_fault[observer_mismatch_idx] : 0U,
        (unsigned)read8(observer_phys + fault_idx), expected_observer_after_fault[fault_idx],
        (unsigned)read8(observer_phys + idx_plus_one), expected_observer_after_fault[idx_plus_one],
        prefix_probe, fault_probe, idx_plus_one_probe
    );
    TEST_ASSERT("the staged 33-byte source payload remains intact across the late31 leaf-no-write fault and the vector-plus-scalar double-observer sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );

    ai_micro_vec_strided_cleanup_environment();

    TEST_END();
}

static inline void ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(
    bool leaf_write_deny, unsigned probe_idx, uint64_t expected_cause,
    bool *test_status)
{
    const unsigned requested_vl = AI_MICRO_VEC_STRIDED_VL;
    const unsigned active_vl = AI_MICRO_VEC_STRIDED_ACTIVE_VL;
    const unsigned fault_idx = 29U;
    const unsigned last_active_idx = active_vl - 1U;
    const unsigned inactive_idx = active_vl;
    const enum test_page source_page = SWITCH1;
    const enum test_page fill_page = SCRATCHPAD;
    const uintptr_t target_base_vaddr = ai_micro_vec_strided_target_vaddr(0U);
    const uintptr_t fault_vaddr = ai_micro_vec_strided_target_vaddr(fault_idx);
    const uintptr_t probe_vaddr = ai_micro_vec_strided_target_vaddr(probe_idx);
    const uintptr_t source_vaddr = hs_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t source_phys = phys_page_base(source_page) +
        AI_MICRO_VEC_STRIDED_SOURCE_OFFSET;
    const uintptr_t fill_vaddr_base = hs_page_base(fill_page);
    const uintptr_t fill_phys_base = phys_page_base(fill_page);
    const uint64_t pressure_first_seed = 0x3141592653589793ULL;
    const uint64_t pressure_second_seed = 0x2718281828459045ULL;
    const uint64_t pressure_third_seed = 0x1122334455667788ULL;
    const uint64_t pressure_fourth_seed = 0x8877665544332211ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    uintptr_t old_medeleg = 0;
    uintptr_t old_stvec = 0;
    uintptr_t old_sscratch = 0;
    uint8_t old_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t new_image[AI_MICRO_VEC_STRIDED_VL];
    uint8_t expected_after_fault[AI_MICRO_VEC_STRIDED_VL];
    uint8_t older_scalar_value = 0;
    uint8_t target_mismatch_actual = 0;
    uint8_t inactive_value = 0;
    uint64_t observed_source_vl = 0;
    int target_mismatch_idx = -1;
    bool target_prewarm_visible = false;

    ai_micro_vec_strided_fill_patterns(old_image, new_image, expected_after_fault,
        requested_vl, fault_idx);
    if (leaf_write_deny) {
        ai_micro_vec_strided_prepare_leaf_write_deny_environment(old_image, new_image,
            requested_vl, fault_idx);
    } else {
        ai_micro_vec_strided_prepare_environment(old_image, new_image, requested_vl,
            fault_idx);
    }

    goto_priv(PRIV_M);
    for (uintptr_t i = 0; i < AI_MICRO_VEC_STRIDED_PRESSURE_LINE_COUNT; ++i) {
        write64(fill_phys_base + i * AI_MICRO_VEC_STRIDED_FILL_STRIDE, 0ULL);
    }
    ai_micro_vec_strided_set_hs_leaf(fill_page, leaf_flags);

    goto_priv(PRIV_HS);
    sfence_vma();

    goto_priv(PRIV_M);
    old_medeleg = CSRR(medeleg);
    CSRW(medeleg, old_medeleg | (1ULL << CAUSE_SPF) | (1ULL << CAUSE_SAF));
    ai_micro_vec_strided_reset_trap_probe_state(probe_vaddr);

    goto_priv(PRIV_HS);
    old_stvec = CSRR(stvec);
    old_sscratch = CSRR(sscratch);
    CSRW(sscratch, (uintptr_t)&ai_micro_vec_strided_trap_scratch[
        sizeof(ai_micro_vec_strided_trap_scratch) /
        sizeof(ai_micro_vec_strided_trap_scratch[0])]);
    CSRW(stvec, ai_micro_vec_strided_trap_entry_byte_load_handler);

    TEST_SETUP_EXCEPT();
    ai_micro_vec_strided_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_VEC_STRIDED_PRESSURE_LINE_COUNT, pressure_first_seed,
        pressure_second_seed);
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the first dense translated-store pressure burst completes without exception before the late29 trap-entry byte probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_vec_strided_dense_store_pressure_window(fill_vaddr_base,
        AI_MICRO_VEC_STRIDED_PRESSURE_LINE_COUNT, pressure_third_seed,
        pressure_fourth_seed);
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the second dense translated-store pressure burst also completes without exception before the late29 trap-entry byte probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    older_scalar_value = lbu(ai_micro_vec_strided_target_vaddr(0U));
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the older scalar load in front of the late29 high-pressure trap-entry byte probe still sees the original first-byte image",
        excpt.triggered == false &&
        older_scalar_value == old_image[0]
    );

    TEST_SETUP_EXCEPT();
    target_prewarm_visible = ai_micro_vec_strided_vaddr_window_matches(old_image,
        active_vl);
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "a full active-window scalar prewarm across all 32 target pages still sees the old image before the late29 high-pressure trap-entry byte probe store fault is issued",
        excpt.triggered == false &&
        target_prewarm_visible
    );

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vle8_to_v8_m2((const uint8_t *)source_vaddr, requested_vl);
    observed_source_vl = CSRR(CSR_VL);
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the source vector load for the late29 high-pressure trap-entry byte probe resolves requested AVL=33 into active vl=32 before the store fault fires",
        excpt.triggered == false &&
        observed_source_vl == active_vl,
        "triggered=%d observed_vl=%lu expected_active_vl=%u",
        excpt.triggered, observed_source_vl, active_vl
    );

    ai_micro_vec_strided_dense_store_tail_refresh(fill_vaddr_base,
        AI_MICRO_VEC_STRIDED_PRESSURE_LINE_COUNT -
            AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT,
        AI_MICRO_VEC_STRIDED_FINAL_PRESSURE_LINE_COUNT, pressure_fourth_seed);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_micro_vec_vsse8_from_v8_m2((uint8_t *)target_base_vaddr, PAGE_SIZE,
        requested_vl);
    excpt.testing = false;

    CSRW(stvec, old_stvec);
    CSRW(sscratch, old_sscratch);

    goto_priv(PRIV_M);
    CSRW(medeleg, old_medeleg);
    target_mismatch_idx = ai_micro_vec_strided_first_target_mismatch(
        expected_after_fault, active_vl, &target_mismatch_actual);
    inactive_value = (uint8_t)read8(ai_micro_vec_strided_target_phys(inactive_idx));

    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the late29 high-pressure trap-entry byte probe case reports the precise expected store fault before the handler issues its byte load",
        ai_micro_vec_strided_trap_cause == expected_cause &&
        ai_micro_vec_strided_trap_tval == fault_vaddr,
        "cause=0x%lx expected=0x%lx tval=0x%lx expected_tval=0x%lx",
        ai_micro_vec_strided_trap_cause, expected_cause,
        ai_micro_vec_strided_trap_tval, fault_vaddr
    );
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the late29 high-pressure trap-entry byte probe case records vstart=29 for the first faulting element",
        ai_micro_vec_strided_trap_vstart == fault_idx
    );
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the trap-entry handler's earliest byte probe under dense translated-store pressure still reads the old byte instead of a ghost-forwarded payload",
        ai_micro_vec_strided_trap_probe_fired == 1ULL &&
        ai_micro_vec_strided_trap_probe_value == old_image[probe_idx],
        "probe_fired=%lu probe_value=0x%lx expected_old=0x%x probe_idx=%u",
        ai_micro_vec_strided_trap_probe_fired,
        ai_micro_vec_strided_trap_probe_value, old_image[probe_idx], probe_idx
    );
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "after the earliest trap-entry probe, the handler also replays the prepared dense translated-store tail burst so the exception-return window stays under active store pressure",
        ai_micro_vec_strided_handler_pressure_fired == 1ULL
    );
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the late29 high-pressure episode still leaves active prefix elements0-28 committed while keeping active fault-plus-suffix elements29-31 old in memory and leaving inactive idx32 old",
        target_mismatch_idx < 0 &&
        inactive_value == old_image[inactive_idx],
        "target_mismatch_idx=%d actual=0x%x expected=0x%x mem29=0x%x exp29=0x%x mem31=0x%x exp31=0x%x mem32=0x%x exp32=0x%x",
        target_mismatch_idx,
        target_mismatch_idx >= 0 ? target_mismatch_actual : 0U,
        target_mismatch_idx >= 0 ? expected_after_fault[target_mismatch_idx] : 0U,
        (unsigned)read8(ai_micro_vec_strided_target_phys(fault_idx)),
        expected_after_fault[fault_idx],
        (unsigned)read8(ai_micro_vec_strided_target_phys(last_active_idx)),
        expected_after_fault[last_active_idx],
        inactive_value, old_image[inactive_idx]
    );
    AI_MICRO_VEC_STRIDED_LOCAL_ASSERT(test_status, "the staged 33-byte source payload remains intact across the late29 high-pressure trap-entry byte probe sequence",
        ai_micro_vec_strided_bytes_match(source_phys, new_image, requested_vl)
    );
    ai_micro_vec_strided_cleanup_environment();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_fault_byte_probe_under_dense_store_pressure_sees_old_fault_image_corner()
{
    TEST_START();
    ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(false,
        29U, CAUSE_SAF, &test_status);
    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_next_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner()
{
    TEST_START();
    ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(false,
        30U, CAUSE_SAF, &test_status);
    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_last_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner()
{
    TEST_START();
    ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(false,
        AI_MICRO_VEC_STRIDED_ACTIVE_VL - 1U, CAUSE_SAF, &test_status);
    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_fault_byte_probe_under_dense_store_pressure_sees_old_fault_image_corner()
{
    TEST_START();
    ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(true,
        29U, CAUSE_SPF, &test_status);
    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_next_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner()
{
    TEST_START();
    ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(true,
        30U, CAUSE_SPF, &test_status);
    TEST_END();
}

bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_last_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner()
{
    TEST_START();
    ai_micro_vec_strided_run_late29_trap_probe_under_dense_store_pressure(true,
        AI_MICRO_VEC_STRIDED_ACTIVE_VL - 1U, CAUSE_SPF, &test_status);
    TEST_END();
}
