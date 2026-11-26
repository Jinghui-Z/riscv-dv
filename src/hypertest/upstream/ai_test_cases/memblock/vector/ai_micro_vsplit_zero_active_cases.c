#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_MICRO_VSPLIT_ZERO_BYTES 16U
#define AI_MICRO_VSPLIT_ZERO_ROUNDS 4U

static inline void ai_micro_vsplit_zero_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_micro_vsplit_zero_write_bytes(uintptr_t addr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_vsplit_zero_bytes_match(uintptr_t addr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_vsplit_zero_load_window(uintptr_t seed_src,
    uintptr_t zero_src, uintptr_t zero_dump, uintptr_t live_src,
    uintptr_t live_dump, uint64_t *zero_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed_src])\n\t"
        "li t1, 0\n\t"
        "vsetvli t0, t1, e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[zero_src])\n\t"
        "csrr %[zero_vl], vl\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[zero_dump])\n\t"
        "vle8.v v16, (%[live_src])\n\t"
        "vse8.v v16, (%[live_dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [zero_vl] "=&r"(*zero_vl)
        : [seed_src] "r"(seed_src),
          [zero_src] "r"(zero_src),
          [zero_dump] "r"(zero_dump),
          [live_src] "r"(live_src),
          [live_dump] "r"(live_dump),
          [vl] "r"((unsigned)AI_MICRO_VSPLIT_ZERO_BYTES)
        : "t0", "t1", "memory");
}

static inline void ai_micro_vsplit_zero_store_window(uintptr_t store_src,
    uintptr_t zero_dst, uintptr_t live_src, uintptr_t live_dst,
    uint64_t *zero_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[store_src])\n\t"
        "li t1, 0\n\t"
        "vsetvli t0, t1, e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[zero_dst])\n\t"
        "csrr %[zero_vl], vl\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v16, (%[live_src])\n\t"
        "vse8.v v16, (%[live_dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [zero_vl] "=&r"(*zero_vl)
        : [store_src] "r"(store_src),
          [zero_dst] "r"(zero_dst),
          [live_src] "r"(live_src),
          [live_dst] "r"(live_dst),
          [vl] "r"((unsigned)AI_MICRO_VSPLIT_ZERO_BYTES)
        : "t0", "t1", "memory");
}

bool ai_micro_hs_vsplit_vl0_load_store_no_ghost_mergebuffer_corner()
{
    TEST_START();

    static const uint8_t seed_patterns[AI_MICRO_VSPLIT_ZERO_ROUNDS]
        [AI_MICRO_VSPLIT_ZERO_BYTES] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
          0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f },
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x55, 0xaa, 0x44, 0xbb, 0x33, 0xcc, 0x22, 0xdd,
          0x11, 0xee, 0x00, 0xff, 0x99, 0x66, 0x88, 0x77 },
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 },
    };
    static const uint8_t live_patterns[AI_MICRO_VSPLIT_ZERO_ROUNDS]
        [AI_MICRO_VSPLIT_ZERO_BYTES] = {
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
        { 0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08,
          0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
        { 0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28,
          0x19, 0x0a, 0xfb, 0xec, 0xdd, 0xce, 0xbf, 0xa0 },
        { 0xc0, 0xb1, 0xa2, 0x93, 0x84, 0x75, 0x66, 0x57,
          0x48, 0x39, 0x2a, 0x1b, 0x0c, 0xfd, 0xee, 0xdf },
    };
    static const uint8_t zero_target_patterns[AI_MICRO_VSPLIT_ZERO_ROUNDS]
        [AI_MICRO_VSPLIT_ZERO_BYTES] = {
        { 0x6c, 0x5b, 0x4a, 0x39, 0x28, 0x17, 0x06, 0xf5,
          0xe4, 0xd3, 0xc2, 0xb1, 0xa0, 0x9f, 0x8e, 0x7d },
        { 0x80, 0x71, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17,
          0x08, 0xf9, 0xea, 0xdb, 0xcc, 0xbd, 0xae, 0x9f },
        { 0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7a,
          0x8b, 0x9c, 0xad, 0xbe, 0xcf, 0xd0, 0xe1, 0xf2 },
        { 0xfa, 0xeb, 0xdc, 0xcd, 0xbe, 0xaf, 0x90, 0x81,
          0x72, 0x63, 0x54, 0x45, 0x36, 0x27, 0x18, 0x09 },
    };
    const uintptr_t seed_src_vaddr = hs_page_base(SWITCH1) + 0x100U;
    const uintptr_t zero_load_vaddr = hs_page_base(VSX_GRWX) + 0x180U;
    const uintptr_t zero_load_dump_vaddr = hs_page_base(SCRATCHPAD) + 0x200U;
    const uintptr_t live_load_src_vaddr = hs_page_base(SWITCH2) + 0x280U;
    const uintptr_t live_load_dump_vaddr = hs_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t zero_store_src_vaddr = hs_page_base(SWITCH1) + 0x340U;
    const uintptr_t zero_store_dst_vaddr = hs_page_base(VSR_GRWX) + 0x180U;
    const uintptr_t live_store_src_vaddr = hs_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t live_store_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x440U;
    const uintptr_t seed_src_paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t zero_load_paddr = phys_page_base(VSX_GRWX) + 0x180U;
    const uintptr_t zero_load_dump_paddr = phys_page_base(SCRATCHPAD) + 0x200U;
    const uintptr_t live_load_src_paddr = phys_page_base(SWITCH2) + 0x280U;
    const uintptr_t live_load_dump_paddr = phys_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t zero_store_src_paddr = phys_page_base(SWITCH1) + 0x340U;
    const uintptr_t zero_store_dst_paddr = phys_page_base(VSR_GRWX) + 0x180U;
    const uintptr_t live_store_src_paddr = phys_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t live_store_dst_paddr = phys_page_base(SCRATCHPAD) + 0x440U;
    const uintptr_t zero_load_before = zero_load_dump_paddr - 1U;
    const uintptr_t zero_load_after =
        zero_load_dump_paddr + AI_MICRO_VSPLIT_ZERO_BYTES;
    const uintptr_t live_load_before = live_load_dump_paddr - 1U;
    const uintptr_t live_load_after =
        live_load_dump_paddr + AI_MICRO_VSPLIT_ZERO_BYTES;
    const uintptr_t zero_store_before = zero_store_dst_paddr - 1U;
    const uintptr_t zero_store_after =
        zero_store_dst_paddr + AI_MICRO_VSPLIT_ZERO_BYTES;
    const uintptr_t live_store_before = live_store_dst_paddr - 1U;
    const uintptr_t live_store_after =
        live_store_dst_paddr + AI_MICRO_VSPLIT_ZERO_BYTES;
    const uint8_t zero_load_before_sentinel = 0x35U;
    const uint8_t zero_load_after_sentinel = 0x53U;
    const uint8_t live_load_before_sentinel = 0x4aU;
    const uint8_t live_load_after_sentinel = 0xa4U;
    const uint8_t zero_store_before_sentinel = 0x27U;
    const uint8_t zero_store_after_sentinel = 0x72U;
    const uint8_t live_store_before_sentinel = 0x18U;
    const uint8_t live_store_after_sentinel = 0x81U;

    ai_micro_vsplit_zero_enable_vector_state();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_VSPLIT_ZERO_ROUNDS; ++round) {
        uint64_t zero_vl = 0xffffffffffffffffULL;

        goto_priv(PRIV_M);
        ai_micro_vsplit_zero_write_bytes(seed_src_paddr, seed_patterns[round],
            AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(zero_load_paddr,
            zero_target_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(zero_load_dump_paddr,
            zero_target_patterns[(round + 1U) % AI_MICRO_VSPLIT_ZERO_ROUNDS],
            AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(live_load_src_paddr,
            live_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(live_load_dump_paddr,
            zero_target_patterns[(round + 2U) % AI_MICRO_VSPLIT_ZERO_ROUNDS],
            AI_MICRO_VSPLIT_ZERO_BYTES);
        write8(zero_load_before, zero_load_before_sentinel);
        write8(zero_load_after, zero_load_after_sentinel);
        write8(live_load_before, live_load_before_sentinel);
        write8(live_load_after, live_load_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vsplit_zero_load_window(seed_src_vaddr, zero_load_vaddr,
            zero_load_dump_vaddr, live_load_src_vaddr, live_load_dump_vaddr,
            &zero_vl);
        TEST_ASSERT("vl=0 vector load to a no-read page retires as zero-active and the immediate live vector load still completes",
            excpt.triggered == false &&
            zero_vl == 0ULL &&
            CSRR(CSR_VL) == AI_MICRO_VSPLIT_ZERO_BYTES,
            "zero_vl=%lu final_vl=%lu", zero_vl, CSRR(CSR_VL)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("vl=0 vector load keeps the preloaded destination image while the live follow-up owns its dump",
            ai_micro_vsplit_zero_bytes_match(zero_load_dump_paddr,
                seed_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES) &&
            ai_micro_vsplit_zero_bytes_match(live_load_dump_paddr,
                live_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES) &&
            ai_micro_vsplit_zero_bytes_match(zero_load_paddr,
                zero_target_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES)
        );
        TEST_ASSERT("vl=0 vector load zero-active window preserves dump and live-followup guard bytes",
            read8(zero_load_before) == zero_load_before_sentinel &&
            read8(zero_load_after) == zero_load_after_sentinel &&
            read8(live_load_before) == live_load_before_sentinel &&
            read8(live_load_after) == live_load_after_sentinel
        );

        ai_micro_vsplit_zero_write_bytes(zero_store_src_paddr,
            seed_patterns[(round + 1U) % AI_MICRO_VSPLIT_ZERO_ROUNDS],
            AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(zero_store_dst_paddr,
            zero_target_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(live_store_src_paddr,
            live_patterns[(round + 1U) % AI_MICRO_VSPLIT_ZERO_ROUNDS],
            AI_MICRO_VSPLIT_ZERO_BYTES);
        ai_micro_vsplit_zero_write_bytes(live_store_dst_paddr,
            zero_target_patterns[(round + 3U) % AI_MICRO_VSPLIT_ZERO_ROUNDS],
            AI_MICRO_VSPLIT_ZERO_BYTES);
        write8(zero_store_before, zero_store_before_sentinel);
        write8(zero_store_after, zero_store_after_sentinel);
        write8(live_store_before, live_store_before_sentinel);
        write8(live_store_after, live_store_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_vsplit_zero_store_window(zero_store_src_vaddr,
            zero_store_dst_vaddr, live_store_src_vaddr, live_store_dst_vaddr,
            &zero_vl);
        TEST_ASSERT("vl=0 vector store to a no-write page retires as zero-active and the immediate live vector store still completes",
            excpt.triggered == false &&
            zero_vl == 0ULL &&
            CSRR(CSR_VL) == AI_MICRO_VSPLIT_ZERO_BYTES,
            "zero_vl=%lu final_vl=%lu", zero_vl, CSRR(CSR_VL)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("vl=0 vector store leaves the protected destination unchanged while the live follow-up writes only its payload",
            ai_micro_vsplit_zero_bytes_match(zero_store_dst_paddr,
                zero_target_patterns[round], AI_MICRO_VSPLIT_ZERO_BYTES) &&
            ai_micro_vsplit_zero_bytes_match(live_store_dst_paddr,
                live_patterns[(round + 1U) % AI_MICRO_VSPLIT_ZERO_ROUNDS],
                AI_MICRO_VSPLIT_ZERO_BYTES)
        );
        TEST_ASSERT("vl=0 vector store zero-active window preserves protected and live destination guard bytes",
            read8(zero_store_before) == zero_store_before_sentinel &&
            read8(zero_store_after) == zero_store_after_sentinel &&
            read8(live_store_before) == live_store_before_sentinel &&
            read8(live_store_after) == live_store_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
