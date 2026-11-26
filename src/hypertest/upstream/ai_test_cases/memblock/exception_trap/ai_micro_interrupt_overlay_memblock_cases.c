#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_INTERRUPT_OVERLAY_ASSERT(message, cond) \
    TEST_ASSERT(message, (cond))

static inline uint64_t ai_interrupt_overlay_replace_byte(uint64_t value,
    unsigned byte_index, uint8_t byte_value)
{
    const uint64_t mask = 0xffULL << (byte_index * 8U);
    return (value & ~mask) | ((uint64_t)byte_value << (byte_index * 8U));
}

bool ai_micro_m_software_interrupt_between_store_and_amo_memblock_marker_progress_corner()
{
    TEST_START();

    const enum test_page pre_page = VSRWX_GRWX;
    const enum test_page load_page = SWITCH1;
    const enum test_page amo_page = SWITCH2;
    const uintptr_t pre_addr = phys_page_base(pre_page) + 0x300U;
    const uintptr_t marker_addr = phys_page_base(pre_page) + 0x340U;
    const uintptr_t load_addr = phys_page_base(load_page) + 0x300U;
    const uintptr_t amo_addr = phys_page_base(amo_page) + 0x300U;
    const uintptr_t guard_addr = phys_page_base(amo_page) + 0x340U;
    const uint64_t pre_seed = 0x1111222233334444ULL;
    const uint64_t pre_update = 0x5555666677778888ULL;
    const uint64_t load_seed = 0xa5a55a5adeadbeefULL;
    const uint64_t marker_seed = 0x0102030405060708ULL;
    const uint64_t marker_post = 0x8877665544332211ULL;
    const uint8_t marker_byte = 0x5aU;
    const uint64_t amo_seed = 0x123456789abcdef0ULL;
    const uint64_t amo_addend = 0x0000000000000123ULL;
    const uint64_t guard_seed = 0xfedcba9876543210ULL;
    uint64_t load_observed = 0ULL;
    uint64_t amo_old = 0ULL;
    uint64_t marker_low = 0ULL;
    uint64_t marker_high = 0ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);

    goto_priv(PRIV_M);
    CSRC(CSR_MIP, MIP_MSIP);
    CSRC(CSR_MIE, MIE_MSIE);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIDELEG, MIP_MSIP);

    write64(pre_addr, pre_seed);
    write64(load_addr, load_seed);
    write64(marker_addr, marker_seed);
    write64(marker_addr + sizeof(uint64_t), 0ULL);
    write64(amo_addr, amo_seed);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIE, MIE_MSIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[pre_update], 0(%[pre_addr])\n\t"
        "csrs mip, %[msip]\n\t"
        "ld %[load_observed], 0(%[load_addr])\n\t"
        "sb %[marker_byte], 1(%[marker_addr])\n\t"
        "amoadd.d %[amo_old], %[amo_addend], (%[amo_addr])\n\t"
        "sd %[marker_post], 8(%[marker_addr])\n\t"
        "fence iorw, iorw\n\t"
        "ld %[marker_low], 0(%[marker_addr])\n\t"
        "ld %[marker_high], 8(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_observed] "=&r"(load_observed),
          [amo_old] "=&r"(amo_old),
          [marker_low] "=&r"(marker_low),
          [marker_high] "=&r"(marker_high)
        : [pre_addr] "r"(pre_addr),
          [pre_update] "r"(pre_update),
          [msip] "r"(MIP_MSIP),
          [load_addr] "r"(load_addr),
          [marker_addr] "r"(marker_addr),
          [marker_byte] "r"((uint64_t)marker_byte),
          [amo_addr] "r"(amo_addr),
          [amo_addend] "r"(amo_addend),
          [marker_post] "r"(marker_post)
        : "memory");

    const bool interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MSI;
    const uint64_t expected_marker_low =
        ai_interrupt_overlay_replace_byte(marker_seed, 1U, marker_byte);

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, MIP_MSIP);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MSTATUS, saved_mstatus);

    AI_INTERRUPT_OVERLAY_ASSERT("the pending M software interrupt is taken and recorded before the post-interrupt memblock marker window completes",
        interrupt_seen);
    AI_INTERRUPT_OVERLAY_ASSERT("the store immediately before the software interrupt remains visible after interrupt return",
        read64(pre_addr) == pre_update);
    AI_INTERRUPT_OVERLAY_ASSERT("the scalar load immediately after interrupt return observes its current seed and is not replaced by handler state",
        load_observed == load_seed);
    AI_INTERRUPT_OVERLAY_ASSERT("the byte store after interrupt return updates exactly one marker byte",
        marker_low == expected_marker_low &&
        read64(marker_addr) == expected_marker_low);
    AI_INTERRUPT_OVERLAY_ASSERT("the AMO after interrupt return reports the old value and applies its update exactly once",
        amo_old == amo_seed &&
        read64(amo_addr) == amo_seed + amo_addend);
    AI_INTERRUPT_OVERLAY_ASSERT("the post-AMO marker store and adjacent guard both survive the interrupt window",
        marker_high == marker_post &&
        read64(marker_addr + sizeof(uint64_t)) == marker_post &&
        read64(guard_addr) == guard_seed);

    TEST_END(ai_micro_m_software_interrupt_between_store_and_amo_memblock_marker_progress_corner);
}
