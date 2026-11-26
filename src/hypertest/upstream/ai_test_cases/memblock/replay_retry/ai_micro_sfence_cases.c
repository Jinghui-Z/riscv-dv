#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_SFENCE_ASID 0x1234ULL

static inline pte_t *ai_micro_sfence_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_sfence_prepare_env(uint64_t asid)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    hspt_init();

    uint64_t satp_value = CSRR(CSR_SATP);
    satp_value &= ~(0xffffULL << 44);
    satp_value |= ((asid & 0xffffULL) << 44);
    CSRW(CSR_SATP, satp_value);
    sfence_vma();
}

static inline void ai_micro_sfence_write_backing(enum test_page page, uint64_t value)
{
    goto_priv(PRIV_M);
    write64(phys_page_base(page), value);
}

static inline void ai_micro_sfence_set_global(enum test_page page, bool enabled)
{
    goto_priv(PRIV_M);
    if (enabled) {
        *ai_micro_sfence_leaf_entry(page) |= PTE_G;
    } else {
        *ai_micro_sfence_leaf_entry(page) &= ~PTE_G;
    }
}

static inline void ai_micro_sfence_clear_read(enum test_page page)
{
    goto_priv(PRIV_M);
    *ai_micro_sfence_leaf_entry(page) &= ~PTE_R;
}

bool ai_micro_hs_sfence_vma_store_preserves_data_before_flush()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);
    const uintptr_t paddr = phys_page_base(page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t stored_value = 0xa1b2c3d4e5f60718ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, false);
    ai_micro_sfence_write_backing(page, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the translated HS page is readable before probing the store-plus-sfence interaction",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the older HS store completes normally before the sfence.vma probe",
        true
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs(vaddr, AI_MICRO_SFENCE_ASID);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,asid itself retires without surfacing an exception after the older store",
        true
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after sfence.vma flushes the cached translation, the updated non-readable leaf reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older store payload is still visible in backing memory after sfence.vma passively flushes the pending store path",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_all_flushes_all_non_global()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, 0x1122334455667788ULL);
    ai_micro_sfence_write_backing(second_page, 0x8877665544332211ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the all-entry sfence probe",
        first_before == 0x1122334455667788ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the all-entry sfence probe",
        second_before == 0x8877665544332211ULL
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_stale = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without sfence the cached translation for the first page remains usable",
        first_stale == 0x1122334455667788ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_stale = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without sfence the cached translation for the second page remains usable",
        second_stale == 0x8877665544332211ULL
    );

    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("sfence.vma x0,x0 flushes the first cached non-global translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(second_vaddr);
    TEST_ASSERT("sfence.vma x0,x0 flushes the second cached non-global translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_all_repeated_flush_stable_corner()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);
    const uint64_t first_value = 0x1122334455667788ULL;
    const uint64_t second_value = 0x8877665544332211ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, first_value);
    ai_micro_sfence_write_backing(second_page, second_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the repeated all-entry sfence corner",
        first_before == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the repeated all-entry sfence corner",
        second_before == second_value
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("repeating sfence.vma x0,x0 keeps the first cached non-global translation flushed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(second_vaddr);
    TEST_ASSERT("repeating sfence.vma x0,x0 keeps the second cached non-global translation flushed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_asid_flushes_matching_non_global()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, false);
    ai_micro_sfence_write_backing(page, 0x1020304050607080ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t before = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the non-global page is readable before the ASID-targeted sfence probe",
        before == 0x1020304050607080ULL
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t stale = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without sfence the cached translation remains usable for the matching ASID",
        stale == 0x1020304050607080ULL
    );

    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("sfence.vma x0,asid flushes cached non-global translations for the matching ASID",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_asid_repeated_flush_non_global_but_preserves_global_corner()
{
    TEST_START();

    const enum test_page nonglobal_page = VSRWX_GRWX;
    const enum test_page global_page = VSRW_GRWX;
    const uintptr_t nonglobal_vaddr = hs_page_base(nonglobal_page);
    const uintptr_t global_vaddr = hs_page_base(global_page);
    const uint64_t nonglobal_value = 0x1020304050607080ULL;
    const uint64_t global_value = 0xa1b2c3d4e5f60718ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(nonglobal_page, false);
    ai_micro_sfence_set_global(global_page, true);
    ai_micro_sfence_write_backing(nonglobal_page, nonglobal_value);
    ai_micro_sfence_write_backing(global_page, global_value);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t nonglobal_before = ld(nonglobal_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the non-global page is readable before the repeated ASID-targeted mixed global/non-global sfence corner",
        nonglobal_before == nonglobal_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t global_before = ld(global_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the global page is readable before the repeated ASID-targeted mixed global/non-global sfence corner",
        global_before == global_value
    );

    ai_micro_sfence_clear_read(nonglobal_page);
    ai_micro_sfence_clear_read(global_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);
    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(nonglobal_vaddr);
    TEST_ASSERT("repeating sfence.vma x0,asid keeps the matching cached non-global translation flushed in the mixed global/non-global corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == nonglobal_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t global_after = ld(global_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating sfence.vma x0,asid still preserves the cached global translation in the mixed global/non-global corner",
        global_after == global_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_asid_other_asid_preserves_current_non_global_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);
    const uint64_t value = 0x1020304050607080ULL;
    const uint64_t other_asid = AI_MICRO_SFENCE_ASID + 1ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, false);
    ai_micro_sfence_write_backing(page, value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t before = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the non-global page is readable before the mismatched-ASID sfence probe",
        before == value
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    sfence_vma_rs((uintptr_t)-1, other_asid);

    TEST_SETUP_EXCEPT();
    uint64_t after_wrong_asid = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma x0,other-asid leaves the current ASID's cached non-global translation untouched",
        after_wrong_asid == value
    );

    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the matching ASID is finally targeted, the same cached non-global translation is flushed and reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_asid_flushes_non_global_but_preserves_global_corner()
{
    TEST_START();

    const enum test_page nonglobal_page = VSRWX_GRWX;
    const enum test_page global_page = VSRW_GRWX;
    const uintptr_t nonglobal_vaddr = hs_page_base(nonglobal_page);
    const uintptr_t global_vaddr = hs_page_base(global_page);
    const uint64_t nonglobal_value = 0x1020304050607080ULL;
    const uint64_t global_value = 0xa1b2c3d4e5f60718ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(nonglobal_page, false);
    ai_micro_sfence_set_global(global_page, true);
    ai_micro_sfence_write_backing(nonglobal_page, nonglobal_value);
    ai_micro_sfence_write_backing(global_page, global_value);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t nonglobal_before = ld(nonglobal_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the non-global page is readable before the mixed global/non-global ASID-targeted sfence probe",
        nonglobal_before == nonglobal_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t global_before = ld(global_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the global page is readable before the mixed global/non-global ASID-targeted sfence probe",
        global_before == global_value
    );

    ai_micro_sfence_clear_read(nonglobal_page);
    ai_micro_sfence_clear_read(global_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(nonglobal_vaddr);
    TEST_ASSERT("sfence.vma x0,asid flushes the matching cached non-global translation in the mixed global/non-global corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == nonglobal_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t global_after = ld(global_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma x0,asid still preserves the cached global translation in the mixed global/non-global corner",
        global_after == global_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_rs2_scope_mixed_global_non_global_corner()
{
    TEST_START();

    const enum test_page nonglobal_page = VSRWX_GRWX;
    const enum test_page global_page = VSRW_GRWX;
    const uintptr_t nonglobal_vaddr = hs_page_base(nonglobal_page);
    const uintptr_t global_vaddr = hs_page_base(global_page);
    const uint64_t nonglobal_value = 0x1029384756abcdefULL;
    const uint64_t global_value = 0xfedcba6547382910ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(nonglobal_page, false);
    ai_micro_sfence_set_global(global_page, true);
    ai_micro_sfence_write_backing(nonglobal_page, nonglobal_value);
    ai_micro_sfence_write_backing(global_page, global_value);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t nonglobal_before = ld(nonglobal_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the non-global page is readable before the mixed rs2-scope sfence probe",
        nonglobal_before == nonglobal_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t global_before = ld(global_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the global page is readable before the mixed rs2-scope sfence probe",
        global_before == global_value
    );

    ai_micro_sfence_clear_read(nonglobal_page);
    ai_micro_sfence_clear_read(global_page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma x0,asid retires while targeting only non-global entries in the mixed rs2-scope probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ld(nonglobal_vaddr);
    TEST_ASSERT("sfence.vma x0,asid flushes the matching cached non-global translation in the mixed rs2-scope probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == nonglobal_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t global_after_asid = ld(global_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma x0,asid preserves the cached global translation in the mixed rs2-scope probe",
        global_after_asid == global_value
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma x0,x0 retires before flushing the preserved global entry in the mixed rs2-scope probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ld(global_vaddr);
    TEST_ASSERT("sfence.vma x0,x0 flushes the previously preserved cached global translation in the mixed rs2-scope probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == global_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_asid_preserves_global_translation()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, true);
    ai_micro_sfence_write_backing(page, 0xa1b2c3d4e5f60718ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t before = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the global page is readable before the ASID-targeted sfence preservation probe",
        before == 0xa1b2c3d4e5f60718ULL
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t stale = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("without sfence the cached global translation remains usable",
        stale == 0xa1b2c3d4e5f60718ULL
    );

    sfence_vma_rs((uintptr_t)-1, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    uint64_t after = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma x0,asid preserves cached global translations",
        after == 0xa1b2c3d4e5f60718ULL
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_vaddr_all_asids_flushes_exact_non_global()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, 0x1111222233334444ULL);
    ai_micro_sfence_write_backing(second_page, 0x5555666677778888ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the vaddr-only sfence probe",
        first_before == 0x1111222233334444ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the vaddr-only sfence probe",
        second_before == 0x5555666677778888ULL
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(first_vaddr, (uint64_t)-1);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("sfence.vma vaddr,x0 flushes the matching non-global translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,x0 leaves other cached translations untouched",
        second_after == 0x5555666677778888ULL
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_vaddr_all_asids_repeated_flush_stable_corner()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);
    const uint64_t first_value = 0x1111222233334444ULL;
    const uint64_t second_value = 0x5555666677778888ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, first_value);
    ai_micro_sfence_write_backing(second_page, second_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the repeated vaddr-only sfence stability corner",
        first_before == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the repeated vaddr-only sfence stability corner",
        second_before == second_value
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(first_vaddr, (uint64_t)-1);
    sfence_vma_rs(first_vaddr, (uint64_t)-1);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("repeating sfence.vma vaddr,x0 keeps the targeted cached non-global translation flushed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating sfence.vma vaddr,x0 still leaves the other cached non-global translation untouched",
        second_after == second_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_other_vaddr_all_asids_preserves_current_non_global_corner()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);
    const uint64_t first_value = 0x1111222233334444ULL;
    const uint64_t second_value = 0x5555666677778888ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, first_value);
    ai_micro_sfence_write_backing(second_page, second_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the wrong-vaddr all-ASIDs sfence probe",
        first_before == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the wrong-vaddr all-ASIDs sfence probe",
        second_before == second_value
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(second_vaddr, (uint64_t)-1);

    TEST_SETUP_EXCEPT();
    uint64_t first_after_wrong_vaddr = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma other-vaddr,x0 leaves the current page's cached non-global translation untouched",
        first_after_wrong_vaddr == first_value
    );

    TEST_SETUP_EXCEPT();
    ld(second_vaddr);
    TEST_ASSERT("sfence.vma other-vaddr,x0 still flushes the specifically targeted cached non-global translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_vaddr
    );

    sfence_vma_rs(first_vaddr, (uint64_t)-1);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("once the exact current vaddr is finally targeted, the previously preserved cached non-global translation is flushed and reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_vaddr_asid_flushes_exact_non_global()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, 0x0123456789abcdefULL);
    ai_micro_sfence_write_backing(second_page, 0xfedcba9876543210ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the vaddr+asid sfence probe",
        first_before == 0x0123456789abcdefULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the vaddr+asid sfence probe",
        second_before == 0xfedcba9876543210ULL
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(first_vaddr, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("sfence.vma vaddr,asid flushes the matching non-global translation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,asid leaves other cached translations untouched",
        second_after == 0xfedcba9876543210ULL
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_other_vaddr_asid_preserves_current_non_global_corner()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);
    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0xfedcba9876543210ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, first_value);
    ai_micro_sfence_write_backing(second_page, second_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the wrong-vaddr matching-ASID sfence probe",
        first_before == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the wrong-vaddr matching-ASID sfence probe",
        second_before == second_value
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(second_vaddr, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    uint64_t first_after_wrong_vaddr = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma other-vaddr,asid leaves the current page's cached non-global translation untouched",
        first_after_wrong_vaddr == first_value
    );

    TEST_SETUP_EXCEPT();
    ld(second_vaddr);
    TEST_ASSERT("sfence.vma other-vaddr,asid still flushes the specifically targeted cached non-global translation for the matching ASID",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_vaddr
    );

    sfence_vma_rs(first_vaddr, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("once the exact current vaddr is finally targeted for the matching ASID, the previously preserved cached non-global translation is flushed and reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_vaddr_other_asid_preserves_current_non_global_corner()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);
    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0xfedcba9876543210ULL;
    const uint64_t other_asid = AI_MICRO_SFENCE_ASID + 1ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, first_value);
    ai_micro_sfence_write_backing(second_page, second_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the vaddr+other-asid sfence probe",
        first_before == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the vaddr+other-asid sfence probe",
        second_before == second_value
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(first_vaddr, other_asid);

    TEST_SETUP_EXCEPT();
    uint64_t first_after_wrong_asid = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,other-asid leaves the current ASID's cached targeted translation untouched",
        first_after_wrong_asid == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after_wrong_asid = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,other-asid also leaves the untargeted cached translation untouched",
        second_after_wrong_asid == second_value
    );

    sfence_vma_rs(first_vaddr, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("once the matching ASID is targeted for the same vaddr, the cached translation is flushed and reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after_matching_flush = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("flushing the matching ASID for one vaddr still leaves the other cached translation untouched",
        second_after_matching_flush == second_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_vaddr_asid_repeated_flush_stable_corner()
{
    TEST_START();

    const enum test_page first_page = VSRWX_GRWX;
    const enum test_page second_page = VSRW_GRWX;
    const uintptr_t first_vaddr = hs_page_base(first_page);
    const uintptr_t second_vaddr = hs_page_base(second_page);
    const uint64_t first_value = 0x0123456789abcdefULL;
    const uint64_t second_value = 0xfedcba9876543210ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(first_page, false);
    ai_micro_sfence_set_global(second_page, false);
    ai_micro_sfence_write_backing(first_page, first_value);
    ai_micro_sfence_write_backing(second_page, second_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t first_before = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first non-global page is readable before the repeated vaddr+asid sfence stability corner",
        first_before == first_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_before = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second non-global page is readable before the repeated vaddr+asid sfence stability corner",
        second_before == second_value
    );

    ai_micro_sfence_clear_read(first_page);
    ai_micro_sfence_clear_read(second_page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(first_vaddr, AI_MICRO_SFENCE_ASID);
    sfence_vma_rs(first_vaddr, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    ld(first_vaddr);
    TEST_ASSERT("repeating sfence.vma vaddr,asid keeps the targeted cached non-global translation flushed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_after = ld(second_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating sfence.vma vaddr,asid still leaves the other cached translation untouched",
        second_after == second_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_vaddr_asid_preserves_global_translation()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, true);
    ai_micro_sfence_write_backing(page, 0x0fedcba987654321ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t before = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the global page is readable before the vaddr+asid preservation probe",
        before == 0x0fedcba987654321ULL
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    sfence_vma_rs(vaddr, AI_MICRO_SFENCE_ASID);

    TEST_SETUP_EXCEPT();
    uint64_t after = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("sfence.vma vaddr,asid preserves cached global translations",
        after == 0x0fedcba987654321ULL
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_store_preserves_data_across_repeated_flush_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);
    const uintptr_t paddr = phys_page_base(page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t stored_value = 0xa1b2c3d4e5f60718ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, false);
    ai_micro_sfence_write_backing(page, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the translated HS page is readable before probing the repeated sfence-plus-store interaction",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the older HS store completes normally before the repeated sfence.vma probe",
        true
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs(vaddr, AI_MICRO_SFENCE_ASID);
    AI_ASSERT_NO_EXCEPT_AND("the first sfence.vma vaddr,asid itself retires without exception after the older store in the repeated-flush corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs(vaddr, AI_MICRO_SFENCE_ASID);
    AI_ASSERT_NO_EXCEPT_AND("the second sfence.vma vaddr,asid also retires without exception in the repeated-flush corner",
        true
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after repeated sfence.vma flushes the cached translation, the updated non-readable leaf still reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older store payload is still visible in backing memory after repeated sfence.vma flushes",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_sfence_vma_all_store_preserves_data_across_repeated_flush_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(page);
    const uintptr_t paddr = phys_page_base(page);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t stored_value = 0xa1b2c3d4e5f60718ULL;

    ai_micro_sfence_prepare_env(AI_MICRO_SFENCE_ASID);
    ai_micro_sfence_set_global(page, false);
    ai_micro_sfence_write_backing(page, initial_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the translated HS page is readable before probing the repeated all-entry sfence-plus-store interaction",
        warmed == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the older HS store completes normally before the repeated all-entry sfence corner",
        true
    );

    ai_micro_sfence_clear_read(page);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_ASSERT_NO_EXCEPT_AND("the first sfence.vma x0,x0 retires without exception after the older store in the repeated all-entry corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sfence_vma_rs((uintptr_t)-1, (uint64_t)-1);
    AI_ASSERT_NO_EXCEPT_AND("the second sfence.vma x0,x0 also retires without exception in the repeated all-entry corner",
        true
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after repeated sfence.vma x0,x0 flushes the cached translation, the updated non-readable leaf still reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older store payload is still visible in backing memory after repeated sfence.vma x0,x0 flushes",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_sfence_vma_privilege_interaction_illegal()
{
    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("HU mode execution of sfence.vma raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("HS mode execution of sfence.vma raises illegal instruction when mstatus.TVM=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sfence_vma();
    AI_ASSERT_NO_EXCEPT_AND("clearing mstatus.TVM restores legal HS execution of sfence.vma",
        true
    );

    TEST_END();
}
