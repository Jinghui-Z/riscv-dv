#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_MICRO_VFOF_MASK_FULL_VL 16U
#define AI_MICRO_VFOF_MASK_E16_VL 8U
#define AI_MICRO_VFOF_MASK_E32_VL 4U
#define AI_MICRO_VFOF_MASK_E64_VL 4U
#define AI_MICRO_VFOF_MASK_LOW_BYTES 8U

typedef struct {
    bool triggered;
    uint64_t cause;
    uint64_t tval;
} ai_micro_vfof_mask_excpt_snapshot_t;

static inline ai_micro_vfof_mask_excpt_snapshot_t
ai_micro_vfof_mask_snapshot_excpt(void)
{
    ai_micro_vfof_mask_excpt_snapshot_t snapshot = {
        .triggered = excpt.triggered,
        .cause = excpt.cause,
        .tval = excpt.tval,
    };
    return snapshot;
}

static inline pte_t *ai_micro_vfof_mask_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_vfof_mask_set_hs_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_micro_vfof_mask_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_micro_vfof_mask_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_micro_vfof_mask_write_bytes(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline void ai_micro_vfof_mask_fill_bytes(uintptr_t paddr,
    uint8_t value, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, value);
    }
}

static inline bool ai_micro_vfof_mask_bytes_match(uintptr_t paddr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_vfof_mask_byte_range_is(uintptr_t paddr,
    unsigned start, unsigned end, uint8_t expected)
{
    for (unsigned i = start; i < end; ++i) {
        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_vfof_mask_byte_range_is_agnostic_or_old(
    uintptr_t paddr, const uint8_t *old, unsigned start, unsigned end)
{
    for (unsigned i = start; i < end; ++i) {
        uint8_t observed = read8(paddr + i);
        if (observed != old[i] && observed != 0xffU) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_vfof_mask_hword_is_agnostic_or_old(
    uintptr_t paddr, uint16_t old)
{
    uint16_t observed = read16(paddr);
    return observed == old || observed == 0xffffU;
}

static inline bool ai_micro_vfof_mask_word_is_agnostic_or_old(
    uintptr_t paddr, uint32_t old)
{
    uint32_t observed = read32(paddr);
    return observed == old || observed == 0xffffffffU;
}

static inline bool ai_micro_vfof_mask_dword_is_agnostic_or_old(
    uintptr_t paddr, uint64_t old)
{
    uint64_t observed = read64(paddr);
    return observed == old || observed == 0xffffffffffffffffULL;
}

static inline void ai_micro_vfof_mask_prepare_hs_env(enum test_page low_page,
    enum test_page high_page, enum test_page aux_page)
{
    goto_priv(PRIV_HS);
    hspt_init();
    ai_micro_vfof_mask_set_hs_leaf(low_page,
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_vfof_mask_set_hs_leaf(high_page,
        PTE_V | PTE_AD | PTE_X);
    ai_micro_vfof_mask_set_hs_leaf(aux_page,
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
}

static inline void ai_micro_vfof_mask_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_micro_vfof_masked_vle8ff_full_dump(
    uintptr_t seed_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump_vaddr,
    unsigned req_vl,
    int agnostic,
    unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    if (agnostic) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, ma\n\t"
            "vle8.v v8, (%[seed])\n\t"
            "vlm.v v0, (%[mask])\n\t"
            "csrw vstart, zero\n\t"
            "vle8ff.v v8, (%[src]), v0.t\n\t"
            "csrr %[out_vl], vl\n\t"
            "csrr %[out_vstart], vstart\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, ma\n\t"
            "vse8.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [out_vstart] "=&r"(captured_vstart)
            : [seed] "r"(seed_vaddr),
              [src] "r"(fof_src_vaddr),
              [mask] "r"(mask_vaddr),
              [dump] "r"(dump_vaddr),
              [req_vl] "r"(req_vl)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vle8.v v8, (%[seed])\n\t"
            "vlm.v v0, (%[mask])\n\t"
            "csrw vstart, zero\n\t"
            "vle8ff.v v8, (%[src]), v0.t\n\t"
            "csrr %[out_vl], vl\n\t"
            "csrr %[out_vstart], vstart\n\t"
            "csrw vstart, zero\n\t"
            "vsetvli t0, %[req_vl], e8, m1, tu, mu\n\t"
            "vse8.v v8, (%[dump])\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [out_vl] "=&r"(captured_vl),
              [out_vstart] "=&r"(captured_vstart)
            : [seed] "r"(seed_vaddr),
              [src] "r"(fof_src_vaddr),
              [mask] "r"(mask_vaddr),
              [dump] "r"(dump_vaddr),
              [req_vl] "r"(req_vl)
            : "t0", "memory");
    }

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_micro_vfof_masked_vle8ff_vstart_full_dump(
    uintptr_t seed_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump_vaddr,
    unsigned req_vl,
    unsigned start_index,
    unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, ma\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, %[start]\n\t"
        "vle8ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[req_vl], e8, m1, tu, ma\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(fof_src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [req_vl] "r"(req_vl),
          [start] "r"(start_index)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_micro_vfof_masked_vle8ff_tail_full_dump(
    uintptr_t seed_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump_vaddr,
    unsigned req_vl,
    unsigned full_vl,
    unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "vsetvli t0, %[req_vl], e8, m1, ta, ma\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[full_vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed_vaddr),
          [src] "r"(fof_src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [req_vl] "r"(req_vl),
          [full_vl] "r"(full_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_micro_vfof_masked_vle16ff_full_dump(
    uintptr_t seed_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump_vaddr,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, ma\n\t"
        "vle16.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle16ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, ma\n\t"
        "vse16.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [seed] "r"(seed_vaddr),
          [src] "r"(fof_src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"((unsigned)AI_MICRO_VFOF_MASK_E16_VL)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_vfof_masked_vle32ff_full_dump(
    uintptr_t seed_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump_vaddr,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, ma\n\t"
        "vle32.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle32ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, ma\n\t"
        "vse32.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [seed] "r"(seed_vaddr),
          [src] "r"(fof_src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"((unsigned)AI_MICRO_VFOF_MASK_E32_VL)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_vfof_masked_vle64ff_full_dump(
    uintptr_t seed_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump_vaddr,
    unsigned *observed_vl)
{
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, ma\n\t"
        "vle64.v v8, (%[seed])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vle64ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, ma\n\t"
        "vse64.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [seed] "r"(seed_vaddr),
          [src] "r"(fof_src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump] "r"(dump_vaddr),
          [vl] "r"((unsigned)AI_MICRO_VFOF_MASK_E64_VL)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

static inline void ai_micro_vfof_masked_vlseg2e8ff_full_dump(
    uintptr_t seed0_vaddr,
    uintptr_t seed1_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t dump0_vaddr,
    uintptr_t dump1_vaddr,
    unsigned *observed_vl,
    unsigned *observed_vstart)
{
    unsigned captured_vl = 0U;
    unsigned captured_vstart = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, ma\n\t"
        "vle8.v v8, (%[seed0])\n\t"
        "vle8.v v9, (%[seed1])\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8ff.v v8, (%[src]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, ma\n\t"
        "vse8.v v8, (%[dump0])\n\t"
        "vse8.v v9, (%[dump1])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed0] "r"(seed0_vaddr),
          [seed1] "r"(seed1_vaddr),
          [src] "r"(fof_src_vaddr),
          [mask] "r"(mask_vaddr),
          [dump0] "r"(dump0_vaddr),
          [dump1] "r"(dump1_vaddr),
          [vl] "r"((unsigned)AI_MICRO_VFOF_MASK_FULL_VL)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

bool ai_micro_hs_vfof_mask_agnostic_suffix_full_dump_after_first_fault_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t fof_src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x180U;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x180U;
    const uint8_t payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87
    };
    const uint8_t blocked[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    const uint8_t seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50
    };
    const uint8_t mask[2] = { 0xffU, 0x01U };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(low_tail_paddr, payload, sizeof(payload));
    ai_micro_vfof_mask_write_bytes(high_head_paddr, blocked, sizeof(blocked));
    ai_micro_vfof_mask_write_bytes(seed_paddr, seed, sizeof(seed));
    ai_micro_vfof_mask_write_bytes(mask_paddr, mask, sizeof(mask));
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x5aU,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, fof_src_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 1,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t vfof_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("masked vle8ff first nonzero fault keeps the prefix payload, shortens vl, and applies vma=1 agnostic policy to masked-off suffix lanes in the full dump",
        vfof_excpt.triggered == false &&
        observed_vl == AI_MICRO_VFOF_MASK_LOW_BYTES &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, payload,
            AI_MICRO_VFOF_MASK_LOW_BYTES) &&
        read8(dump_paddr + AI_MICRO_VFOF_MASK_LOW_BYTES) ==
            seed[AI_MICRO_VFOF_MASK_LOW_BYTES] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            seed, AI_MICRO_VFOF_MASK_LOW_BYTES + 1U,
            AI_MICRO_VFOF_MASK_FULL_VL),
        "vl=%u vstart=%u lane8=0x%x suffix=%x,%x,%x,%x,%x,%x,%x",
        observed_vl, observed_vstart,
        read8(dump_paddr + 8U), read8(dump_paddr + 9U),
        read8(dump_paddr + 10U), read8(dump_paddr + 11U),
        read8(dump_paddr + 12U), read8(dump_paddr + 13U),
        read8(dump_paddr + 14U), read8(dump_paddr + 15U)
    );
    TEST_ASSERT("masked vle8ff AP-47 probe does not read or clobber backing bytes from the execute-only high page",
        ai_micro_vfof_mask_bytes_match(high_head_paddr, blocked,
            sizeof(blocked))
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vfof_mask_undisturbed_suffix_full_dump_after_first_fault_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t fof_src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x200U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x200U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x280U;
    const uint8_t payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28
    };
    const uint8_t seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf
    };
    const uint8_t mask[2] = { 0xffU, 0x01U };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(low_tail_paddr, payload, sizeof(payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, seed, sizeof(seed));
    ai_micro_vfof_mask_write_bytes(mask_paddr, mask, sizeof(mask));
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x6bU,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, fof_src_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 0,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t vfof_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("masked vle8ff vma=0 control shortens vl but keeps the fault lane and masked-off suffix destination bytes undisturbed",
        vfof_excpt.triggered == false &&
        observed_vl == AI_MICRO_VFOF_MASK_LOW_BYTES &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, payload,
            AI_MICRO_VFOF_MASK_LOW_BYTES) &&
        ai_micro_vfof_mask_bytes_match(
            dump_paddr + AI_MICRO_VFOF_MASK_LOW_BYTES,
            &seed[AI_MICRO_VFOF_MASK_LOW_BYTES],
            AI_MICRO_VFOF_MASK_FULL_VL -
                AI_MICRO_VFOF_MASK_LOW_BYTES),
        "vl=%u vstart=%u tail=%x,%x,%x,%x,%x,%x,%x,%x",
        observed_vl, observed_vstart, read8(dump_paddr + 8U),
        read8(dump_paddr + 9U), read8(dump_paddr + 10U),
        read8(dump_paddr + 11U), read8(dump_paddr + 12U),
        read8(dump_paddr + 13U), read8(dump_paddr + 14U),
        read8(dump_paddr + 15U)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_arch_vfof_mask_undisturbed_repeated_full_dump_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src4_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t src8_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t src4_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t src8_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xb80U;
    const uint8_t first_payload[4] = { 0x14, 0x25, 0x36, 0x47 };
    const uint8_t second_payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27, 0x18
    };
    const uint8_t first_seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f
    };
    const uint8_t second_seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
    };
    const uint8_t blocked[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(src4_paddr, first_payload,
        sizeof(first_payload));
    ai_micro_vfof_mask_write_bytes(high_head_paddr, blocked,
        sizeof(blocked));
    ai_micro_vfof_mask_write_bytes(seed_paddr, first_seed,
        sizeof(first_seed));
    write8(mask_paddr, 0x1fU);
    write8(mask_paddr + 1U, 0x00U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x52U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, src4_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 0,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t first_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF tu/mu first-fault episode at lane4 shortens vl, keeps prefix payload, and preserves the full undisturbed suffix dump",
        first_excpt.triggered == false &&
        observed_vl == 4U &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, first_payload,
            sizeof(first_payload)) &&
        ai_micro_vfof_mask_bytes_match(dump_paddr + 4U,
            first_seed + 4U, AI_MICRO_VFOF_MASK_FULL_VL - 4U),
        "vl=%u vstart=%u lane4=0x%x lane8=0x%x lane15=0x%x",
        observed_vl, observed_vstart, read8(dump_paddr + 4U),
        read8(dump_paddr + 8U), read8(dump_paddr + 15U)
    );

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(src8_paddr, second_payload,
        sizeof(second_payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, second_seed,
        sizeof(second_seed));
    write8(mask_paddr, 0xffU);
    write8(mask_paddr + 1U, 0x01U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x63U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    goto_priv(PRIV_HS);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, src8_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 0,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t second_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("back-to-back VFOF tu/mu episode uses its own fault index and destination seed instead of stale first-episode suffix state",
        second_excpt.triggered == false &&
        observed_vl == AI_MICRO_VFOF_MASK_LOW_BYTES &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, second_payload,
            sizeof(second_payload)) &&
        ai_micro_vfof_mask_bytes_match(
            dump_paddr + AI_MICRO_VFOF_MASK_LOW_BYTES,
            second_seed + AI_MICRO_VFOF_MASK_LOW_BYTES,
            AI_MICRO_VFOF_MASK_FULL_VL -
                AI_MICRO_VFOF_MASK_LOW_BYTES) &&
        ai_micro_vfof_mask_bytes_match(high_head_paddr, blocked,
            sizeof(blocked)),
        "vl=%u vstart=%u lane8=0x%x lane9=0x%x high0=0x%x",
        observed_vl, observed_vstart,
        read8(dump_paddr + AI_MICRO_VFOF_MASK_LOW_BYTES),
        read8(dump_paddr + AI_MICRO_VFOF_MASK_LOW_BYTES + 1U),
        read8(high_head_paddr)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vfof_eew_matrix_mask_agnostic_suffix_after_first_fault_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t low_tail_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x380U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x400U;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x380U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x400U;
    const uint8_t e8_payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
    };
    const uint8_t e8_seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(low_tail_paddr, e8_payload,
        sizeof(e8_payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, e8_seed, sizeof(e8_seed));
    write8(mask_paddr, 0xffU);
    write8(mask_paddr + 1U, 0x01U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x71U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, low_tail_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 1,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t e8_excpt =
        ai_micro_vfof_mask_snapshot_excpt();
    goto_priv(PRIV_M);
    TEST_ASSERT("EEW=e8 masked vleff suffix agnostic path keeps prefix bytes and agnostic-updates the masked suffix",
        e8_excpt.triggered == false &&
        observed_vl == 8U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, e8_payload,
            sizeof(e8_payload)) &&
        read8(dump_paddr + 8U) == e8_seed[8] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            e8_seed, 9U, 16U)
    );

    goto_priv(PRIV_M);
    write16(seed_paddr, 0x1110U);
    write16(seed_paddr + 2U, 0x2220U);
    write16(seed_paddr + 4U, 0x3330U);
    write16(seed_paddr + 6U, 0x4440U);
    write16(seed_paddr + 8U, 0x5550U);
    write16(seed_paddr + 10U, 0x6660U);
    write16(seed_paddr + 12U, 0x7770U);
    write16(seed_paddr + 14U, 0x8880U);
    write16(low_tail_paddr, 0xa101U);
    write16(low_tail_paddr + 2U, 0xb202U);
    write16(low_tail_paddr + 4U, 0xc303U);
    write16(low_tail_paddr + 6U, 0xd404U);
    write8(mask_paddr, 0x1fU);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x72U, 16U);

    goto_priv(PRIV_HS);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle16ff_full_dump(seed_vaddr, low_tail_vaddr,
        mask_vaddr, dump_vaddr, &observed_vl);
    const ai_micro_vfof_mask_excpt_snapshot_t e16_excpt =
        ai_micro_vfof_mask_snapshot_excpt();
    goto_priv(PRIV_M);
    TEST_ASSERT("EEW=e16 masked vleff suffix agnostic path preserves the first fault element and writes all masked suffix elements to the project agnostic value",
        e16_excpt.triggered == false &&
        observed_vl == 4U &&
        read16(dump_paddr) == 0xa101U &&
        read16(dump_paddr + 2U) == 0xb202U &&
        read16(dump_paddr + 4U) == 0xc303U &&
        read16(dump_paddr + 6U) == 0xd404U &&
        read16(dump_paddr + 8U) == 0x5550U &&
        ai_micro_vfof_mask_hword_is_agnostic_or_old(
            dump_paddr + 10U, 0x6660U) &&
        ai_micro_vfof_mask_hword_is_agnostic_or_old(
            dump_paddr + 12U, 0x7770U) &&
        ai_micro_vfof_mask_hword_is_agnostic_or_old(
            dump_paddr + 14U, 0x8880U)
    );

    goto_priv(PRIV_M);
    write32(seed_paddr, 0x11111110U);
    write32(seed_paddr + 4U, 0x22222220U);
    write32(seed_paddr + 8U, 0x33333330U);
    write32(seed_paddr + 12U, 0x44444440U);
    write32(low_tail_paddr, 0xa1b2c3d4U);
    write32(low_tail_paddr + 4U, 0x11223344U);
    write8(mask_paddr, 0x07U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x73U, 16U);

    goto_priv(PRIV_HS);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle32ff_full_dump(seed_vaddr, low_tail_vaddr,
        mask_vaddr, dump_vaddr, &observed_vl);
    const ai_micro_vfof_mask_excpt_snapshot_t e32_excpt =
        ai_micro_vfof_mask_snapshot_excpt();
    goto_priv(PRIV_M);
    TEST_ASSERT("EEW=e32 masked vleff suffix agnostic path covers word-sized destination indexing after the first fault",
        e32_excpt.triggered == false &&
        observed_vl == 2U &&
        read32(dump_paddr) == 0xa1b2c3d4U &&
        read32(dump_paddr + 4U) == 0x11223344U &&
        read32(dump_paddr + 8U) == 0x33333330U &&
        ai_micro_vfof_mask_word_is_agnostic_or_old(
            dump_paddr + 12U, 0x44444440U)
    );

    goto_priv(PRIV_M);
    write64(seed_paddr, 0x1111111111111110ULL);
    write64(seed_paddr + 8U, 0x2222222222222220ULL);
    write64(seed_paddr + 16U, 0x3333333333333330ULL);
    write64(seed_paddr + 24U, 0x4444444444444440ULL);
    write64(low_tail_paddr, 0x0123456789abcdefULL);
    write8(mask_paddr, 0x03U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x74U, 32U);

    goto_priv(PRIV_HS);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle64ff_full_dump(seed_vaddr, low_tail_vaddr,
        mask_vaddr, dump_vaddr, &observed_vl);
    const ai_micro_vfof_mask_excpt_snapshot_t e64_excpt =
        ai_micro_vfof_mask_snapshot_excpt();
    goto_priv(PRIV_M);
    TEST_ASSERT("EEW=e64 masked vleff suffix agnostic path covers m2 register-group destination indexing after the first fault",
        e64_excpt.triggered == false &&
        observed_vl == 1U &&
        read64(dump_paddr) == 0x0123456789abcdefULL &&
        read64(dump_paddr + 8U) == 0x2222222222222220ULL &&
        ai_micro_vfof_mask_dword_is_agnostic_or_old(
            dump_paddr + 16U, 0x3333333333333330ULL) &&
        ai_micro_vfof_mask_dword_is_agnostic_or_old(
            dump_paddr + 24U, 0x4444444444444440ULL)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vlseg2e8ff_mask_agnostic_suffix_fields_after_first_fault_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seg_src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed0_vaddr = hs_page_base(aux_page) + 0x500U;
    const uintptr_t seed1_vaddr = hs_page_base(aux_page) + 0x540U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x580U;
    const uintptr_t dump0_vaddr = hs_page_base(aux_page) + 0x5c0U;
    const uintptr_t dump1_vaddr = hs_page_base(aux_page) + 0x600U;
    const uintptr_t seg_src_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed0_paddr = phys_page_base(aux_page) + 0x500U;
    const uintptr_t seed1_paddr = phys_page_base(aux_page) + 0x540U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x580U;
    const uintptr_t dump0_paddr = phys_page_base(aux_page) + 0x5c0U;
    const uintptr_t dump1_paddr = phys_page_base(aux_page) + 0x600U;
    const uint8_t seed0[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t seed1[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f
    };
    const uint8_t field0_prefix[4] = { 0x81, 0x82, 0x83, 0x84 };
    const uint8_t field1_prefix[4] = { 0xa1, 0xa2, 0xa3, 0xa4 };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < 4U; ++i) {
        write8(seg_src_paddr + 2U * i, field0_prefix[i]);
        write8(seg_src_paddr + 2U * i + 1U, field1_prefix[i]);
    }
    ai_micro_vfof_mask_write_bytes(seed0_paddr, seed0, sizeof(seed0));
    ai_micro_vfof_mask_write_bytes(seed1_paddr, seed1, sizeof(seed1));
    write8(mask_paddr, 0x1fU);
    write8(mask_paddr + 1U, 0x00U);
    ai_micro_vfof_mask_fill_bytes(dump0_paddr, 0x31U,
        AI_MICRO_VFOF_MASK_FULL_VL);
    ai_micro_vfof_mask_fill_bytes(dump1_paddr, 0x32U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vlseg2e8ff_full_dump(seed0_vaddr, seed1_vaddr,
        seg_src_vaddr, mask_vaddr, dump0_vaddr, dump1_vaddr,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t vfof_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("vlseg2e8ff first nonzero fault shortens vl at the element boundary and applies vma=1 agnostic policy to every masked-off suffix field",
        vfof_excpt.triggered == false &&
        observed_vl == 4U &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump0_paddr, field0_prefix,
            sizeof(field0_prefix)) &&
        ai_micro_vfof_mask_bytes_match(dump1_paddr, field1_prefix,
            sizeof(field1_prefix)) &&
        read8(dump0_paddr + 4U) == seed0[4] &&
        read8(dump1_paddr + 4U) == seed1[4] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump0_paddr,
            seed0, 5U, 16U) &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump1_paddr,
            seed1, 5U, 16U),
        "vl=%u vstart=%u f0_4=0x%x f1_4=0x%x f0_5=0x%x f1_5=0x%x",
        observed_vl, observed_vstart, read8(dump0_paddr + 4U),
        read8(dump1_paddr + 4U), read8(dump0_paddr + 5U),
        read8(dump1_paddr + 5U)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vfof_repeated_mask_policy_no_stale_suffix_context_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src4_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t src8_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x680U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x6c0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x700U;
    const uintptr_t src4_paddr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t src8_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x680U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x6c0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x700U;
    const uint8_t first_payload[4] = { 0x11, 0x22, 0x33, 0x44 };
    const uint8_t second_payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
    };
    const uint8_t first_seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60
    };
    const uint8_t second_seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
        0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(src4_paddr, first_payload,
        sizeof(first_payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, first_seed,
        sizeof(first_seed));
    write8(mask_paddr, 0x1fU);
    write8(mask_paddr + 1U, 0x00U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x33U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, src4_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 1,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t first_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("first repeated AP-47 VFOF episode agnostic-updates suffix lanes from its own destination context",
        first_excpt.triggered == false &&
        observed_vl == 4U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, first_payload,
            sizeof(first_payload)) &&
        read8(dump_paddr + 4U) == first_seed[4] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            first_seed, 5U, 16U)
    );

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(src8_paddr, second_payload,
        sizeof(second_payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, second_seed,
        sizeof(second_seed));
    write8(mask_paddr, 0xffU);
    write8(mask_paddr + 1U, 0x01U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x44U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    goto_priv(PRIV_HS);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, src8_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 1,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t second_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("second repeated AP-47 VFOF episode uses the new fault index, mask, and destination guard instead of retaining stale suffix context",
        second_excpt.triggered == false &&
        observed_vl == 8U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, second_payload,
            sizeof(second_payload)) &&
        read8(dump_paddr + 8U) == second_seed[8] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            second_seed, 9U, 16U)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vfof_vstart_nonzero_mask_agnostic_suffix_after_first_fault_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t fof_src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 5U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x7c0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x800U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE - 5U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x780U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x7c0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x800U;
    const uint8_t seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8,
        0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0
    };
    const unsigned start_index = 3U;
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(seed_paddr, seed, sizeof(seed));
    write8(low_tail_paddr + 3U, 0x3dU);
    write8(low_tail_paddr + 4U, 0x4eU);
    write8(mask_paddr, 0x38U);
    write8(mask_paddr + 1U, 0x00U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x81U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_vstart_full_dump(seed_vaddr,
        fof_src_vaddr, mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL,
        start_index, &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t vfof_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("vstart=3 masked VFOF preserves prestart lanes, reports the absolute first-fault element, and applies vma=1 only to post-fault masked suffix lanes",
        vfof_excpt.triggered == false &&
        observed_vl == 5U &&
        observed_vstart == 0U &&
        read8(dump_paddr) == seed[0] &&
        read8(dump_paddr + 1U) == seed[1] &&
        read8(dump_paddr + 2U) == seed[2] &&
        read8(dump_paddr + 3U) == 0x3dU &&
        read8(dump_paddr + 4U) == 0x4eU &&
        read8(dump_paddr + 5U) == seed[5] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            seed, 6U, 16U),
        "vl=%u vstart=%u pre=%x,%x,%x lane5=0x%x lane6=0x%x",
        observed_vl, observed_vstart, read8(dump_paddr),
        read8(dump_paddr + 1U), read8(dump_paddr + 2U),
        read8(dump_paddr + 5U), read8(dump_paddr + 6U)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vfof_second_fault_keeps_first_vl_and_mask_suffix_agnostic_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t fof_src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x880U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x8c0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x880U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x8c0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0x900U;
    const uint8_t payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x09, 0x18, 0x27, 0x36, 0x45, 0x54, 0x63, 0x72
    };
    const uint8_t seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
        0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(low_tail_paddr, payload, sizeof(payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, seed, sizeof(seed));
    write8(mask_paddr, 0xffU);
    write8(mask_paddr + 1U, 0x05U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0x91U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_full_dump(seed_vaddr, fof_src_vaddr,
        mask_vaddr, dump_vaddr, AI_MICRO_VFOF_MASK_FULL_VL, 1,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t vfof_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-47/AP-94 combined VFOF keeps vl at the first active fault while a later active fault stays silent and masked suffix lanes still get vma agnostic handling",
        vfof_excpt.triggered == false &&
        observed_vl == 8U &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, payload,
            sizeof(payload)) &&
        read8(dump_paddr + 8U) == seed[8] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            seed, 9U, 10U) &&
        read8(dump_paddr + 10U) == seed[10] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            seed, 11U, 16U),
        "vl=%u lane8=0x%x lane9=0x%x lane10=0x%x lane11=0x%x",
        observed_vl, read8(dump_paddr + 8U), read8(dump_paddr + 9U),
        read8(dump_paddr + 10U), read8(dump_paddr + 11U)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_vfof_tail_agnostic_split_from_mask_suffix_after_first_fault_spike_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t fof_src_vaddr =
        hs_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x980U;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x9c0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE - AI_MICRO_VFOF_MASK_LOW_BYTES;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x980U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x9c0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xa00U;
    const uint8_t payload[AI_MICRO_VFOF_MASK_LOW_BYTES] = {
        0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08
    };
    const uint8_t seed[AI_MICRO_VFOF_MASK_FULL_VL] = {
        0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
        0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0
    };
    unsigned observed_vl = 0U;
    unsigned observed_vstart = 0U;

    ai_micro_vfof_mask_enable_vector_state();

    goto_priv(PRIV_M);
    ai_micro_vfof_mask_write_bytes(low_tail_paddr, payload, sizeof(payload));
    ai_micro_vfof_mask_write_bytes(seed_paddr, seed, sizeof(seed));
    write8(mask_paddr, 0xffU);
    write8(mask_paddr + 1U, 0x01U);
    ai_micro_vfof_mask_fill_bytes(dump_paddr, 0xa1U,
        AI_MICRO_VFOF_MASK_FULL_VL);

    ai_micro_vfof_mask_prepare_hs_env(low_page, high_page, aux_page);

    TEST_SETUP_EXCEPT();
    ai_micro_vfof_masked_vle8ff_tail_full_dump(seed_vaddr, fof_src_vaddr,
        mask_vaddr, dump_vaddr, 12U, AI_MICRO_VFOF_MASK_FULL_VL,
        &observed_vl, &observed_vstart);
    const ai_micro_vfof_mask_excpt_snapshot_t vfof_excpt =
        ai_micro_vfof_mask_snapshot_excpt();

    goto_priv(PRIV_M);
    TEST_ASSERT("VFOF with vta=1/vma=1 keeps the shortened-vl prefix distinct from in-vl masked suffix lanes and original-vl tail lanes in the full dump",
        vfof_excpt.triggered == false &&
        observed_vl == 8U &&
        observed_vstart == 0U &&
        ai_micro_vfof_mask_bytes_match(dump_paddr, payload,
            sizeof(payload)) &&
        read8(dump_paddr + 8U) == seed[8] &&
        ai_micro_vfof_mask_byte_range_is_agnostic_or_old(dump_paddr,
            seed, 9U, 16U),
        "vl=%u vstart=%u lane8=0x%x lane9=0x%x lane12=0x%x lane15=0x%x",
        observed_vl, observed_vstart, read8(dump_paddr + 8U),
        read8(dump_paddr + 9U), read8(dump_paddr + 12U),
        read8(dump_paddr + 15U)
    );

    ai_micro_vfof_mask_reset_hs_env();

    TEST_END();
}
