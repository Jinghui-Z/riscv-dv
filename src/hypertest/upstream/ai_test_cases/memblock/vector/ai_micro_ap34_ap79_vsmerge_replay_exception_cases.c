#include "../boundary/ai_micro_cross_page_fault_common.h"
#include <instructions.h>
#include <stdint.h>

#define AI_AP34_VL 4U
#define AI_AP34_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_AP34_MCONTROL6_STORE 0x2ULL

static inline uintptr_t ai_ap34_page_offset(enum test_page base_page,
    enum test_page page, uintptr_t offset)
{
    return hs_page_base(page) + offset - hs_page_base(base_page);
}

static inline void ai_ap34_write_words(uintptr_t paddr,
    const uint32_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        write32(paddr + i * sizeof(uint32_t), words[i]);
    }
}

static inline bool ai_ap34_words_match(uintptr_t paddr,
    const uint32_t *words, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if (read32(paddr + i * sizeof(uint32_t)) != words[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_ap34_prepare_leaf(enum test_page page, uint64_t flags)
{
    ai_micro_set_hs_leaf(page, flags);
}

static inline uint32_t ai_ap34_hs_load32(uintptr_t vaddr)
{
    uint32_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lw %[value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(vaddr)
        : "memory");

    return value;
}

static inline void ai_ap34_vsoxei32_store(const uint32_t *src,
    const uint32_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsoxei32.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_AP34_VL)
        : "t0", "memory");
}

static inline void ai_ap34_vsuxei32_store(const uint32_t *src,
    const uint32_t *offsets, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, ta, ma\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vsuxei32.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_AP34_VL)
        : "t0", "memory");
}

static inline void ai_ap34_masked_vsoxei32_store(const uint32_t *src,
    const uint32_t *offsets, const uint8_t *mask, uint8_t *base)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vle32.v v8, (%[offsets])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vsoxei32.v v6, (%[base]), v8, v0.t\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [mask] "r"(mask),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_AP34_VL)
        : "t0", "memory");
}

static inline void ai_ap34_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, 0ULL);
}

static inline void ai_ap34_arm_store_trigger(size_t slot, uintptr_t addr)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0" ::: "memory");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, AI_AP34_MCONTROL6_MSU_BASE | AI_AP34_MCONTROL6_STORE);
}

static inline uint64_t ai_ap34_enable_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_ap34_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_ap34_storequeue_pressure(uintptr_t paddr)
{
    for (unsigned i = 0; i < 48U; ++i) {
        write32(paddr + i * sizeof(uint32_t),
            0x66000000U + (uint32_t)i);
    }
}

static inline void ai_ap34_cleanup(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

bool ai_micro_ap34_vsoxei32_exception_before_cold_replay_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page cold_page = VSRWX_GRX;
    const enum test_page suffix_page = VSRWX_GR;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x100U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x100U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x100U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x100U;
    const uintptr_t prefix_paddr = phys_page_base(base_page) + 0x100U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x100U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x100U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x100U;
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x100U;
    const uint32_t src[AI_AP34_VL] = {
        0x34010000U, 0x34010001U, 0x34010002U, 0x34010003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x100U),
        (uint32_t)ai_ap34_page_offset(base_page, fault_page, 0x100U),
        (uint32_t)ai_ap34_page_offset(base_page, cold_page, 0x100U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x100U),
    };
    const uint32_t initial[AI_AP34_VL] = {
        0x11110000U, 0x11110001U, 0x11110002U, 0x11110003U,
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write32(prefix_paddr, initial[0]);
    write32(fault_paddr, initial[1]);
    write32(cold_paddr, initial[2]);
    write32(suffix_paddr, initial[3]);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(cold_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(fault_vaddr);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsoxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-34 ordered indexed store with an older warm store-page-fault element and a younger cold replay candidate reports the older exception directly",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-34 older-exception path commits only the prefix element and leaves the exception/replay/suffix targets old",
        read32(prefix_paddr) == src[0] &&
        read32(fault_paddr) == initial[1] &&
        read32(cold_paddr) == initial[2] &&
        read32(suffix_paddr) == initial[3]
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap34_vsoxei32_cold_replay_before_exception_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page replay_page = VSRWX_GRW;
    const enum test_page fault_page = VSRWX_GRX;
    const enum test_page suffix_page = VSRWX_GR;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x140U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x140U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x140U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x140U;
    const uintptr_t replay_paddr = phys_page_base(replay_page) + 0x140U;
    const uintptr_t warm_paddr = phys_page_base(base_page) + 0x140U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x140U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x140U;
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x140U;
    const uint32_t src[AI_AP34_VL] = {
        0x34020000U, 0x34020001U, 0x34020002U, 0x34020003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, replay_page, 0x140U),
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x140U),
        (uint32_t)ai_ap34_page_offset(base_page, fault_page, 0x140U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x140U),
    };
    const uint32_t initial[AI_AP34_VL] = {
        0x22220000U, 0x22220001U, 0x22220002U, 0x22220003U,
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write32(replay_paddr, initial[0]);
    write32(warm_paddr, initial[1]);
    write32(fault_paddr, initial[2]);
    write32(suffix_paddr, initial[3]);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(replay_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(hs_page_base(base_page) + 0x140U);
    (void)ai_ap34_hs_load32(fault_vaddr);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsoxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-34 ordered indexed store with an older cold replay candidate still replays far enough to report the later exception element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 2ULL,
        "cause=0x%lx tval=0x%lx expected=0x%lx vstart=%lu",
        excpt.cause, excpt.tval, fault_vaddr, observed_vstart
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-34 cold-before-exception path keeps the later exception and suffix targets old while allowing only older prefix stores",
        read32(replay_paddr) == src[0] &&
        read32(warm_paddr) == src[1] &&
        read32(fault_paddr) == initial[2] &&
        read32(suffix_paddr) == initial[3]
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap34_vsoxei32_same_element_cold_fault_no_replay_loop_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page suffix_page = VSRWX_GRX;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x180U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x180U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x180U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x180U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x180U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x180U;
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x180U;
    const uint32_t src[AI_AP34_VL] = {
        0x34030000U, 0x34030001U, 0x34030002U, 0x34030003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, fault_page, 0x180U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x180U),
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x180U),
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x184U),
    };
    const uint32_t initial_fault = 0x33330000U;
    const uint32_t initial_suffix = 0x33330001U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write32(fault_paddr, initial_fault);
    write32(suffix_paddr, initial_suffix);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsoxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-34 same-element cold translation plus store-page-fault reports element0 instead of falling into a replay loop",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-34 same-element fault path leaves the fault and suffix words old",
        read32(fault_paddr) == initial_fault &&
        read32(suffix_paddr) == initial_suffix
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap34_masked_vsoxei32_inactive_fault_ignored_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page active_prefix_page = VSRWX_GRW;
    const enum test_page masked_fault_page = VSRWX_GRX;
    const enum test_page active_fault_page = VSRWX_GR;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x1c0U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x1c0U;
    const uintptr_t mask_vaddr = hs_page_base(offsets_page) + 0x1e0U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x1c0U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x1c0U;
    const uintptr_t mask_paddr = phys_page_base(offsets_page) + 0x1e0U;
    const uintptr_t active_prefix_paddr =
        phys_page_base(active_prefix_page) + 0x1c0U;
    const uintptr_t masked_fault_paddr =
        phys_page_base(masked_fault_page) + 0x1c0U;
    const uintptr_t active_fault_vaddr =
        hs_page_base(active_fault_page) + 0x1c0U;
    const uintptr_t active_fault_paddr =
        phys_page_base(active_fault_page) + 0x1c0U;
    const uint32_t src[AI_AP34_VL] = {
        0x34040000U, 0x34040001U, 0x34040002U, 0x34040003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, active_prefix_page, 0x1c0U),
        (uint32_t)ai_ap34_page_offset(base_page, masked_fault_page, 0x1c0U),
        (uint32_t)ai_ap34_page_offset(base_page, active_fault_page, 0x1c0U),
        (uint32_t)ai_ap34_page_offset(base_page, masked_fault_page, 0x1c4U),
    };
    const uint32_t initial_prefix = 0x44440000U;
    const uint32_t initial_masked = 0x44440001U;
    const uint32_t initial_fault = 0x44440002U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write8(mask_paddr, 0x05U);
    write32(active_prefix_paddr, initial_prefix);
    write32(masked_fault_paddr, initial_masked);
    write32(active_fault_paddr, initial_fault);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(active_prefix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(masked_fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(active_fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(hs_page_base(masked_fault_page) + 0x1c0U);
    (void)ai_ap34_hs_load32(active_fault_vaddr);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_masked_vsoxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (const uint8_t *)mask_vaddr,
        (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-34 masked ordered indexed store ignores inactive fault candidates and reports the first active protected element",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == active_fault_vaddr &&
        observed_vstart == 2ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-34 masked path writes only active prefix data and leaves inactive fault candidates untouched",
        read32(active_prefix_paddr) == src[0] &&
        read32(masked_fault_paddr) == initial_masked &&
        read32(active_fault_paddr) == initial_fault
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap34_store_trigger_before_cold_replay_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page trigger_page = VSRWX_GRW;
    const enum test_page cold_page = VSRWX_GRX;
    const enum test_page suffix_page = VSRWX_GR;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x200U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x200U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x200U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x200U;
    const uintptr_t prefix_paddr = phys_page_base(base_page) + 0x200U;
    const uintptr_t trigger_vaddr = hs_page_base(trigger_page) + 0x200U;
    const uintptr_t trigger_paddr = phys_page_base(trigger_page) + 0x200U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x200U;
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x200U;
    const uint32_t src[AI_AP34_VL] = {
        0x34050000U, 0x34050001U, 0x34050002U, 0x34050003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x200U),
        (uint32_t)ai_ap34_page_offset(base_page, trigger_page, 0x200U),
        (uint32_t)ai_ap34_page_offset(base_page, cold_page, 0x200U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x200U),
    };
    const uint32_t initial[AI_AP34_VL] = {
        0x55550000U, 0x55550001U, 0x55550002U, 0x55550003U,
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write32(prefix_paddr, initial[0]);
    write32(trigger_paddr, initial[1]);
    write32(cold_paddr, initial[2]);
    write32(suffix_paddr, initial[3]);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(trigger_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(cold_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(trigger_vaddr);
    ai_ap34_clear_mcontrol6_slot(0);
    ai_ap34_arm_store_trigger(0, trigger_vaddr);
    const uint64_t saved_mstatus = ai_ap34_enable_breakpoint_delivery();
    goto_priv(PRIV_HS);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsoxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-34 ordered indexed store treats a store-address trigger as an exception flow that is not swallowed by a younger cold replay candidate",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        observed_vstart == 1ULL
    );

    ai_ap34_restore_machine_status(saved_mstatus);
    ai_ap34_clear_mcontrol6_slot(0);

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-34 trigger-before-replay path commits only the prefix element and leaves the trigger/replay/suffix targets old",
        read32(prefix_paddr) == src[0] &&
        read32(trigger_paddr) == initial[1] &&
        read32(cold_paddr) == initial[2] &&
        read32(suffix_paddr) == initial[3]
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap79_vsuxei32_misaligned_exception_before_later_cold_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page high_fault_page = VSRWX_GRW;
    const enum test_page cold_page = VSRWX_GRX;
    const enum test_page suffix_page = VSRWX_GR;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x240U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x240U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x240U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x240U;
    const uintptr_t low_fault_paddr = phys_page_base(base_page) + PAGE_SIZE - 1U;
    const uintptr_t high_fault_vaddr = hs_page_base(high_fault_page);
    const uintptr_t high_fault_paddr = phys_page_base(high_fault_page);
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x240U;
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x240U;
    const uint32_t src[AI_AP34_VL] = {
        0x79010000U, 0x79010001U, 0x79010002U, 0x79010003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, base_page, PAGE_SIZE - 1U),
        (uint32_t)ai_ap34_page_offset(base_page, cold_page, 0x240U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x240U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x244U),
    };
    const uint8_t low_initial = 0xa1U;
    const uint8_t high_initial[3] = { 0xa2U, 0xa3U, 0xa4U };
    const uint32_t cold_initial = 0x7901c001U;
    const uint32_t suffix_initial = 0x7901c002U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write8(low_fault_paddr, low_initial);
    ai_micro_write_bytes(high_fault_paddr, high_initial, sizeof(high_initial));
    write32(cold_paddr, cold_initial);
    write32(suffix_paddr, suffix_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(high_fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(cold_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(high_fault_vaddr);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsuxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-79 unordered indexed store reports the earlier non-aligned faulting element before any later cold replay candidate can reissue it",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 0ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-79 earlier non-aligned exception path leaves the split faulting element and later replay/suffix targets old",
        read8(low_fault_paddr) == low_initial &&
        ai_micro_bytes_match(high_fault_paddr, high_initial,
            sizeof(high_initial)) &&
        read32(cold_paddr) == cold_initial &&
        read32(suffix_paddr) == suffix_initial
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap79_vsuxei32_cold_before_misaligned_exception_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page high_fault_page = VSRWX_GRW;
    const enum test_page suffix_page = VSRWX_GRX;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x280U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x280U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x280U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x280U;
    const uintptr_t cold_paddr = phys_page_base(base_page) + 0x280U;
    const uintptr_t low_fault_paddr = phys_page_base(base_page) + PAGE_SIZE - 1U;
    const uintptr_t high_fault_vaddr = hs_page_base(high_fault_page);
    const uintptr_t high_fault_paddr = phys_page_base(high_fault_page);
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x280U;
    const uint32_t src[AI_AP34_VL] = {
        0x79020000U, 0x79020001U, 0x79020002U, 0x79020003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x280U),
        (uint32_t)ai_ap34_page_offset(base_page, base_page, PAGE_SIZE - 1U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x280U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x284U),
    };
    const uint32_t cold_initial = 0x7902c000U;
    const uint8_t low_initial = 0xb1U;
    const uint8_t high_initial[3] = { 0xb2U, 0xb3U, 0xb4U };
    const uint32_t suffix_initial = 0x7902c002U;
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write32(cold_paddr, cold_initial);
    write8(low_fault_paddr, low_initial);
    ai_micro_write_bytes(high_fault_paddr, high_initial, sizeof(high_initial));
    write32(suffix_paddr, suffix_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(high_fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(high_fault_vaddr);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsuxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-79 unordered indexed store with a cold older element still preserves the later non-aligned exception through replay",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == high_fault_vaddr &&
        observed_vstart == 1ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-79 cold-before-nonaligned-exception path allows only the older element and never reissues the split faulting element after StoreQueue progress",
        read32(cold_paddr) == src[0] &&
        read8(low_fault_paddr) == low_initial &&
        ai_micro_bytes_match(high_fault_paddr, high_initial,
            sizeof(high_initial)) &&
        read32(suffix_paddr) == suffix_initial
    );

    ai_ap34_cleanup();

    TEST_END();
}

bool ai_micro_ap34_vsmerge_pressure_sqwrap_exception_replay_manual_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page cold_page = VSRWX_GRX;
    const enum test_page suffix_page = VSRWX_GR;
    const enum test_page src_page = SWITCH1;
    const enum test_page offsets_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t offsets_vaddr = hs_page_base(offsets_page) + 0x300U;
    const uintptr_t pressure_paddr = phys_page_base(src_page) + 0x380U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x300U;
    const uintptr_t offsets_paddr = phys_page_base(offsets_page) + 0x300U;
    const uintptr_t prefix_paddr = phys_page_base(base_page) + 0x300U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x300U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x300U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x300U;
    const uintptr_t suffix_paddr = phys_page_base(suffix_page) + 0x300U;
    const uint32_t src[AI_AP34_VL] = {
        0x34080000U, 0x34080001U, 0x34080002U, 0x34080003U,
    };
    const uint32_t offsets[AI_AP34_VL] = {
        (uint32_t)ai_ap34_page_offset(base_page, base_page, 0x300U),
        (uint32_t)ai_ap34_page_offset(base_page, fault_page, 0x300U),
        (uint32_t)ai_ap34_page_offset(base_page, cold_page, 0x300U),
        (uint32_t)ai_ap34_page_offset(base_page, suffix_page, 0x300U),
    };
    const uint32_t initial[AI_AP34_VL] = {
        0x88880000U, 0x88880001U, 0x88880002U, 0x88880003U,
    };
    uint64_t observed_vstart = 0ULL;

    goto_priv(PRIV_M);
    ai_ap34_storequeue_pressure(pressure_paddr);
    ai_ap34_write_words(src_paddr, src, AI_AP34_VL);
    ai_ap34_write_words(offsets_paddr, offsets, AI_AP34_VL);
    write32(prefix_paddr, initial[0]);
    write32(fault_paddr, initial[1]);
    write32(cold_paddr, initial[2]);
    write32(suffix_paddr, initial[3]);

    goto_priv(PRIV_HS);
    hspt_init();
    ai_ap34_prepare_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(fault_page, PTE_V | PTE_AD | PTE_RX);
    ai_ap34_prepare_leaf(cold_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(suffix_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_ap34_prepare_leaf(offsets_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    (void)ai_ap34_hs_load32(fault_vaddr);

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_ap34_vsoxei32_store((const uint32_t *)src_vaddr,
        (const uint32_t *)offsets_vaddr, (uint8_t *)base_vaddr);
    observed_vstart = CSRR(CSR_VSTART);
    TEST_ASSERT("AP-34 pressure probe keeps VSMergeBuffer flow selection tied to the correct element when exception and replay feedback are mixed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr &&
        observed_vstart == 1ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-34 pressure probe does not clear the wrong flowMask bit or commit younger replay/suffix targets after the exception",
        read32(prefix_paddr) == src[0] &&
        read32(fault_paddr) == initial[1] &&
        read32(cold_paddr) == initial[2] &&
        read32(suffix_paddr) == initial[3]
    );

    ai_ap34_cleanup();

    TEST_END();
}
