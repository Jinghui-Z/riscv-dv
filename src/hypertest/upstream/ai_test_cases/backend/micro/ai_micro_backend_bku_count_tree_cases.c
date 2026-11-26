#include <rvh_test.h>
#include <stdint.h>

static inline uint64_t ai_backend_bku_clz(uint64_t source)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zbb\n\t"
        "clz %0, %1\n\t"
        ".option pop\n\t"
        : "=r"(result)
        : "r"(source)
        : "memory");
    return result;
}

static inline uint64_t ai_backend_bku_ctz(uint64_t source)
{
    uint64_t result;

    asm volatile(
        ".option push\n\t"
        ".option arch,+zbb\n\t"
        "ctz %0, %1\n\t"
        ".option pop\n\t"
        : "=r"(result)
        : "r"(source)
        : "memory");
    return result;
}

bool ai_micro_backend_bku_count_tree_asymmetric_zero_subblocks_corner()
{
    TEST_START();

    static const uint64_t clz_sources[5] = {
        0x4444444444444444ULL,
        0x1010101010101010ULL,
        0x0100010001000100ULL,
        0x0001000000010000ULL,
        0x0000000100000000ULL
    };
    static const uint64_t ctz_sources[5] = {
        0x2222222222222222ULL,
        0x0808080808080808ULL,
        0x0080008000800080ULL,
        0x0000800000008000ULL,
        0x0000000080000000ULL
    };
    static const uint64_t expected_counts[5] = {1ULL, 3ULL, 7ULL, 15ULL, 31ULL};
    volatile unsigned repetitions = 4U;
    uint64_t clz_mismatch_mask = 0ULL;
    uint64_t ctz_mismatch_mask = 0ULL;

    goto_priv(PRIV_M);
    for (unsigned level = 0; level < 5U; ++level) {
        for (unsigned repeat = 0; repeat < repetitions; ++repeat) {
            const uint64_t clz_result = ai_backend_bku_clz(clz_sources[level]);

            if (clz_result != expected_counts[level]) {
                clz_mismatch_mask |= 1ULL << level;
            }
        }
        for (unsigned repeat = 0; repeat < repetitions; ++repeat) {
            const uint64_t ctz_result = ai_backend_bku_ctz(ctz_sources[level]);

            if (ctz_result != expected_counts[level]) {
                ctz_mismatch_mask |= 1ULL << level;
            }
        }
    }

    TEST_ASSERT("BKU CLZ covers asymmetric zero subblocks at every CountModule reduction level",
        clz_mismatch_mask == 0ULL,
        "mismatch_mask=0x%llx",
        (unsigned long long)clz_mismatch_mask);
    TEST_ASSERT("BKU CTZ mirrored operands cover the same internal reduction shapes after bit reversal",
        ctz_mismatch_mask == 0ULL,
        "mismatch_mask=0x%llx",
        (unsigned long long)ctz_mismatch_mask);

    TEST_END();
}
