#include "ai_directed_random_stress_common.h"

#include <page_tables.h>

#define AI_STRESS_CACHE_LINE_BYTES 64U
#define AI_STRESS_LINE_WORDS       8U
#define AI_STRESS_LOAD_LINES       64U
#define AI_STRESS_STORE_LINES      32U
#define AI_STRESS_BRANCH_STEPS     64U
#define AI_STRESS_VECTOR_BYTES     128U
#define AI_STRESS_VECTOR_STORAGE   192U
#define AI_STRESS_VECTOR_REPEATS   8U
#define AI_STRESS_MMU_PAGES        16U
#define AI_STRESS_MSTATUS_VS_DIRTY (3ULL << 9U)

#define AI_STRESS_LEFT_SENTINEL  0x4c4546542d534545ULL
#define AI_STRESS_RIGHT_SENTINEL 0x52494748542d5345ULL
#define AI_STRESS_GUARD_SENTINEL 0x47554152442d5345ULL

static volatile uint64_t ai_stress_load_lines
    [AI_STRESS_LOAD_LINES][AI_STRESS_LINE_WORDS]
    __attribute__((aligned(4096)));
static volatile uint64_t ai_stress_store_lines
    [AI_STRESS_STORE_LINES][AI_STRESS_LINE_WORDS]
    __attribute__((aligned(4096)));
static volatile uint64_t ai_stress_branch_conditions
    [AI_STRESS_BRANCH_STEPS][AI_STRESS_LINE_WORDS]
    __attribute__((aligned(4096)));
static volatile uint64_t ai_stress_branch_slots
    [AI_STRESS_BRANCH_STEPS][AI_STRESS_LINE_WORDS]
    __attribute__((aligned(4096)));
static volatile uint8_t ai_stress_vector_src[AI_STRESS_VECTOR_BYTES]
    __attribute__((aligned(64)));
static volatile uint8_t ai_stress_vector_dst
    [AI_STRESS_VECTOR_REPEATS][AI_STRESS_VECTOR_STORAGE]
    __attribute__((aligned(64)));
static volatile uint64_t ai_stress_vector_conditions
    [AI_STRESS_VECTOR_REPEATS][AI_STRESS_LINE_WORDS]
    __attribute__((aligned(64)));
static volatile uint64_t ai_stress_atomic_line[AI_STRESS_LINE_WORDS]
    __attribute__((aligned(64)));
static uintptr_t ai_stress_mmu_vaddrs[AI_STRESS_MMU_PAGES]
    __attribute__((aligned(64)));

static const enum test_page ai_stress_mmu_pages[AI_STRESS_MMU_PAGES] = {
    VSRWX_GURWX,
    VSRWX_GURW,
    VSRWX_GURX,
    VSRWX_GUR,
    VSRWX_GUX,
    VSRW_GURWX,
    VSRW_GURW,
    VSRW_GURX,
    VSRW_GUR,
    VSRW_GUX,
    VSRWX_GRWX,
    VSRWX_GRW,
    VSRWX_GRX,
    VSRWX_GR,
    VSRWX_GX,
    VSRW_GRWX,
};

struct ai_stress_mix_args {
    uintptr_t condition;
    uint64_t store_payload;
    uint64_t store_delta;
    uint64_t index_zero;
    uint64_t index_one;
    uint64_t backend_seed;
};

static inline void ai_stress_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_STRESS_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_stress_flush_lines(uintptr_t base, unsigned line_count)
{
    for (unsigned index = 0; index < line_count; ++index) {
        cbo_flush(base + (uintptr_t)index * AI_STRESS_CACHE_LINE_BYTES);
    }
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_stress_scalar_load_burst(uintptr_t base)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[base]\n\t"
        "li t1, 64\n\t"
        "mv %[result], zero\n\t"
        "1:\n\t"
        "ld t2, 0(t0)\n\t"
        "xor %[result], %[result], t2\n\t"
        "addi t0, t0, 64\n\t"
        "addi t1, t1, -1\n\t"
        "bnez t1, 1b\n\t"
        ".option pop\n\t"
        : [result] "=&r"(result)
        : [base] "r"(base)
        : "t0", "t1", "t2", "memory");

    return result;
}

__attribute__((noinline, aligned(64))) static void
ai_stress_scalar_store_burst(uintptr_t base, uint64_t payload,
    uint64_t delta)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[base]\n\t"
        "mv t1, %[payload]\n\t"
        "mv t2, %[delta]\n\t"
        "li t3, 32\n\t"
        "1:\n\t"
        "sd t1, 0(t0)\n\t"
        "add t1, t1, t2\n\t"
        "addi t0, t0, 64\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base), [payload] "r"(payload), [delta] "r"(delta)
        : "t0", "t1", "t2", "t3", "memory");
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_stress_branch_store_step(uintptr_t condition, uintptr_t left,
    uintptr_t right, uint64_t payload, uint64_t salt)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t0, 0(%[condition])\n\t"
        "li t1, 97\n\t"
        "remu t0, t0, t1\n\t"
        "beqz t0, 1f\n\t"
        "xor t2, %[payload], %[salt]\n\t"
        "slli t3, t2, 7\n\t"
        "srli t4, t2, 57\n\t"
        "or t2, t3, t4\n\t"
        "sd t2, 0(%[right])\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "add t2, %[payload], %[salt]\n\t"
        "sd t2, 0(%[left])\n\t"
        "2:\n\t"
        "mv %[result], t2\n\t"
        ".option pop\n\t"
        : [result] "=&r"(result)
        : [condition] "r"(condition), [left] "r"(left),
          [right] "r"(right), [payload] "r"(payload),
          [salt] "r"(salt)
        : "t0", "t1", "t2", "t3", "t4", "memory");

    return result;
}

static inline size_t ai_stress_vector_vl(void)
{
    size_t vl;

    asm volatile(
        "vsetvli %0, %1, e8, m8, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        : "=r"(vl)
        : "r"((size_t)AI_STRESS_VECTOR_BYTES)
        : "memory");
    return vl;
}

__attribute__((noinline, aligned(64))) static size_t
ai_stress_vector65_branch_gather(uintptr_t condition,
    const volatile uint8_t *src, volatile uint8_t *dst,
    size_t avl, uint64_t index_zero, uint64_t index_one)
{
    size_t actual_vl;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli %[actual_vl], %[avl], e8, m8, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v16, (%[src])\n\t"
        "ld t0, 0(%[condition])\n\t"
        "li t1, 97\n\t"
        "remu t0, t0, t1\n\t"
        "beqz t0, 1f\n\t"
        "vrgather.vx v8, v16, %[index_one]\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "vrgather.vx v8, v16, %[index_zero]\n\t"
        "2:\n\t"
        "vse8.v v8, (%[dst])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [actual_vl] "=&r"(actual_vl)
        : [condition] "r"(condition), [src] "r"(src), [dst] "r"(dst),
          [avl] "r"(avl), [index_zero] "r"(index_zero),
          [index_one] "r"(index_one)
        : "t0", "t1", "memory");

    return actual_vl;
}

__attribute__((noinline, aligned(64))) static void
ai_stress_three_source_mix_kernel(uintptr_t load_base,
    uintptr_t store_base, const volatile uint8_t *vector_src,
    volatile uint8_t *vector_dst, size_t avl,
    const struct ai_stress_mix_args *args, uint64_t *load_signature,
    uint64_t *backend_signature, size_t *actual_vl)
{
    uint64_t load_result;
    uint64_t backend_result;
    size_t vl_result;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[load_base]\n\t"
        "mv t1, %[store_base]\n\t"
        "mv %[load_result], zero\n\t"
        "vsetvli %[vl_result], %[avl], e8, m8, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v16, (%[vector_src])\n\t"
        "ld t2, 40(%[args])\n\t"
        "li t3, 97\n\t"
        ".rept 24\n\t"
        "divu %[backend_result], t2, t3\n\t"
        ".endr\n\t"
        "ld t2, 0(%[args])\n\t"
        "ld t2, 0(t2)\n\t"
        "remu t2, t2, t3\n\t"
        "ld t6, 24(%[args])\n\t"
        "beqz t2, 1f\n\t"
        "ld t6, 32(%[args])\n\t"
        "vrgather.vx v8, v16, t6\n\t"
        "j 2f\n\t"
        "1:\n\t"
        "vrgather.vx v8, v16, t6\n\t"
        "2:\n\t"
        "ld t5, 8(%[args])\n\t"
        "ld t4, 16(%[args])\n\t"
        "li t3, 8\n\t"
        "3:\n\t"
        "ld t2, 0(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 64(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 128(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 192(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 256(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 320(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 384(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "ld t2, 448(t0)\n\t"
        "xor %[load_result], %[load_result], t2\n\t"
        "sd t5, 0(t1)\n\t"
        "add t5, t5, t4\n\t"
        "sd t5, 64(t1)\n\t"
        "add t5, t5, t4\n\t"
        "sd t5, 128(t1)\n\t"
        "add t5, t5, t4\n\t"
        "sd t5, 192(t1)\n\t"
        "add t5, t5, t4\n\t"
        "vrgather.vx v8, v16, t6\n\t"
        "addi t0, t0, 512\n\t"
        "addi t1, t1, 256\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 3b\n\t"
        "vse8.v v8, (%[vector_dst])\n\t"
        ".option pop\n\t"
        : [load_result] "=&r"(load_result),
          [backend_result] "=&r"(backend_result),
          [vl_result] "=&r"(vl_result)
        : [load_base] "r"(load_base), [store_base] "r"(store_base),
          [vector_src] "r"(vector_src), [vector_dst] "r"(vector_dst),
          [avl] "r"(avl), [args] "r"(args)
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");

    *load_signature = load_result;
    *backend_signature = backend_result;
    *actual_vl = vl_result;
}

__attribute__((noinline, aligned(64))) static uint64_t
ai_stress_mmu_load_burst(const uintptr_t *addresses)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[addresses]\n\t"
        "li t1, 16\n\t"
        "mv %[result], zero\n\t"
        "1:\n\t"
        "ld t2, 0(t0)\n\t"
        "ld t3, 0(t2)\n\t"
        "xor %[result], %[result], t3\n\t"
        "addi t0, t0, 8\n\t"
        "addi t1, t1, -1\n\t"
        "bnez t1, 1b\n\t"
        ".option pop\n\t"
        : [result] "=&r"(result)
        : [addresses] "r"(addresses)
        : "t0", "t1", "t2", "t3", "memory");

    return result;
}

__attribute__((noinline, aligned(64))) static void
ai_stress_mmu_store_burst(const uintptr_t *addresses, uint64_t payload,
    uint64_t delta)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[addresses]\n\t"
        "mv t1, %[payload]\n\t"
        "mv t2, %[delta]\n\t"
        "li t3, 16\n\t"
        "1:\n\t"
        "ld t4, 0(t0)\n\t"
        "sd t1, 0(t4)\n\t"
        "add t1, t1, t2\n\t"
        "addi t0, t0, 8\n\t"
        "addi t3, t3, -1\n\t"
        "bnez t3, 1b\n\t"
        ".option pop\n\t"
        :
        : [addresses] "r"(addresses), [payload] "r"(payload),
          [delta] "r"(delta)
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

static inline pte_t *ai_stress_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

bool ai_stress_directed_random_branch_mem_cleanup_signature_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    for (unsigned round = 0; round < AI_STRESS_SEED_COUNT; ++round) {
        const uint64_t seed = ai_stress_seed(1U, round);
        struct ai_stress_rng rng;
        uint64_t expected_slots[AI_STRESS_BRANCH_STEPS][2];
        uint64_t expected_load_signature = 0ULL;
        uint64_t branch_signature = 0x243f6a8885a308d3ULL;
        uint64_t pattern;
        uint64_t store_payload;
        uint64_t store_delta;
        bool memory_ok = true;
        bool store_ok = true;
        bool guard_ok = true;

        ai_stress_rng_init(&rng, seed);
        pattern = ai_stress_balanced_branch_pattern(&rng);
        store_payload = ai_stress_rng_next(&rng);
        store_delta = ai_stress_rng_next(&rng) | 1ULL;

        for (unsigned index = 0; index < AI_STRESS_LOAD_LINES; ++index) {
            const uint64_t value = ai_stress_rng_next(&rng) ^
                ((uint64_t)index << 32U);

            ai_stress_load_lines[index][0] = value;
            ai_stress_load_lines[index][1] = AI_STRESS_GUARD_SENTINEL ^ index;
            expected_load_signature ^= value;
        }
        for (unsigned index = 0; index < AI_STRESS_STORE_LINES; ++index) {
            ai_stress_store_lines[index][0] =
                0xdddd000000000000ULL | index;
            ai_stress_store_lines[index][1] =
                AI_STRESS_GUARD_SENTINEL ^ (0x200U + index);
        }
        for (unsigned index = 0; index < AI_STRESS_BRANCH_STEPS; ++index) {
            const unsigned direction = (unsigned)((pattern >> index) & 1ULL);
            const uint64_t token_base =
                ai_stress_rng_next(&rng) & 0x00ffffffffffffffULL;
            const uint64_t payload = ai_stress_rng_next(&rng);
            const uint64_t salt = ai_stress_rng_next(&rng);

            ai_stress_branch_conditions[index][0] =
                token_base * 97ULL + direction;
            ai_stress_branch_slots[index][0] = AI_STRESS_LEFT_SENTINEL;
            ai_stress_branch_slots[index][1] = AI_STRESS_RIGHT_SENTINEL;
            ai_stress_branch_slots[index][2] = AI_STRESS_GUARD_SENTINEL;
            expected_slots[index][0] = direction == 0U ?
                payload + salt : AI_STRESS_LEFT_SENTINEL;
            expected_slots[index][1] = direction != 0U ?
                ai_stress_rotl64(payload ^ salt, 7U) :
                AI_STRESS_RIGHT_SENTINEL;
            ai_stress_branch_slots[index][3] = payload;
            ai_stress_branch_slots[index][4] = salt;
        }

        TEST_SETUP_EXCEPT();
        ai_stress_flush_lines((uintptr_t)&ai_stress_load_lines[0][0],
            AI_STRESS_LOAD_LINES);
        ai_stress_flush_lines((uintptr_t)&ai_stress_store_lines[0][0],
            AI_STRESS_STORE_LINES);
        ai_stress_flush_lines(
            (uintptr_t)&ai_stress_branch_conditions[0][0],
            AI_STRESS_BRANCH_STEPS);
        fence_iorw_iorw();

        const uint64_t observed_load_signature =
            ai_stress_scalar_load_burst(
                (uintptr_t)&ai_stress_load_lines[0][0]);
        ai_stress_scalar_store_burst(
            (uintptr_t)&ai_stress_store_lines[0][0],
            store_payload, store_delta);

        for (unsigned index = 0; index < AI_STRESS_BRANCH_STEPS; ++index) {
            const uint64_t result = ai_stress_branch_store_step(
                (uintptr_t)&ai_stress_branch_conditions[index][0],
                (uintptr_t)&ai_stress_branch_slots[index][0],
                (uintptr_t)&ai_stress_branch_slots[index][1],
                ai_stress_branch_slots[index][3],
                ai_stress_branch_slots[index][4]);

            branch_signature = ai_stress_signature_fold(branch_signature,
                result ^ index);
        }
        fence_iorw_iorw();

        for (unsigned index = 0; index < AI_STRESS_BRANCH_STEPS; ++index) {
            memory_ok = memory_ok &&
                ai_stress_branch_slots[index][0] ==
                    expected_slots[index][0] &&
                ai_stress_branch_slots[index][1] ==
                    expected_slots[index][1];
            guard_ok = guard_ok &&
                ai_stress_branch_slots[index][2] ==
                    AI_STRESS_GUARD_SENTINEL;
        }
        for (unsigned index = 0; index < AI_STRESS_STORE_LINES; ++index) {
            store_ok = store_ok &&
                ai_stress_store_lines[index][0] ==
                    store_payload + store_delta * index &&
                ai_stress_store_lines[index][1] ==
                    (AI_STRESS_GUARD_SENTINEL ^ (0x200U + index));
        }

        const unsigned pressure_mask = AI_STRESS_PRESSURE_BRANCH |
            AI_STRESS_PRESSURE_SCALAR_LOAD |
            AI_STRESS_PRESSURE_SCALAR_STORE |
            AI_STRESS_PRESSURE_BACKEND;
        const unsigned cleanup_mask = AI_STRESS_CLEANUP_REDIRECT |
            AI_STRESS_CLEANUP_FENCE;
        const bool contract_ok = ai_stress_popcount(pressure_mask) >= 2U &&
            cleanup_mask != 0U;
        const bool seed_ok = contract_ok && excpt.triggered == false &&
            observed_load_signature == expected_load_signature &&
            memory_ok && store_ok && guard_ok;
        const uint64_t signature = ai_stress_signature_fold(
            branch_signature, observed_load_signature);

        TEST_ASSERT("a constrained-random balanced branch stream keeps every wrong-path store invisible while a 64-line cold-load burst and delayed REMU branch resolution are active",
            seed_ok,
            "seed=0x%llx load=0x%llx expected=0x%llx memory_ok=%d store_ok=%d guard_ok=%d",
            (unsigned long long)seed,
            (unsigned long long)observed_load_signature,
            (unsigned long long)expected_load_signature,
            memory_ok, store_ok, guard_ok);
        ai_stress_log_seed("branch_mem_wrongpath", seed, round,
            pressure_mask, cleanup_mask,
            "LQ/MissQueue:64-line-cold-load-candidate",
            "SQ/SBuffer:32-line+64-exclusive-store-candidate",
            "backend:REMU-delayed-branch-resolution",
            "redirect+fence", "branch-slot-image+signature",
            signature, seed_ok);
    }

    TEST_END();
}

bool ai_stress_directed_random_vector65_redirect_signature_corner()
{
    TEST_START();

    ai_stress_enable_vector_state();
    TEST_SETUP_EXCEPT();
    const size_t vl = ai_stress_vector_vl();
    TEST_ASSERT("the vector65 stress profile obtains a nonzero e8,m8 VL within its 128-byte scoreboard",
        excpt.triggered == false && vl > 0U &&
        vl <= AI_STRESS_VECTOR_BYTES,
        "vl=%lu", (unsigned long)vl);

    for (unsigned round = 0; round < AI_STRESS_SEED_COUNT; ++round) {
        const uint64_t seed = ai_stress_seed(2U, round);
        struct ai_stress_rng rng;
        const size_t safe_vl = vl != 0U ? vl : 1U;
        uint64_t signature = 0x13198a2e03707344ULL;
        bool image_ok = true;

        ai_stress_rng_init(&rng, seed);
        for (unsigned index = 0; index < AI_STRESS_VECTOR_BYTES; ++index) {
            ai_stress_vector_src[index] =
                (uint8_t)ai_stress_rng_next(&rng);
        }
        for (unsigned repeat = 0; repeat < AI_STRESS_VECTOR_REPEATS;
             ++repeat) {
            const uint8_t sentinel = (uint8_t)(0xa5U ^ (repeat * 13U));
            const unsigned direction =
                (unsigned)(ai_stress_rng_next(&rng) & 1ULL);
            const uint64_t token_base =
                ai_stress_rng_next(&rng) & 0x00ffffffffffffffULL;
            const uint64_t index_zero =
                ai_stress_rng_next(&rng) % safe_vl;
            const uint64_t index_one =
                ai_stress_rng_next(&rng) % safe_vl;

            ai_stress_vector_conditions[repeat][0] =
                token_base * 97ULL + direction;
            for (unsigned byte = 0; byte < AI_STRESS_VECTOR_STORAGE;
                 ++byte) {
                ai_stress_vector_dst[repeat][byte] = sentinel;
            }

            TEST_SETUP_EXCEPT();
            ai_stress_flush_lines(
                (uintptr_t)&ai_stress_vector_src[0],
                AI_STRESS_VECTOR_BYTES / AI_STRESS_CACHE_LINE_BYTES);
            ai_stress_flush_lines(
                (uintptr_t)&ai_stress_vector_dst[repeat][0],
                AI_STRESS_VECTOR_STORAGE / AI_STRESS_CACHE_LINE_BYTES);
            cbo_flush((uintptr_t)&ai_stress_vector_conditions[repeat][0]);
            fence_iorw_iorw();

            const size_t actual_vl = ai_stress_vector65_branch_gather(
                (uintptr_t)&ai_stress_vector_conditions[repeat][0],
                &ai_stress_vector_src[0],
                &ai_stress_vector_dst[repeat][0], vl,
                index_zero, index_one);
            const uint64_t selected_index = direction == 0U ?
                index_zero : index_one;
            const uint8_t expected =
                ai_stress_vector_src[selected_index];

            image_ok = image_ok && excpt.triggered == false &&
                actual_vl == vl;
            for (unsigned byte = 0; byte < AI_STRESS_VECTOR_STORAGE;
                 ++byte) {
                const uint8_t expected_byte = byte < vl ?
                    expected : sentinel;
                image_ok = image_ok &&
                    ai_stress_vector_dst[repeat][byte] == expected_byte;
            }
            signature = ai_stress_signature_fold(signature,
                ai_stress_byte_signature(
                    &ai_stress_vector_dst[repeat][0],
                    AI_STRESS_VECTOR_STORAGE));
        }

        const unsigned pressure_mask = AI_STRESS_PRESSURE_BRANCH |
            AI_STRESS_PRESSURE_SCALAR_LOAD |
            AI_STRESS_PRESSURE_VECTOR |
            AI_STRESS_PRESSURE_BACKEND;
        const unsigned cleanup_mask = AI_STRESS_CLEANUP_REDIRECT |
            AI_STRESS_CLEANUP_FENCE;
        const bool contract_ok = ai_stress_popcount(pressure_mask) >= 2U &&
            cleanup_mask != 0U;
        const bool seed_ok = contract_ok && image_ok;

        TEST_ASSERT("eight constrained-random redirect windows each retire the selected e8,m8 vrgather.vx image and preserve the unselected tail/guard image",
            seed_ok,
            "seed=0x%llx vl=%lu image_ok=%d signature=0x%llx",
            (unsigned long long)seed, (unsigned long)vl, image_ok,
            (unsigned long long)signature);
        ai_stress_log_seed("vector65_redirect", seed, round,
            pressure_mask, cleanup_mask,
            "DecodeComplex:vrgather.vx-e8-m8-65uop",
            "VLSU/VMerge:8-load-gather-store-windows",
            "branch:load+REMU-delayed-resolution",
            "redirect+fence", "vector-byte-image+tail-guards",
            signature, seed_ok);
    }

    TEST_END();
}

bool ai_stress_directed_random_three_source_dram_scoreboard_corner()
{
    TEST_START();

    ai_stress_enable_vector_state();
    TEST_SETUP_EXCEPT();
    const size_t vl = ai_stress_vector_vl();
    TEST_ASSERT("the ordinary-DRAM three-source profile obtains a nonzero e8,m8 VL within its scoreboard",
        excpt.triggered == false && vl > 0U &&
        vl <= AI_STRESS_VECTOR_BYTES,
        "vl=%lu", (unsigned long)vl);

    for (unsigned round = 0; round < AI_STRESS_SEED_COUNT; ++round) {
        const uint64_t seed = ai_stress_seed(3U, round);
        struct ai_stress_rng rng;
        struct ai_stress_mix_args args;
        const size_t safe_vl = vl != 0U ? vl : 1U;
        uint64_t expected_load_signature = 0ULL;
        uint64_t observed_load_signature = 0ULL;
        uint64_t observed_backend_signature = 0ULL;
        size_t actual_vl = 0U;
        bool load_guards_ok = true;
        bool store_image_ok = true;
        bool vector_image_ok = true;

        ai_stress_rng_init(&rng, seed);
        for (unsigned index = 0; index < AI_STRESS_LOAD_LINES; ++index) {
            const uint64_t value = ai_stress_rng_next(&rng) ^
                (0x100000001b3ULL * index);

            ai_stress_load_lines[index][0] = value;
            ai_stress_load_lines[index][1] =
                AI_STRESS_GUARD_SENTINEL ^ index;
            expected_load_signature ^= value;
        }
        for (unsigned index = 0; index < AI_STRESS_STORE_LINES; ++index) {
            ai_stress_store_lines[index][0] =
                0xcccc000000000000ULL | index;
            ai_stress_store_lines[index][1] =
                AI_STRESS_GUARD_SENTINEL ^ (0x100U + index);
        }
        for (unsigned index = 0; index < AI_STRESS_VECTOR_BYTES; ++index) {
            ai_stress_vector_src[index] =
                (uint8_t)ai_stress_rng_next(&rng);
        }
        for (unsigned index = 0; index < AI_STRESS_VECTOR_STORAGE; ++index) {
            ai_stress_vector_dst[0][index] = 0x5aU;
        }

        const unsigned direction =
            (unsigned)(ai_stress_rng_next(&rng) & 1ULL);
        const uint64_t token_base =
            ai_stress_rng_next(&rng) & 0x00ffffffffffffffULL;
        ai_stress_vector_conditions[0][0] =
            token_base * 97ULL + direction;
        args.condition =
            (uintptr_t)&ai_stress_vector_conditions[0][0];
        args.store_payload = ai_stress_rng_next(&rng);
        args.store_delta = ai_stress_rng_next(&rng) | 1ULL;
        args.index_zero = ai_stress_rng_next(&rng) % safe_vl;
        args.index_one = ai_stress_rng_next(&rng) % safe_vl;
        args.backend_seed = ai_stress_rng_next(&rng);

        const uint64_t atomic_seed = ai_stress_rng_next(&rng);
        const uint64_t atomic_addend = ai_stress_rng_next(&rng) | 1ULL;
        ai_stress_atomic_line[0] = atomic_seed;
        ai_stress_atomic_line[1] = AI_STRESS_GUARD_SENTINEL;

        TEST_SETUP_EXCEPT();
        ai_stress_flush_lines((uintptr_t)&ai_stress_load_lines[0][0],
            AI_STRESS_LOAD_LINES);
        ai_stress_flush_lines((uintptr_t)&ai_stress_store_lines[0][0],
            AI_STRESS_STORE_LINES);
        ai_stress_flush_lines((uintptr_t)&ai_stress_vector_src[0],
            AI_STRESS_VECTOR_BYTES / AI_STRESS_CACHE_LINE_BYTES);
        ai_stress_flush_lines((uintptr_t)&ai_stress_vector_dst[0][0],
            AI_STRESS_VECTOR_STORAGE / AI_STRESS_CACHE_LINE_BYTES);
        cbo_flush((uintptr_t)&ai_stress_vector_conditions[0][0]);
        cbo_flush((uintptr_t)&ai_stress_atomic_line[0]);
        fence_iorw_iorw();

        ai_stress_three_source_mix_kernel(
            (uintptr_t)&ai_stress_load_lines[0][0],
            (uintptr_t)&ai_stress_store_lines[0][0],
            &ai_stress_vector_src[0], &ai_stress_vector_dst[0][0],
            vl, &args, &observed_load_signature,
            &observed_backend_signature, &actual_vl);
        const uint64_t atomic_old = amoadd_d(
            (uintptr_t)&ai_stress_atomic_line[0], atomic_addend);
        fence_iorw_iorw();

        for (unsigned index = 0; index < AI_STRESS_LOAD_LINES; ++index) {
            load_guards_ok = load_guards_ok &&
                ai_stress_load_lines[index][1] ==
                    (AI_STRESS_GUARD_SENTINEL ^ index);
        }
        for (unsigned index = 0; index < AI_STRESS_STORE_LINES; ++index) {
            store_image_ok = store_image_ok &&
                ai_stress_store_lines[index][0] ==
                    args.store_payload + args.store_delta * index &&
                ai_stress_store_lines[index][1] ==
                    (AI_STRESS_GUARD_SENTINEL ^ (0x100U + index));
        }
        const uint64_t selected_index = direction == 0U ?
            args.index_zero : args.index_one;
        const uint8_t expected_vector_byte =
            ai_stress_vector_src[selected_index];
        for (unsigned byte = 0; byte < AI_STRESS_VECTOR_STORAGE; ++byte) {
            vector_image_ok = vector_image_ok &&
                ai_stress_vector_dst[0][byte] ==
                    (byte < vl ? expected_vector_byte : 0x5aU);
        }

        const unsigned pressure_mask = AI_STRESS_PRESSURE_BRANCH |
            AI_STRESS_PRESSURE_SCALAR_LOAD |
            AI_STRESS_PRESSURE_SCALAR_STORE |
            AI_STRESS_PRESSURE_VECTOR |
            AI_STRESS_PRESSURE_BACKEND |
            AI_STRESS_PRESSURE_ATOMIC;
        const unsigned cleanup_mask = AI_STRESS_CLEANUP_REDIRECT |
            AI_STRESS_CLEANUP_FENCE;
        const bool contract_ok = ai_stress_popcount(pressure_mask) >= 3U &&
            cleanup_mask != 0U;
        const bool seed_ok = contract_ok && excpt.triggered == false &&
            observed_load_signature == expected_load_signature &&
            observed_backend_signature == args.backend_seed / 97ULL &&
            actual_vl == vl && load_guards_ok && store_image_ok &&
            vector_image_ok && atomic_old == atomic_seed &&
            ai_stress_atomic_line[0] == atomic_seed + atomic_addend &&
            ai_stress_atomic_line[1] == AI_STRESS_GUARD_SENTINEL;
        uint64_t signature = ai_stress_signature_fold(
            observed_load_signature, observed_backend_signature);
        signature = ai_stress_signature_fold(signature,
            ai_stress_byte_signature(&ai_stress_vector_dst[0][0],
                AI_STRESS_VECTOR_STORAGE));
        signature = ai_stress_signature_fold(signature,
            ai_stress_atomic_line[0]);

        TEST_ASSERT("every ordinary-DRAM seed schedules scalar load, scalar store, vector65/backend, atomic and redirect pressure while the byte/word scoreboard explains the final image",
            seed_ok,
            "seed=0x%llx load=0x%llx expected=0x%llx backend=0x%llx store_ok=%d vector_ok=%d atomic_old=0x%llx",
            (unsigned long long)seed,
            (unsigned long long)observed_load_signature,
            (unsigned long long)expected_load_signature,
            (unsigned long long)observed_backend_signature,
            store_image_ok, vector_image_ok,
            (unsigned long long)atomic_old);
        ai_stress_log_seed("three_source_dram", seed, round,
            pressure_mask, cleanup_mask,
            "LQ/MissQueue:64-line-cold-load-candidate",
            "SQ/SBuffer:32-line-store-candidate",
            "Decode/VMerge:9x-vrgather.vx-65uop",
            "redirect+fence+AMO",
            "scalar-word+vector-byte+atomic-scoreboard",
            signature, seed_ok);
    }

    TEST_END();
}

bool ai_stress_directed_random_mmu_sfence_recovery_signature_corner()
{
    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_STRESS_SEED_COUNT; ++round) {
        const uint64_t seed = ai_stress_seed(4U, round);
        struct ai_stress_rng rng;
        const uintptr_t offset = 0x100U +
            ((seed >> 8U) & 0x3fULL) * sizeof(uint64_t);
        uint64_t initial_expected = 0ULL;
        uint64_t recovery_expected = 0ULL;
        bool backing_ok = true;

        ai_stress_rng_init(&rng, seed);
        goto_priv(PRIV_M);
        for (unsigned index = 0; index < AI_STRESS_MMU_PAGES; ++index) {
            const uint64_t value = ai_stress_rng_next(&rng) ^
                ((uint64_t)index << 48U);
            const uintptr_t vaddr =
                hs_page_base(ai_stress_mmu_pages[index]) + offset;
            const uintptr_t paddr =
                phys_page_base(ai_stress_mmu_pages[index]) + offset;

            ai_stress_mmu_vaddrs[index] = vaddr;
            write64(paddr, value);
            initial_expected ^= value;
        }
        const unsigned fault_index =
            (unsigned)(ai_stress_rng_next(&rng) % AI_STRESS_MMU_PAGES);
        const uintptr_t fault_vaddr = ai_stress_mmu_vaddrs[fault_index];
        pte_t *const fault_pte =
            ai_stress_hs_leaf_entry(ai_stress_mmu_pages[fault_index]);
        const pte_t saved_pte = *fault_pte;

        goto_priv(PRIV_HS);
        sfence_vma();
        TEST_SETUP_EXCEPT();
        const uint64_t warm_signature =
            ai_stress_mmu_load_burst(ai_stress_mmu_vaddrs);
        const bool warm_ok = excpt.triggered == false &&
            warm_signature == initial_expected;

        goto_priv(PRIV_M);
        *fault_pte = saved_pte & ~PTE_V;
        sfence_vma_rs(fault_vaddr, (uint64_t)-1);
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t fault_value = ld(fault_vaddr);
        (void)fault_value;
        const uint64_t fault_cause = excpt.cause;
        const uint64_t fault_tval = excpt.tval;
        const bool fault_ok = excpt.triggered == true &&
            fault_cause == CAUSE_LPF && fault_tval == fault_vaddr;
        const uint64_t fault_metadata =
            fault_cause ^ ai_stress_rotl64(fault_tval, 11U);

        goto_priv(PRIV_M);
        *fault_pte = saved_pte;
        sfence_vma_rs(fault_vaddr, (uint64_t)-1);
        const uint64_t store_payload = ai_stress_rng_next(&rng);
        const uint64_t store_delta = ai_stress_rng_next(&rng) | 1ULL;

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_stress_mmu_store_burst(ai_stress_mmu_vaddrs,
            store_payload, store_delta);
        fence_iorw_iorw();
        const bool stores_retired = excpt.triggered == false;

        sfence_vma();
        TEST_SETUP_EXCEPT();
        const uint64_t recovered_signature =
            ai_stress_mmu_load_burst(ai_stress_mmu_vaddrs);
        const bool recovered_without_exception = excpt.triggered == false;

        goto_priv(PRIV_M);
        for (unsigned index = 0; index < AI_STRESS_MMU_PAGES; ++index) {
            const uint64_t expected =
                store_payload + store_delta * index;
            const uintptr_t paddr =
                phys_page_base(ai_stress_mmu_pages[index]) + offset;

            recovery_expected ^= expected;
            backing_ok = backing_ok && read64(paddr) == expected;
        }

        const unsigned pressure_mask = AI_STRESS_PRESSURE_SCALAR_LOAD |
            AI_STRESS_PRESSURE_SCALAR_STORE |
            AI_STRESS_PRESSURE_TRANSLATION;
        const unsigned cleanup_mask = AI_STRESS_CLEANUP_SFENCE |
            AI_STRESS_CLEANUP_EXCEPTION;
        const bool contract_ok = ai_stress_popcount(pressure_mask) >= 2U &&
            cleanup_mask != 0U;
        const bool seed_ok = contract_ok && warm_ok && fault_ok &&
            stores_retired && recovered_without_exception && backing_ok &&
            recovered_signature == recovery_expected;
        uint64_t signature = ai_stress_signature_fold(warm_signature,
            fault_metadata);
        signature = ai_stress_signature_fold(signature,
            recovered_signature);

        TEST_ASSERT("a randomized invalid-leaf exception is followed by PTE repair, SFENCE.VMA cleanup and a 16-page translated store/load scoreboard with fresh trap metadata",
            seed_ok,
            "seed=0x%llx fault_index=%u cause=0x%llx tval=0x%llx warm=0x%llx recovered=0x%llx expected=0x%llx backing_ok=%d",
            (unsigned long long)seed, fault_index,
            (unsigned long long)fault_cause,
            (unsigned long long)fault_tval,
            (unsigned long long)warm_signature,
            (unsigned long long)recovered_signature,
            (unsigned long long)recovery_expected, backing_ok);
        ai_stress_log_seed("mmu_sfence_recovery", seed, round,
            pressure_mask, cleanup_mask,
            "PTW-load-filter:16-page-candidate",
            "PTW-store-filter:16-page-candidate",
            "LQ/SQ:16-translated-accesses",
            "load-page-fault+sfence-repair",
            "trap-metadata+translated-backing-image",
            signature, seed_ok);
    }

    goto_priv(PRIV_M);
    sfence_vma();
    TEST_END();
}
