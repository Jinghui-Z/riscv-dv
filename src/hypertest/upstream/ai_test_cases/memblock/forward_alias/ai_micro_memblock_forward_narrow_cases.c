#include "ai_micro_overlap_common.h"

static inline void ai_micro_forward_narrow_store_then_probe(uintptr_t vaddr,
    uint64_t *signed_byte, uint64_t *unsigned_byte,
    uint64_t *signed_half, uint64_t *unsigned_half, uint64_t *full_word)
{
    uint64_t signed_byte_value;
    uint64_t unsigned_byte_value;
    uint64_t signed_half_value;
    uint64_t unsigned_half_value;
    uint64_t full_word_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0x80\n\t"
        "sb t0, 1(%[addr])\n\t"
        "li t0, 0x8001\n\t"
        "sh t0, 4(%[addr])\n\t"
        "lb %[signed_byte_value], 1(%[addr])\n\t"
        "lbu %[unsigned_byte_value], 1(%[addr])\n\t"
        "lh %[signed_half_value], 4(%[addr])\n\t"
        "lhu %[unsigned_half_value], 4(%[addr])\n\t"
        "ld %[full_word_value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [signed_byte_value] "=&r" (signed_byte_value),
          [unsigned_byte_value] "=&r" (unsigned_byte_value),
          [signed_half_value] "=&r" (signed_half_value),
          [unsigned_half_value] "=&r" (unsigned_half_value),
          [full_word_value] "=&r" (full_word_value)
        : [addr] "r" (vaddr)
        : "t0", "memory");

    *signed_byte = signed_byte_value;
    *unsigned_byte = unsigned_byte_value;
    *signed_half = signed_half_value;
    *unsigned_half = unsigned_half_value;
    *full_word = full_word_value;
}

static inline void ai_micro_forward_halfword_byte_age_probe(uintptr_t vaddr,
    uint64_t *signed_half, uint64_t *unsigned_half, uint64_t *full_word)
{
    uint64_t signed_half_value;
    uint64_t unsigned_half_value;
    uint64_t full_word_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0x007f\n\t"
        "sh t0, 2(%[addr])\n\t"
        "li t0, 0x80\n\t"
        "sb t0, 3(%[addr])\n\t"
        "lh %[signed_half_value], 2(%[addr])\n\t"
        "lhu %[unsigned_half_value], 2(%[addr])\n\t"
        "ld %[full_word_value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [signed_half_value] "=&r" (signed_half_value),
          [unsigned_half_value] "=&r" (unsigned_half_value),
          [full_word_value] "=&r" (full_word_value)
        : [addr] "r" (vaddr)
        : "t0", "memory");

    *signed_half = signed_half_value;
    *unsigned_half = unsigned_half_value;
    *full_word = full_word_value;
}

static inline void ai_micro_forward_word_byte_age_probe(uintptr_t vaddr,
    uint64_t *signed_word, uint64_t *unsigned_word, uint64_t *full_word)
{
    uint64_t signed_word_value;
    uint64_t unsigned_word_value;
    uint64_t full_word_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0x007f0061\n\t"
        "sw t0, 0(%[addr])\n\t"
        "li t0, 0x8001\n\t"
        "sh t0, 2(%[addr])\n\t"
        "lw %[signed_word_value], 0(%[addr])\n\t"
        "lwu %[unsigned_word_value], 0(%[addr])\n\t"
        "ld %[full_word_value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [signed_word_value] "=&r" (signed_word_value),
          [unsigned_word_value] "=&r" (unsigned_word_value),
          [full_word_value] "=&r" (full_word_value)
        : [addr] "r" (vaddr)
        : "t0", "memory");

    *signed_word = signed_word_value;
    *unsigned_word = unsigned_word_value;
    *full_word = full_word_value;
}

static inline void ai_micro_forward_word_sign_clear_probe(uintptr_t vaddr,
    uint64_t *signed_word, uint64_t *unsigned_word, uint64_t *full_word)
{
    uint64_t signed_word_value;
    uint64_t unsigned_word_value;
    uint64_t full_word_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0x80010061\n\t"
        "sw t0, 0(%[addr])\n\t"
        "li t0, 0x7f\n\t"
        "sb t0, 3(%[addr])\n\t"
        "lw %[signed_word_value], 0(%[addr])\n\t"
        "lwu %[unsigned_word_value], 0(%[addr])\n\t"
        "ld %[full_word_value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [signed_word_value] "=&r" (signed_word_value),
          [unsigned_word_value] "=&r" (unsigned_word_value),
          [full_word_value] "=&r" (full_word_value)
        : [addr] "r" (vaddr)
        : "t0", "memory");

    *signed_word = signed_word_value;
    *unsigned_word = unsigned_word_value;
    *full_word = full_word_value;
}

static inline void ai_micro_forward_disjoint_narrow_probe(uintptr_t vaddr,
    uint64_t *signed_byte, uint64_t *unsigned_byte,
    uint64_t *signed_half, uint64_t *unsigned_half, uint64_t *full_word)
{
    uint64_t signed_byte_value;
    uint64_t unsigned_byte_value;
    uint64_t signed_half_value;
    uint64_t unsigned_half_value;
    uint64_t full_word_value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0x7e01\n\t"
        "sh t0, 4(%[addr])\n\t"
        "lb %[signed_byte_value], 1(%[addr])\n\t"
        "lbu %[unsigned_byte_value], 1(%[addr])\n\t"
        "lh %[signed_half_value], 2(%[addr])\n\t"
        "lhu %[unsigned_half_value], 2(%[addr])\n\t"
        "ld %[full_word_value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [signed_byte_value] "=&r" (signed_byte_value),
          [unsigned_byte_value] "=&r" (unsigned_byte_value),
          [signed_half_value] "=&r" (signed_half_value),
          [unsigned_half_value] "=&r" (unsigned_half_value),
          [full_word_value] "=&r" (full_word_value)
        : [addr] "r" (vaddr)
        : "t0", "memory");

    *signed_byte = signed_byte_value;
    *unsigned_byte = unsigned_byte_value;
    *signed_half = signed_half_value;
    *unsigned_half = unsigned_half_value;
    *full_word = full_word_value;
}

bool ai_micro_hs_same_vaddr_partial_store_forward_narrow_load_sign_zero_extension_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t target_offset = 0x780U;
    const uintptr_t vaddr = hs_page_base(page) + target_offset;
    const uintptr_t before_vaddr = vaddr - sizeof(uint64_t);
    const uintptr_t after_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + target_offset;
    const uintptr_t before_paddr = paddr - sizeof(uint64_t);
    const uintptr_t after_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t expected_merged_value = 0x1020800150608080ULL;
    const uint64_t before_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t after_value = 0x8877665544332211ULL;
    const uint64_t expected_signed_byte = 0xffffffffffffff80ULL;
    const uint64_t expected_unsigned_byte = 0x80ULL;
    const uint64_t expected_signed_half = 0xffffffffffff8001ULL;
    const uint64_t expected_unsigned_half = 0x8001ULL;

    uint64_t signed_byte = 0ULL;
    uint64_t unsigned_byte = 0ULL;
    uint64_t signed_half = 0ULL;
    uint64_t unsigned_half = 0ULL;
    uint64_t full_word = 0ULL;

    goto_priv(PRIV_M);
    write64(before_paddr, before_value);
    write64(paddr, initial_value);
    write64(after_paddr, after_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_before = ld(before_vaddr);
    uint64_t warmed_target = ld(vaddr);
    uint64_t warmed_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the aligned same-VA forward-narrow probe first warms the target and adjacent guard words with the seeded payloads",
        warmed_before == before_value &&
        warmed_target == initial_value &&
        warmed_after == after_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_forward_narrow_store_then_probe(vaddr, &signed_byte,
        &unsigned_byte, &signed_half, &unsigned_half, &full_word);
    AI_ASSERT_NO_EXCEPT_AND("immediate signed and unsigned narrow loads after same-VA byte and halfword stores use the latest forwarded byte lanes and their own extension mode",
        signed_byte == expected_signed_byte &&
        unsigned_byte == expected_unsigned_byte &&
        signed_half == expected_signed_half &&
        unsigned_half == expected_unsigned_half &&
        full_word == expected_merged_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_before = ld(before_vaddr);
    uint64_t reloaded_target = ld(vaddr);
    uint64_t reloaded_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the narrow-load forwarding window, only the target doubleword keeps the byte-age merged image and both guards remain unchanged",
        reloaded_before == before_value &&
        reloaded_target == expected_merged_value &&
        reloaded_after == after_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory records only the same-VA byte and halfword stores on the target doubleword while preserving both adjacent guard words",
        read64(before_paddr) == before_value &&
        read64(paddr) == expected_merged_value &&
        read64(after_paddr) == after_value
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_overlapping_partial_store_word_forward_youngest_byte_clears_sign_extension_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t target_offset = 0x860U;
    const uintptr_t vaddr = hs_page_base(page) + target_offset;
    const uintptr_t before_vaddr = vaddr - sizeof(uint64_t);
    const uintptr_t after_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + target_offset;
    const uintptr_t before_paddr = paddr - sizeof(uint64_t);
    const uintptr_t after_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t expected_merged_value = 0x102030407f010061ULL;
    const uint64_t before_value = 0x3141592653589793ULL;
    const uint64_t after_value = 0x2718281828459045ULL;
    const uint64_t expected_signed_word = 0x7f010061ULL;
    const uint64_t expected_unsigned_word = 0x7f010061ULL;

    uint64_t signed_word = 0ULL;
    uint64_t unsigned_word = 0ULL;
    uint64_t full_word = 0ULL;

    goto_priv(PRIV_M);
    write64(before_paddr, before_value);
    write64(paddr, initial_value);
    write64(after_paddr, after_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_before = ld(before_vaddr);
    uint64_t warmed_target = ld(vaddr);
    uint64_t warmed_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the youngest-byte sign-clear word forward probe first warms the aligned target and both adjacent guard words",
        warmed_before == before_value &&
        warmed_target == initial_value &&
        warmed_after == after_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_forward_word_sign_clear_probe(vaddr, &signed_word,
        &unsigned_word, &full_word);
    AI_ASSERT_NO_EXCEPT_AND("an immediate word load after older negative sw plus younger high-byte sb uses the final cleared sign bit when applying lw and lwu extension",
        signed_word == expected_signed_word &&
        unsigned_word == expected_unsigned_word &&
        full_word == expected_merged_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_before = ld(before_vaddr);
    uint64_t reloaded_target = ld(vaddr);
    uint64_t reloaded_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the youngest-byte sign-clear forwarding window, only the low word of the target doubleword is updated while both guards remain unchanged",
        reloaded_before == before_value &&
        reloaded_target == expected_merged_value &&
        reloaded_after == after_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory records the older negative word store with the younger high-byte sign clear and preserves both adjacent guard words",
        read64(before_paddr) == before_value &&
        read64(paddr) == expected_merged_value &&
        read64(after_paddr) == after_value
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_partial_store_disjoint_narrow_load_no_false_forward_sign_zero_extension_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t target_offset = 0x860U;
    const uintptr_t vaddr = hs_page_base(page) + target_offset;
    const uintptr_t before_vaddr = vaddr - sizeof(uint64_t);
    const uintptr_t after_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + target_offset;
    const uintptr_t before_paddr = paddr - sizeof(uint64_t);
    const uintptr_t after_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1122334480618010ULL;
    const uint64_t expected_merged_value = 0x11227e0180618010ULL;
    const uint64_t before_value = 0x2468ace013579bdfULL;
    const uint64_t after_value = 0xfdb97531eca86420ULL;
    const uint64_t expected_signed_byte = 0xffffffffffffff80ULL;
    const uint64_t expected_unsigned_byte = 0x80ULL;
    const uint64_t expected_signed_half = 0xffffffffffff8061ULL;
    const uint64_t expected_unsigned_half = 0x8061ULL;

    uint64_t signed_byte = 0ULL;
    uint64_t unsigned_byte = 0ULL;
    uint64_t signed_half = 0ULL;
    uint64_t unsigned_half = 0ULL;
    uint64_t full_word = 0ULL;

    goto_priv(PRIV_M);
    write64(before_paddr, before_value);
    write64(paddr, initial_value);
    write64(after_paddr, after_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_before = ld(before_vaddr);
    uint64_t warmed_target = ld(vaddr);
    uint64_t warmed_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the disjoint-mask partial-store narrow-load probe first warms the aligned target and both adjacent guard words",
        warmed_before == before_value &&
        warmed_target == initial_value &&
        warmed_after == after_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_forward_disjoint_narrow_probe(vaddr, &signed_byte,
        &unsigned_byte, &signed_half, &unsigned_half, &full_word);
    AI_ASSERT_NO_EXCEPT_AND("narrow loads whose byte masks do not overlap the preceding halfword store must keep their original bytes and extension mode while the full load observes only the true store bytes",
        signed_byte == expected_signed_byte &&
        unsigned_byte == expected_unsigned_byte &&
        signed_half == expected_signed_half &&
        unsigned_half == expected_unsigned_half &&
        full_word == expected_merged_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_before = ld(before_vaddr);
    uint64_t reloaded_target = ld(vaddr);
    uint64_t reloaded_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the disjoint-mask forwarding window, only bytes four and five of the target doubleword are updated while both guards remain unchanged",
        reloaded_before == before_value &&
        reloaded_target == expected_merged_value &&
        reloaded_after == after_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory records only the halfword store bytes and preserves both disjoint observed bytes plus adjacent guard words",
        read64(before_paddr) == before_value &&
        read64(paddr) == expected_merged_value &&
        read64(after_paddr) == after_value
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_overlapping_partial_store_word_forward_byte_age_sign_zero_extension_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t target_offset = 0x820U;
    const uintptr_t vaddr = hs_page_base(page) + target_offset;
    const uintptr_t before_vaddr = vaddr - sizeof(uint64_t);
    const uintptr_t after_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + target_offset;
    const uintptr_t before_paddr = paddr - sizeof(uint64_t);
    const uintptr_t after_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t expected_merged_value = 0x1020304080010061ULL;
    const uint64_t before_value = 0x123456789abcdef0ULL;
    const uint64_t after_value = 0x0fedcba987654321ULL;
    const uint64_t expected_signed_word = 0xffffffff80010061ULL;
    const uint64_t expected_unsigned_word = 0x80010061ULL;

    uint64_t signed_word = 0ULL;
    uint64_t unsigned_word = 0ULL;
    uint64_t full_word = 0ULL;

    goto_priv(PRIV_M);
    write64(before_paddr, before_value);
    write64(paddr, initial_value);
    write64(after_paddr, after_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_before = ld(before_vaddr);
    uint64_t warmed_target = ld(vaddr);
    uint64_t warmed_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the overlapping partial-store word forward probe first warms the aligned target and both adjacent guard words",
        warmed_before == before_value &&
        warmed_target == initial_value &&
        warmed_after == after_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_forward_word_byte_age_probe(vaddr, &signed_word,
        &unsigned_word, &full_word);
    AI_ASSERT_NO_EXCEPT_AND("an immediate word load after older sw plus younger overlapping sh merges low bytes from the older store and high bytes from the younger store before applying signed or unsigned extension",
        signed_word == expected_signed_word &&
        unsigned_word == expected_unsigned_word &&
        full_word == expected_merged_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_before = ld(before_vaddr);
    uint64_t reloaded_target = ld(vaddr);
    uint64_t reloaded_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the byte-age word forwarding window, only the low word of the target doubleword is updated while both guards remain unchanged",
        reloaded_before == before_value &&
        reloaded_target == expected_merged_value &&
        reloaded_after == after_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory records the older word store with the younger high-halfword overwrite and preserves both adjacent guard words",
        read64(before_paddr) == before_value &&
        read64(paddr) == expected_merged_value &&
        read64(after_paddr) == after_value
    );

    TEST_END();
}

bool ai_micro_hs_same_vaddr_overlapping_partial_store_halfword_forward_byte_age_sign_zero_extension_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t target_offset = 0x7c0U;
    const uintptr_t vaddr = hs_page_base(page) + target_offset;
    const uintptr_t before_vaddr = vaddr - sizeof(uint64_t);
    const uintptr_t after_vaddr = vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(page) + target_offset;
    const uintptr_t before_paddr = paddr - sizeof(uint64_t);
    const uintptr_t after_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1020304050607080ULL;
    const uint64_t expected_merged_value = 0x10203040807f7080ULL;
    const uint64_t before_value = 0x1122334455667788ULL;
    const uint64_t after_value = 0x99aabbccddeeff00ULL;
    const uint64_t expected_signed_half = 0xffffffffffff807fULL;
    const uint64_t expected_unsigned_half = 0x807fULL;

    uint64_t signed_half = 0ULL;
    uint64_t unsigned_half = 0ULL;
    uint64_t full_word = 0ULL;

    goto_priv(PRIV_M);
    write64(before_paddr, before_value);
    write64(paddr, initial_value);
    write64(after_paddr, after_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_before = ld(before_vaddr);
    uint64_t warmed_target = ld(vaddr);
    uint64_t warmed_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the overlapping partial-store halfword forward probe first warms the aligned target and both adjacent guard words",
        warmed_before == before_value &&
        warmed_target == initial_value &&
        warmed_after == after_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_forward_halfword_byte_age_probe(vaddr, &signed_half,
        &unsigned_half, &full_word);
    AI_ASSERT_NO_EXCEPT_AND("an immediate halfword load after older sh plus younger overlapping sb merges one byte from each store entry and then applies its own signed or unsigned extension",
        signed_half == expected_signed_half &&
        unsigned_half == expected_unsigned_half &&
        full_word == expected_merged_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded_before = ld(before_vaddr);
    uint64_t reloaded_target = ld(vaddr);
    uint64_t reloaded_after = ld(after_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the byte-age halfword forwarding window, only bytes two and three of the target doubleword are updated while both guards remain unchanged",
        reloaded_before == before_value &&
        reloaded_target == expected_merged_value &&
        reloaded_after == after_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory records the older halfword store with the younger byte overwrite and preserves both adjacent guard words",
        read64(before_paddr) == before_value &&
        read64(paddr) == expected_merged_value &&
        read64(after_paddr) == after_value
    );

    TEST_END();
}
