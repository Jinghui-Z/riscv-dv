#include <rvh_test.h>
#include <stdbool.h>
#include <stdint.h>

#define AI_CRYPTO_ATTR \
    __attribute__((noinline, optimize("no-tree-vectorize")))

#define AI_CRYPTO_RR_HELPER(name_, arch_, insn_) \
static inline uint64_t name_(uint64_t lhs, uint64_t rhs) \
{ \
    uint64_t result; \
    asm volatile( \
        ".option push\n\t" \
        ".option arch," arch_ "\n\t" \
        insn_ " %0, %1, %2\n\t" \
        ".option pop\n\t" \
        : "=r"(result) \
        : "r"(lhs), "r"(rhs) \
        : "memory"); \
    return result; \
}

#define AI_CRYPTO_R_HELPER(name_, arch_, insn_) \
static inline uint64_t name_(uint64_t source) \
{ \
    uint64_t result; \
    asm volatile( \
        ".option push\n\t" \
        ".option arch," arch_ "\n\t" \
        insn_ " %0, %1\n\t" \
        ".option pop\n\t" \
        : "=r"(result) \
        : "r"(source) \
        : "memory"); \
    return result; \
}

#define AI_CRYPTO_AES_KS1I_HELPER(rnum_) \
static inline uint64_t ai_crypto_aes64ks1i_##rnum_(uint64_t source) \
{ \
    uint64_t result; \
    asm volatile( \
        ".option push\n\t" \
        ".option arch,+zknd\n\t" \
        "aes64ks1i %0, %1, " #rnum_ "\n\t" \
        ".option pop\n\t" \
        : "=r"(result) \
        : "r"(source) \
        : "memory"); \
    return result; \
}

AI_CRYPTO_RR_HELPER(ai_crypto_pack, "+zbkb", "pack")
AI_CRYPTO_RR_HELPER(ai_crypto_packh, "+zbkb", "packh")
AI_CRYPTO_RR_HELPER(ai_crypto_packw, "+zbkb", "packw")
AI_CRYPTO_R_HELPER(ai_crypto_brev8, "+zbkb", "brev8")
AI_CRYPTO_RR_HELPER(ai_crypto_xperm4, "+zbkx", "xperm4")
AI_CRYPTO_RR_HELPER(ai_crypto_xperm8, "+zbkx", "xperm8")

AI_CRYPTO_R_HELPER(ai_crypto_sha256sum0, "+zknh", "sha256sum0")
AI_CRYPTO_R_HELPER(ai_crypto_sha256sum1, "+zknh", "sha256sum1")
AI_CRYPTO_R_HELPER(ai_crypto_sha256sig0, "+zknh", "sha256sig0")
AI_CRYPTO_R_HELPER(ai_crypto_sha256sig1, "+zknh", "sha256sig1")
AI_CRYPTO_R_HELPER(ai_crypto_sha512sum0, "+zknh", "sha512sum0")
AI_CRYPTO_R_HELPER(ai_crypto_sha512sum1, "+zknh", "sha512sum1")
AI_CRYPTO_R_HELPER(ai_crypto_sha512sig0, "+zknh", "sha512sig0")
AI_CRYPTO_R_HELPER(ai_crypto_sha512sig1, "+zknh", "sha512sig1")
AI_CRYPTO_R_HELPER(ai_crypto_sm3p0, "+zksh", "sm3p0")
AI_CRYPTO_R_HELPER(ai_crypto_sm3p1, "+zksh", "sm3p1")
AI_CRYPTO_R_HELPER(ai_crypto_aes64im, "+zknd", "aes64im")
AI_CRYPTO_RR_HELPER(ai_crypto_aes64ks2, "+zknd", "aes64ks2")

AI_CRYPTO_AES_KS1I_HELPER(0)
AI_CRYPTO_AES_KS1I_HELPER(1)
AI_CRYPTO_AES_KS1I_HELPER(2)
AI_CRYPTO_AES_KS1I_HELPER(3)
AI_CRYPTO_AES_KS1I_HELPER(4)
AI_CRYPTO_AES_KS1I_HELPER(5)
AI_CRYPTO_AES_KS1I_HELPER(6)
AI_CRYPTO_AES_KS1I_HELPER(7)
AI_CRYPTO_AES_KS1I_HELPER(8)
AI_CRYPTO_AES_KS1I_HELPER(9)
AI_CRYPTO_AES_KS1I_HELPER(10)

static AI_CRYPTO_ATTR uint64_t ai_crypto_sext32(uint32_t value)
{
    return (uint64_t)(int64_t)(int32_t)value;
}

static AI_CRYPTO_ATTR uint32_t ai_crypto_rotr32(uint32_t value,
    unsigned shamt)
{
    shamt &= 31U;
    if (shamt == 0U) {
        return value;
    }
    return (value >> shamt) | (value << (32U - shamt));
}

static AI_CRYPTO_ATTR uint64_t ai_crypto_rotr64(uint64_t value,
    unsigned shamt)
{
    shamt &= 63U;
    if (shamt == 0U) {
        return value;
    }
    return (value >> shamt) | (value << (64U - shamt));
}

static AI_CRYPTO_ATTR uint64_t ai_crypto_brev8_ref(uint64_t value)
{
    uint64_t result = 0ULL;

    for (unsigned byte = 0; byte < 8U; ++byte) {
        uint8_t source = (uint8_t)(value >> (8U * byte));
        uint8_t reversed = 0U;
        for (unsigned bit = 0; bit < 8U; ++bit) {
            reversed |= ((source >> bit) & 1U) << (7U - bit);
        }
        result |= (uint64_t)reversed << (8U * byte);
    }
    return result;
}

static AI_CRYPTO_ATTR uint64_t ai_crypto_xperm_ref(uint64_t source,
    uint64_t indices, unsigned log2_element_bits)
{
    const unsigned element_bits = 1U << log2_element_bits;
    const unsigned element_count = 64U / element_bits;
    const uint64_t element_mask = (1ULL << element_bits) - 1ULL;
    uint64_t result = 0ULL;

    for (unsigned i = 0; i < element_count; ++i) {
        const unsigned index = (unsigned)((indices >> (i * element_bits)) &
            element_mask);
        if (index < element_count) {
            result |= ((source >> (index * element_bits)) & element_mask) <<
                (i * element_bits);
        }
    }
    return result;
}

static AI_CRYPTO_ATTR uint8_t ai_crypto_gf_mul(uint8_t lhs, uint8_t rhs)
{
    uint8_t result = 0U;

    for (unsigned i = 0; i < 8U; ++i) {
        if ((rhs & 1U) != 0U) {
            result ^= lhs;
        }
        const bool high = (lhs & 0x80U) != 0U;
        lhs <<= 1;
        if (high) {
            lhs ^= 0x1bU;
        }
        rhs >>= 1;
    }
    return result;
}

static AI_CRYPTO_ATTR uint8_t ai_crypto_gf_pow(uint8_t value,
    unsigned exponent)
{
    uint8_t result = 1U;

    while (exponent != 0U) {
        if ((exponent & 1U) != 0U) {
            result = ai_crypto_gf_mul(result, value);
        }
        value = ai_crypto_gf_mul(value, value);
        exponent >>= 1;
    }
    return result;
}

static inline uint8_t ai_crypto_rotl8(uint8_t value, unsigned shamt)
{
    shamt &= 7U;
    return (uint8_t)((value << shamt) |
        (value >> ((8U - shamt) & 7U)));
}

static AI_CRYPTO_ATTR uint8_t ai_crypto_aes_sbox_ref(uint8_t value)
{
    const uint8_t inverse = value == 0U ? 0U :
        ai_crypto_gf_pow(value, 254U);

    return inverse ^ ai_crypto_rotl8(inverse, 1U) ^
        ai_crypto_rotl8(inverse, 2U) ^ ai_crypto_rotl8(inverse, 3U) ^
        ai_crypto_rotl8(inverse, 4U) ^ 0x63U;
}

static AI_CRYPTO_ATTR uint32_t ai_crypto_aes_inv_mix_word(uint32_t value)
{
    const uint8_t b0 = (uint8_t)value;
    const uint8_t b1 = (uint8_t)(value >> 8U);
    const uint8_t b2 = (uint8_t)(value >> 16U);
    const uint8_t b3 = (uint8_t)(value >> 24U);
    const uint8_t o0 = ai_crypto_gf_mul(0x0eU, b0) ^
        ai_crypto_gf_mul(0x0bU, b1) ^ ai_crypto_gf_mul(0x0dU, b2) ^
        ai_crypto_gf_mul(0x09U, b3);
    const uint8_t o1 = ai_crypto_gf_mul(0x09U, b0) ^
        ai_crypto_gf_mul(0x0eU, b1) ^ ai_crypto_gf_mul(0x0bU, b2) ^
        ai_crypto_gf_mul(0x0dU, b3);
    const uint8_t o2 = ai_crypto_gf_mul(0x0dU, b0) ^
        ai_crypto_gf_mul(0x09U, b1) ^ ai_crypto_gf_mul(0x0eU, b2) ^
        ai_crypto_gf_mul(0x0bU, b3);
    const uint8_t o3 = ai_crypto_gf_mul(0x0bU, b0) ^
        ai_crypto_gf_mul(0x0dU, b1) ^ ai_crypto_gf_mul(0x09U, b2) ^
        ai_crypto_gf_mul(0x0eU, b3);

    return (uint32_t)o0 | ((uint32_t)o1 << 8U) |
        ((uint32_t)o2 << 16U) | ((uint32_t)o3 << 24U);
}

static AI_CRYPTO_ATTR uint64_t ai_crypto_aes64im_ref(uint64_t source)
{
    return (uint64_t)ai_crypto_aes_inv_mix_word((uint32_t)source) |
        ((uint64_t)ai_crypto_aes_inv_mix_word((uint32_t)(source >> 32U)) <<
            32U);
}

static AI_CRYPTO_ATTR uint64_t ai_crypto_aes64ks1i_ref(uint64_t source,
    unsigned rnum)
{
    static const uint8_t round_constant[11] = {
        0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x20U,
        0x40U, 0x80U, 0x1bU, 0x36U, 0x00U
    };
    uint32_t word = (uint32_t)(source >> 32U);
    uint32_t transformed = 0U;

    if (rnum != 10U) {
        word = ai_crypto_rotr32(word, 8U);
    }
    for (unsigned byte = 0; byte < 4U; ++byte) {
        transformed |= (uint32_t)ai_crypto_aes_sbox_ref(
            (uint8_t)(word >> (8U * byte))) << (8U * byte);
    }
    transformed ^= round_constant[rnum];
    return (uint64_t)transformed | ((uint64_t)transformed << 32U);
}

static AI_CRYPTO_ATTR uint64_t ai_crypto_aes64ks2_ref(uint64_t lhs,
    uint64_t rhs)
{
    const uint32_t low = (uint32_t)(lhs >> 32U) ^ (uint32_t)rhs;
    const uint32_t high = low ^ (uint32_t)(rhs >> 32U);

    return (uint64_t)low | ((uint64_t)high << 32U);
}

static AI_CRYPTO_ATTR unsigned ai_crypto_hash_mismatch_count(uint64_t source)
{
    const uint32_t word = (uint32_t)source;
    unsigned mismatches = 0U;

    mismatches += ai_crypto_sha256sum0(source) != ai_crypto_sext32(
        ai_crypto_rotr32(word, 2U) ^ ai_crypto_rotr32(word, 13U) ^
        ai_crypto_rotr32(word, 22U));
    mismatches += ai_crypto_sha256sum1(source) != ai_crypto_sext32(
        ai_crypto_rotr32(word, 6U) ^ ai_crypto_rotr32(word, 11U) ^
        ai_crypto_rotr32(word, 25U));
    mismatches += ai_crypto_sha256sig0(source) != ai_crypto_sext32(
        ai_crypto_rotr32(word, 7U) ^ ai_crypto_rotr32(word, 18U) ^
        (word >> 3U));
    mismatches += ai_crypto_sha256sig1(source) != ai_crypto_sext32(
        ai_crypto_rotr32(word, 17U) ^ ai_crypto_rotr32(word, 19U) ^
        (word >> 10U));
    mismatches += ai_crypto_sha512sum0(source) !=
        (ai_crypto_rotr64(source, 28U) ^ ai_crypto_rotr64(source, 34U) ^
            ai_crypto_rotr64(source, 39U));
    mismatches += ai_crypto_sha512sum1(source) !=
        (ai_crypto_rotr64(source, 14U) ^ ai_crypto_rotr64(source, 18U) ^
            ai_crypto_rotr64(source, 41U));
    mismatches += ai_crypto_sha512sig0(source) !=
        (ai_crypto_rotr64(source, 1U) ^ ai_crypto_rotr64(source, 8U) ^
            (source >> 7U));
    mismatches += ai_crypto_sha512sig1(source) !=
        (ai_crypto_rotr64(source, 19U) ^ ai_crypto_rotr64(source, 61U) ^
            (source >> 6U));
    mismatches += ai_crypto_sm3p0(source) != ai_crypto_sext32(word ^
        ai_crypto_rotr32(word, 23U) ^ ai_crypto_rotr32(word, 15U));
    mismatches += ai_crypto_sm3p1(source) != ai_crypto_sext32(word ^
        ai_crypto_rotr32(word, 9U) ^ ai_crypto_rotr32(word, 17U));
    return mismatches;
}

bool ai_arch_scalar_crypto_bitmanip_permute_reference_matrix_corner()
{
    TEST_START();

    const uint64_t lhs = 0x0011223344556677ULL;
    const uint64_t rhs = 0x8899aabbccddeeffULL;
    const uint64_t xperm_source = 0x0123456789abcdefULL;
    const uint64_t xperm4_indices = 0xfedcba9876543210ULL;
    const uint64_t xperm8_indices = 0x000708ff02010603ULL;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();

    TEST_ASSERT("Zbkb PACK/PACKH/PACKW exact results preserve the specified low subwords and PACKW sign extension",
        ai_crypto_pack(lhs, rhs) ==
            (((rhs & 0xffffffffULL) << 32U) | (lhs & 0xffffffffULL)) &&
        ai_crypto_packh(lhs, rhs) ==
            (((rhs & 0xffULL) << 8U) | (lhs & 0xffULL)) &&
        ai_crypto_packw(lhs, rhs) ==
            ai_crypto_sext32(((uint32_t)rhs << 16U) |
                ((uint32_t)lhs & 0xffffU)));

    TEST_ASSERT("Zbkb BREV8 reverses bits independently in every byte",
        ai_crypto_brev8(lhs) == ai_crypto_brev8_ref(lhs) &&
        ai_crypto_brev8(UINT64_MAX ^ lhs) ==
            ai_crypto_brev8_ref(UINT64_MAX ^ lhs));

    TEST_ASSERT("Zbkx XPERM4 and XPERM8 match nibble/byte lookup models including zero fill for out-of-range byte indices",
        ai_crypto_xperm4(xperm_source, xperm4_indices) ==
            ai_crypto_xperm_ref(xperm_source, xperm4_indices, 2U) &&
        ai_crypto_xperm8(xperm_source, xperm8_indices) ==
            ai_crypto_xperm_ref(xperm_source, xperm8_indices, 3U));

    TEST_ASSERT("scalar cryptographic bit-manipulation and permutation probes execute without exception",
        excpt.triggered == false);

    TEST_END();
}

bool ai_arch_scalar_crypto_hash_aes_schedule_reference_matrix_corner()
{
    TEST_START();

    static const uint64_t hash_sources[] = {
        0x0000000000000000ULL,
        0x0011223344556677ULL,
        0x80000001deadbeefULL,
        0xffffffffffffffffULL
    };
    const uint64_t aes_source = 0x0011223344556677ULL;
    const uint64_t aes_rhs = 0x8899aabbccddeeffULL;
    unsigned hash_mismatches = 0U;
    unsigned aes_mismatches = 0U;

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();

    for (unsigned i = 0; i < sizeof(hash_sources) / sizeof(hash_sources[0]);
         ++i) {
        hash_mismatches += ai_crypto_hash_mismatch_count(hash_sources[i]);
    }

    const uint64_t ks1_observed[11] = {
        ai_crypto_aes64ks1i_0(aes_source),
        ai_crypto_aes64ks1i_1(aes_source),
        ai_crypto_aes64ks1i_2(aes_source),
        ai_crypto_aes64ks1i_3(aes_source),
        ai_crypto_aes64ks1i_4(aes_source),
        ai_crypto_aes64ks1i_5(aes_source),
        ai_crypto_aes64ks1i_6(aes_source),
        ai_crypto_aes64ks1i_7(aes_source),
        ai_crypto_aes64ks1i_8(aes_source),
        ai_crypto_aes64ks1i_9(aes_source),
        ai_crypto_aes64ks1i_10(aes_source)
    };

    for (unsigned rnum = 0; rnum <= 10U; ++rnum) {
        aes_mismatches += ks1_observed[rnum] !=
            ai_crypto_aes64ks1i_ref(aes_source, rnum);
    }
    aes_mismatches += ai_crypto_aes64im(aes_source) !=
        ai_crypto_aes64im_ref(aes_source);
    aes_mismatches += ai_crypto_aes64im(aes_rhs) !=
        ai_crypto_aes64im_ref(aes_rhs);
    aes_mismatches += ai_crypto_aes64ks2(aes_source, aes_rhs) !=
        ai_crypto_aes64ks2_ref(aes_source, aes_rhs);

    TEST_ASSERT("Zknh SHA-256/SHA-512 and Zksh SM3 transforms match independent rotate/shift reference models",
        hash_mismatches == 0U,
        "mismatches=%u", hash_mismatches);
    TEST_ASSERT("Zknd AES64IM, AES64KS2 and every legal AES64KS1I rnum 0 through 10 match independent GF/key-schedule models",
        aes_mismatches == 0U,
        "mismatches=%u", aes_mismatches);
    TEST_ASSERT("scalar hash and AES key-schedule probes execute without exception",
        excpt.triggered == false);

    TEST_END();
}
