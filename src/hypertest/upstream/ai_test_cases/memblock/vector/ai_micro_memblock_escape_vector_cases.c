#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#ifndef CSR_SMBLOCKCTL
#define CSR_SMBLOCKCTL 0x5c3
#endif

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_ESCAPE_VECTOR_ENABLE    (1ULL << 0)
#define AI_MEMBLOCK_ESCAPE_VECTOR_TIME_MASK (0xFULL << 1)
#define AI_MEMBLOCK_ESCAPE_VECTOR_BYTES 16U
#define AI_MEMBLOCK_ESCAPE_VECTOR_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static inline pte_t *ai_memblock_escape_vector_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_escape_vector_map_alias_to_base(
    enum test_page alias_page, enum test_page base_page)
{
    *ai_memblock_escape_vector_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline uint64_t ai_memblock_escape_vector_read_smblockctl(void)
{
    return CSRR(CSR_SMBLOCKCTL);
}

static inline void ai_memblock_escape_vector_write_smblockctl(uint64_t value)
{
    CSRW(CSR_SMBLOCKCTL, value);
}

static inline uint64_t ai_memblock_escape_vector_compose_smblockctl(
    uint64_t original, uint64_t threshold_bits)
{
    return (original & ~AI_MEMBLOCK_ESCAPE_VECTOR_TIME_MASK) |
        AI_MEMBLOCK_ESCAPE_VECTOR_ENABLE | threshold_bits;
}

static inline void ai_memblock_escape_vector_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_escape_vector_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_escape_vector_store_vload_between_alias_loads(
    uintptr_t store_vaddr,
    uintptr_t load0_vaddr,
    uintptr_t load1_vaddr,
    uintptr_t vec_src_vaddr,
    uintptr_t vec_dst_vaddr,
    uint64_t store_value,
    uint64_t *load0_out,
    uint64_t *load1_out)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    uint64_t load0_value = 0ULL;
    uint64_t load1_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0xfedcba9876543210\n\t"
        "li t1, 7\n\t"
        "divu t2, t0, t1\n\t"
        "addi t2, t2, 1\n\t"
        "divu t2, t0, t2\n\t"
        "andi t2, t2, 0\n\t"
        "add t2, t2, %[store]\n\t"
        "sd %[value], 0(t2)\n\t"
        "ld %[load0_value], 0(%[load0])\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[vec_src])\n\t"
        "vse8.v v8, (%[vec_dst])\n\t"
        "ld %[load1_value], 0(%[load1])\n\t"
        ".option pop\n\t"
        : [load0_value] "=&r"(load0_value),
          [load1_value] "=&r"(load1_value)
        : [store] "r"(store_vaddr),
          [load0] "r"(load0_vaddr),
          [load1] "r"(load1_vaddr),
          [vec_src] "r"(vec_src_vaddr),
          [vec_dst] "r"(vec_dst_vaddr),
          [value] "r"(store_value),
          [vl] "r"((int)AI_MEMBLOCK_ESCAPE_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "t1", "t2", "memory");

    *load0_out = load0_value;
    *load1_out = load1_value;
}

bool ai_micro_hs_replay_escape_unrelated_vector_wb_does_not_clear_scalar_escape_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const enum test_page vec_src_page = VSRW_GRWX;
    const enum test_page vec_dst_page = VSRW_GURWX;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x2c0U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x2c0U;
    const uintptr_t load0_vaddr = hs_page_base(load0_page) + 0x2c0U;
    const uintptr_t load1_vaddr = hs_page_base(load1_page) + 0x2c0U;
    const uintptr_t adjacent_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t vec_src_vaddr = hs_page_base(vec_src_page) + 0x440U;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_dst_page) + 0x540U;
    const uintptr_t base_paddr = phys_page_base(base_page) + 0x2c0U;
    const uintptr_t adjacent_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t vec_src_paddr = phys_page_base(vec_src_page) + 0x440U;
    const uintptr_t vec_dst_paddr = phys_page_base(vec_dst_page) + 0x540U;
    const uintptr_t vec_dst_before_paddr = vec_dst_paddr - 1U;
    const uintptr_t vec_dst_after_paddr =
        vec_dst_paddr + AI_MEMBLOCK_ESCAPE_VECTOR_BYTES;
    const uint64_t initial_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t store_values[] = {
        0x5566778899aabbccULL,
        0xa1b2c3d4e5f60718ULL,
    };
    const uint64_t adjacent_values[] = {
        0x99aabbccddeeff00ULL,
        0x0123456789abcdefULL,
    };
    const uint8_t vec_patterns[][AI_MEMBLOCK_ESCAPE_VECTOR_BYTES] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
          0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f },
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
    };
    const uint8_t vec_dst_fill = 0x5aU;
    const uint8_t vec_dst_before = 0x6cU;
    const uint8_t vec_dst_after = 0xc6U;
    uint64_t original_smblockctl = 0ULL;
    uint64_t original_menvcfg = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    TEST_SETUP_EXCEPT();
    original_smblockctl = ai_memblock_escape_vector_read_smblockctl();
    TEST_ASSERT("smblockctl is readable before enabling replay_escape for the vector-writeback escapeSuccess probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_memblock_escape_vector_write_smblockctl(
        ai_memblock_escape_vector_compose_smblockctl(original_smblockctl,
            0x1ULL << 1));
    TEST_ASSERT("replay_escape can be enabled with threshold=1 before the unrelated-vector-writeback scalar replay probe",
        excpt.triggered == false
    );

    CSRW(CSR_MENVCFG, original_menvcfg | MENVCFG_CBIE01);
    hspt_init();
    ai_memblock_escape_vector_map_alias_to_base(store_page, base_page);
    ai_memblock_escape_vector_map_alias_to_base(load0_page, base_page);
    ai_memblock_escape_vector_map_alias_to_base(load1_page, base_page);
    sfence_vma();

    for (size_t round = 0;
         round < AI_MEMBLOCK_ESCAPE_VECTOR_ARRAY_SIZE(store_values);
         ++round) {
        uint64_t load0_value = 0ULL;
        uint64_t load1_value = 0ULL;

        goto_priv(PRIV_M);
        write64(base_paddr, initial_values[round]);
        write64(adjacent_paddr, adjacent_values[round]);
        ai_memblock_escape_vector_write_bytes(vec_src_paddr,
            vec_patterns[round], AI_MEMBLOCK_ESCAPE_VECTOR_BYTES);
        for (unsigned i = 0; i < AI_MEMBLOCK_ESCAPE_VECTOR_BYTES; ++i) {
            write8(vec_dst_paddr + i, vec_dst_fill);
        }
        write8(vec_dst_before_paddr, vec_dst_before);
        write8(vec_dst_after_paddr, vec_dst_after);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_base = ld(base_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the scalar replay plus vector writeback probe starts, the same-PA base alias is readable with the seeded payload",
            warmed_base == initial_values[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(base_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("the same-PA scalar replay target can be invalidated before inserting the unrelated vector writeback",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the fence after invalidating the scalar replay target retires before the vector-writeback escapeSuccess probe",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_escape_vector_store_vload_between_alias_loads(
            store_vaddr, load0_vaddr, load1_vaddr,
            vec_src_vaddr, vec_dst_vaddr, store_values[round],
            &load0_value, &load1_value);
        AI_ASSERT_NO_EXCEPT_AND("with replay_escape enabled, an unrelated vector load/store writeback between two same-PA scalar alias loads must not clear scalar escape state early or expose stale data",
            load0_value == store_values[round] &&
            load1_value == store_values[round]
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the mixed scalar-replay/vector-writeback window retires without exception",
            true
        );

        TEST_SETUP_EXCEPT();
        const uint64_t observed_base = ld(base_vaddr);
        const uint64_t observed_load0 = ld(load0_vaddr);
        const uint64_t observed_load1 = ld(load1_vaddr);
        const uint64_t observed_adjacent = ld(adjacent_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("all same-PA aliases reload the current scalar store payload after the unrelated vector writeback, while the adjacent word remains unchanged",
            observed_base == store_values[round] &&
            observed_load0 == store_values[round] &&
            observed_load1 == store_values[round] &&
            observed_adjacent == adjacent_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the scalar replay plus unrelated vector writeback window keeps only the current scalar store, vector copy, and sentinels",
            read64(base_paddr) == store_values[round] &&
            read64(adjacent_paddr) == adjacent_values[round] &&
            ai_memblock_escape_vector_bytes_match(vec_dst_paddr,
                vec_patterns[round], AI_MEMBLOCK_ESCAPE_VECTOR_BYTES) &&
            read8(vec_dst_before_paddr) == vec_dst_before &&
            read8(vec_dst_after_paddr) == vec_dst_after
        );
    }

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_memblock_escape_vector_write_smblockctl(original_smblockctl);
    TEST_ASSERT("smblockctl is restored after the vector-writeback escapeSuccess probe",
        excpt.triggered == false
    );
    CSRW(CSR_MENVCFG, original_menvcfg);

    TEST_END();
}
