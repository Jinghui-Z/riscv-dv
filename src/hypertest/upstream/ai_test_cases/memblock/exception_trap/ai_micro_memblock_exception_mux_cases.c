#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_MEB_VSEG_FIELDS 2U
#define AI_MEMBLOCK_MEB_VSEG_VL 8U
#define AI_MEMBLOCK_MEB_VSEG_BYTES \
    (AI_MEMBLOCK_MEB_VSEG_FIELDS * AI_MEMBLOCK_MEB_VSEG_VL)
#define AI_MEMBLOCK_MEB_CBO_OFFSET 0x80U
#define AI_MEMBLOCK_MEB_CBO_WORDS 8U

static inline pte_t *ai_memblock_meb_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_meb_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_meb_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_meb_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_meb_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_meb_expected_cross_page(
    uint8_t *expected, const uint8_t *low, const uint8_t *high)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_MEB_VSEG_VL; ++i) {
        expected[i] = low[i];
        expected[AI_MEMBLOCK_MEB_VSEG_VL + i] = high[i];
    }
}

static inline void ai_memblock_meb_seed_cbo_block(uintptr_t paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_MEB_CBO_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            seed + (i * 0x0101010101010101ULL));
    }
}

static inline bool ai_memblock_meb_cbo_block_matches(uintptr_t paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_MEB_CBO_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            seed + (i * 0x0101010101010101ULL)) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_meb_vlseg2e8_to_v8(const uint8_t *src)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"((int)AI_MEMBLOCK_MEB_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_meb_vsseg2e8_from_v8(uint8_t *dst)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        ".option pop\n\t"
        : [dst] "+r"(dst)
        : [vl] "r"((int)AI_MEMBLOCK_MEB_VSEG_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_mebsingleport_amo_vsegment_fault_switch_no_stale_exception_corner()
{
    TEST_START();

    const enum test_page amo_page = VSRWX_GRWX;
    const enum test_page vseg_low_page = SWITCH1;
    const enum test_page vseg_high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x180U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x180U;
    const uintptr_t amo_guard_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t vseg_src_vaddr =
        hs_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_low_tail_paddr =
        phys_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_high_head_paddr = phys_page_base(vseg_high_page);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x280U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x280U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_MEB_VSEG_BYTES;
    const uint64_t amo_seeds[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
    };
    const uint64_t amo_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
    };
    const uint8_t low_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
    };
    const uint8_t high_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
        { 0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7 },
    };
    const uint8_t dst_fill[] = { 0x3eU, 0x4dU };
    const uint8_t dst_before[] = { 0x6aU, 0x7bU };
    const uint8_t dst_after[] = { 0xa6U, 0xb7U };

    for (unsigned round = 0; round < 2U; ++round) {
        uint8_t expected[AI_MEMBLOCK_MEB_VSEG_BYTES];
        const bool amo_first = (round == 0U);
        const uint64_t expected_amo_final =
            amo_seeds[round] + amo_addends[round];

        ai_memblock_meb_expected_cross_page(expected, low_bytes[round],
            high_bytes[round]);

        goto_priv(PRIV_M);
        write64(amo_paddr, amo_seeds[round]);
        write64(amo_guard_paddr, amo_guards[round]);
        ai_memblock_meb_write_bytes(vseg_low_tail_paddr, low_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        ai_memblock_meb_write_bytes(vseg_high_head_paddr, high_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        for (unsigned i = 0; i < AI_MEMBLOCK_MEB_VSEG_BYTES; ++i) {
            write8(dst_paddr + i, dst_fill[round]);
        }
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_meb_set_hs_leaf(amo_page, PTE_V | PTE_AD | PTE_R);
        ai_memblock_meb_set_hs_leaf(vseg_low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        if (amo_first) {
            TEST_SETUP_EXCEPT();
            (void)amoadd_d(amo_vaddr, amo_addends[round]);
            TEST_ASSERT("the first MEBSinglePort producer-switch probe reports the AMO store page fault with tval bound to the AMO address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == amo_vaddr
            );

            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("after the earlier AMO fault occupied MEBSinglePort, the follow-up VSegment load fault still reports a load page fault instead of stale AMO metadata",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF
            );
        } else {
            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("the first MEBSinglePort producer-switch probe reports the VSegment load page fault",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF
            );

            TEST_SETUP_EXCEPT();
            (void)amoadd_d(amo_vaddr, amo_addends[round]);
            TEST_ASSERT("after the earlier VSegment fault occupied MEBSinglePort, the follow-up AMO fault refreshes cause and tval to the newer AMO-side address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == amo_vaddr
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("AMO and VSegment fault episodes leave target memory, destination sentinels, and guards unchanged before repair",
            read64(amo_paddr) == amo_seeds[round] &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_meb_set_hs_leaf(amo_page, PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t amo_old = amoadd_d(amo_vaddr, amo_addends[round]);
        ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
        ai_memblock_meb_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after repairing both MEBSinglePort producers, AMO and VSegment complete normally without stale exception state",
            amo_old == amo_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("repaired AMO and VSegment producers keep their own payloads with no stale single-port exception metadata or adjacent corruption",
            read64(amo_paddr) == expected_amo_final &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            ai_memblock_meb_bytes_match(dst_paddr, expected,
                AI_MEMBLOCK_MEB_VSEG_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_mebsingleport_cbo_vsegment_fault_switch_no_stale_cause_tval_corner()
{
    TEST_START();

    const enum test_page cbo_page = VSRWX_GRWX;
    const enum test_page vseg_low_page = SWITCH1;
    const enum test_page vseg_high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t cbo_vaddr =
        hs_page_base(cbo_page) + AI_MEMBLOCK_MEB_CBO_OFFSET;
    const uintptr_t cbo_paddr =
        phys_page_base(cbo_page) + AI_MEMBLOCK_MEB_CBO_OFFSET;
    const uintptr_t vseg_src_vaddr =
        hs_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_fault_vaddr = hs_page_base(vseg_high_page);
    const uintptr_t vseg_low_tail_paddr =
        phys_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_high_head_paddr = phys_page_base(vseg_high_page);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x3c0U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x3c0U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_MEB_VSEG_BYTES;
    const uint64_t cbo_seeds[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
    };
    const uint8_t low_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98 },
        { 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90 },
    };
    const uint8_t high_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f, 0x10 },
        { 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18 },
    };
    const uint8_t dst_fill[] = { 0x4eU, 0x5fU };
    const uint8_t dst_before[] = { 0x6cU, 0x7dU };
    const uint8_t dst_after[] = { 0xc6U, 0xd7U };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);

    for (unsigned round = 0; round < 2U; ++round) {
        uint8_t expected[AI_MEMBLOCK_MEB_VSEG_BYTES];
        const bool cbo_first = (round == 0U);

        ai_memblock_meb_expected_cross_page(expected, low_bytes[round],
            high_bytes[round]);

        goto_priv(PRIV_M);
        ai_memblock_meb_seed_cbo_block(cbo_paddr, cbo_seeds[round]);
        ai_memblock_meb_write_bytes(vseg_low_tail_paddr, low_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        ai_memblock_meb_write_bytes(vseg_high_head_paddr, high_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        for (unsigned i = 0; i < AI_MEMBLOCK_MEB_VSEG_BYTES; ++i) {
            write8(dst_paddr + i, dst_fill[round]);
        }
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_meb_set_hs_leaf(cbo_page, PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(vseg_low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        if (cbo_first) {
            TEST_SETUP_EXCEPT();
            cbo_clean(cbo_vaddr);
            TEST_ASSERT("the first MEBSinglePort CBO producer-switch probe reports the CBO store page fault with tval bound to the CBO address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == cbo_vaddr
            );

            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("after the earlier CBO fault occupied the LSQ single-step path, the follow-up VSegment fault refreshes to the segment load page fault",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == vseg_fault_vaddr
            );
        } else {
            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("the first MEBSinglePort producer-switch probe reports the VSegment load page fault before the CBO fault",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == vseg_fault_vaddr
            );

            TEST_SETUP_EXCEPT();
            cbo_flush(cbo_vaddr);
            TEST_ASSERT("after the earlier VSegment fault occupied MEBSinglePort, the follow-up CBO fault refreshes cause and tval to the CBO address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == cbo_vaddr
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("CBO and VSegment fault episodes leave CBO block, destination sentinels, and segment source bytes unchanged before repair",
            ai_memblock_meb_cbo_block_matches(cbo_paddr, cbo_seeds[round]) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_meb_set_hs_leaf(cbo_page, PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        if (cbo_first) {
            cbo_clean(cbo_vaddr);
        } else {
            cbo_flush(cbo_vaddr);
        }
        ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
        ai_memblock_meb_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after repairing both single-step producers, CBO and VSegment complete without stale MEBSinglePort metadata",
            true
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("repaired CBO and VSegment producers keep their own payloads with no stale single-port cause/tval or adjacent corruption",
            ai_memblock_meb_cbo_block_matches(cbo_paddr, cbo_seeds[round]) &&
            ai_memblock_meb_bytes_match(dst_paddr, expected,
                AI_MEMBLOCK_MEB_VSEG_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_mebsingleport_cbo_amo_fault_switch_no_stale_cause_tval_corner()
{
    TEST_START();

    const enum test_page cbo_page = SWITCH1;
    const enum test_page amo_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t cbo_vaddr =
        hs_page_base(cbo_page) + AI_MEMBLOCK_MEB_CBO_OFFSET;
    const uintptr_t cbo_paddr =
        phys_page_base(cbo_page) + AI_MEMBLOCK_MEB_CBO_OFFSET;
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0x240U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0x240U;
    const uintptr_t amo_guard_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x340U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x340U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t cbo_seeds[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t amo_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
    };
    const uint64_t amo_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1020304050607080ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);

    for (unsigned round = 0; round < 2U; ++round) {
        const bool cbo_first = (round == 0U);
        const uint64_t expected_amo_final =
            amo_seeds[round] + amo_addends[round];

        goto_priv(PRIV_M);
        ai_memblock_meb_seed_cbo_block(cbo_paddr, cbo_seeds[round]);
        write64(amo_paddr, amo_seeds[round]);
        write64(amo_guard_paddr, amo_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_meb_set_hs_leaf(cbo_page, PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(amo_page, PTE_V | PTE_AD | PTE_R);
        ai_memblock_meb_set_hs_leaf(marker_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        if (cbo_first) {
            TEST_SETUP_EXCEPT();
            cbo_clean(cbo_vaddr);
            TEST_ASSERT("the first CBO/AMO MEBSinglePort probe reports the CBO store page fault with tval bound to the CBO address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == cbo_vaddr
            );

            TEST_SETUP_EXCEPT();
            (void)amoadd_d(amo_vaddr, amo_addends[round]);
            TEST_ASSERT("after the earlier CBO fault occupied the LSQ single-step path, the follow-up AMO fault refreshes to the AMO store page fault",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == amo_vaddr
            );
        } else {
            TEST_SETUP_EXCEPT();
            (void)amoadd_d(amo_vaddr, amo_addends[round]);
            TEST_ASSERT("the first CBO/AMO MEBSinglePort probe reports the AMO store page fault before the CBO fault",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == amo_vaddr
            );

            TEST_SETUP_EXCEPT();
            cbo_flush(cbo_vaddr);
            TEST_ASSERT("after the earlier AMO fault occupied MEBSinglePort, the follow-up CBO fault refreshes tval to the CBO address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == cbo_vaddr
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("CBO and AMO fault episodes leave CBO block, AMO target, marker, and guards unchanged before repair",
            ai_memblock_meb_cbo_block_matches(cbo_paddr, cbo_seeds[round]) &&
            read64(amo_paddr) == amo_seeds[round] &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            read64(marker_paddr) == marker_seeds[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_meb_set_hs_leaf(cbo_page, PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(amo_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        if (cbo_first) {
            cbo_clean(cbo_vaddr);
        } else {
            cbo_flush(cbo_vaddr);
        }
        const uint64_t amo_old = amoadd_d(amo_vaddr, amo_addends[round]);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("after repairing both CBO/AMO single-step producers, CBO, AMO, and marker store complete without stale MEBSinglePort metadata",
            amo_old == amo_seeds[round] &&
            ld(amo_vaddr) == expected_amo_final &&
            ld(marker_vaddr) == marker_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("repaired CBO and AMO producers keep their own payloads with no stale single-port cause/tval or adjacent corruption",
            ai_memblock_meb_cbo_block_matches(cbo_paddr, cbo_seeds[round]) &&
            read64(amo_paddr) == expected_amo_final &&
            read64(amo_guard_paddr) == amo_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_meb_loadport_vsegment_fault_switch_no_stale_tval_corner()
{
    TEST_START();

    const enum test_page scalar_page = VSRWX_GRWX;
    const enum test_page vseg_low_page = SWITCH1;
    const enum test_page vseg_high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t scalar_vaddr = hs_page_base(scalar_page) + 0x1c0U;
    const uintptr_t scalar_paddr = phys_page_base(scalar_page) + 0x1c0U;
    const uintptr_t scalar_guard_paddr = scalar_paddr + sizeof(uint64_t);
    const uintptr_t vseg_src_vaddr =
        hs_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_fault_vaddr = hs_page_base(vseg_high_page);
    const uintptr_t vseg_low_tail_paddr =
        phys_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_high_head_paddr = phys_page_base(vseg_high_page);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x300U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x300U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_MEB_VSEG_BYTES;
    const uint64_t scalar_seeds[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
    };
    const uint64_t scalar_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
    };
    const uint8_t low_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
    };
    const uint8_t high_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
        { 0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7 },
    };
    const uint8_t dst_fill[] = { 0x3eU, 0x4dU };
    const uint8_t dst_before[] = { 0x6aU, 0x7bU };
    const uint8_t dst_after[] = { 0xa6U, 0xb7U };

    for (unsigned round = 0; round < 2U; ++round) {
        uint8_t expected[AI_MEMBLOCK_MEB_VSEG_BYTES];
        const bool scalar_first = (round == 0U);

        ai_memblock_meb_expected_cross_page(expected, low_bytes[round],
            high_bytes[round]);

        goto_priv(PRIV_M);
        write64(scalar_paddr, scalar_seeds[round]);
        write64(scalar_guard_paddr, scalar_guards[round]);
        ai_memblock_meb_write_bytes(vseg_low_tail_paddr, low_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        ai_memblock_meb_write_bytes(vseg_high_head_paddr, high_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        for (unsigned i = 0; i < AI_MEMBLOCK_MEB_VSEG_BYTES; ++i) {
            write8(dst_paddr + i, dst_fill[round]);
        }
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_meb_set_hs_leaf(scalar_page, PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(vseg_low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        if (scalar_first) {
            TEST_SETUP_EXCEPT();
            (void)ld(scalar_vaddr);
            TEST_ASSERT("the first MEBLoadPort producer-switch probe reports the scalar load page fault with tval bound to the scalar address",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == scalar_vaddr
            );

            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("after the earlier scalar load fault occupied MEBLoadPort, the follow-up VSegment single-port fault refreshes to the segment high-page tval",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == vseg_fault_vaddr
            );
        } else {
            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("the first MEBSinglePort producer-switch probe reports the VSegment load page fault with tval bound to the segment high page",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == vseg_fault_vaddr
            );

            TEST_SETUP_EXCEPT();
            (void)ld(scalar_vaddr);
            TEST_ASSERT("after the earlier VSegment fault occupied MEBSinglePort, the follow-up scalar load fault refreshes tval to the scalar load address",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == scalar_vaddr
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("scalar-load and VSegment fault-switch episodes leave source, destination sentinels, and guards unchanged before repair",
            read64(scalar_paddr) == scalar_seeds[round] &&
            read64(scalar_guard_paddr) == scalar_guards[round] &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_meb_set_hs_leaf(scalar_page, PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t scalar_observed = ld(scalar_vaddr);
        ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
        ai_memblock_meb_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after repairing scalar and VSegment fault pages, both producer paths complete without stale load-port or single-port exception state",
            scalar_observed == scalar_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("repaired scalar load and VSegment paths keep their own payloads with no stale cross-port tval or adjacent corruption",
            read64(scalar_paddr) == scalar_seeds[round] &&
            read64(scalar_guard_paddr) == scalar_guards[round] &&
            ai_memblock_meb_bytes_match(dst_paddr, expected,
                AI_MEMBLOCK_MEB_VSEG_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_meb_storeport_vsegment_fault_switch_no_stale_cause_tval_corner()
{
    TEST_START();

    const enum test_page store_page = VSRWX_GRWX;
    const enum test_page vseg_low_page = SWITCH1;
    const enum test_page vseg_high_page = SWITCH2;
    const enum test_page dst_page = SCRATCHPAD;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x240U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x240U;
    const uintptr_t store_guard_paddr = store_paddr + sizeof(uint64_t);
    const uintptr_t vseg_src_vaddr =
        hs_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_fault_vaddr = hs_page_base(vseg_high_page);
    const uintptr_t vseg_low_tail_paddr =
        phys_page_base(vseg_low_page) + PAGE_SIZE - AI_MEMBLOCK_MEB_VSEG_VL;
    const uintptr_t vseg_high_head_paddr = phys_page_base(vseg_high_page);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x340U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x340U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_MEB_VSEG_BYTES;
    const uint64_t store_seeds[] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
    };
    const uint64_t store_updates[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t store_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadbeeff00dbabeULL,
    };
    const uint8_t low_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
        { 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f, 0x80 },
    };
    const uint8_t high_bytes[2][AI_MEMBLOCK_MEB_VSEG_VL] = {
        { 0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0 },
        { 0x81, 0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8 },
    };
    const uint8_t dst_fill[] = { 0x5cU, 0x6dU };
    const uint8_t dst_before[] = { 0x7aU, 0x8bU };
    const uint8_t dst_after[] = { 0xb6U, 0xc7U };

    for (unsigned round = 0; round < 2U; ++round) {
        uint8_t expected[AI_MEMBLOCK_MEB_VSEG_BYTES];
        const bool store_first = (round == 0U);

        ai_memblock_meb_expected_cross_page(expected, low_bytes[round],
            high_bytes[round]);

        goto_priv(PRIV_M);
        write64(store_paddr, store_seeds[round]);
        write64(store_guard_paddr, store_guards[round]);
        ai_memblock_meb_write_bytes(vseg_low_tail_paddr, low_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        ai_memblock_meb_write_bytes(vseg_high_head_paddr, high_bytes[round],
            AI_MEMBLOCK_MEB_VSEG_VL);
        for (unsigned i = 0; i < AI_MEMBLOCK_MEB_VSEG_BYTES; ++i) {
            write8(dst_paddr + i, dst_fill[round]);
        }
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_meb_set_hs_leaf(store_page, PTE_V | PTE_AD | PTE_R);
        ai_memblock_meb_set_hs_leaf(vseg_low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_meb_set_hs_leaf(dst_page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        if (store_first) {
            TEST_SETUP_EXCEPT();
            sd(store_vaddr, store_updates[round]);
            TEST_ASSERT("the first MEBStorePort producer-switch probe reports the scalar store page fault with tval bound to the store address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == store_vaddr
            );

            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("after the earlier scalar store fault occupied MEBStorePort, the follow-up VSegment single-port fault refreshes cause and tval to the segment high page",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == vseg_fault_vaddr
            );
        } else {
            TEST_SETUP_EXCEPT();
            ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
            TEST_ASSERT("the first MEBSinglePort producer-switch probe reports the VSegment load page fault with tval bound to the segment high page",
                excpt.triggered &&
                excpt.cause == CAUSE_LPF &&
                excpt.tval == vseg_fault_vaddr
            );

            TEST_SETUP_EXCEPT();
            sd(store_vaddr, store_updates[round]);
            TEST_ASSERT("after the earlier VSegment fault occupied MEBSinglePort, the follow-up scalar store fault refreshes cause and tval to the store address",
                excpt.triggered &&
                excpt.cause == CAUSE_SPF &&
                excpt.tval == store_vaddr
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("scalar-store and VSegment fault-switch episodes leave source, destination sentinels, and guards unchanged before repair",
            read64(store_paddr) == store_seeds[round] &&
            read64(store_guard_paddr) == store_guards[round] &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_meb_set_hs_leaf(store_page, PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_meb_set_hs_leaf(vseg_high_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sd(store_vaddr, store_updates[round]);
        const uint64_t store_observed = ld(store_vaddr);
        ai_memblock_meb_vlseg2e8_to_v8((const uint8_t *)vseg_src_vaddr);
        ai_memblock_meb_vsseg2e8_from_v8((uint8_t *)dst_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after repairing scalar store and VSegment fault pages, both producer paths complete without stale store-port or single-port exception state",
            store_observed == store_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("repaired scalar store and VSegment paths keep their own payloads with no stale cross-port cause/tval or adjacent corruption",
            read64(store_paddr) == store_updates[round] &&
            read64(store_guard_paddr) == store_guards[round] &&
            ai_memblock_meb_bytes_match(dst_paddr, expected,
                AI_MEMBLOCK_MEB_VSEG_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );
    }

    TEST_END();
}
