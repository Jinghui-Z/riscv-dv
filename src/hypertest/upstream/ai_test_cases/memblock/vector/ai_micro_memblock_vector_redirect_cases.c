#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_VEC_REDIRECT_BYTES 16U
#define AI_MEMBLOCK_VEC_REDIRECT_ROUNDS 2U
#define AI_MEMBLOCK_VEC_REDIRECT_TRAIN_ROUNDS 17U
#define AI_MEMBLOCK_VEC_REDIRECT_PROBE_ROUNDS 5U
#define AI_MEMBLOCK_VLQ_REDIRECT_ROUNDS 3U
#define AI_MEMBLOCK_VSEG_REDIRECT_FIELDS 2U
#define AI_MEMBLOCK_VSEG_REDIRECT_VL 8U
#define AI_MEMBLOCK_VSEG_REDIRECT_BYTES \
    (AI_MEMBLOCK_VSEG_REDIRECT_FIELDS * AI_MEMBLOCK_VSEG_REDIRECT_VL)

static inline pte_t *ai_memblock_vec_redirect_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_vec_redirect_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_memblock_vec_redirect_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_vec_redirect_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline void ai_memblock_vec_redirect_prefill(uintptr_t paddr,
    uint8_t fill,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, fill);
    }
}

static inline bool ai_memblock_vec_redirect_bytes_match(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_vec_redirect_fill_matches(uintptr_t paddr,
    uint8_t fill,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != fill) {
            return false;
        }
    }

    return true;
}

static __attribute__((noinline)) void
ai_memblock_wrong_path_vector_branch_window(
    uintptr_t cond_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uintptr_t rounds)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 1\n\t"
        "ld t1, 0(%[cond_addr])\n\t"
        "beqz t1, 2f\n\t"
        "vsetvl t2, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "2:\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [cond_addr] "r"(cond_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [rounds] "r"(rounds),
          [vl] "r"((int)AI_MEMBLOCK_VEC_REDIRECT_BYTES),
          [vtype] "r"(vtype)
        : "t0", "t1", "t2", "memory");
}

static inline void ai_memblock_vector_copy_window(uintptr_t src_vaddr,
    uintptr_t dst_vaddr)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [vl] "r"((int)AI_MEMBLOCK_VEC_REDIRECT_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static __attribute__((noinline)) void
ai_memblock_vlq_near_full_redirect_window(
    uintptr_t cond_vaddr,
    uintptr_t wrong_src_vaddr,
    uintptr_t good_src_vaddr,
    uintptr_t dump_vaddr,
    uintptr_t marker_vaddr,
    uintptr_t poison_vaddr,
    uint64_t marker_value,
    uint64_t poison_value)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t1, 0(%[cond_addr])\n\t"
        "beqz t1, 1f\n\t"
        "vsetvl t2, %[vl], %[vtype]\n\t"
        "mv t3, %[wrong_src]\n\t"
        "vle8.v v8, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v9, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v10, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v11, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v12, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v13, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v14, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v15, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v16, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v17, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v18, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v19, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v20, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v21, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v22, (t3)\n\t"
        "addi t3, t3, 16\n\t"
        "vle8.v v23, (t3)\n\t"
        "sd %[poison_value], 0(%[poison_addr])\n\t"
        "1:\n\t"
        "vsetvl t2, %[vl], %[vtype]\n\t"
        "vle8.v v24, (%[good_src])\n\t"
        "vse8.v v24, (%[dump_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [cond_addr] "r"(cond_vaddr),
          [wrong_src] "r"(wrong_src_vaddr),
          [good_src] "r"(good_src_vaddr),
          [dump_addr] "r"(dump_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [poison_addr] "r"(poison_vaddr),
          [marker_value] "r"(marker_value),
          [poison_value] "r"(poison_value),
          [vl] "r"((int)AI_MEMBLOCK_VEC_REDIRECT_BYTES),
          [vtype] "r"(vtype)
        : "t1", "t2", "t3", "memory");
}

static __attribute__((noinline)) void
ai_memblock_wrong_path_vsegment_branch_window(
    uintptr_t cond_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uintptr_t rounds)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 1\n\t"
        "ld t1, 0(%[cond_addr])\n\t"
        "beqz t1, 2f\n\t"
        "vsetvl t2, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src_addr])\n\t"
        "vsseg2e8.v v8, (%[dst_addr])\n\t"
        "2:\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "bltu t0, %[rounds], 1b\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [cond_addr] "r"(cond_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [rounds] "r"(rounds),
          [vl] "r"((int)AI_MEMBLOCK_VSEG_REDIRECT_VL),
          [vtype] "r"(vtype)
        : "t0", "t1", "t2", "memory");
}

static inline void ai_memblock_vsegment_copy_window(uintptr_t src_vaddr,
    uintptr_t dst_vaddr)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vlseg2e8.v v8, (%[src_addr])\n\t"
        "vsseg2e8.v v8, (%[dst_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [vl] "r"((int)AI_MEMBLOCK_VSEG_REDIRECT_VL),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_wrong_path_vector_load_store_redirect_no_spurious_fault_or_payload_leak_corner()
{
    TEST_START();

    static const uint8_t src_patterns[AI_MEMBLOCK_VEC_REDIRECT_ROUNDS]
        [AI_MEMBLOCK_VEC_REDIRECT_BYTES] = {
        { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
          0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xf0, 0x0f },
        { 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13,
          0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b },
    };
    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page control_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x780U;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x880U;
    const uintptr_t control_vaddr = hs_page_base(control_page) + 0xa00U;
    const uintptr_t marker_vaddr = hs_page_base(control_page) + 0xa40U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x780U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x880U;
    const uintptr_t control_paddr = phys_page_base(control_page) + 0xa00U;
    const uintptr_t marker_paddr = phys_page_base(control_page) + 0xa40U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_VEC_REDIRECT_BYTES;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t dst_fills[] = { 0x5aU, 0xa5U };
    const uint8_t dst_before[] = { 0x6cU, 0xc6U };
    const uint8_t dst_after[] = { 0x3dU, 0xd3U };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
    };
    const uint64_t marker_training[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_probe[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VEC_REDIRECT_ROUNDS;
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_vec_redirect_write_bytes(src_paddr, src_patterns[round],
            AI_MEMBLOCK_VEC_REDIRECT_BYTES);
        ai_memblock_vec_redirect_prefill(dst_paddr, dst_fills[round],
            AI_MEMBLOCK_VEC_REDIRECT_BYTES);
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);
        write64(control_paddr, 1ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vec_redirect_set_hs_leaf(src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(control_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_wrong_path_vector_branch_window(control_vaddr, src_vaddr,
            dst_vaddr, marker_vaddr, marker_training[round],
            AI_MEMBLOCK_VEC_REDIRECT_TRAIN_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("fall-through normal vector load/store training retires cleanly before the redirect probe",
            ld(marker_vaddr) == marker_training[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("training vector load/store writes the expected payload and preserves destination guards",
            ai_memblock_vec_redirect_bytes_match(dst_paddr,
                src_patterns[round], AI_MEMBLOCK_VEC_REDIRECT_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );

        ai_memblock_vec_redirect_prefill(dst_paddr, dst_fills[round],
            AI_MEMBLOCK_VEC_REDIRECT_BYTES);
        write64(control_paddr, 0ULL);
        write64(marker_paddr, marker_seeds[round]);
        ai_memblock_vec_redirect_set_hs_leaf(src_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_vec_redirect_set_hs_leaf(dst_page,
            PTE_V | PTE_AD | PTE_X);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_wrong_path_vector_branch_window(control_vaddr, src_vaddr,
            dst_vaddr, marker_vaddr, marker_probe[round],
            AI_MEMBLOCK_VEC_REDIRECT_PROBE_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("taken-branch redirect kills wrong-path normal vector load/store before no-R/no-W leaves can fault or leak payload",
            ld(marker_vaddr) == marker_probe[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("wrong-path normal vector load/store leaves the denied destination, marker guard, and destination guards unchanged except for the taken-path marker",
            ai_memblock_vec_redirect_fill_matches(dst_paddr,
                dst_fills[round], AI_MEMBLOCK_VEC_REDIRECT_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round] &&
            read64(marker_paddr) == marker_probe[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_vec_redirect_set_hs_leaf(src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vector_copy_window(src_vaddr, dst_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after repairing both leaves, a real normal vector load/store uses fresh state and completes without stale redirect metadata",
            true
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("valid repaired normal vector load/store writes only the current source payload while preserving all guards",
            ai_memblock_vec_redirect_bytes_match(dst_paddr,
                src_patterns[round], AI_MEMBLOCK_VEC_REDIRECT_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round] &&
            read64(marker_paddr) == marker_probe[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_virtual_lq_near_full_wrong_path_redirect_recovery_no_ghost_full_corner()
{
    TEST_START();

    static const uint8_t good_patterns[AI_MEMBLOCK_VLQ_REDIRECT_ROUNDS]
        [AI_MEMBLOCK_VEC_REDIRECT_BYTES] = {
        { 0x41, 0x52, 0x63, 0x74, 0x85, 0x96, 0xa7, 0xb8,
          0xc9, 0xda, 0xeb, 0xfc, 0x0d, 0x1e, 0x2f, 0x30 },
        { 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a, 0x39, 0x28,
          0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2, 0xb1, 0xa0 },
        { 0x13, 0x35, 0x57, 0x79, 0x9b, 0xbd, 0xdf, 0xf1,
          0xe2, 0xc4, 0xa6, 0x88, 0x6a, 0x4c, 0x2e, 0x10 },
    };
    const enum test_page wrong_page = SWITCH1;
    const enum test_page good_page = SWITCH2;
    const enum test_page dump_page = VSRW_GRW;
    const enum test_page control_page = VSRWX_GRWX;
    const uintptr_t wrong_src_vaddr = hs_page_base(wrong_page) + 0x100U;
    const uintptr_t good_src_vaddr = hs_page_base(good_page) + 0x300U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x500U;
    const uintptr_t control_vaddr = hs_page_base(control_page) + 0x700U;
    const uintptr_t marker_vaddr = hs_page_base(control_page) + 0x740U;
    const uintptr_t poison_vaddr = hs_page_base(control_page) + 0x780U;
    const uintptr_t wrong_src_paddr = phys_page_base(wrong_page) + 0x100U;
    const uintptr_t good_src_paddr = phys_page_base(good_page) + 0x300U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x500U;
    const uintptr_t control_paddr = phys_page_base(control_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(control_page) + 0x740U;
    const uintptr_t poison_paddr = phys_page_base(control_page) + 0x780U;
    const uintptr_t dump_before_paddr = dump_paddr - 1U;
    const uintptr_t dump_after_paddr =
        dump_paddr + AI_MEMBLOCK_VEC_REDIRECT_BYTES;
    const uint64_t marker_seed = 0x0101010101010101ULL;
    const uint64_t poison_seed = 0x0202020202020202ULL;
    const uint8_t dump_fill = 0xa6U;
    const uint8_t dump_before = 0x6aU;
    const uint8_t dump_after = 0xa9U;

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VLQ_REDIRECT_ROUNDS;
         ++round) {
        const uint64_t training_marker =
            0x1337000000001000ULL + (uint64_t)round;
        const uint64_t training_poison =
            0xfeed000000002000ULL + (uint64_t)round;
        const uint64_t probe_marker =
            0x3559000000003000ULL + (uint64_t)round;
        const uint64_t probe_poison =
            0xbeef000000004000ULL + (uint64_t)round;
        uint8_t wrong_fill = (uint8_t)(0x20U + round * 0x31U);

        goto_priv(PRIV_M);
        ai_memblock_vec_redirect_prefill(wrong_src_paddr, wrong_fill, 272U);
        ai_memblock_vec_redirect_write_bytes(good_src_paddr,
            good_patterns[round], AI_MEMBLOCK_VEC_REDIRECT_BYTES);
        ai_memblock_vec_redirect_prefill(dump_paddr, dump_fill,
            AI_MEMBLOCK_VEC_REDIRECT_BYTES);
        write8(dump_before_paddr, dump_before);
        write8(dump_after_paddr, dump_after);
        write64(control_paddr, 1ULL);
        write64(marker_paddr, marker_seed);
        write64(poison_paddr, poison_seed);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vec_redirect_set_hs_leaf(wrong_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(good_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(dump_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(control_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vlq_near_full_redirect_window(control_vaddr,
            wrong_src_vaddr, good_src_vaddr, dump_vaddr, marker_vaddr,
            poison_vaddr, training_marker, training_poison);
        AI_ASSERT_NO_EXCEPT_AND("P1-230 not-taken training executes the dense vector-load window and the following real vector load without trapping",
            ld(marker_vaddr) == training_marker
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("P1-230 training leaves the real vector dump bound to the current good payload and the wrong-path poison visibly trained",
            ai_memblock_vec_redirect_bytes_match(dump_paddr,
                good_patterns[round], AI_MEMBLOCK_VEC_REDIRECT_BYTES) &&
            read64(poison_paddr) == training_poison &&
            read8(dump_before_paddr) == dump_before &&
            read8(dump_after_paddr) == dump_after
        );

        ai_memblock_vec_redirect_prefill(dump_paddr, dump_fill,
            AI_MEMBLOCK_VEC_REDIRECT_BYTES);
        write64(control_paddr, 0ULL);
        write64(marker_paddr, marker_seed);
        write64(poison_paddr, poison_seed);

        goto_priv(PRIV_HS);
        ai_memblock_vec_redirect_set_hs_leaf(wrong_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_vec_redirect_set_hs_leaf(good_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(dump_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(control_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vlq_near_full_redirect_window(control_vaddr,
            wrong_src_vaddr, good_src_vaddr, dump_vaddr, marker_vaddr,
            poison_vaddr, probe_marker, probe_poison);
        AI_ASSERT_NO_EXCEPT_AND("P1-230 taken redirect cancels the near-full wrong-path vector-load window and immediately accepts a fresh post-redirect vector load",
            ld(marker_vaddr) == probe_marker
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("P1-230 after redirect, the new vector load is not ghost-full or bound to a canceled VLQ entry, and the wrong-path poison store remains killed",
            ai_memblock_vec_redirect_bytes_match(dump_paddr,
                good_patterns[round], AI_MEMBLOCK_VEC_REDIRECT_BYTES) &&
            read64(marker_paddr) == probe_marker &&
            read64(poison_paddr) == poison_seed &&
            read8(dump_before_paddr) == dump_before &&
            read8(dump_after_paddr) == dump_after,
            "marker=0x%lx poison=0x%lx dump0=0x%x",
            read64(marker_paddr), read64(poison_paddr), read8(dump_paddr)
        );
    }

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_wrong_path_vsegment_redirect_no_stale_flag_or_payload_leak_corner()
{
    TEST_START();

    static const uint8_t src_patterns[AI_MEMBLOCK_VEC_REDIRECT_ROUNDS]
        [AI_MEMBLOCK_VSEG_REDIRECT_BYTES] = {
        { 0x10, 0xa0, 0x11, 0xa1, 0x12, 0xa2, 0x13, 0xa3,
          0x14, 0xa4, 0x15, 0xa5, 0x16, 0xa6, 0x17, 0xa7 },
        { 0x80, 0x08, 0x81, 0x18, 0x82, 0x28, 0x83, 0x38,
          0x84, 0x48, 0x85, 0x58, 0x86, 0x68, 0x87, 0x78 },
    };
    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page control_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xb00U;
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0xc00U;
    const uintptr_t control_vaddr = hs_page_base(control_page) + 0xd00U;
    const uintptr_t marker_vaddr = hs_page_base(control_page) + 0xd40U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xb00U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0xc00U;
    const uintptr_t control_paddr = phys_page_base(control_page) + 0xd00U;
    const uintptr_t marker_paddr = phys_page_base(control_page) + 0xd40U;
    const uintptr_t dst_before_paddr = dst_paddr - 1U;
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_VSEG_REDIRECT_BYTES;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint8_t dst_fills[] = { 0x3cU, 0xc3U };
    const uint8_t dst_before[] = { 0x5eU, 0xe5U };
    const uint8_t dst_after[] = { 0x72U, 0x27U };
    const uint64_t marker_seeds[] = {
        0x0badc0ffee0ddf00ULL,
        0xf00dd0c0ffee1234ULL,
    };
    const uint64_t marker_training[] = {
        0x1020304050607080ULL,
        0x8877665544332210ULL,
    };
    const uint64_t marker_probe[] = {
        0x13572468ace0bdf9ULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0xcafebabedeadbeefULL,
        0xfeedface01234567ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MEMBLOCK_VEC_REDIRECT_ROUNDS;
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_vec_redirect_write_bytes(src_paddr, src_patterns[round],
            AI_MEMBLOCK_VSEG_REDIRECT_BYTES);
        ai_memblock_vec_redirect_prefill(dst_paddr, dst_fills[round],
            AI_MEMBLOCK_VSEG_REDIRECT_BYTES);
        write8(dst_before_paddr, dst_before[round]);
        write8(dst_after_paddr, dst_after[round]);
        write64(control_paddr, 1ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_vec_redirect_set_hs_leaf(src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(control_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_wrong_path_vsegment_branch_window(control_vaddr, src_vaddr,
            dst_vaddr, marker_vaddr, marker_training[round],
            AI_MEMBLOCK_VEC_REDIRECT_TRAIN_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("fall-through VSegment load/store training retires cleanly before the redirect probe",
            ld(marker_vaddr) == marker_training[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("training VSegment load/store writes the expected interleaved payload and preserves destination guards",
            ai_memblock_vec_redirect_bytes_match(dst_paddr,
                src_patterns[round], AI_MEMBLOCK_VSEG_REDIRECT_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round]
        );

        ai_memblock_vec_redirect_prefill(dst_paddr, dst_fills[round],
            AI_MEMBLOCK_VSEG_REDIRECT_BYTES);
        write64(control_paddr, 0ULL);
        write64(marker_paddr, marker_seeds[round]);
        ai_memblock_vec_redirect_set_hs_leaf(src_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_vec_redirect_set_hs_leaf(dst_page,
            PTE_V | PTE_AD | PTE_X);
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_wrong_path_vsegment_branch_window(control_vaddr, src_vaddr,
            dst_vaddr, marker_vaddr, marker_probe[round],
            AI_MEMBLOCK_VEC_REDIRECT_PROBE_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("taken-branch redirect kills wrong-path VSegment before no-R/no-W leaves can fault or leak segment payload",
            ld(marker_vaddr) == marker_probe[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("wrong-path VSegment leaves the denied destination, marker guard, and destination guards unchanged except for the taken-path marker",
            ai_memblock_vec_redirect_fill_matches(dst_paddr,
                dst_fills[round], AI_MEMBLOCK_VSEG_REDIRECT_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round] &&
            read64(marker_paddr) == marker_probe[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );

        goto_priv(PRIV_HS);
        ai_memblock_vec_redirect_set_hs_leaf(src_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_vec_redirect_set_hs_leaf(dst_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_vsegment_copy_window(src_vaddr, dst_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("after repairing both leaves, a real VSegment load/store uses fresh state and completes without stale segment flag or redirect metadata",
            true
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("valid repaired VSegment load/store writes only the current interleaved source payload while preserving all guards",
            ai_memblock_vec_redirect_bytes_match(dst_paddr,
                src_patterns[round], AI_MEMBLOCK_VSEG_REDIRECT_BYTES) &&
            read8(dst_before_paddr) == dst_before[round] &&
            read8(dst_after_paddr) == dst_after[round] &&
            read64(marker_paddr) == marker_probe[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
