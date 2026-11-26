#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_ARCH_CMO_DISPATCH_BLOCK_BYTES 64U
#define AI_ARCH_CMO_DISPATCH_VECTOR_BYTES 16U
#define AI_ARCH_CMO_DISPATCH_MAIN_OBS 10U
#define AI_ARCH_CMO_DISPATCH_B2B_OBS 4U
#define AI_ARCH_CMO_DISPATCH_SCALAR_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize", \
                                      "no-tree-loop-distribute-patterns")))

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR uint8_t
ai_arch_cmo_dispatch_seed_byte(unsigned index, uint8_t salt)
{
    return (uint8_t)(salt + (uint8_t)(index * 23U) ^
        (uint8_t)(index << 3));
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR void
ai_arch_cmo_dispatch_fill_line(uintptr_t paddr, uint8_t salt)
{
    for (unsigned i = 0; i < AI_ARCH_CMO_DISPATCH_BLOCK_BYTES; ++i) {
        write8(paddr + i, ai_arch_cmo_dispatch_seed_byte(i, salt));
    }
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR void
ai_arch_cmo_dispatch_fill_expected(uint8_t *expected, uint8_t value)
{
    volatile uint8_t *expected_v = (volatile uint8_t *)expected;

    for (unsigned i = 0; i < AI_ARCH_CMO_DISPATCH_BLOCK_BYTES; ++i) {
        expected_v[i] = value;
    }
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR void
ai_arch_cmo_dispatch_fill_u64(uint64_t *dst, unsigned count, uint64_t value)
{
    volatile uint64_t *dst_v = (volatile uint64_t *)dst;

    for (unsigned i = 0; i < count; ++i) {
        dst_v[i] = value ^ ((uint64_t)i << 48);
    }
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR void
ai_arch_cmo_dispatch_write_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    volatile const uint8_t *bytes_v = (volatile const uint8_t *)bytes;

    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes_v[i]);
    }
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR bool
ai_arch_cmo_dispatch_phys_bytes_equal(uintptr_t paddr,
    const uint8_t *expected, unsigned len, unsigned *bad_index)
{
    volatile const uint8_t *expected_v = (volatile const uint8_t *)expected;

    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != expected_v[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = 0U;
    return true;
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR bool
ai_arch_cmo_dispatch_phys_line_zero(uintptr_t paddr)
{
    for (unsigned i = 0; i < AI_ARCH_CMO_DISPATCH_BLOCK_BYTES; ++i) {
        if (read8(paddr + i) != 0U) {
            return false;
        }
    }

    return true;
}

static AI_ARCH_CMO_DISPATCH_SCALAR_ATTR bool
ai_arch_cmo_dispatch_phys_line_seed(uintptr_t paddr, uint8_t salt)
{
    for (unsigned i = 0; i < AI_ARCH_CMO_DISPATCH_BLOCK_BYTES; ++i) {
        if (read8(paddr + i) !=
            ai_arch_cmo_dispatch_seed_byte(i, salt)) {
            return false;
        }
    }

    return true;
}

static inline uint64_t ai_arch_cmo_dispatch_load_le64(const uint8_t *bytes,
    unsigned offset)
{
    volatile const uint8_t *bytes_v = (volatile const uint8_t *)bytes;
    uint64_t value = 0ULL;

    for (unsigned i = 0; i < 8U; ++i) {
        value |= ((uint64_t)bytes_v[offset + i]) << (8U * i);
    }

    return value;
}

static inline uint64_t ai_arch_cmo_dispatch_sext16(uint16_t value)
{
    return (uint64_t)(int64_t)(int16_t)value;
}

static inline void ai_arch_cmo_dispatch_prepare_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBIE01);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_cmo_dispatch_reset_env(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_VSTART, 0ULL);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_cmo_dispatch_main_episode(uintptr_t target,
    uintptr_t vector_src, uintptr_t mask, uintptr_t marker,
    uint64_t marker_value, uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[target]\n\t"
        "mv a2, %[obs]\n\t"
        ".insn i 0x0f, 0x2, x0, a1, 4\n\t"
        "c.lbu a3, 0(a1)\n\t"
        "sd a3, 0(a2)\n\t"
        "li a4, 0x81\n\t"
        "c.sb a4, 1(a1)\n\t"
        "c.lbu a3, 1(a1)\n\t"
        "sd a3, 8(a2)\n\t"
        "li a4, -128\n\t"
        "c.sh a4, 2(a1)\n\t"
        "c.lh a3, 2(a1)\n\t"
        "sd a3, 16(a2)\n\t"
        "c.lhu a3, 2(a1)\n\t"
        "sd a3, 24(a2)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t0, 0(a1)\n\t"
        "sd t0, 32(a2)\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "vle8.v v8, (%[vector_src])\n\t"
        "addi t1, a1, 16\n\t"
        "vse8.v v8, (t1), v0.t\n\t"
        "csrr t2, vl\n\t"
        "sd t2, 40(a2)\n\t"
        "csrr t2, vstart\n\t"
        "sd t2, 48(a2)\n\t"
        "ld t3, 16(a1)\n\t"
        "sd t3, 56(a2)\n\t"
        "ld t4, 24(a1)\n\t"
        "sd t4, 64(a2)\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        "ld t5, 0(%[marker])\n\t"
        "sd t5, 72(a2)\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        ".option pop\n\t"
        :
        : [target] "r"(target),
          [vector_src] "r"(vector_src),
          [mask] "r"(mask),
          [marker] "r"(marker),
          [marker_value] "r"(marker_value),
          [obs] "r"(obs),
          [vl] "r"((unsigned)AI_ARCH_CMO_DISPATCH_VECTOR_BYTES)
        : "a1", "a2", "a3", "a4", "t0", "t1", "t2", "t3",
          "t4", "t5", "memory");
}

static inline void ai_arch_cmo_dispatch_back_to_back_episode(
    uintptr_t first, uintptr_t second, uintptr_t marker,
    uint64_t scalar_payload, uint64_t marker_value, uint64_t *obs)
{
    asm volatile(
        ".option push\n\t"
        ".option arch,+zcb\n\t"
        "mv a1, %[first]\n\t"
        "mv a2, %[second]\n\t"
        "mv a3, %[obs]\n\t"
        ".insn i 0x0f, 0x2, x0, a1, 4\n\t"
        ".insn i 0x0f, 0x2, x0, a2, 4\n\t"
        "li a4, 0x5a\n\t"
        "c.sb a4, 0(a1)\n\t"
        "li a4, 0x7788\n\t"
        "c.sh a4, 2(a2)\n\t"
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[scalar_payload], 40(a1)\n\t"
        "ld t0, 0(a1)\n\t"
        "sd t0, 0(a3)\n\t"
        "ld t1, 40(a1)\n\t"
        "sd t1, 8(a3)\n\t"
        "ld t2, 0(a2)\n\t"
        "sd t2, 16(a3)\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        "ld t3, 0(%[marker])\n\t"
        "sd t3, 24(a3)\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        ".option pop\n\t"
        :
        : [first] "r"(first),
          [second] "r"(second),
          [marker] "r"(marker),
          [scalar_payload] "r"(scalar_payload),
          [marker_value] "r"(marker_value),
          [obs] "r"(obs)
        : "a1", "a2", "a3", "a4", "t0", "t1", "t2", "t3",
          "memory");
}

#pragma GCC push_options
#pragma GCC optimize ("no-tree-vectorize", "no-tree-loop-distribute-patterns")
bool ai_arch_cbozero_dispatch_younger_mixed_mem_owner_corner()
{
    TEST_START();

    const enum test_page data_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t base_off = 0x2400U;
    const uintptr_t target_vaddr = hs_page_base(data_page) + base_off;
    const uintptr_t guard_vaddr = target_vaddr + AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t first_b2b_vaddr =
        target_vaddr + 2U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t second_b2b_vaddr =
        target_vaddr + 3U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t vector_src_vaddr =
        target_vaddr + 4U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t mask_vaddr =
        target_vaddr + 5U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t target_paddr = phys_page_base(data_page) + base_off;
    const uintptr_t guard_paddr = target_paddr + AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t first_b2b_paddr =
        target_paddr + 2U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t second_b2b_paddr =
        target_paddr + 3U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t vector_src_paddr =
        target_paddr + 4U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t mask_paddr =
        target_paddr + 5U * AI_ARCH_CMO_DISPATCH_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2400U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2400U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t marker_main = 0xa1a2a3a4b1b2b3b4ULL;
    const uint64_t marker_b2b = 0xc5c6c7c8d5d6d7d8ULL;
    const uint64_t marker_guard = 0x8877665544332211ULL;
    const uint64_t scalar_payload = 0x1029384756abcdefULL;
    const uint8_t vector_mask[2] = { 0xadU, 0x53U };
    uint8_t vector_src[AI_ARCH_CMO_DISPATCH_VECTOR_BYTES];
    uint8_t expected_target[AI_ARCH_CMO_DISPATCH_BLOCK_BYTES];
    uint8_t expected_first[AI_ARCH_CMO_DISPATCH_BLOCK_BYTES];
    uint8_t expected_second[AI_ARCH_CMO_DISPATCH_BLOCK_BYTES];
    uint64_t main_obs[AI_ARCH_CMO_DISPATCH_MAIN_OBS];
    uint64_t b2b_obs[AI_ARCH_CMO_DISPATCH_B2B_OBS];
    unsigned bad_index = 0U;

    for (unsigned i = 0; i < AI_ARCH_CMO_DISPATCH_VECTOR_BYTES; ++i) {
        vector_src[i] = (uint8_t)(0x31U + (uint8_t)(i * 13U));
    }

    ai_arch_cmo_dispatch_fill_expected(expected_target, 0U);
    expected_target[1] = 0x81U;
    expected_target[2] = 0x80U;
    expected_target[3] = 0xffU;
    for (unsigned i = 0; i < AI_ARCH_CMO_DISPATCH_VECTOR_BYTES; ++i) {
        if (((vector_mask[i >> 3] >> (i & 7U)) & 1U) != 0U) {
            expected_target[16U + i] = vector_src[i];
        }
    }

    ai_arch_cmo_dispatch_fill_expected(expected_first, 0U);
    expected_first[0] = 0x5aU;
    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        expected_first[40U + i] =
            (uint8_t)(scalar_payload >> (8U * i));
    }

    ai_arch_cmo_dispatch_fill_expected(expected_second, 0U);
    expected_second[2] = 0x88U;
    expected_second[3] = 0x77U;

    goto_priv(PRIV_M);
    ai_arch_cmo_dispatch_fill_line(target_paddr, 0x41U);
    ai_arch_cmo_dispatch_fill_line(guard_paddr, 0x7bU);
    ai_arch_cmo_dispatch_fill_line(first_b2b_paddr, 0x93U);
    ai_arch_cmo_dispatch_fill_line(second_b2b_paddr, 0xb5U);
    ai_arch_cmo_dispatch_write_bytes(vector_src_paddr, vector_src,
        AI_ARCH_CMO_DISPATCH_VECTOR_BYTES);
    ai_arch_cmo_dispatch_write_bytes(mask_paddr, vector_mask,
        sizeof(vector_mask));
    write64(marker_paddr, marker_seed);
    write64(marker_guard_paddr, marker_guard);
    ai_arch_cmo_dispatch_fill_u64(main_obs, AI_ARCH_CMO_DISPATCH_MAIN_OBS,
        0xbad1000000000000ULL);
    ai_arch_cmo_dispatch_fill_u64(b2b_obs, AI_ARCH_CMO_DISPATCH_B2B_OBS,
        0xbad2000000000000ULL);
    ai_arch_cmo_dispatch_prepare_env();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_cmo_dispatch_main_episode(target_vaddr, vector_src_vaddr,
        mask_vaddr, marker_vaddr, marker_main, main_obs);
    TEST_ASSERT("CBO dispatch owner: CBO.ZERO followed immediately by Zcb byte/half reads, Zcb stores, masked vector store, vl/vstart capture, and marker store observes strict younger-memory program order",
        excpt.triggered == false &&
        main_obs[0] == 0ULL &&
        main_obs[1] == 0x81ULL &&
        main_obs[2] == ai_arch_cmo_dispatch_sext16(0xff80U) &&
        main_obs[3] == 0xff80ULL &&
        main_obs[4] == ai_arch_cmo_dispatch_load_le64(expected_target, 0U) &&
        main_obs[5] == AI_ARCH_CMO_DISPATCH_VECTOR_BYTES &&
        main_obs[6] == 0ULL &&
        main_obs[7] == ai_arch_cmo_dispatch_load_le64(expected_target, 16U) &&
        main_obs[8] == ai_arch_cmo_dispatch_load_le64(expected_target, 24U) &&
        main_obs[9] == marker_main
    );

    TEST_SETUP_EXCEPT();
    ai_arch_cmo_dispatch_back_to_back_episode(first_b2b_vaddr,
        second_b2b_vaddr, marker_vaddr, scalar_payload, marker_b2b,
        b2b_obs);
    TEST_ASSERT("CBO dispatch owner: back-to-back CBO.ZERO episodes cannot reuse stale finish generation, drop younger compressed/scalar stores, or lose the progress marker",
        excpt.triggered == false &&
        b2b_obs[0] == ai_arch_cmo_dispatch_load_le64(expected_first, 0U) &&
        b2b_obs[1] == scalar_payload &&
        b2b_obs[2] == ai_arch_cmo_dispatch_load_le64(expected_second, 0U) &&
        b2b_obs[3] == marker_b2b
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO dispatch owner: target line backing image is zero except younger compressed and masked-vector lanes, proving younger memory was not executed before the CBO.ZERO side effect",
        ai_arch_cmo_dispatch_phys_bytes_equal(target_paddr, expected_target,
            AI_ARCH_CMO_DISPATCH_BLOCK_BYTES, &bad_index),
        "first_bad_byte=%u actual=0x%02x expected=0x%02x",
        bad_index,
        (unsigned)read8(target_paddr + bad_index),
        (unsigned)expected_target[bad_index]
    );
    TEST_ASSERT("CBO dispatch owner: back-to-back zeroed lines keep only their post-CBO younger stores and do not cross-release or overwrite each other",
        ai_arch_cmo_dispatch_phys_bytes_equal(first_b2b_paddr, expected_first,
            AI_ARCH_CMO_DISPATCH_BLOCK_BYTES, &bad_index) &&
        ai_arch_cmo_dispatch_phys_bytes_equal(second_b2b_paddr,
            expected_second, AI_ARCH_CMO_DISPATCH_BLOCK_BYTES, &bad_index)
    );
    TEST_ASSERT("CBO dispatch owner: adjacent guard line, vector source, mask bytes, marker value, and marker guard remain independently observable after the mixed CBO episode",
        ai_arch_cmo_dispatch_phys_line_seed(guard_paddr, 0x7bU) &&
        read64(vector_src_paddr) ==
            ai_arch_cmo_dispatch_load_le64(vector_src, 0U) &&
        read8(mask_paddr) == vector_mask[0] &&
        read8(mask_paddr + 1U) == vector_mask[1] &&
        read64(marker_paddr) == marker_b2b &&
        read64(marker_guard_paddr) == marker_guard
    );
    TEST_ASSERT("CBO dispatch owner: no untouched b2b zero line byte remains seeded, catching a lost CBO.ZERO side effect rather than just lost younger stores",
        !ai_arch_cmo_dispatch_phys_line_seed(first_b2b_paddr, 0x93U) &&
        !ai_arch_cmo_dispatch_phys_line_seed(second_b2b_paddr, 0xb5U) &&
        !ai_arch_cmo_dispatch_phys_line_zero(first_b2b_paddr) &&
        !ai_arch_cmo_dispatch_phys_line_zero(second_b2b_paddr)
    );

    ai_arch_cmo_dispatch_reset_env();

    TEST_END();
}
#pragma GCC pop_options
