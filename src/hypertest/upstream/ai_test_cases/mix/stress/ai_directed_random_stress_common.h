#ifndef AI_DIRECTED_RANDOM_STRESS_COMMON_H
#define AI_DIRECTED_RANDOM_STRESS_COMMON_H

#include <rvh_test.h>
#include <stddef.h>
#include <stdint.h>

#ifndef AI_STRESS_BASE_SEED
#define AI_STRESS_BASE_SEED 0x6a09e667f3bcc909ULL
#endif

#ifndef AI_STRESS_SEED_COUNT
#define AI_STRESS_SEED_COUNT 4U
#endif

#if AI_STRESS_SEED_COUNT == 0
#error "AI_STRESS_SEED_COUNT must be greater than zero"
#endif

/*
 * Every profile follows SETUP -> WARMUP -> FILL -> COLLIDE -> OVERLAY ->
 * DRAIN -> ORACLE. Random choices select operands and paths, while the
 * pressure and cleanup masks enforce the minimum per-seed contract.
 */
enum ai_stress_pressure {
    AI_STRESS_PRESSURE_BRANCH       = 1U << 0,
    AI_STRESS_PRESSURE_SCALAR_LOAD  = 1U << 1,
    AI_STRESS_PRESSURE_SCALAR_STORE = 1U << 2,
    AI_STRESS_PRESSURE_VECTOR       = 1U << 3,
    AI_STRESS_PRESSURE_BACKEND      = 1U << 4,
    AI_STRESS_PRESSURE_ATOMIC       = 1U << 5,
    AI_STRESS_PRESSURE_TRANSLATION  = 1U << 6,
};

enum ai_stress_cleanup {
    AI_STRESS_CLEANUP_REDIRECT  = 1U << 0,
    AI_STRESS_CLEANUP_FENCE     = 1U << 1,
    AI_STRESS_CLEANUP_SFENCE    = 1U << 2,
    AI_STRESS_CLEANUP_EXCEPTION = 1U << 3,
    AI_STRESS_CLEANUP_CMO       = 1U << 4,
};

struct ai_stress_rng {
    uint64_t state;
};

static inline uint64_t ai_stress_mix64(uint64_t value)
{
    value ^= value >> 30U;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27U;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

static inline uint64_t ai_stress_seed(unsigned profile, unsigned round)
{
    return ai_stress_mix64(AI_STRESS_BASE_SEED ^
        (0x9e3779b97f4a7c15ULL * (uint64_t)(round + 1U)) ^
        ((uint64_t)profile << 56U));
}

static inline uint64_t ai_stress_sharded_seed(unsigned profile,
    unsigned round, uint64_t shard)
{
    return ai_stress_mix64(ai_stress_seed(profile, round) ^
        ai_stress_mix64(shard + 0xd1b54a32d192ed03ULL));
}

static inline void ai_stress_rng_init(struct ai_stress_rng *rng,
    uint64_t seed)
{
    rng->state = seed != 0ULL ? seed : 0x2545f4914f6cdd1dULL;
}

static inline uint64_t ai_stress_rng_next(struct ai_stress_rng *rng)
{
    uint64_t value = rng->state;

    value ^= value >> 12U;
    value ^= value << 25U;
    value ^= value >> 27U;
    rng->state = value;
    return value * 0x2545f4914f6cdd1dULL;
}

static inline uint64_t ai_stress_rotl64(uint64_t value, unsigned shift)
{
    shift &= 63U;
    return shift == 0U ? value :
        (value << shift) | (value >> (64U - shift));
}

static inline uint64_t ai_stress_signature_fold(uint64_t signature,
    uint64_t value)
{
    return ai_stress_rotl64(signature ^ ai_stress_mix64(value), 17U) +
        0x9e3779b97f4a7c15ULL;
}

static inline uint64_t ai_stress_byte_signature(const volatile uint8_t *data,
    size_t size)
{
    uint64_t signature = 0xcbf29ce484222325ULL;

    for (size_t index = 0; index < size; ++index) {
        signature ^= data[index];
        signature *= 0x100000001b3ULL;
    }
    return signature;
}

static inline unsigned ai_stress_popcount(unsigned value)
{
    unsigned count = 0U;

    while (value != 0U) {
        value &= value - 1U;
        ++count;
    }
    return count;
}

/* Each adjacent pair has one taken and one not-taken outcome. */
static inline uint64_t ai_stress_balanced_branch_pattern(
    struct ai_stress_rng *rng)
{
    const uint64_t even_mask = 0x5555555555555555ULL;
    const uint64_t even_bits = ai_stress_rng_next(rng) & even_mask;

    return even_bits | ((~even_bits & even_mask) << 1U);
}

static inline void ai_stress_log_seed(const char *profile, uint64_t seed,
    unsigned round, unsigned pressure_mask, unsigned cleanup_mask,
    const char *primary, const char *secondary, const char *tertiary,
    const char *overlay, const char *oracle, uint64_t signature,
    bool accepted)
{
    INFO("HYPSTRESS seed=0x%016llx profile=%s round=%u "
         "scheduled_pressure=0x%x cleanup=0x%x primary=%s secondary=%s "
         "tertiary=%s overlay=%s oracle=%s signature=0x%016llx "
         "status=%s coverage_contract=linknan_high_watermark_required",
        (unsigned long long)seed, profile, round, pressure_mask,
        cleanup_mask, primary, secondary, tertiary, overlay, oracle,
        (unsigned long long)signature,
        accepted ? "accepted" : "rejected");
}

#endif
