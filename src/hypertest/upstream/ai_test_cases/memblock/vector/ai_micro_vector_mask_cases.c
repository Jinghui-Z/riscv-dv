#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_MICRO_VECTOR_MASK_BYTES 16U

static inline void ai_micro_vector_mask_write_bytes(uintptr_t addr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_vector_mask_bytes_match(uintptr_t addr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_masked_vle8_capture_then_followup(uintptr_t seed_src,
    uintptr_t masked_src,
    uintptr_t masked_dst,
    uintptr_t follow_src,
    uintptr_t follow_dst,
    unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle8.v v8, (%[masked_src]), v0.t\n\t"
        "vse8.v v8, (%[masked_dst])\n\t"
        "vle8.v v16, (%[follow_src])\n\t"
        "vse8.v v16, (%[follow_dst])\n\t"
        ".option pop\n\t"
        :
        : [seed_src] "r" (seed_src),
          [masked_src] "r" (masked_src),
          [masked_dst] "r" (masked_dst),
          [follow_src] "r" (follow_src),
          [follow_dst] "r" (follow_dst),
          [vl] "r" (vl),
          [vtype] "r" (vtype)
        : "t0", "memory");
}

static inline void ai_micro_masked_vse8_then_followup(uintptr_t store_src,
    uintptr_t masked_dst,
    uintptr_t follow_src,
    uintptr_t follow_dst,
    unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[store_src])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vse8.v v8, (%[masked_dst]), v0.t\n\t"
        "vle8.v v16, (%[follow_src])\n\t"
        "vse8.v v16, (%[follow_dst])\n\t"
        ".option pop\n\t"
        :
        : [store_src] "r" (store_src),
          [masked_dst] "r" (masked_dst),
          [follow_src] "r" (follow_src),
          [follow_dst] "r" (follow_dst),
          [vl] "r" (vl),
          [vtype] "r" (vtype)
        : "t0", "memory");
}

static bool ai_micro_masked_unitstride_load_all_inactive_window(void)
{
    static const uint8_t seed_patterns[][AI_MICRO_VECTOR_MASK_BYTES] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
          0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f },
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x55, 0xaa, 0x44, 0xbb, 0x33, 0xcc, 0x22, 0xdd,
          0x11, 0xee, 0x00, 0xff, 0x99, 0x66, 0x88, 0x77 },
    };
    static const uint8_t bad_src_patterns[][AI_MICRO_VECTOR_MASK_BYTES] = {
        { 0xc1, 0xd2, 0xe3, 0xf4, 0x15, 0x26, 0x37, 0x48,
          0x59, 0x6a, 0x7b, 0x8c, 0x9d, 0xae, 0xbf, 0xc0 },
        { 0x1f, 0x2e, 0x3d, 0x4c, 0x5b, 0x6a, 0x79, 0x88,
          0x97, 0xa6, 0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x00 },
        { 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d, 0x1c,
          0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5, 0x94 },
    };
    static const uint8_t follow_patterns[][AI_MICRO_VECTOR_MASK_BYTES] = {
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
        { 0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08,
          0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
        { 0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28,
          0x19, 0x0a, 0xfb, 0xec, 0xdd, 0xce, 0xbf, 0xa0 },
    };
    const uintptr_t seed_src_vaddr = hs_page_base(SWITCH1) + 0x100U;
    const uintptr_t masked_src_vaddr = hs_page_base(VSX_GRWX) + 0x180U;
    const uintptr_t masked_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x200U;
    const uintptr_t follow_src_vaddr = hs_page_base(SWITCH2) + 0x280U;
    const uintptr_t follow_dst_vaddr = hs_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t seed_src_paddr = phys_page_base(SWITCH1) + 0x100U;
    const uintptr_t masked_src_paddr = phys_page_base(VSX_GRWX) + 0x180U;
    const uintptr_t masked_dst_paddr = phys_page_base(SCRATCHPAD) + 0x200U;
    const uintptr_t follow_src_paddr = phys_page_base(SWITCH2) + 0x280U;
    const uintptr_t follow_dst_paddr = phys_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t masked_before = masked_dst_paddr - 1U;
    const uintptr_t masked_after =
        masked_dst_paddr + AI_MICRO_VECTOR_MASK_BYTES;
    const uintptr_t follow_before = follow_dst_paddr - 1U;
    const uintptr_t follow_after =
        follow_dst_paddr + AI_MICRO_VECTOR_MASK_BYTES;
    const uint8_t masked_before_sentinel = 0x35U;
    const uint8_t masked_after_sentinel = 0x53U;
    const uint8_t follow_before_sentinel = 0x4aU;
    const uint8_t follow_after_sentinel = 0xa4U;
    const unsigned rounds =
        sizeof(seed_patterns) / sizeof(seed_patterns[0]);
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_vector_mask_write_bytes(seed_src_paddr, seed_patterns[round],
            AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(masked_src_paddr,
            bad_src_patterns[round], AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(masked_dst_paddr,
            bad_src_patterns[(round + 1U) % rounds], AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(follow_src_paddr, follow_patterns[round],
            AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(follow_dst_paddr,
            seed_patterns[(round + 2U) % rounds], AI_MICRO_VECTOR_MASK_BYTES);
        write8(masked_before, masked_before_sentinel);
        write8(masked_after, masked_after_sentinel);
        write8(follow_before, follow_before_sentinel);
        write8(follow_after, follow_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_masked_vle8_capture_then_followup(seed_src_vaddr,
            masked_src_vaddr, masked_dst_vaddr, follow_src_vaddr,
            follow_dst_vaddr, AI_MICRO_VECTOR_MASK_BYTES);
        fence_iorw_iorw();
        TEST_ASSERT("the all-inactive masked unit-stride vector load through an unreadable page retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the all-inactive masked unit-stride vector load preserves the preloaded destination payload and the immediate live follow-up vector load still writes only its own payload",
            ai_micro_vector_mask_bytes_match(masked_dst_paddr,
                seed_patterns[round], AI_MICRO_VECTOR_MASK_BYTES) &&
            ai_micro_vector_mask_bytes_match(follow_dst_paddr,
                follow_patterns[round], AI_MICRO_VECTOR_MASK_BYTES)
        );
        TEST_ASSERT("the masked-load inactive-path probe also preserves adjacent sentinel bytes around both writeback destinations",
            read8(masked_before) == masked_before_sentinel &&
            read8(masked_after) == masked_after_sentinel &&
            read8(follow_before) == follow_before_sentinel &&
            read8(follow_after) == follow_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_masked_unitstride_store_all_inactive_window(void)
{
    static const uint8_t store_patterns[][AI_MICRO_VECTOR_MASK_BYTES] = {
        { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x16, 0x27,
          0x38, 0x49, 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf },
        { 0x5f, 0x4e, 0x3d, 0x2c, 0x1b, 0x0a, 0xf9, 0xe8,
          0xd7, 0xc6, 0xb5, 0xa4, 0x93, 0x82, 0x71, 0x60 },
        { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
          0x0f, 0xed, 0xcb, 0xa9, 0x87, 0x65, 0x43, 0x21 },
    };
    static const uint8_t old_target_patterns[][AI_MICRO_VECTOR_MASK_BYTES] = {
        { 0x6c, 0x5b, 0x4a, 0x39, 0x28, 0x17, 0x06, 0xf5,
          0xe4, 0xd3, 0xc2, 0xb1, 0xa0, 0x9f, 0x8e, 0x7d },
        { 0x80, 0x71, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17,
          0x08, 0xf9, 0xea, 0xdb, 0xcc, 0xbd, 0xae, 0x9f },
        { 0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7a,
          0x8b, 0x9c, 0xad, 0xbe, 0xcf, 0xd0, 0xe1, 0xf2 },
    };
    static const uint8_t follow_patterns[][AI_MICRO_VECTOR_MASK_BYTES] = {
        { 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a,
          0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02 },
        { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
          0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
        { 0x44, 0x11, 0x55, 0x22, 0x66, 0x33, 0x77, 0x88,
          0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00 },
    };
    const uintptr_t store_src_vaddr = hs_page_base(SWITCH1) + 0x380U;
    const uintptr_t masked_dst_vaddr = hs_page_base(VSR_GRWX) + 0x180U;
    const uintptr_t follow_src_vaddr = hs_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t follow_dst_vaddr = hs_page_base(SCRATCHPAD) + 0x440U;
    const uintptr_t store_src_paddr = phys_page_base(SWITCH1) + 0x380U;
    const uintptr_t masked_dst_paddr = phys_page_base(VSR_GRWX) + 0x180U;
    const uintptr_t follow_src_paddr = phys_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t follow_dst_paddr = phys_page_base(SCRATCHPAD) + 0x440U;
    const uintptr_t masked_before = masked_dst_paddr - 1U;
    const uintptr_t masked_after =
        masked_dst_paddr + AI_MICRO_VECTOR_MASK_BYTES;
    const uintptr_t follow_before = follow_dst_paddr - 1U;
    const uintptr_t follow_after =
        follow_dst_paddr + AI_MICRO_VECTOR_MASK_BYTES;
    const uint8_t masked_before_sentinel = 0x27U;
    const uint8_t masked_after_sentinel = 0x72U;
    const uint8_t follow_before_sentinel = 0x18U;
    const uint8_t follow_after_sentinel = 0x81U;
    const unsigned rounds =
        sizeof(store_patterns) / sizeof(store_patterns[0]);
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        goto_priv(PRIV_M);
        ai_micro_vector_mask_write_bytes(store_src_paddr, store_patterns[round],
            AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(masked_dst_paddr,
            old_target_patterns[round], AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(follow_src_paddr, follow_patterns[round],
            AI_MICRO_VECTOR_MASK_BYTES);
        ai_micro_vector_mask_write_bytes(follow_dst_paddr,
            old_target_patterns[(round + 1U) % rounds],
            AI_MICRO_VECTOR_MASK_BYTES);
        write8(masked_before, masked_before_sentinel);
        write8(masked_after, masked_after_sentinel);
        write8(follow_before, follow_before_sentinel);
        write8(follow_after, follow_after_sentinel);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_masked_vse8_then_followup(store_src_vaddr, masked_dst_vaddr,
            follow_src_vaddr, follow_dst_vaddr, AI_MICRO_VECTOR_MASK_BYTES);
        fence_iorw_iorw();
        TEST_ASSERT("the all-inactive masked unit-stride vector store to an unwritable page retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the all-inactive masked unit-stride vector store leaves the protected target bytes untouched and the immediate live follow-up vector store still writes only its own payload",
            ai_micro_vector_mask_bytes_match(masked_dst_paddr,
                old_target_patterns[round], AI_MICRO_VECTOR_MASK_BYTES) &&
            ai_micro_vector_mask_bytes_match(follow_dst_paddr,
                follow_patterns[round], AI_MICRO_VECTOR_MASK_BYTES)
        );
        TEST_ASSERT("the masked-store inactive-path probe also preserves adjacent sentinel bytes around both the protected target and the valid follow-up destination",
            read8(masked_before) == masked_before_sentinel &&
            read8(masked_after) == masked_after_sentinel &&
            read8(follow_before) == follow_before_sentinel &&
            read8(follow_after) == follow_after_sentinel
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

bool ai_micro_hs_vsplit_masked_unitstride_load_all_inactive_no_fault_or_dest_clobber_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_masked_unitstride_load_all_inactive_window();
    TEST_ASSERT("all-inactive masked unit-stride vector load no-access window completes every no-fault, no-clobber, and live-followup subcheck",
        test_status);
    TEST_END();
}

bool ai_micro_hs_vsplit_masked_unitstride_store_all_inactive_no_fault_or_side_effect_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_masked_unitstride_store_all_inactive_window();
    TEST_ASSERT("all-inactive masked unit-stride vector store no-access window completes every no-fault, no-side-effect, and live-followup subcheck",
        test_status);
    TEST_END();
}
