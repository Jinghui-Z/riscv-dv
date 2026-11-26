#include <rvh_test.h>
#include <page_tables.h>
#include <stddef.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_EXCEPTION_ORDER_PF_PAGE     VSI_GI
#define AI_MICRO_EXCEPTION_ORDER_AF_PAGE     VSRWX_GRWX
#define AI_MICRO_EXCEPTION_ORDER_MA_PAGE     VSRWXPbmt1_GURWXPbmt1
#define AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE SWITCH1
#define AI_MICRO_EXCEPTION_ORDER_MA_OFFSET   3ULL

#if PT_LVLS == 4
#define AI_MICRO_EXCEPTION_ORDER_LEAF_LEVEL 4
#else
#define AI_MICRO_EXCEPTION_ORDER_LEAF_LEVEL 2
#endif

struct ai_micro_exception_snapshot {
    bool triggered;
    uint64_t cause;
    uint64_t tval;
};

static inline uintptr_t ai_micro_exception_order_ma_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline void ai_micro_exception_order_reset_hs_translation(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_exception_order_make_leaf_access_fault(enum test_page page)
{
    goto_priv(PRIV_M);
    hspt[AI_MICRO_EXCEPTION_ORDER_LEAF_LEVEL][page] |= (1ULL << 46);
    sfence_vma();
}

static inline void ai_micro_exception_order_prepare_fault_pair_env(void)
{
    ai_micro_exception_order_reset_hs_translation();
    ai_micro_exception_order_make_leaf_access_fault(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    goto_priv(PRIV_HS);
}

static inline void ai_micro_exception_order_prepare_misaligned_fault_pair_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(AI_MICRO_EXCEPTION_ORDER_MA_PAGE);
    sfence_vma();
    ai_micro_exception_order_make_leaf_access_fault(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    goto_priv(PRIV_HS);
}

static inline void ai_micro_exception_order_reset_pbmt_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_load_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    struct ai_micro_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a3)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a3)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a3)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "ld t0, 0(a1)\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_exception_snapshot, tval))
        : "t0", "t1", "t2", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_store_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    uint64_t first_value, uint64_t second_value,
    struct ai_micro_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a3, 0(a0)\n"
        "la t0, excpt\n"
        "lbu t1, %c[exc_triggered](t0)\n"
        "sb t1, %c[snap_triggered](a5)\n"
        "ld t1, %c[exc_cause](t0)\n"
        "sd t1, %c[snap_cause](a5)\n"
        "ld t1, %c[exc_tval](t0)\n"
        "sd t1, %c[snap_tval](a5)\n"
        "sb zero, %c[exc_triggered](t0)\n"
        "li t1, 1\n"
        "sb t1, %c[exc_testing](t0)\n"
        "sd a4, 0(a1)\n"
        "li t1, 1\n"
        "sd t1, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_exception_snapshot, tval))
        : "t0", "t1", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_load_store_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    uint64_t second_value, struct ai_micro_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "sd a3, 0(a1)\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_exception_snapshot, tval))
        : "t0", "t1", "t2", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_store_load_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    uint64_t first_value, struct ai_micro_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a3, 0(a0)\n"
        "la t0, excpt\n"
        "lbu t1, %c[exc_triggered](t0)\n"
        "sb t1, %c[snap_triggered](a4)\n"
        "ld t1, %c[exc_cause](t0)\n"
        "sd t1, %c[snap_cause](a4)\n"
        "ld t1, %c[exc_tval](t0)\n"
        "sd t1, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t0)\n"
        "li t1, 1\n"
        "sb t1, %c[exc_testing](t0)\n"
        "ld t2, 0(a1)\n"
        "li t1, 1\n"
        "sd t1, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_exception_snapshot, tval))
        : "t0", "t1", "t2", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_load_amo_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    uint64_t second_value, struct ai_micro_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "amoadd.d t0, a3, (a1)\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_exception_snapshot, tval))
        : "t0", "t1", "t2", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_amo_load_fault_pair_capture(
    uintptr_t first_vaddr, uintptr_t second_vaddr, uintptr_t marker_vaddr,
    uint64_t first_value, struct ai_micro_exception_snapshot *first_snapshot)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "amoadd.d t0, a3, (a0)\n"
        "la t1, excpt\n"
        "lbu t2, %c[exc_triggered](t1)\n"
        "sb t2, %c[snap_triggered](a4)\n"
        "ld t2, %c[exc_cause](t1)\n"
        "sd t2, %c[snap_cause](a4)\n"
        "ld t2, %c[exc_tval](t1)\n"
        "sd t2, %c[snap_tval](a4)\n"
        "sb zero, %c[exc_triggered](t1)\n"
        "li t2, 1\n"
        "sb t2, %c[exc_testing](t1)\n"
        "ld t2, 0(a1)\n"
        "li t2, 1\n"
        "sd t2, 0(a2)\n"
        "ret\n"
        ".option pop\n"
        :
        : [exc_testing] "i" (offsetof(struct exception, testing)),
          [exc_triggered] "i" (offsetof(struct exception, triggered)),
          [exc_cause] "i" (offsetof(struct exception, cause)),
          [exc_tval] "i" (offsetof(struct exception, tval)),
          [snap_triggered] "i" (offsetof(struct ai_micro_exception_snapshot, triggered)),
          [snap_cause] "i" (offsetof(struct ai_micro_exception_snapshot, cause)),
          [snap_tval] "i" (offsetof(struct ai_micro_exception_snapshot, tval))
        : "t0", "t1", "t2", "memory");
}

bool ai_micro_hs_load_exception_pair_oldest_page_fault_first_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr, &first);

    TEST_ASSERT("in a diagnostic HS two-load fault pair with older PF and younger AF, the first observed exception is the older load page fault",
        first.triggered == true &&
        first.cause == CAUSE_LPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic pair falls through to the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the diagnostic two-load fault pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_exception_pair_oldest_access_fault_first_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr, &first);

    TEST_ASSERT("in a diagnostic HS two-load fault pair with older AF and younger PF, the first observed exception is the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic pair falls through to the expected load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the diagnostic AF-then-PF two-load sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_exception_pair_oldest_page_fault_first_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t pf_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t pf_seed = 0x1111222233334444ULL;
    const uint64_t af_seed = 0x5555666677778888ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(pf_paddr, pf_seed);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr,
        0xa1a2a3a4a5a6a7a8ULL, 0xb1b2b3b4b5b6b7b8ULL, &first);

    TEST_ASSERT("in a diagnostic HS two-store fault pair with older PF and younger AF, the first observed exception is the older store page fault",
        first.triggered == true &&
        first.cause == CAUSE_SPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic pair falls through to the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("neither faulting destination is modified across the PF-then-AF diagnostic two-store sequence",
        read64(pf_paddr) == pf_seed &&
        read64(af_paddr) == af_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic two-store fault pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_exception_pair_oldest_access_fault_first_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t pf_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t pf_seed = 0x8899aabbccddeeffULL;
    const uint64_t af_seed = 0x0123456789abcdefULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(pf_paddr, pf_seed);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr,
        0xc1c2c3c4c5c6c7c8ULL, 0xd1d2d3d4d5d6d7d8ULL, &first);

    TEST_ASSERT("in a diagnostic HS two-store fault pair with older AF and younger PF, the first observed exception is the older store access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic pair falls through to the expected store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("neither faulting destination is modified across the AF-then-PF diagnostic two-store sequence",
        read64(af_paddr) == af_seed &&
        read64(pf_paddr) == pf_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic AF-then-PF two-store sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_store_exception_pair_oldest_load_page_fault_first_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x13579bdf2468ace0ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_store_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr,
        0x1111222233334444ULL, &first);

    TEST_ASSERT("in a diagnostic HS load-then-store fault pair with older load PF and younger store AF, the first observed exception is the older load page fault",
        first.triggered == true &&
        first.cause == CAUSE_LPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic load-store pair falls through to the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger faulting store target remains unchanged across the diagnostic load-PF then store-AF sequence",
        read64(af_paddr) == af_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic load-store PF-then-AF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_store_exception_pair_oldest_load_access_fault_first_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t pf_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t pf_seed = 0x0fedcba987654321ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(pf_paddr, pf_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_store_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr,
        0x5555666677778888ULL, &first);

    TEST_ASSERT("in a diagnostic HS load-then-store fault pair with older load AF and younger store PF, the first observed exception is the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic load-store pair falls through to the expected store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger faulting store target remains unchanged across the diagnostic load-AF then store-PF sequence",
        read64(pf_paddr) == pf_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic load-store AF-then-PF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_load_exception_pair_oldest_store_page_fault_first_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t pf_seed = 0x55aa33cc77ee11ffULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(pf_paddr, pf_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_load_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr,
        0x9999aaaabbbbccccULL, &first);

    TEST_ASSERT("in a diagnostic HS store-then-load fault pair with older store PF and younger load AF, the first observed exception is the older store page fault",
        first.triggered == true &&
        first.cause == CAUSE_SPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic store-load pair falls through to the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older faulting store target remains unchanged across the diagnostic store-PF then load-AF sequence",
        read64(pf_paddr) == pf_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic store-load PF-then-AF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_load_exception_pair_oldest_store_access_fault_first_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x0123012301230123ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_load_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr,
        0xddddeeeeffff0000ULL, &first);

    TEST_ASSERT("in a diagnostic HS store-then-load fault pair with older store AF and younger load PF, the first observed exception is the older store access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic store-load pair falls through to the expected load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older faulting store target remains unchanged across the diagnostic store-AF then load-PF sequence",
        read64(af_paddr) == af_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic store-load AF-then-PF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_exception_pair_oldest_misaligned_then_access_fault_corner()
{
    TEST_START();

    const uintptr_t ma_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE) +
        AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_fault_pair_capture(ma_vaddr, af_vaddr, marker_vaddr, &first);

    TEST_ASSERT("in a diagnostic HS two-load fault pair with older load misaligned and younger load AF, the first observed exception is the older load misaligned trap",
        first.triggered == true &&
        first.cause == CAUSE_MISALIGNED_LOAD &&
        first.tval == ma_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic pair falls through to the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the diagnostic load-misaligned then load-AF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_exception_pair_oldest_access_fault_then_misaligned_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE) +
        AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_fault_pair_capture(af_vaddr, ma_vaddr, marker_vaddr, &first);

    TEST_ASSERT("in a diagnostic HS two-load fault pair with older load AF and younger load misaligned, the first observed exception is the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic pair falls through to the expected load misaligned trap",
        excpt.triggered == true &&
        excpt.cause == CAUSE_MISALIGNED_LOAD &&
        excpt.tval == ma_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the diagnostic load-AF then load-misaligned sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_exception_pair_oldest_misaligned_then_access_fault_corner()
{
    TEST_START();

    const uintptr_t ma_base_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE);
    const uintptr_t ma_vaddr = ma_base_vaddr + AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_paddr = ai_micro_exception_order_ma_backing_paddr();
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t ma_low_seed = 0x0102030405060708ULL;
    const uint64_t ma_high_seed = 0x1112131415161718ULL;
    const uint64_t af_seed = 0x2122232425262728ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(ma_paddr, ma_low_seed);
    write64(ma_paddr + sizeof(uint64_t), ma_high_seed);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_fault_pair_capture(ma_vaddr, af_vaddr, marker_vaddr,
        0xa1a2a3a4a5a6a7a8ULL, 0xb1b2b3b4b5b6b7b8ULL, &first);

    TEST_ASSERT("in a diagnostic HS two-store fault pair with older store misaligned and younger store AF, the first observed exception is the older store misaligned trap",
        first.triggered == true &&
        first.cause == CAUSE_MISALIGNED_STORE &&
        first.tval == ma_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic pair falls through to the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("neither the misaligned-store target window nor the younger AF target is modified across the diagnostic store-misaligned then store-AF sequence",
        read64(ma_paddr) == ma_low_seed &&
        read64(ma_paddr + sizeof(uint64_t)) == ma_high_seed &&
        read64(af_paddr) == af_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic store-misaligned then store-AF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_exception_pair_oldest_access_fault_then_misaligned_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_base_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE);
    const uintptr_t ma_vaddr = ma_base_vaddr + AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t ma_paddr = ai_micro_exception_order_ma_backing_paddr();
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x3132333435363738ULL;
    const uint64_t ma_low_seed = 0x4142434445464748ULL;
    const uint64_t ma_high_seed = 0x5152535455565758ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(ma_paddr, ma_low_seed);
    write64(ma_paddr + sizeof(uint64_t), ma_high_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_fault_pair_capture(af_vaddr, ma_vaddr, marker_vaddr,
        0xc1c2c3c4c5c6c7c8ULL, 0xd1d2d3d4d5d6d7d8ULL, &first);

    TEST_ASSERT("in a diagnostic HS two-store fault pair with older store AF and younger store misaligned, the first observed exception is the older store access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic pair falls through to the expected store misaligned trap",
        excpt.triggered == true &&
        excpt.cause == CAUSE_MISALIGNED_STORE &&
        excpt.tval == ma_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("neither the older AF store target nor the younger misaligned-store window is modified across the diagnostic store-AF then store-misaligned sequence",
        read64(af_paddr) == af_seed &&
        read64(ma_paddr) == ma_low_seed &&
        read64(ma_paddr + sizeof(uint64_t)) == ma_high_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic store-AF then store-misaligned sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_store_exception_pair_oldest_load_misaligned_then_store_access_fault_corner()
{
    TEST_START();

    const uintptr_t ma_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE) +
        AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x6162636465666768ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_store_fault_pair_capture(ma_vaddr, af_vaddr, marker_vaddr,
        0xe1e2e3e4e5e6e7e8ULL, &first);

    TEST_ASSERT("in a diagnostic HS load-then-store fault pair with older load misaligned and younger store AF, the first observed exception is the older load misaligned trap",
        first.triggered == true &&
        first.cause == CAUSE_MISALIGNED_LOAD &&
        first.tval == ma_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic load-store pair falls through to the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger faulting store target remains unchanged across the diagnostic load-misaligned then store-AF sequence",
        read64(af_paddr) == af_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic load-misaligned then store-AF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_load_store_exception_pair_oldest_load_access_fault_then_store_misaligned_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_base_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE);
    const uintptr_t ma_vaddr = ma_base_vaddr + AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_paddr = ai_micro_exception_order_ma_backing_paddr();
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x7172737475767778ULL;
    const uint64_t ma_low_seed = 0x8182838485868788ULL;
    const uint64_t ma_high_seed = 0x9192939495969798ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(ma_paddr, ma_low_seed);
    write64(ma_paddr + sizeof(uint64_t), ma_high_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_store_fault_pair_capture(af_vaddr, ma_vaddr, marker_vaddr,
        0xf1f2f3f4f5f6f7f8ULL, &first);

    TEST_ASSERT("in a diagnostic HS load-then-store fault pair with older load AF and younger store misaligned, the first observed exception is the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS store in the same diagnostic load-store pair falls through to the expected store misaligned trap",
        excpt.triggered == true &&
        excpt.cause == CAUSE_MISALIGNED_STORE &&
        excpt.tval == ma_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("both the older AF load target image and the younger misaligned-store destination window remain unchanged across the diagnostic load-AF then store-misaligned sequence",
        read64(af_paddr) == af_seed &&
        read64(ma_paddr) == ma_low_seed &&
        read64(ma_paddr + sizeof(uint64_t)) == ma_high_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic load-AF then store-misaligned sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_load_exception_pair_oldest_store_misaligned_then_load_access_fault_corner()
{
    TEST_START();

    const uintptr_t ma_base_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE);
    const uintptr_t ma_vaddr = ma_base_vaddr + AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_paddr = ai_micro_exception_order_ma_backing_paddr();
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t ma_low_seed = 0xa1a3a5a7a9abadafULL;
    const uint64_t ma_high_seed = 0xb1b3b5b7b9bbbdbfULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(ma_paddr, ma_low_seed);
    write64(ma_paddr + sizeof(uint64_t), ma_high_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_load_fault_pair_capture(ma_vaddr, af_vaddr, marker_vaddr,
        0xc1c3c5c7c9cbcdcfULL, &first);

    TEST_ASSERT("in a diagnostic HS store-then-load fault pair with older store misaligned and younger load AF, the first observed exception is the older store misaligned trap",
        first.triggered == true &&
        first.cause == CAUSE_MISALIGNED_STORE &&
        first.tval == ma_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic store-load pair falls through to the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older misaligned-store destination window remains unchanged across the diagnostic store-misaligned then load-AF sequence",
        read64(ma_paddr) == ma_low_seed &&
        read64(ma_paddr + sizeof(uint64_t)) == ma_high_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic store-misaligned then load-AF sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_store_load_exception_pair_oldest_store_access_fault_then_load_misaligned_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t ma_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MA_PAGE) +
        AI_MICRO_EXCEPTION_ORDER_MA_OFFSET;
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0xd1d3d5d7d9dbdddfULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_misaligned_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_load_fault_pair_capture(af_vaddr, ma_vaddr, marker_vaddr,
        0xe1e3e5e7e9ebedefULL, &first);

    TEST_ASSERT("in a diagnostic HS store-then-load fault pair with older store AF and younger load misaligned, the first observed exception is the older store access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger HS load in the same diagnostic store-load pair falls through to the expected load misaligned trap",
        excpt.triggered == true &&
        excpt.cause == CAUSE_MISALIGNED_LOAD &&
        excpt.tval == ma_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older AF store target remains unchanged across the diagnostic store-AF then load-misaligned sequence",
        read64(af_paddr) == af_seed
    );
    TEST_ASSERT("the trailing marker store in the diagnostic store-AF then load-misaligned sequence executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );
    ai_micro_exception_order_reset_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_single_step_exception_multi_source_same_cycle_oldest_must_win_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, 0x1122334455667788ULL);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_amo_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr,
        0x0102030405060708ULL, &first);

    TEST_ASSERT("in the LSQ-plus-AMO diagnostic pair with older load PF and younger AMO AF, the first observed precise trap remains the older load page fault",
        first.triggered == true &&
        first.cause == CAUSE_LPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger AMO in that diagnostic pair still reports the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger AF AMO leaves its destination word untouched in the older-load-first LSQ-plus-AMO diagnostic pair",
        read64(af_paddr) == 0x1122334455667788ULL
    );
    TEST_ASSERT("the trailing marker store in the older-load-first LSQ-plus-AMO diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    first = (struct ai_micro_exception_snapshot){0};

    goto_priv(PRIV_M);
    write64(af_paddr, 0x8899aabbccddeeffULL);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_amo_load_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr,
        0x1111111122222222ULL, &first);

    TEST_ASSERT("in the reverse LSQ-plus-AMO diagnostic pair with older AMO AF and younger load PF, the first observed precise trap remains the older AMO access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger load in the reverse LSQ-plus-AMO diagnostic pair still reports the expected page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older AF AMO leaves its destination word untouched in the reverse LSQ-plus-AMO diagnostic pair",
        read64(af_paddr) == 0x8899aabbccddeeffULL
    );
    TEST_ASSERT("the trailing marker store in the reverse LSQ-plus-AMO diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_lsq_amo_same_cycle_exception_no_mux1h_age_loss_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, 0x13579bdf2468ace0ULL);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_amo_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr,
        0x3333333344444444ULL, &first);

    TEST_ASSERT("in the LSQ-plus-AMO diagnostic pair with older load AF and younger AMO PF, the first observed precise trap remains the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger AMO in that diagnostic pair still reports the expected store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older AF load and younger PF AMO sequence leaves the AF backing word unchanged in the LSQ-plus-AMO diagnostic pair",
        read64(af_paddr) == 0x13579bdf2468ace0ULL
    );
    TEST_ASSERT("the trailing marker store in the older-load-AF LSQ-plus-AMO diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    first = (struct ai_micro_exception_snapshot){0};

    goto_priv(PRIV_M);
    write64(af_paddr, 0x0fedcba987654321ULL);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_amo_load_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr,
        0x5555555566666666ULL, &first);

    TEST_ASSERT("in the reverse LSQ-plus-AMO diagnostic pair with older AMO PF and younger load AF, the first observed precise trap remains the older AMO page fault",
        first.triggered == true &&
        first.cause == CAUSE_SPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger load in the reverse LSQ-plus-AMO diagnostic pair still reports the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger AF load leaves its backing word unchanged in the reverse LSQ-plus-AMO diagnostic pair",
        read64(af_paddr) == 0x0fedcba987654321ULL
    );
    TEST_ASSERT("the trailing marker store in the reverse LSQ-plus-AMO diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    TEST_END();
}

bool ai_micro_hs_mbexceptionbuffer_flushed_resident_replaced_by_same_cycle_new_exception_corner()
{
    TEST_START();

    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x2468ace013579bdfULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_load_store_fault_pair_capture(pf_vaddr, af_vaddr, marker_vaddr,
        0xdeadbeefcafef00dULL, &first);

    TEST_ASSERT("in the resident-exception replacement diagnostic sequence, the older load page fault is observed first before the trap redirect flushes it",
        first.triggered == true &&
        first.cause == CAUSE_LPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after that redirect, the younger store access fault reports its own address instead of a stale buffered older exception entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the younger AF store target remains unchanged and the marker store still executes only after both precise traps in the resident-replacement diagnostic sequence",
        read64(af_paddr) == af_seed &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_exception_order_reset_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t recovered = ld(af_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after resetting HS translation, the formerly faulting younger store target is immediately readable, showing no stale resident exception remains visible",
        recovered == af_seed
    );

    TEST_END();
}

bool ai_micro_hs_redirect_flush_old_exception_no_stale_resident_entry_after_new_enqueue_corner()
{
    TEST_START();

    const uintptr_t af_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t af_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_AF_PAGE);
    const uintptr_t pf_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_PF_PAGE);
    const uintptr_t marker_vaddr = hs_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uintptr_t marker_paddr = phys_page_base(AI_MICRO_EXCEPTION_ORDER_MARKER_PAGE);
    const uint64_t af_seed = 0x3141592653589793ULL;
    const uint64_t pf_seed = 0x0102030405060708ULL;
    struct ai_micro_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, af_seed);
    write64(pf_paddr, pf_seed);
    write64(marker_paddr, 0ULL);

    ai_micro_exception_order_prepare_fault_pair_env();

    TEST_SETUP_EXCEPT();
    ai_micro_store_load_fault_pair_capture(af_vaddr, pf_vaddr, marker_vaddr,
        0x9988776655443322ULL, &first);

    TEST_ASSERT("in the redirect replacement diagnostic sequence, the older store access fault is observed first before the trap redirect flushes that resident exception",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == af_vaddr
    );
    TEST_ASSERT("after that redirect, the younger load page fault reports its own address instead of the stale older access-fault entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the older AF store target, the younger page-fault backing word, and the trailing marker all keep their expected state across the redirect replacement diagnostic sequence",
        read64(af_paddr) == af_seed &&
        read64(pf_paddr) == pf_seed &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_exception_order_reset_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t recovered = ld(af_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after resetting HS translation, the previously access-faulting backing word is readable immediately, further confirming that no stale resident exception entry survives the redirect path",
        recovered == af_seed
    );

    TEST_END();
}
