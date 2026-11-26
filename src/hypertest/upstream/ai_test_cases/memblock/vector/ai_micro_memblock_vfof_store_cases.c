#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_MEMBLOCK_VFOF_STORE_BYTES 16U
#define AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES 8U
#define AI_MEMBLOCK_VFOF_STORE_ROUNDS 3U

static inline pte_t *ai_memblock_vfof_store_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_vfof_store_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_vfof_store_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_vfof_store_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_vfof_store_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_vfof_store_prefill(uintptr_t paddr,
    uint8_t fill, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, fill);
    }
}

static inline void ai_memblock_vle8ff_then_vse8_window(uintptr_t store_src,
    uintptr_t fof_src,
    uintptr_t store_dst,
    uintptr_t fof_dst,
    unsigned req_vl,
    unsigned *observed_fof_vl)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vle8.v v16, (%[store_src])\n\t"
        "vle8ff.v v8, (%[fof_src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vsetvl t0, %[req_vl], %[vtype]\n\t"
        "vse8.v v16, (%[store_dst])\n\t"
        "vsetvl t0, %[out_vl], %[vtype]\n\t"
        "vse8.v v8, (%[fof_dst])\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r" (captured_vl)
        : [store_src] "r" (store_src),
          [fof_src] "r" (fof_src),
          [store_dst] "r" (store_dst),
          [fof_dst] "r" (fof_dst),
          [req_vl] "r" (req_vl),
          [vtype] "r" (vtype)
        : "t0", "memory");

    *observed_fof_vl = captured_vl;
}

bool ai_micro_hs_vfof_partial_stop_then_vector_store_no_writeback_crossbind_corner()
{
    TEST_START();

    static const uint8_t fof_low_patterns[AI_MEMBLOCK_VFOF_STORE_ROUNDS]
        [AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES] = {
        { 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89 },
        { 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18 },
        { 0x5e, 0x4d, 0x3c, 0x2b, 0x1a, 0x09, 0xf8, 0xe7 },
    };
    static const uint8_t fof_blocked_patterns[AI_MEMBLOCK_VFOF_STORE_ROUNDS]
        [AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES] = {
        { 0x8a, 0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1 },
        { 0x19, 0x28, 0x37, 0x46, 0x55, 0x64, 0x73, 0x82 },
        { 0xf0, 0xd1, 0xb2, 0x93, 0x74, 0x55, 0x36, 0x17 },
    };
    static const uint8_t store_patterns[AI_MEMBLOCK_VFOF_STORE_ROUNDS]
        [AI_MEMBLOCK_VFOF_STORE_BYTES] = {
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
        { 0x7c, 0x6d, 0x5e, 0x4f, 0x30, 0x21, 0x12, 0x03,
          0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a },
    };
    const enum test_page fof_low_page = SWITCH1;
    const enum test_page fof_high_page = SWITCH2;
    const enum test_page store_src_page = VSRWX_GRWX;
    const enum test_page store_dst_page = SCRATCHPAD;
    const enum test_page fof_dst_page = VSRW_GRWX;
    const uintptr_t fof_src_vaddr = hs_page_base(fof_low_page) + PAGE_SIZE -
        AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES;
    const uintptr_t store_src_vaddr = hs_page_base(store_src_page) + 0x280U;
    const uintptr_t store_dst_vaddr = hs_page_base(store_dst_page) + 0x340U;
    const uintptr_t fof_dst_vaddr = hs_page_base(fof_dst_page) + 0x440U;
    const uintptr_t fof_low_tail_paddr = phys_page_base(fof_low_page) +
        PAGE_SIZE - AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES;
    const uintptr_t fof_high_head_paddr = phys_page_base(fof_high_page);
    const uintptr_t store_src_paddr = phys_page_base(store_src_page) + 0x280U;
    const uintptr_t store_dst_paddr = phys_page_base(store_dst_page) + 0x340U;
    const uintptr_t fof_dst_paddr = phys_page_base(fof_dst_page) + 0x440U;
    const uintptr_t store_before_paddr = store_dst_paddr - 1U;
    const uintptr_t store_after_paddr =
        store_dst_paddr + AI_MEMBLOCK_VFOF_STORE_BYTES;
    const uintptr_t fof_before_paddr = fof_dst_paddr - 1U;
    const uintptr_t fof_after_paddr =
        fof_dst_paddr + AI_MEMBLOCK_VFOF_STORE_BYTES;
    const uint8_t store_fill = 0x4aU;
    const uint8_t fof_fill = 0xc5U;
    const uint8_t store_before = 0x31U;
    const uint8_t store_after = 0x13U;
    const uint8_t fof_before = 0x6dU;
    const uint8_t fof_after = 0xd6U;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VFOF_STORE_ROUNDS; ++round) {
        unsigned observed_fof_vl = 0U;

        goto_priv(PRIV_M);
        ai_memblock_vfof_store_write_bytes(fof_low_tail_paddr,
            fof_low_patterns[round], AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES);
        ai_memblock_vfof_store_write_bytes(fof_high_head_paddr,
            fof_blocked_patterns[round],
            AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES);
        ai_memblock_vfof_store_write_bytes(store_src_paddr,
            store_patterns[round], AI_MEMBLOCK_VFOF_STORE_BYTES);
        ai_memblock_vfof_store_prefill(store_dst_paddr, store_fill,
            AI_MEMBLOCK_VFOF_STORE_BYTES);
        ai_memblock_vfof_store_prefill(fof_dst_paddr, fof_fill,
            AI_MEMBLOCK_VFOF_STORE_BYTES);
        write8(store_before_paddr, store_before);
        write8(store_after_paddr, store_after);
        write8(fof_before_paddr, fof_before);
        write8(fof_after_paddr, fof_after);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vfof_store_set_hs_leaf(fof_low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vfof_store_set_hs_leaf(fof_high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_vfof_store_set_hs_leaf(store_src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vfof_store_set_hs_leaf(store_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vfof_store_set_hs_leaf(fof_dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vle8ff_then_vse8_window(store_src_vaddr, fof_src_vaddr,
            store_dst_vaddr, fof_dst_vaddr, AI_MEMBLOCK_VFOF_STORE_BYTES,
            &observed_fof_vl);
        fence_iorw_iorw();

        TEST_ASSERT("partial VFOF followed by a normal vector store stops at the readable first page without raising a visible exception",
            excpt.triggered == false &&
            observed_fof_vl == AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the normal vector store after the partial VFOF writes its own full payload rather than a shortened or VFOF-sourced image",
            ai_memblock_vfof_store_bytes_match(store_dst_paddr,
                store_patterns[round], AI_MEMBLOCK_VFOF_STORE_BYTES)
        );
        TEST_ASSERT("the stored-out VFOF result still contains only the readable low-half bytes",
            ai_memblock_vfof_store_bytes_match(fof_dst_paddr,
                fof_low_patterns[round],
                AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES)
        );
        for (unsigned i = AI_MEMBLOCK_VFOF_STORE_PARTIAL_BYTES;
             i < AI_MEMBLOCK_VFOF_STORE_BYTES; ++i) {
            TEST_ASSERT("the blocked upper half of the VFOF store-out destination stays at its fill value",
                read8(fof_dst_paddr + i) == fof_fill
            );
        }
        TEST_ASSERT("partial VFOF plus normal vector-store completion preserves both destination guard bytes",
            read8(store_before_paddr) == store_before &&
            read8(store_after_paddr) == store_after &&
            read8(fof_before_paddr) == fof_before &&
            read8(fof_after_paddr) == fof_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
