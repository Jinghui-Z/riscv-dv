#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_FENCE_I_CODE_PAGE VSRWX_GRWX
#define AI_ARCH_FENCE_I_ALT_PAGE SWITCH1
#define AI_ARCH_FENCE_I_GUARD_BEFORE 0x46U
#define AI_ARCH_FENCE_I_GUARD_AFTER 0x91U

static inline pte_t *ai_arch_fence_i_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_fence_i_set_hs_leaf_to_paddr(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_arch_fence_i_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline void ai_arch_fence_i_sync_hs_leaf(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    ai_arch_fence_i_set_hs_leaf_to_paddr(page, paddr, flags);
    sfence_vma();
}

static inline void ai_arch_fence_i(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".4byte 0x0000100f\n\t"
        ".option pop\n\t"
        :
        :
        : "memory");
}

static inline uint64_t ai_arch_fence_i_ret_imm(uint64_t imm)
{
    return 0x0000806700000513ULL | ((imm & 0xfffULL) << 20);
}

static inline uint64_t ai_arch_fence_i_amoswap_d(uintptr_t addr, uint64_t value)
{
    uint64_t old_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.d %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value)
        : "r"(addr), "r"(value)
        : "memory");
    return old_value;
}

static inline void ai_arch_fence_i_lrsc_d_write(uintptr_t addr, uint64_t value,
    uint64_t *loaded, uint64_t *status)
{
    uint64_t lr_value;
    uint64_t sc_status;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %0, 0(%2)\n\t"
        "sc.d %1, %3, 0(%2)\n\t"
        ".option pop\n\t"
        : "=&r"(lr_value), "=&r"(sc_status)
        : "r"(addr), "r"(value)
        : "memory");
    *loaded = lr_value;
    *status = sc_status;
}

static inline uint64_t ai_arch_fence_i_sc_d_without_reservation(uintptr_t addr,
    uint64_t value)
{
    uint64_t status;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sc.d %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(status)
        : "r"(addr), "r"(value)
        : "memory");
    return status;
}

static inline void ai_arch_fence_i_store_d(uintptr_t addr, uint64_t value)
{
    asm volatile("sd %1, 0(%0)" :: "r"(addr), "r"(value) : "memory");
}

static inline void ai_arch_fence_i_seed_code(uintptr_t paddr, uint64_t insn)
{
    write8(paddr - 1U, AI_ARCH_FENCE_I_GUARD_BEFORE);
    write64(paddr, insn);
    write8(paddr + sizeof(insn), AI_ARCH_FENCE_I_GUARD_AFTER);
}

static inline bool ai_arch_fence_i_code_and_guards_match(uintptr_t paddr,
    uint64_t insn)
{
    return read8(paddr - 1U) == AI_ARCH_FENCE_I_GUARD_BEFORE &&
        read64(paddr) == insn &&
        read8(paddr + sizeof(insn)) == AI_ARCH_FENCE_I_GUARD_AFTER;
}

static inline void ai_arch_fence_i_prepare_hs_code_page(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_arch_fence_i_sync_hs_leaf(page, paddr & PAGE_ADDR_MSK, flags);
    goto_priv(PRIV_HS);
    ai_arch_fence_i();
}

bool ai_arch_fence_i_amo_sc_code_update_visibility_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x180U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t amo_ret7 = ai_arch_fence_i_ret_imm(7);
    const uint64_t sc_ret9 = ai_arch_fence_i_ret_imm(9);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t lr_value = 0;
    uint64_t sc_status = 1;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the warm HS code block returns the original instruction value before atomic code rewrite",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t amo_old = ai_arch_fence_i_amoswap_d(vaddr, amo_ret7);
    TEST_ASSERT("amoswap.d can atomically replace the executable HS instruction doubleword before fence.i",
        excpt.triggered == false &&
        amo_old == old_ret3
    );

    goto_priv(PRIV_HS);
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after AMO code rewrite plus fence.i, the same hart executes the AMO-written instruction image",
        excpt.triggered == false &&
        observed == 7ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_lrsc_d_write(vaddr, sc_ret9, &lr_value, &sc_status);
    TEST_ASSERT("a contiguous lr.d/sc.d pair can successfully rewrite the executable HS instruction doubleword",
        excpt.triggered == false &&
        lr_value == amo_ret7 &&
        sc_status == 0ULL
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after SC-success code rewrite plus fence.i, the same hart executes the SC-written instruction image",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AMO and SC instruction rewrites leave the final backing code image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, sc_ret9)
    );

    TEST_END();
}

bool ai_arch_fence_i_sc_fail_and_store_fault_no_code_update_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x1c0U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t denied_ret7 = ai_arch_fence_i_ret_imm(7);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the warm HS code block returns the original instruction value before failed rewrite probes",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t sc_status = ai_arch_fence_i_sc_d_without_reservation(vaddr,
        denied_ret7);
    TEST_ASSERT("sc.d without a live reservation fails architecturally and does not rewrite the executable code doubleword",
        excpt.triggered == false &&
        sc_status != 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the failed SC leaves the original backing code image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, old_ret3)
    );

    goto_priv(PRIV_HS);
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after a failed SC plus fence.i, the same hart still executes the original instruction image",
        excpt.triggered == false &&
        observed == 3ULL
    );

    ai_arch_fence_i_sync_hs_leaf(page, paddr & PAGE_ADDR_MSK,
        PTE_V | PTE_AD | PTE_RX);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, denied_ret7);
    TEST_ASSERT("a faulting store to an execute-only HS code mapping reports store page fault and cannot rewrite code",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the faulting store plus fence.i, the same hart still executes the original instruction image",
        excpt.triggered == false &&
        observed == 3ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the faulting store leaves the original backing code image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, old_ret3)
    );

    TEST_END();
}

bool ai_arch_fence_i_prefetch_i_old_response_no_override_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x200U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t new_ret7 = ai_arch_fence_i_ret_imm(7);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the warm HS code block returns the original instruction value before prefetch.i",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("prefetch.i to the old instruction image retires silently before the code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, new_ret7);
    TEST_ASSERT("the HS store rewrites the prefetched executable code block before fence.i",
        excpt.triggered == false
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the rewrite plus fence.i, an earlier prefetch.i response cannot override the real fetched instruction image",
        excpt.triggered == false &&
        observed == 7ULL
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("a later prefetch.i to the updated instruction image also retires silently",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the prefetch.i rewrite sequence leaves the final backing code image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, new_ret7)
    );

    TEST_END();
}

bool ai_arch_fence_i_repeated_idempotent_code_visibility_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x240U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t single_ret7 = ai_arch_fence_i_ret_imm(7);
    const uint64_t repeated_ret9 = ai_arch_fence_i_ret_imm(9);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the warm HS code block returns the original instruction value before the idempotence probe",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, single_ret7);
    TEST_ASSERT("the first HS code rewrite before a single fence.i completes without exception",
        excpt.triggered == false
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after a single fence.i, the same hart executes the first rewritten instruction image",
        excpt.triggered == false &&
        observed == 7ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, repeated_ret9);
    TEST_ASSERT("the second HS code rewrite before repeated fence.i completes without exception",
        excpt.triggered == false
    );

    ai_arch_fence_i();
    ai_arch_fence_i();
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("repeating fence.i remains idempotent and exposes the latest rewritten instruction image",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated-fence.i sequence leaves the final backing code image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, repeated_ret9)
    );

    TEST_END();
}

bool ai_arch_fence_i_sfence_vma_translation_pair_visibility_corner()
{
    TEST_START();

    const enum test_page exec_page = AI_ARCH_FENCE_I_CODE_PAGE;
    const enum test_page alt_page = AI_ARCH_FENCE_I_ALT_PAGE;
    const uintptr_t offset = 0x280U;
    const uintptr_t exec_vaddr = hs_page_base(exec_page) + offset;
    const uintptr_t old_paddr = phys_page_base(exec_page) + offset;
    const uintptr_t alt_paddr = phys_page_base(alt_page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t alt_ret7 = ai_arch_fence_i_ret_imm(7);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))exec_vaddr;
    uint64_t observed = 0xbadc0deULL;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(old_paddr, old_ret3);
    ai_arch_fence_i_seed_code(alt_paddr, alt_ret7);
    ai_arch_fence_i_prepare_hs_code_page(exec_page, old_paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the executable HS VA initially fetches from the original translated code page",
        excpt.triggered == false &&
        observed == 3ULL
    );

    ai_arch_fence_i_set_hs_leaf_to_paddr(exec_page, alt_paddr & PAGE_ADDR_MSK,
        PTE_V | PTE_AD | PTE_RW);
    goto_priv(PRIV_HS);

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(exec_vaddr);
    TEST_ASSERT("after changing the PTE but issuing only fence.i, either stale executable translation or eager no-X fault is the only accepted outcome",
        excpt.triggered == false ||
        (excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == exec_vaddr)
    );

    goto_priv(PRIV_M);
    sfence_vma();
    goto_priv(PRIV_HS);
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(exec_vaddr);
    TEST_ASSERT("after sfence.vma plus fence.i, the updated no-X translation is exposed as an instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == exec_vaddr
    );

    ai_arch_fence_i_sync_hs_leaf(exec_page, alt_paddr & PAGE_ADDR_MSK,
        PTE_V | PTE_AD | PTE_RWX);
    goto_priv(PRIV_HS);
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after granting execute permission and issuing sfence.vma plus fence.i, the same HS VA executes the new code image",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the translation plus fence.i pairing keeps both old and new backing code images intact",
        ai_arch_fence_i_code_and_guards_match(old_paddr, old_ret3) &&
        ai_arch_fence_i_code_and_guards_match(alt_paddr, alt_ret7)
    );

    TEST_END();
}

bool ai_arch_fence_i_cbo_clean_flush_code_visibility_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x300U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t clean_flush_ret7 = ai_arch_fence_i_ret_imm(7);
    const uint64_t second_ret9 = ai_arch_fence_i_ret_imm(9);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the warm HS code block returns the original instruction value before CBO plus fence.i probes",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, clean_flush_ret7);
    TEST_ASSERT("the HS store rewrites the executable code block before CBO clean/flush",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    TEST_ASSERT("cbo.clean on the rewritten executable code block retires before fence.i",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    TEST_ASSERT("cbo.flush on the rewritten executable code block retires before fence.i",
        excpt.triggered == false
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after CBO clean/flush plus fence.i, the same hart executes the committed rewritten instruction image",
        excpt.triggered == false &&
        observed == 7ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, second_ret9);
    TEST_ASSERT("a later HS code rewrite after the first CBO plus fence.i sequence completes without exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(vaddr);
    TEST_ASSERT("cbo.clean by itself is treated only as data-side maintenance before the final fence.i",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    cbo_flush(vaddr);
    TEST_ASSERT("cbo.flush by itself is treated only as data-side maintenance before the final fence.i",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the final fence.i, CBO clean/flush alone does not provide a stronger instruction-fetch oracle than the legal old-or-new executable image set",
        excpt.triggered == false &&
        (observed == 7ULL || observed == 9ULL)
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the final fence.i, the later CBO-maintained code rewrite becomes visible to same-hart instruction fetch",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the CBO plus fence.i sequence leaves the final backing code image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, second_ret9)
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);

    TEST_END();
}

bool ai_arch_fence_i_cbo_inval_dirty_profile_boundary_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x340U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t inval_ret7 = ai_arch_fence_i_ret_imm(7);
    const uint64_t final_ret9 = ai_arch_fence_i_ret_imm(9);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE11);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the warm HS code block returns the original instruction value before the CBO inval dirty-boundary probe",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, inval_ret7);
    TEST_ASSERT("the HS store creates a modified executable-code image before cbo.inval",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr);
    TEST_ASSERT("cbo.inval on the rewritten executable code block retires under the profile-selected CBIE mode",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before fence.i, cbo.inval is not treated as a substitute instruction-cache visibility point and only a legal old-or-new image is accepted",
        excpt.triggered == false &&
        (observed == 3ULL || observed == 7ULL)
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, the CBO-inval dirty-code episode does not fabricate any value outside the committed code images",
        excpt.triggered == false &&
        (observed == 3ULL || observed == 7ULL)
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, final_ret9);
    TEST_ASSERT("a later clean code rewrite after the profile-boundary CBO episode completes without exception",
        excpt.triggered == false
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the final clean rewrite plus fence.i, the same hart executes the final committed instruction image",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the CBO inval dirty-boundary sequence never fabricates an instruction image and leaves final backing code plus guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, final_ret9)
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE11);

    TEST_END();
}

bool ai_arch_fence_i_data_order_requires_explicit_fence_boundary_corner()
{
    TEST_START();

    const enum test_page code_page = AI_ARCH_FENCE_I_CODE_PAGE;
    const enum test_page data_page = VSRW_GRW;
    const uintptr_t code_offset = 0x340U;
    const uintptr_t data_offset = 0x340U;
    const uintptr_t code_vaddr = hs_page_base(code_page) + code_offset;
    const uintptr_t code_paddr = phys_page_base(code_page) + code_offset;
    const uintptr_t data0_vaddr = hs_page_base(data_page) + data_offset;
    const uintptr_t data1_vaddr = data0_vaddr + sizeof(uint64_t);
    const uintptr_t data0_paddr = phys_page_base(data_page) + data_offset;
    const uintptr_t data1_paddr = data0_paddr + sizeof(uint64_t);
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t new_ret7 = ai_arch_fence_i_ret_imm(7);
    const uint64_t data0_seed = 0x0102030405060708ULL;
    const uint64_t data1_seed = 0x1112131415161718ULL;
    const uint64_t data0_update = 0x8877665544332211ULL;
    const uint64_t data1_update = 0x123456789abcdef0ULL;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(code_paddr, old_ret3);
    write64(data0_paddr, data0_seed);
    write64(data1_paddr, data1_seed);
    ai_arch_fence_i_prepare_hs_code_page(code_page, code_paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the executable HS block starts from the old image before the fence.i data-order boundary probe",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(data0_vaddr, data0_update);
    ai_arch_fence_i();
    ai_arch_fence_i_store_d(data1_vaddr, data1_update);
    TEST_ASSERT("data stores around fence.i retire without treating fence.i itself as the data-ordering oracle",
        excpt.triggered == false
    );

    fence_iorw_iorw();
    TEST_SETUP_EXCEPT();
    uint64_t read0 = ld(data0_vaddr);
    uint64_t read1 = ld(data1_vaddr);
    TEST_ASSERT("the data visibility check is made after an explicit ordinary fence, keeping data ordering separate from fence.i",
        excpt.triggered == false &&
        read0 == data0_update &&
        read1 == data1_update
    );

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(code_vaddr, new_ret7);
    TEST_ASSERT("the same probe still rewrites code explicitly before the instruction-side fence.i point",
        excpt.triggered == false
    );

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the instruction visibility check remains tied to fence.i and observes the rewritten code image",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fence.i data-order boundary probe leaves the final backing code image and data payloads intact",
        ai_arch_fence_i_code_and_guards_match(code_paddr, new_ret7) &&
        read64(data0_paddr) == data0_update &&
        read64(data1_paddr) == data1_update
    );

    TEST_END();
}

bool ai_arch_fence_i_committed_code_fetch_fault_no_rollback_corner()
{
    TEST_START();

    const enum test_page page = AI_ARCH_FENCE_I_CODE_PAGE;
    const uintptr_t offset = 0x380U;
    const uintptr_t vaddr = hs_page_base(page) + offset;
    const uintptr_t paddr = phys_page_base(page) + offset;
    const uint64_t old_ret3 = ai_arch_fence_i_ret_imm(3);
    const uint64_t committed_ret7 = ai_arch_fence_i_ret_imm(7);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_arch_fence_i_seed_code(paddr, old_ret3);
    ai_arch_fence_i_prepare_hs_code_page(page, paddr,
        PTE_V | PTE_AD | PTE_RWX);

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("the fetch-fault rollback probe starts from the original executable instruction image",
        excpt.triggered == false &&
        observed == 3ULL);

    TEST_SETUP_EXCEPT();
    ai_arch_fence_i_store_d(vaddr, committed_ret7);
    TEST_ASSERT("the code store that precedes fence.i commits the new instruction image",
        excpt.triggered == false);

    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the committed store plus fence.i, the same hart executes the new instruction image",
        excpt.triggered == false &&
        observed == 7ULL);

    ai_arch_fence_i_sync_hs_leaf(page, paddr & PAGE_ADDR_MSK,
        PTE_V | PTE_AD | PTE_RW);
    goto_priv(PRIV_HS);
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after fence.i has exposed the committed image, a later no-X fetch reports IPF instead of rolling code back",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("the post-fence.i fetch fault leaves the committed code backing image and adjacent guards intact",
        ai_arch_fence_i_code_and_guards_match(paddr, committed_ret7));

    ai_arch_fence_i_sync_hs_leaf(page, paddr & PAGE_ADDR_MSK,
        PTE_V | PTE_AD | PTE_RWX);
    goto_priv(PRIV_HS);
    ai_arch_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission and issuing fence.i, the already committed instruction image is still executed",
        excpt.triggered == false &&
        observed == 7ULL);

    TEST_END();
}
