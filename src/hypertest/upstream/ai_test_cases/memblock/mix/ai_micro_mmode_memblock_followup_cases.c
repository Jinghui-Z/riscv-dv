#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep follow-up no-H MemBlock probes in a separate file so the original
 * ai_micro_mmode_memblock_cases.c does not grow without bound.
 */

#define AI_MICRO_FOLLOWUP_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_FOLLOWUP_PMP_TOR_NO_R 0x0cULL
#define AI_MICRO_FOLLOWUP_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_FOLLOWUP_BOUNDARY0_LOW_PAGE SWITCH1
#define AI_MICRO_FOLLOWUP_BOUNDARY0_HIGH_PAGE SWITCH2
#define AI_MICRO_FOLLOWUP_BOUNDARY1_LOW_PAGE VSRWXPbmt0_GURWXPbmt0
#define AI_MICRO_FOLLOWUP_BOUNDARY1_HIGH_PAGE VSRWXPbmt0_GURWXPbmt1

#define AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET 12U
#define AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET 15U
#define AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET 44U
#define AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET 47U

static inline pte_t *ai_micro_followup_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_followup_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_followup_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_micro_followup_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_followup_set_leaf_paddr(page, phys_page_base(page), flags);
}

static inline void ai_micro_followup_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_followup_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_FOLLOWUP_PMP_TOR_RWX);
}

static inline void ai_micro_followup_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_followup_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    const uintptr_t pmp_deny_start = deny_start & PAGE_ADDR_MSK;
    const uintptr_t pmp_deny_end = pmp_deny_start + PAGE_SIZE;

    (void)deny_end;
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, pmp_deny_start >> 2);
    CSRW(CSR_PMPADDR1, pmp_deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_FOLLOWUP_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_FOLLOWUP_PMP_TOR_RWX << 16));
}

static inline void ai_micro_followup_reset_env(void)
{
    ai_micro_followup_restore_pmp_full_access();
    ai_micro_followup_clear_machine_access_mode();
}

static inline enum test_page ai_micro_followup_low_page(unsigned boundary_index)
{
    return boundary_index == 0U ?
        AI_MICRO_FOLLOWUP_BOUNDARY0_LOW_PAGE :
        AI_MICRO_FOLLOWUP_BOUNDARY1_LOW_PAGE;
}

static inline enum test_page ai_micro_followup_high_page(
    unsigned boundary_index)
{
    return boundary_index == 0U ?
        AI_MICRO_FOLLOWUP_BOUNDARY0_HIGH_PAGE :
        AI_MICRO_FOLLOWUP_BOUNDARY1_HIGH_PAGE;
}

static inline uintptr_t ai_micro_followup_boundary_phys(unsigned boundary_index)
{
    return phys_page_base(ai_micro_followup_low_page(boundary_index)) +
        PAGE_SIZE - sizeof(uint64_t);
}

static inline uintptr_t ai_micro_followup_boundary_high_phys(
    unsigned boundary_index)
{
    return phys_page_base(ai_micro_followup_high_page(boundary_index));
}

static inline uintptr_t ai_micro_followup_guard_before_phys(void)
{
    return ai_micro_followup_boundary_phys(0U) - sizeof(uint64_t);
}

static inline uintptr_t ai_micro_followup_guard_middle_phys(void)
{
    return ai_micro_followup_boundary_high_phys(0U) + 16U;
}

static inline uintptr_t ai_micro_followup_guard_after_phys(void)
{
    return ai_micro_followup_boundary_high_phys(1U) + sizeof(uint64_t);
}

static inline uintptr_t ai_micro_followup_boundary_vaddr(
    unsigned boundary_index, uintptr_t start_byte)
{
    return hs_page_base(ai_micro_followup_low_page(boundary_index)) +
        PAGE_SIZE - sizeof(uint64_t) + start_byte;
}

static inline uintptr_t ai_micro_followup_cross_16b_second_half_vaddr(
    uintptr_t vaddr)
{
    return (vaddr & PAGE_ADDR_MSK) + PAGE_SIZE;
}

static inline void ai_micro_followup_set_cross_16b_leaves(uint64_t flags)
{
    ai_micro_followup_set_leaf(
        AI_MICRO_FOLLOWUP_BOUNDARY0_LOW_PAGE, flags);
    ai_micro_followup_set_leaf(
        AI_MICRO_FOLLOWUP_BOUNDARY0_HIGH_PAGE, flags);
    ai_micro_followup_set_leaf(
        AI_MICRO_FOLLOWUP_BOUNDARY1_LOW_PAGE, flags);
    ai_micro_followup_set_leaf(
        AI_MICRO_FOLLOWUP_BOUNDARY1_HIGH_PAGE, flags);
}

static inline uintptr_t ai_micro_followup_cross_16b_addr_with_offset(
    enum test_page page, uintptr_t offset)
{
    (void)page;
    switch (offset) {
    case AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET:
        return ai_micro_followup_boundary_vaddr(0U, 4U);
    case AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET:
        return ai_micro_followup_boundary_vaddr(0U, 7U);
    case AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET:
        return ai_micro_followup_boundary_vaddr(1U, 4U);
    case AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET:
        return ai_micro_followup_boundary_vaddr(1U, 7U);
    default:
        return hs_page_base(AI_MICRO_FOLLOWUP_BOUNDARY0_LOW_PAGE) + offset;
    }
}

static inline void ai_micro_followup_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline void ai_micro_followup_overlay_little_endian_bytes(
    uint8_t *dst, const uint8_t *seed, unsigned len, unsigned start_byte,
    uint64_t value, unsigned size)
{
    for (unsigned i = 0; i < len; ++i) {
        dst[i] = seed[i];
    }
    for (unsigned i = 0; i < size; ++i) {
        dst[start_byte + i] = (uint8_t)((value >> (8U * i)) & 0xffU);
    }
}

static inline bool ai_micro_followup_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline uint64_t ai_micro_followup_u64_from_bytes(const uint8_t *bytes)
{
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[i]) << (8U * i);
    }
    return value;
}

#define AI_MICRO_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION(name, inst, type) \
static inline void name(uintptr_t addr, type value) \
{ \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %0\n\t" \
        "csrs mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        inst " %3, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        "csrc mstatus, %0\n\t" \
        : \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(value), "r"(addr) \
        : "memory"); \
}

AI_MICRO_FOLLOWUP_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_followup_translated_sd, "sd", uint64_t)

#define AI_MICRO_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(name, inst) \
static inline uint64_t name(uintptr_t addr) \
{ \
    uint64_t value = 0ULL; \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        "csrs mstatus, %3\n\t" \
        inst " %0, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %2\n\t" \
        "csrc mstatus, %1\n\t" \
        : "=r"(value) \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(addr) \
        : "memory"); \
    return value; \
}

AI_MICRO_FOLLOWUP_TRANSLATED_LOAD_INSTRUCTION(
    ai_micro_followup_translated_ld, "ld")

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_cross_producer_load_fault_refreshes_back_cause_tval_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t return_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned cross-producer load fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned cross-producer load fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned cross-producer load fault probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(return_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, a returned cross-producer load fault on the original boundary must refresh cause and tval back to the current producer and current template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(return_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then returned cross-producer load fault probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_next_boundary_valid_different_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1021324354657687ULL,
        0x8796a5b4c3d2e1f0ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t final_success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_success_boundary,
            expected_success_boundary, 16U, 4U,
            final_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the next-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the next-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the next-boundary-valid-different-template-store probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the next-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the next-boundary-valid-different-template-store probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the next-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final next-boundary valid different-template store",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(final_success_vaddr,
            final_store_values[second]);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next next-boundary valid different-template store must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-returned-fault then next-boundary-valid-different-template-store probe preserves the original boundary image, updates only the current next-boundary template bytes, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_final_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_different_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1021324354657687ULL,
        0x8796a5b4c3d2e1f0ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_return_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_return_boundary,
            init_patterns[first][round], 16U, 4U,
            final_store_values[first], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned-valid-different-template-store probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the returned-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the returned-valid-different-template-store probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the returned-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final returned valid different-template store",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            final_store_values[first]);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next returned valid different-template store on the original boundary must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-same-boundary-fault then returned-valid-different-template-store probe preserves the success-boundary overlay, updates only the current returned boundary template bytes, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                expected_final_return_boundary, 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_cross_producer_load_fault_refreshes_current_cause_tval_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t younger_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the same-boundary cross-producer load fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the same-boundary cross-producer load fault probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the same-boundary cross-producer load fault probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(younger_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, a same-boundary cross-producer load fault on that next boundary must refresh cause and tval to the current producer and current template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(younger_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then same-boundary cross-producer load fault probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_two_cross_producer_load_faults_refresh_latest_cause_tval_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the two-step cross-producer load fault overwrite probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the two-step cross-producer load fault overwrite probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the two-step cross-producer load fault overwrite probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first same-boundary cross-producer load fault must refresh cause and tval to that next-boundary load template address before the returned overwrite probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first same-boundary cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the returned overwrite probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after one same-boundary cross-producer load fault has already been recorded, the returned cross-producer load fault on the original boundary must overwrite cause and tval with the latest producer and latest returned template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then two cross-producer load faults overwrite probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_refreshes_latest_cause_tval_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned-then-same-boundary overwrite probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned-then-same-boundary overwrite probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned-then-same-boundary overwrite probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the same-boundary overwrite probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the same-boundary overwrite probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then returned-then-same-boundary cross-producer load faults overwrite probe preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_valid_template_load_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint64_t final_load_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final valid load",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        final_load_value = ai_micro_followup_translated_ld(success_vaddr);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next valid template load on the success boundary must clear stale exception metadata and return the latest success-store value",
            excpt.triggered == false &&
            final_load_value == success_store_values[second]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then fault-overwrite then valid-load stale-exception-clear probe preserves the success-boundary overlay and all guard regions",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_valid_template_load_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint64_t final_load_value = 0ULL;
    uint64_t expected_return_load_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        expected_return_load_value =
            ai_micro_followup_u64_from_bytes(&init_patterns[first][round][7]);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned-boundary stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first same-boundary cross-producer load fault must refresh cause and tval to the next-boundary template address before the returned-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first same-boundary cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the returned-boundary stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after one same-boundary cross-producer load fault has already been recorded, the returned cross-producer load fault on the original boundary must overwrite cause and tval with the latest producer and latest returned template-specific address before the returned-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second returned cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final returned valid load",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        final_load_value = ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the same-boundary-then-returned cross-producer load faults are complete and access is restored, the next valid returned template load on the original boundary must clear stale exception metadata and return the current original-boundary template value",
            excpt.triggered == false &&
            final_load_value == expected_return_load_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-returned-fault then valid-returned-load stale-exception-clear probe preserves the success-boundary overlay, original boundary image, and all guard regions",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_next_boundary_valid_template_load_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint64_t final_load_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the next-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the next-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the next-boundary stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first same-boundary cross-producer load fault must refresh cause and tval to the next-boundary template address before the next-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first same-boundary cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the next-boundary stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after one same-boundary cross-producer load fault has already been recorded, the returned cross-producer load fault on the original boundary must overwrite cause and tval with the latest producer and latest returned template-specific address before the next-boundary stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second returned cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final next-boundary valid load",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        final_load_value = ai_micro_followup_translated_ld(success_vaddr);
        TEST_ASSERT("after the same-boundary-then-returned cross-producer load faults are complete and access is restored, the next valid load on the success boundary must clear stale exception metadata and return the current success-boundary template value",
            excpt.triggered == false &&
            final_load_value == success_store_values[second]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-returned-fault then valid-next-boundary-load stale-exception-clear probe preserves the success-boundary overlay, original boundary image, and all guard regions",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_template_load_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint64_t final_load_value = 0ULL;
    uint64_t expected_return_load_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        expected_return_load_value =
            ai_micro_followup_u64_from_bytes(&init_patterns[first][round][7]);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned-boundary-after-latest-same stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned-boundary-after-latest-same stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned-boundary-after-latest-same stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the returned-boundary-after-latest-same stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the returned-boundary-after-latest-same stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the returned-boundary-after-latest-same stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final returned valid load",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        final_load_value = ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next valid returned template load on the original boundary must clear stale exception metadata and return the current original-boundary template value",
            excpt.triggered == false &&
            final_load_value == expected_return_load_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-same-boundary-fault then valid-returned-load stale-exception-clear probe preserves the success-boundary overlay, original boundary image, and all guard regions",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_boundary,
            init_patterns[first][round], 16U, 7U,
            final_store_values[first], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned-boundary valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned-boundary valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned-boundary valid-store stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the returned-boundary valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the returned-boundary valid-store stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the returned-boundary valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final returned valid store",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(returned_load_fault_vaddr,
            final_store_values[first]);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next valid returned template store on the original boundary must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-same-boundary-fault then valid-returned-store stale-exception-clear probe preserves the success-boundary overlay, updates only the current returned boundary template bytes, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                expected_final_boundary, 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_next_boundary_valid_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_success_boundary,
            init_patterns[second][round], 16U, 7U,
            final_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the next-boundary-after-latest-returned valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the next-boundary-after-latest-returned valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the next-boundary-after-latest-returned valid-store stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the next-boundary-after-latest-returned valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the next-boundary-after-latest-returned valid-store stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the next-boundary-after-latest-returned valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final next-boundary valid store",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr, final_store_values[second]);
        TEST_ASSERT("after the same-boundary-then-returned cross-producer load faults are complete and access is restored, the next valid store on the success boundary must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-returned-fault then valid-next-boundary-store stale-exception-clear probe preserves the original boundary image, updates only the current success boundary template bytes, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_final_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_returned_valid_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_return_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_return_boundary,
            init_patterns[first][round], 16U, 7U,
            final_store_values[first], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the returned-boundary-after-latest-same valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the returned-boundary-after-latest-same valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the returned-boundary-after-latest-same valid-store stale-exception clear probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the returned-boundary-after-latest-same valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the returned-boundary-after-latest-same valid-store stale-exception clear probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the returned-boundary-after-latest-same valid-store stale-exception clear probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final returned valid store",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(returned_load_fault_vaddr,
            final_store_values[first]);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next valid returned template store on the original boundary must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-same-boundary-fault then valid-returned-store stale-exception-clear probe preserves the success-boundary overlay, updates only the current returned boundary template bytes, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                expected_final_return_boundary, 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_next_boundary_valid_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_success_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_success_boundary,
            init_patterns[second][round], 16U, 7U,
            final_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the next-boundary-after-latest-same valid-store mirror probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the next-boundary-after-latest-same valid-store mirror probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the next-boundary-after-latest-same valid-store mirror probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the next-boundary-after-latest-same valid-store mirror probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the next-boundary-after-latest-same valid-store mirror probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the next-boundary-after-latest-same valid-store mirror probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final next-boundary valid store mirror probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr, final_store_values[second]);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next valid store on the success boundary must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-same-boundary-fault then valid-next-boundary-store mirror probe preserves the original boundary image, updates only the current success boundary template bytes, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_final_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_same_boundary_valid_different_template_store_clears_stale_exception_corner()
{
    TEST_START();

    static const uint64_t first_fault_store_values[2] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t success_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t final_store_values[2] = {
        0x1021324354657687ULL,
        0x8796a5b4c3d2e1f0ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_followup_boundary_phys(0U),
        ai_micro_followup_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_followup_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_followup_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_followup_guard_after_phys();
    uint8_t expected_success_boundary[16];
    uint8_t expected_final_same_boundary[16];

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t success_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t returned_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                first == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_OFFSET);
        const uintptr_t same_boundary_load_fault_vaddr =
            ai_micro_followup_cross_16b_addr_with_offset(page,
                second == 0U ?
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET :
                AI_MICRO_FOLLOWUP_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET);
        const uintptr_t first_write_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_write_deny_end =
            first_write_deny_start + PAGE_SIZE;
        const uintptr_t first_read_deny_start = boundary_phys[first] + 8U;
        const uintptr_t first_read_deny_end =
            first_read_deny_start + PAGE_SIZE;
        const uintptr_t second_read_deny_start = boundary_phys[second] + 8U;
        const uintptr_t second_read_deny_end =
            second_read_deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_followup_reset_env();
        ai_micro_followup_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_followup_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_followup_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_followup_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_followup_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_success_boundary,
            init_patterns[second][round], 16U, 7U,
            success_store_values[second], 8U);
        ai_micro_followup_overlay_little_endian_bytes(
            expected_final_same_boundary,
            expected_success_boundary, 16U, 4U,
            final_store_values[second], 8U);
        ai_micro_followup_configure_pmp_window(
            first_write_deny_start, first_write_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_W);

        ai_micro_followup_prepare_supervisor_translation();
        ai_micro_followup_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("the first no-H same-page same-width different-template boundary store fault reports store access fault on the original template-specific boundary address before the same-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(first_fault_vaddr,
            first_fault_store_values[first]);
        TEST_ASSERT("repeating the no-H same-page same-width different-template boundary store fault keeps the same precise metadata before the same-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(first_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template boundary store fault, both boundaries and the middle gap still keep their seeded images before repair",
            ai_micro_followup_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(success_vaddr,
            success_store_values[second]);
        TEST_ASSERT("after repairing the earlier no-H same-page same-width different-template boundary store fault, the first valid next-boundary store commits cleanly before the same-boundary-valid-different-template-store probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a valid next-boundary store, only the next boundary keeps its template-specific overlay while the original faulting boundary and all guard regions remain unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            first_read_deny_start, first_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(returned_load_fault_vaddr);
        TEST_ASSERT("after the repaired no-H same-page same-width different-template boundary store fault has already been followed by a successful next-boundary store, the first returned cross-producer load fault must refresh cause and tval back to the original-boundary load template address before the same-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(returned_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the first returned cross-producer load fault preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the same-boundary-valid-different-template-store probe",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_configure_pmp_window(
            second_read_deny_start, second_read_deny_end,
            AI_MICRO_FOLLOWUP_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_followup_translated_ld(same_boundary_load_fault_vaddr);
        TEST_ASSERT("after one returned cross-producer load fault has already been recorded, the same-boundary cross-producer load fault on the next boundary must overwrite cause and tval with the latest producer and latest same-boundary template-specific address before the same-boundary-valid-different-template-store probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_followup_cross_16b_second_half_vaddr(same_boundary_load_fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the second same-boundary cross-producer load fault still preserves the already-updated next-boundary overlay while keeping the original faulting boundary and all guard regions unchanged before the final same-boundary valid different-template store",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_success_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_followup_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_followup_translated_sd(same_boundary_load_fault_vaddr,
            final_store_values[second]);
        TEST_ASSERT("after the returned-then-same-boundary cross-producer load faults are complete and access is restored, the next same-boundary valid different-template store must clear stale exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-page same-width different-template repair-clear then latest-same-boundary-fault then same-boundary-valid-different-template-store probe updates only the current same-boundary template bytes, preserves the original boundary image, and keeps all guard regions unchanged",
            ai_micro_followup_bytes_match(boundary_phys[first],
                init_patterns[first][round], 16U) &&
            ai_micro_followup_bytes_match(boundary_phys[second],
                expected_final_same_boundary, 16U) &&
            ai_micro_followup_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_followup_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_followup_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_followup_reset_env();

    TEST_END();
}
