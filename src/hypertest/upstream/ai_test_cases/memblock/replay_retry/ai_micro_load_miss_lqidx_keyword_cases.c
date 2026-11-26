#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_LQIDX_KEYWORD_WORDS_PER_LINE 8U
#define AI_LQIDX_KEYWORD_ROUNDS 32U
#define AI_LQIDX_KEYWORD_PRESSURE_LINES 64U
#define AI_LQIDX_KEYWORD_STORM_ROUNDS 8U
#define AI_LQIDX_KEYWORD_CONSUMER_ROUNDS 16U
#define AI_LQIDX_KEYWORD_NOISE_LINES 6U
#define AI_LQIDX_KEYWORD_LQ_WRAP_ROUNDS 12U

static inline void ai_lqidx_keyword_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01 | MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_lqidx_keyword_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01 | MENVCFG_CBZFE);
    hspt_init();
    sfence_vma();
}

static inline void ai_lqidx_keyword_cbo_flush(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[addr], 2\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(addr)
        : "memory");
}

static inline uint64_t ai_lqidx_keyword_pattern(uint64_t seed, unsigned idx)
{
    return seed ^ (0x0102030405060708ULL * (uint64_t)(idx + 1U)) ^
        ((uint64_t)idx << 48);
}

static inline uint64_t ai_lqidx_keyword_signature4(uint64_t seed)
{
    return ai_lqidx_keyword_pattern(seed, 0U) +
        3U * ai_lqidx_keyword_pattern(seed, 1U) +
        5U * ai_lqidx_keyword_pattern(seed, 4U) +
        7U * ai_lqidx_keyword_pattern(seed, 5U);
}

static inline uint64_t ai_lqidx_keyword_pressure_signature(uint64_t seed)
{
    uint64_t acc = 0;

    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_PRESSURE_LINES; ++i) {
        acc += (uint64_t)(i + 1U) * ai_lqidx_keyword_pattern(seed, i);
    }

    return acc;
}

static inline void ai_lqidx_keyword_seed_line(uintptr_t paddr, uint64_t seed)
{
    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_WORDS_PER_LINE; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            ai_lqidx_keyword_pattern(seed, i));
    }
}

static inline void ai_lqidx_keyword_publish_line(uintptr_t vaddr)
{
    ai_lqidx_keyword_cbo_flush(vaddr);
}

static inline void ai_lqidx_keyword_publish_word(uintptr_t vaddr)
{
    ai_lqidx_keyword_cbo_flush(vaddr);
}

static inline void ai_lqidx_keyword_publish_pressure(uintptr_t vaddr)
{
    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_PRESSURE_LINES; ++i) {
        ai_lqidx_keyword_cbo_flush(vaddr + i * 64U);
    }
}

static inline bool ai_lqidx_keyword_line_matches(uintptr_t paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_WORDS_PER_LINE; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) !=
            ai_lqidx_keyword_pattern(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline void ai_lqidx_keyword_seed_pressure(uintptr_t paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_PRESSURE_LINES; ++i) {
        write64(paddr + i * 64U, ai_lqidx_keyword_pattern(seed, i));
    }
}

static inline void ai_lqidx_keyword_seed_noise_lines(uintptr_t paddr,
    uint64_t seed)
{
    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_NOISE_LINES; ++i) {
        write64(paddr + i * 64U, ai_lqidx_keyword_pattern(seed, i));
    }
}

static inline void ai_lqidx_keyword_publish_noise_lines(uintptr_t vaddr)
{
    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_NOISE_LINES; ++i) {
        ai_lqidx_keyword_cbo_flush(vaddr + i * 64U);
    }
}

static inline uint64_t ai_lqidx_keyword_pressure_loads(uintptr_t vaddr)
{
    uint64_t acc = 0;

    for (unsigned i = 0; i < AI_LQIDX_KEYWORD_PRESSURE_LINES; ++i) {
        acc += (uint64_t)(i + 1U) * ld(vaddr + i * 64U);
    }

    return acc;
}

static inline void ai_lqidx_keyword_probe_0_32_noise(uintptr_t line_vaddr,
    uintptr_t noise0_vaddr, uintptr_t noise1_vaddr, uint64_t *out0,
    uint64_t *out32, uint64_t *out40, uint64_t *out8, uint64_t *noise_sig)
{
    uint64_t r0;
    uint64_t r32;
    uint64_t r40;
    uint64_t r8;
    uint64_t rn0;
    uint64_t rn1;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[r0], 0(%[line])\n\t"
        "ld %[r32], 32(%[line])\n\t"
        "ld %[rn0], 0(%[noise0])\n\t"
        "ld %[rn1], 0(%[noise1])\n\t"
        "ld %[r40], 40(%[line])\n\t"
        "ld %[r8], 8(%[line])\n\t"
        ".option pop\n\t"
        : [r0] "=&r"(r0),
          [r32] "=&r"(r32),
          [rn0] "=&r"(rn0),
          [rn1] "=&r"(rn1),
          [r40] "=&r"(r40),
          [r8] "=&r"(r8)
        : [line] "r"(line_vaddr),
          [noise0] "r"(noise0_vaddr),
          [noise1] "r"(noise1_vaddr)
        : "memory");

    *out0 = r0;
    *out32 = r32;
    *out40 = r40;
    *out8 = r8;
    *noise_sig = rn0 + 3U * rn1;
}

static inline void ai_lqidx_keyword_probe_32_0_noise(uintptr_t line_vaddr,
    uintptr_t noise0_vaddr, uintptr_t noise1_vaddr, uint64_t *out32,
    uint64_t *out0, uint64_t *out8, uint64_t *out40, uint64_t *noise_sig)
{
    uint64_t r32;
    uint64_t r0;
    uint64_t r8;
    uint64_t r40;
    uint64_t rn0;
    uint64_t rn1;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[r32], 32(%[line])\n\t"
        "ld %[r0], 0(%[line])\n\t"
        "ld %[rn0], 0(%[noise0])\n\t"
        "ld %[rn1], 0(%[noise1])\n\t"
        "ld %[r8], 8(%[line])\n\t"
        "ld %[r40], 40(%[line])\n\t"
        ".option pop\n\t"
        : [r32] "=&r"(r32),
          [r0] "=&r"(r0),
          [rn0] "=&r"(rn0),
          [rn1] "=&r"(rn1),
          [r8] "=&r"(r8),
          [r40] "=&r"(r40)
        : [line] "r"(line_vaddr),
          [noise0] "r"(noise0_vaddr),
          [noise1] "r"(noise1_vaddr)
        : "memory");

    *out32 = r32;
    *out0 = r0;
    *out8 = r8;
    *out40 = r40;
    *noise_sig = rn0 + 3U * rn1;
}

static inline void ai_lqidx_keyword_probe_two_lines(uintptr_t line0_vaddr,
    uintptr_t line1_vaddr, uintptr_t noise_vaddr, uint64_t *a0,
    uint64_t *a32, uint64_t *b0, uint64_t *b32, uint64_t *noise_sig)
{
    uint64_t ra0;
    uint64_t ra32;
    uint64_t rb0;
    uint64_t rb32;
    uint64_t rn;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[ra0], 0(%[line0])\n\t"
        "ld %[ra32], 32(%[line0])\n\t"
        "ld %[rb0], 0(%[line1])\n\t"
        "ld %[rb32], 32(%[line1])\n\t"
        "ld %[rn], 0(%[noise])\n\t"
        ".option pop\n\t"
        : [ra0] "=&r"(ra0),
          [ra32] "=&r"(ra32),
          [rb0] "=&r"(rb0),
          [rb32] "=&r"(rb32),
          [rn] "=&r"(rn)
        : [line0] "r"(line0_vaddr),
          [line1] "r"(line1_vaddr),
          [noise] "r"(noise_vaddr)
        : "memory");

    *a0 = ra0;
    *a32 = ra32;
    *b0 = rb0;
    *b32 = rb32;
    *noise_sig = rn;
}

static inline void ai_lqidx_keyword_probe_pingpong_storm(uintptr_t line_vaddr,
    uintptr_t noise0_vaddr, uintptr_t noise1_vaddr, uintptr_t noise2_vaddr,
    uintptr_t noise3_vaddr, uint64_t *line_acc, uint64_t *noise_acc)
{
    uint64_t a0;
    uint64_t a8;
    uint64_t a32;
    uint64_t a40;
    uint64_t b0;
    uint64_t b8;
    uint64_t b32;
    uint64_t b40;
    uint64_t n0;
    uint64_t n1;
    uint64_t n2;
    uint64_t n3;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[a0], 0(%[line])\n\t"
        "ld %[a32], 32(%[line])\n\t"
        "ld %[n0], 0(%[noise0])\n\t"
        "ld %[a40], 40(%[line])\n\t"
        "ld %[n1], 0(%[noise1])\n\t"
        "ld %[a8], 8(%[line])\n\t"
        "ld %[b32], 32(%[line])\n\t"
        "ld %[n2], 0(%[noise2])\n\t"
        "ld %[b0], 0(%[line])\n\t"
        "ld %[b40], 40(%[line])\n\t"
        "ld %[n3], 0(%[noise3])\n\t"
        "ld %[b8], 8(%[line])\n\t"
        ".option pop\n\t"
        : [a0] "=&r"(a0),
          [a8] "=&r"(a8),
          [a32] "=&r"(a32),
          [a40] "=&r"(a40),
          [b0] "=&r"(b0),
          [b8] "=&r"(b8),
          [b32] "=&r"(b32),
          [b40] "=&r"(b40),
          [n0] "=&r"(n0),
          [n1] "=&r"(n1),
          [n2] "=&r"(n2),
          [n3] "=&r"(n3)
        : [line] "r"(line_vaddr),
          [noise0] "r"(noise0_vaddr),
          [noise1] "r"(noise1_vaddr),
          [noise2] "r"(noise2_vaddr),
          [noise3] "r"(noise3_vaddr)
        : "memory");

    *line_acc = a0 + 3U * a8 + 5U * a32 + 7U * a40 +
        11U * b0 + 13U * b8 + 17U * b32 + 19U * b40;
    *noise_acc = n0 + 3U * n1 + 5U * n2 + 7U * n3;
}

static inline void ai_lqidx_keyword_probe_dual_line_storm(uintptr_t line0_vaddr,
    uintptr_t line1_vaddr, uintptr_t noise0_vaddr, uintptr_t noise1_vaddr,
    uint64_t *line0_acc, uint64_t *line1_acc, uint64_t *noise_acc)
{
    uint64_t a0;
    uint64_t a32;
    uint64_t a8;
    uint64_t a40;
    uint64_t b0;
    uint64_t b32;
    uint64_t b8;
    uint64_t b40;
    uint64_t n0;
    uint64_t n1;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[a0], 0(%[line0])\n\t"
        "ld %[a32], 32(%[line0])\n\t"
        "ld %[b32], 32(%[line1])\n\t"
        "ld %[n0], 0(%[noise0])\n\t"
        "ld %[b0], 0(%[line1])\n\t"
        "ld %[a40], 40(%[line0])\n\t"
        "ld %[b8], 8(%[line1])\n\t"
        "ld %[n1], 0(%[noise1])\n\t"
        "ld %[a8], 8(%[line0])\n\t"
        "ld %[b40], 40(%[line1])\n\t"
        ".option pop\n\t"
        : [a0] "=&r"(a0),
          [a32] "=&r"(a32),
          [a8] "=&r"(a8),
          [a40] "=&r"(a40),
          [b0] "=&r"(b0),
          [b32] "=&r"(b32),
          [b8] "=&r"(b8),
          [b40] "=&r"(b40),
          [n0] "=&r"(n0),
          [n1] "=&r"(n1)
        : [line0] "r"(line0_vaddr),
          [line1] "r"(line1_vaddr),
          [noise0] "r"(noise0_vaddr),
          [noise1] "r"(noise1_vaddr)
        : "memory");

    *line0_acc = a0 + 3U * a8 + 5U * a32 + 7U * a40;
    *line1_acc = b0 + 3U * b8 + 5U * b32 + 7U * b40;
    *noise_acc = n0 + 3U * n1;
}

static inline uint64_t ai_lqidx_keyword_shifted_signature(uint64_t seed)
{
    return ai_lqidx_keyword_pattern(seed, 0U) +
        (ai_lqidx_keyword_pattern(seed, 4U) << 1) +
        (ai_lqidx_keyword_pattern(seed, 5U) << 2) +
        (ai_lqidx_keyword_pattern(seed, 1U) << 3) +
        (ai_lqidx_keyword_pattern(seed, 4U) << 4) +
        (ai_lqidx_keyword_pattern(seed, 0U) << 5) +
        (ai_lqidx_keyword_pattern(seed, 1U) << 6) +
        (ai_lqidx_keyword_pattern(seed, 5U) << 7);
}

static inline uint64_t ai_lqidx_keyword_noise_shifted_signature(uint64_t seed)
{
    return ai_lqidx_keyword_pattern(seed, 0U) +
        (ai_lqidx_keyword_pattern(seed, 1U) << 1) +
        (ai_lqidx_keyword_pattern(seed, 2U) << 2) +
        (ai_lqidx_keyword_pattern(seed, 3U) << 3) +
        (ai_lqidx_keyword_pattern(seed, 4U) << 4) +
        (ai_lqidx_keyword_pattern(seed, 5U) << 5);
}

static inline void ai_lqidx_keyword_probe_same_line_consumer_storm(
    uintptr_t line_vaddr, uintptr_t noise_vaddr, uint64_t *line_acc,
    uint64_t *noise_acc)
{
    uint64_t lacc;
    uint64_t nacc;
    uint64_t tmp;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li %[lacc], 0\n\t"
        "li %[nacc], 0\n\t"
        "ld %[tmp], 0(%[line])\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 32(%[line])\n\t"
        "slli %[tmp], %[tmp], 1\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 0(%[noise])\n\t"
        "add %[nacc], %[nacc], %[tmp]\n\t"
        "ld %[tmp], 40(%[line])\n\t"
        "slli %[tmp], %[tmp], 2\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 64(%[noise])\n\t"
        "slli %[tmp], %[tmp], 1\n\t"
        "add %[nacc], %[nacc], %[tmp]\n\t"
        "ld %[tmp], 8(%[line])\n\t"
        "slli %[tmp], %[tmp], 3\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 128(%[noise])\n\t"
        "slli %[tmp], %[tmp], 2\n\t"
        "add %[nacc], %[nacc], %[tmp]\n\t"
        "ld %[tmp], 32(%[line])\n\t"
        "slli %[tmp], %[tmp], 4\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 192(%[noise])\n\t"
        "slli %[tmp], %[tmp], 3\n\t"
        "add %[nacc], %[nacc], %[tmp]\n\t"
        "ld %[tmp], 0(%[line])\n\t"
        "slli %[tmp], %[tmp], 5\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 256(%[noise])\n\t"
        "slli %[tmp], %[tmp], 4\n\t"
        "add %[nacc], %[nacc], %[tmp]\n\t"
        "ld %[tmp], 8(%[line])\n\t"
        "slli %[tmp], %[tmp], 6\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        "ld %[tmp], 320(%[noise])\n\t"
        "slli %[tmp], %[tmp], 5\n\t"
        "add %[nacc], %[nacc], %[tmp]\n\t"
        "ld %[tmp], 40(%[line])\n\t"
        "slli %[tmp], %[tmp], 7\n\t"
        "add %[lacc], %[lacc], %[tmp]\n\t"
        ".option pop\n\t"
        : [lacc] "=&r"(lacc),
          [nacc] "=&r"(nacc),
          [tmp] "=&r"(tmp)
        : [line] "r"(line_vaddr),
          [noise] "r"(noise_vaddr)
        : "memory");

    *line_acc = lacc;
    *noise_acc = nacc;
}

static inline void ai_lqidx_keyword_probe_pressure_then_0_32(
    uintptr_t line_vaddr, uintptr_t pressure_vaddr, uint64_t *out0,
    uint64_t *out32, uint64_t *out8, uint64_t *out40)
{
    uint64_t r0;
    uint64_t r32;
    uint64_t r8;
    uint64_t r40;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t0, 0(%[pressure])\n\t"
        "ld t1, 64(%[pressure])\n\t"
        "ld t2, 128(%[pressure])\n\t"
        "ld t3, 192(%[pressure])\n\t"
        "ld t4, 256(%[pressure])\n\t"
        "ld t5, 320(%[pressure])\n\t"
        "ld t6, 384(%[pressure])\n\t"
        "ld t0, 448(%[pressure])\n\t"
        "ld t1, 512(%[pressure])\n\t"
        "ld t2, 576(%[pressure])\n\t"
        "ld t3, 640(%[pressure])\n\t"
        "ld t4, 704(%[pressure])\n\t"
        "ld t5, 768(%[pressure])\n\t"
        "ld t6, 832(%[pressure])\n\t"
        "ld t0, 896(%[pressure])\n\t"
        "ld t1, 960(%[pressure])\n\t"
        "ld t2, 1024(%[pressure])\n\t"
        "ld t3, 1088(%[pressure])\n\t"
        "ld t4, 1152(%[pressure])\n\t"
        "ld t5, 1216(%[pressure])\n\t"
        "ld t6, 1280(%[pressure])\n\t"
        "ld t0, 1344(%[pressure])\n\t"
        "ld t1, 1408(%[pressure])\n\t"
        "ld t2, 1472(%[pressure])\n\t"
        "ld t3, 1536(%[pressure])\n\t"
        "ld t4, 1600(%[pressure])\n\t"
        "ld t5, 1664(%[pressure])\n\t"
        "ld t6, 1728(%[pressure])\n\t"
        "ld t0, 1792(%[pressure])\n\t"
        "ld t1, 1856(%[pressure])\n\t"
        "ld t2, 1920(%[pressure])\n\t"
        "ld t3, 1984(%[pressure])\n\t"
        "ld %[r0], 0(%[line])\n\t"
        "ld %[r32], 32(%[line])\n\t"
        "ld %[r8], 8(%[line])\n\t"
        "ld %[r40], 40(%[line])\n\t"
        ".option pop\n\t"
        : [r0] "=&r"(r0),
          [r32] "=&r"(r32),
          [r8] "=&r"(r8),
          [r40] "=&r"(r40)
        : [line] "r"(line_vaddr),
          [pressure] "r"(pressure_vaddr)
        : "memory", "t0", "t1", "t2", "t3", "t4", "t5", "t6");

    *out0 = r0;
    *out32 = r32;
    *out8 = r8;
    *out40 = r40;
}

static inline void ai_lqidx_keyword_probe_pressure_then_32_0(
    uintptr_t line_vaddr, uintptr_t pressure_vaddr, uint64_t *out32,
    uint64_t *out0, uint64_t *out40, uint64_t *out8)
{
    uint64_t r32;
    uint64_t r0;
    uint64_t r40;
    uint64_t r8;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld t0, 0(%[pressure])\n\t"
        "ld t1, 64(%[pressure])\n\t"
        "ld t2, 128(%[pressure])\n\t"
        "ld t3, 192(%[pressure])\n\t"
        "ld t4, 256(%[pressure])\n\t"
        "ld t5, 320(%[pressure])\n\t"
        "ld t6, 384(%[pressure])\n\t"
        "ld t0, 448(%[pressure])\n\t"
        "ld t1, 512(%[pressure])\n\t"
        "ld t2, 576(%[pressure])\n\t"
        "ld t3, 640(%[pressure])\n\t"
        "ld t4, 704(%[pressure])\n\t"
        "ld t5, 768(%[pressure])\n\t"
        "ld t6, 832(%[pressure])\n\t"
        "ld t0, 896(%[pressure])\n\t"
        "ld t1, 960(%[pressure])\n\t"
        "ld t2, 1024(%[pressure])\n\t"
        "ld t3, 1088(%[pressure])\n\t"
        "ld t4, 1152(%[pressure])\n\t"
        "ld t5, 1216(%[pressure])\n\t"
        "ld t6, 1280(%[pressure])\n\t"
        "ld t0, 1344(%[pressure])\n\t"
        "ld t1, 1408(%[pressure])\n\t"
        "ld t2, 1472(%[pressure])\n\t"
        "ld t3, 1536(%[pressure])\n\t"
        "ld t4, 1600(%[pressure])\n\t"
        "ld t5, 1664(%[pressure])\n\t"
        "ld t6, 1728(%[pressure])\n\t"
        "ld t0, 1792(%[pressure])\n\t"
        "ld t1, 1856(%[pressure])\n\t"
        "ld t2, 1920(%[pressure])\n\t"
        "ld t3, 1984(%[pressure])\n\t"
        "ld %[r32], 32(%[line])\n\t"
        "ld %[r0], 0(%[line])\n\t"
        "ld %[r40], 40(%[line])\n\t"
        "ld %[r8], 8(%[line])\n\t"
        ".option pop\n\t"
        : [r32] "=&r"(r32),
          [r0] "=&r"(r0),
          [r40] "=&r"(r40),
          [r8] "=&r"(r8)
        : [line] "r"(line_vaddr),
          [pressure] "r"(pressure_vaddr)
        : "memory", "t0", "t1", "t2", "t3", "t4", "t5", "t6");

    *out32 = r32;
    *out0 = r0;
    *out40 = r40;
    *out8 = r8;
}

static inline void ai_lqidx_keyword_probe_miss_then_store_forward(
    uintptr_t line_vaddr, uint64_t store_value, uint64_t *out0,
    uint64_t *out8, uint64_t *out32, uint64_t *out40)
{
    uint64_t r0;
    uint64_t r8;
    uint64_t r32;
    uint64_t r40;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[r0], 0(%[line])\n\t"
        "sd %[store_value], 8(%[line])\n\t"
        "ld %[r8], 8(%[line])\n\t"
        "ld %[r32], 32(%[line])\n\t"
        "ld %[r40], 40(%[line])\n\t"
        ".option pop\n\t"
        : [r0] "=&r"(r0),
          [r8] "=&r"(r8),
          [r32] "=&r"(r32),
          [r40] "=&r"(r40)
        : [line] "r"(line_vaddr),
          [store_value] "r"(store_value)
        : "memory");

    *out0 = r0;
    *out8 = r8;
    *out32 = r32;
    *out40 = r40;
}

bool ai_micro_hs_load_miss_lqidx_keyword_0_then_32_noise_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page noise_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x680U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x680U;
    const uintptr_t noise0_vaddr = hs_page_base(noise_page) + 0x180U;
    const uintptr_t noise1_vaddr = hs_page_base(noise_page) + 0x580U;
    const uintptr_t noise0_paddr = phys_page_base(noise_page) + 0x180U;
    const uintptr_t noise1_paddr = phys_page_base(noise_page) + 0x580U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_ROUNDS; ++round) {
        const uint64_t seed = 0x1100000000000000ULL + round;
        uint64_t r0;
        uint64_t r32;
        uint64_t r40;
        uint64_t r8;
        uint64_t noise;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, seed);
        write64(noise0_paddr, ai_lqidx_keyword_pattern(seed, 16U));
        write64(noise1_paddr, ai_lqidx_keyword_pattern(seed, 17U));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_word(noise0_vaddr);
        ai_lqidx_keyword_publish_word(noise1_vaddr);
        ai_lqidx_keyword_probe_0_32_noise(target_vaddr, noise0_vaddr,
            noise1_vaddr, &r0, &r32, &r40, &r8, &noise);
        ok = ok && !excpt.triggered &&
            r0 == ai_lqidx_keyword_pattern(seed, 0U) &&
            r32 == ai_lqidx_keyword_pattern(seed, 4U) &&
            r40 == ai_lqidx_keyword_pattern(seed, 5U) &&
            r8 == ai_lqidx_keyword_pattern(seed, 1U) &&
            noise == (ai_lqidx_keyword_pattern(seed, 16U) +
                3U * ai_lqidx_keyword_pattern(seed, 17U));
    }

    TEST_ASSERT("same-line lower-half miss followed by upper-half merge and unrelated S0 noise keeps refill/forward data aligned",
        ok
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory remains unchanged after lower-to-upper keyword stress",
        ai_lqidx_keyword_line_matches(target_paddr,
            0x1100000000000000ULL + (AI_LQIDX_KEYWORD_ROUNDS - 1U))
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_pingpong_noise_storm_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page noise_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x900U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x900U;
    const uintptr_t noise_vaddr = hs_page_base(noise_page) + 0x1000U;
    const uintptr_t noise_paddr = phys_page_base(noise_page) + 0x1000U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_STORM_ROUNDS; ++round) {
        const uint64_t target_seed = 0x8800000000000000ULL + round;
        const uint64_t noise_seed = 0x9900000000000000ULL + round;
        uint64_t line_acc;
        uint64_t noise_acc;
        const uint64_t expect_line =
            ai_lqidx_keyword_pattern(target_seed, 0U) +
            3U * ai_lqidx_keyword_pattern(target_seed, 1U) +
            5U * ai_lqidx_keyword_pattern(target_seed, 4U) +
            7U * ai_lqidx_keyword_pattern(target_seed, 5U) +
            11U * ai_lqidx_keyword_pattern(target_seed, 0U) +
            13U * ai_lqidx_keyword_pattern(target_seed, 1U) +
            17U * ai_lqidx_keyword_pattern(target_seed, 4U) +
            19U * ai_lqidx_keyword_pattern(target_seed, 5U);
        const uint64_t expect_noise =
            ai_lqidx_keyword_pattern(noise_seed, 0U) +
            3U * ai_lqidx_keyword_pattern(noise_seed, 1U) +
            5U * ai_lqidx_keyword_pattern(noise_seed, 2U) +
            7U * ai_lqidx_keyword_pattern(noise_seed, 3U);

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);
        ai_lqidx_keyword_seed_line(noise_paddr, noise_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_line(noise_vaddr);
        ai_lqidx_keyword_probe_pingpong_storm(target_vaddr,
            noise_vaddr + 0U, noise_vaddr + 8U, noise_vaddr + 16U,
            noise_vaddr + 24U, &line_acc, &noise_acc);
        ok = ok && !excpt.triggered &&
            line_acc == expect_line &&
            noise_acc == expect_noise;
    }

    TEST_ASSERT("same-line keyword ping-pong storm with S0 noise keeps refill-to-LDQ data aligned",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_dual_line_interleave_storm_data_corner()
{
    TEST_START();

    const enum test_page page0 = SWITCH1;
    const enum test_page page1 = SWITCH2;
    const uintptr_t line0_vaddr = hs_page_base(page0) + 0xa00U;
    const uintptr_t line0_paddr = phys_page_base(page0) + 0xa00U;
    const uintptr_t line1_vaddr = hs_page_base(page1) + 0xa00U;
    const uintptr_t line1_paddr = phys_page_base(page1) + 0xa00U;
    const uintptr_t noise0_vaddr = hs_page_base(page1) + 0xb00U;
    const uintptr_t noise1_vaddr = hs_page_base(page1) + 0xb40U;
    const uintptr_t noise0_paddr = phys_page_base(page1) + 0xb00U;
    const uintptr_t noise1_paddr = phys_page_base(page1) + 0xb40U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_STORM_ROUNDS; ++round) {
        const uint64_t seed0 = 0xaa00000000000000ULL + round;
        const uint64_t seed1 = 0xbb00000000000000ULL + round;
        const uint64_t noise_seed = 0xcc00000000000000ULL + round;
        uint64_t line0_acc;
        uint64_t line1_acc;
        uint64_t noise_acc;
        const uint64_t expect0 = ai_lqidx_keyword_signature4(seed0);
        const uint64_t expect1 = ai_lqidx_keyword_signature4(seed1);
        const uint64_t expect_noise =
            ai_lqidx_keyword_pattern(noise_seed, 0U) +
            3U * ai_lqidx_keyword_pattern(noise_seed, 8U);

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(line0_paddr, seed0);
        ai_lqidx_keyword_seed_line(line1_paddr, seed1);
        write64(noise0_paddr, ai_lqidx_keyword_pattern(noise_seed, 0U));
        write64(noise1_paddr, ai_lqidx_keyword_pattern(noise_seed, 8U));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(line0_vaddr);
        ai_lqidx_keyword_publish_line(line1_vaddr);
        ai_lqidx_keyword_publish_word(noise0_vaddr);
        ai_lqidx_keyword_publish_word(noise1_vaddr);
        ai_lqidx_keyword_probe_dual_line_storm(line0_vaddr, line1_vaddr,
            noise0_vaddr, noise1_vaddr, &line0_acc, &line1_acc, &noise_acc);
        ok = ok && !excpt.triggered &&
            line0_acc == expect0 &&
            line1_acc == expect1 &&
            noise_acc == expect_noise;
    }

    TEST_ASSERT("dual-line keyword interleave storm keeps independent RM/refill paths aligned",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_same_line_consumer_storm_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page noise_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xb80U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xb80U;
    const uintptr_t noise_vaddr = hs_page_base(noise_page) + 0xd00U;
    const uintptr_t noise_paddr = phys_page_base(noise_page) + 0xd00U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_CONSUMER_ROUNDS; ++round) {
        const uint64_t target_seed = 0xdd00000000000000ULL + round;
        const uint64_t noise_seed = 0xee00000000000000ULL + round;
        uint64_t line_acc;
        uint64_t noise_acc;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);
        ai_lqidx_keyword_seed_noise_lines(noise_paddr, noise_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_noise_lines(noise_vaddr);
        ai_lqidx_keyword_probe_same_line_consumer_storm(target_vaddr,
            noise_vaddr, &line_acc, &noise_acc);

        ok = ok && !excpt.triggered &&
            line_acc == ai_lqidx_keyword_shifted_signature(target_seed) &&
            noise_acc == ai_lqidx_keyword_noise_shifted_signature(noise_seed);
    }

    TEST_ASSERT("same-line keyword consumer storm keeps replay/refill data aligned with interleaved S0 load noise",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_age_pressure_consumer_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page pressure_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xc80U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xc80U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x100U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x100U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_STORM_ROUNDS; ++round) {
        const uint64_t target_seed = 0xab00000000000000ULL + round;
        const uint64_t pressure_seed = 0xcd00000000000000ULL + round;
        uint64_t line_acc;
        uint64_t noise_acc;
        uint64_t pressure_acc;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);
        ai_lqidx_keyword_seed_pressure(pressure_paddr, pressure_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_pressure(pressure_vaddr);
        pressure_acc = ai_lqidx_keyword_pressure_loads(pressure_vaddr);
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_probe_same_line_consumer_storm(target_vaddr,
            pressure_vaddr, &line_acc, &noise_acc);

        ok = ok && !excpt.triggered &&
            pressure_acc == ai_lqidx_keyword_pressure_signature(pressure_seed) &&
            line_acc == ai_lqidx_keyword_shifted_signature(target_seed) &&
            noise_acc == ai_lqidx_keyword_noise_shifted_signature(pressure_seed);
    }

    TEST_ASSERT("aged load-queue pressure followed by same-line keyword consumers keeps RM/refill data aligned",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_wrap_window_0_32_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page pressure_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xd80U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xd80U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x100U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x100U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_LQ_WRAP_ROUNDS; ++round) {
        const uint64_t target_seed = 0x9100000000000000ULL + round;
        const uint64_t pressure_seed = 0x9200000000000000ULL + round;
        uint64_t r0;
        uint64_t r32;
        uint64_t r8;
        uint64_t r40;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);
        ai_lqidx_keyword_seed_pressure(pressure_paddr, pressure_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_pressure(pressure_vaddr);
        ai_lqidx_keyword_probe_pressure_then_0_32(target_vaddr,
            pressure_vaddr, &r0, &r32, &r8, &r40);

        ok = ok && !excpt.triggered &&
            r0 == ai_lqidx_keyword_pattern(target_seed, 0U) &&
            r32 == ai_lqidx_keyword_pattern(target_seed, 4U) &&
            r8 == ai_lqidx_keyword_pattern(target_seed, 1U) &&
            r40 == ai_lqidx_keyword_pattern(target_seed, 5U);
    }

    TEST_ASSERT("independent miss pressure before lower-to-upper same-line consumers exposes wrong lqIdx keyword/refill alignment",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_wrap_window_32_0_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page pressure_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xe80U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xe80U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x100U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x100U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_LQ_WRAP_ROUNDS; ++round) {
        const uint64_t target_seed = 0x9300000000000000ULL + round;
        const uint64_t pressure_seed = 0x9400000000000000ULL + round;
        uint64_t r32;
        uint64_t r0;
        uint64_t r40;
        uint64_t r8;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);
        ai_lqidx_keyword_seed_pressure(pressure_paddr, pressure_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_pressure(pressure_vaddr);
        ai_lqidx_keyword_probe_pressure_then_32_0(target_vaddr,
            pressure_vaddr, &r32, &r0, &r40, &r8);

        ok = ok && !excpt.triggered &&
            r32 == ai_lqidx_keyword_pattern(target_seed, 4U) &&
            r0 == ai_lqidx_keyword_pattern(target_seed, 0U) &&
            r40 == ai_lqidx_keyword_pattern(target_seed, 5U) &&
            r8 == ai_lqidx_keyword_pattern(target_seed, 1U);
    }

    TEST_ASSERT("independent miss pressure before upper-to-lower same-line consumers exposes wrong lqIdx keyword/refill alignment",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_store_forward_after_miss_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xf80U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xf80U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_CONSUMER_ROUNDS; ++round) {
        const uint64_t target_seed = 0x9500000000000000ULL + round;
        const uint64_t store_value = 0x9600000000000000ULL + round;
        uint64_t r0;
        uint64_t r8;
        uint64_t r32;
        uint64_t r40;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_probe_miss_then_store_forward(target_vaddr,
            store_value, &r0, &r8, &r32, &r40);

        ok = ok && !excpt.triggered &&
            r0 == ai_lqidx_keyword_pattern(target_seed, 0U) &&
            r8 == store_value &&
            r32 == ai_lqidx_keyword_pattern(target_seed, 4U) &&
            r40 == ai_lqidx_keyword_pattern(target_seed, 5U);
    }

    TEST_ASSERT("load miss followed by same-line store-forward consumer exposes refill/RM/data-forward ordering errors",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_dual_block_wrap_pressure_data_corner()
{
    TEST_START();

    const enum test_page page0 = SWITCH1;
    const enum test_page page1 = SWITCH2;
    const uintptr_t line0_vaddr = hs_page_base(page0) + 0x1000U;
    const uintptr_t line0_paddr = phys_page_base(page0) + 0x1000U;
    const uintptr_t line1_vaddr = hs_page_base(page1) + 0x1000U;
    const uintptr_t line1_paddr = phys_page_base(page1) + 0x1000U;
    const uintptr_t pressure_vaddr = hs_page_base(page1) + 0x1800U;
    const uintptr_t pressure_paddr = phys_page_base(page1) + 0x1800U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_LQ_WRAP_ROUNDS; ++round) {
        const uint64_t seed0 = 0x9700000000000000ULL + round;
        const uint64_t seed1 = 0x9800000000000000ULL + round;
        const uint64_t pressure_seed = 0x9900000000000000ULL + round;
        uint64_t a0;
        uint64_t a32;
        uint64_t a8;
        uint64_t a40;
        uint64_t b32;
        uint64_t b0;
        uint64_t b40;
        uint64_t b8;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(line0_paddr, seed0);
        ai_lqidx_keyword_seed_line(line1_paddr, seed1);
        ai_lqidx_keyword_seed_pressure(pressure_paddr, pressure_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(line0_vaddr);
        ai_lqidx_keyword_publish_line(line1_vaddr);
        ai_lqidx_keyword_publish_pressure(pressure_vaddr);
        ai_lqidx_keyword_probe_pressure_then_0_32(line0_vaddr,
            pressure_vaddr, &a0, &a32, &a8, &a40);
        ai_lqidx_keyword_probe_pressure_then_32_0(line1_vaddr,
            pressure_vaddr, &b32, &b0, &b40, &b8);

        ok = ok && !excpt.triggered &&
            a0 == ai_lqidx_keyword_pattern(seed0, 0U) &&
            a32 == ai_lqidx_keyword_pattern(seed0, 4U) &&
            a8 == ai_lqidx_keyword_pattern(seed0, 1U) &&
            a40 == ai_lqidx_keyword_pattern(seed0, 5U) &&
            b32 == ai_lqidx_keyword_pattern(seed1, 4U) &&
            b0 == ai_lqidx_keyword_pattern(seed1, 0U) &&
            b40 == ai_lqidx_keyword_pattern(seed1, 5U) &&
            b8 == ai_lqidx_keyword_pattern(seed1, 1U);
    }

    TEST_ASSERT("dual-block lower/upper keyword windows under LQ wrap pressure expose RM/refill alignment mistakes",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_32_then_0_noise_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page noise_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x700U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x700U;
    const uintptr_t noise0_vaddr = hs_page_base(noise_page) + 0x200U;
    const uintptr_t noise1_vaddr = hs_page_base(noise_page) + 0x600U;
    const uintptr_t noise0_paddr = phys_page_base(noise_page) + 0x200U;
    const uintptr_t noise1_paddr = phys_page_base(noise_page) + 0x600U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_ROUNDS; ++round) {
        const uint64_t seed = 0x2200000000000000ULL + round;
        uint64_t r32;
        uint64_t r0;
        uint64_t r8;
        uint64_t r40;
        uint64_t noise;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, seed);
        write64(noise0_paddr, ai_lqidx_keyword_pattern(seed, 18U));
        write64(noise1_paddr, ai_lqidx_keyword_pattern(seed, 19U));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_word(noise0_vaddr);
        ai_lqidx_keyword_publish_word(noise1_vaddr);
        ai_lqidx_keyword_probe_32_0_noise(target_vaddr, noise0_vaddr,
            noise1_vaddr, &r32, &r0, &r8, &r40, &noise);
        ok = ok && !excpt.triggered &&
            r32 == ai_lqidx_keyword_pattern(seed, 4U) &&
            r0 == ai_lqidx_keyword_pattern(seed, 0U) &&
            r8 == ai_lqidx_keyword_pattern(seed, 1U) &&
            r40 == ai_lqidx_keyword_pattern(seed, 5U) &&
            noise == (ai_lqidx_keyword_pattern(seed, 18U) +
                3U * ai_lqidx_keyword_pattern(seed, 19U));
    }

    TEST_ASSERT("same-line upper-half miss followed by lower-half merge and unrelated S0 noise keeps refill/forward data aligned",
        ok
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory remains unchanged after upper-to-lower keyword stress",
        ai_lqidx_keyword_line_matches(target_paddr,
            0x2200000000000000ULL + (AI_LQIDX_KEYWORD_ROUNDS - 1U))
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_wrap_pressure_data_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page pressure_page = SWITCH2;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x780U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x780U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x300U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x300U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_ROUNDS; ++round) {
        const uint64_t target_seed = 0x3300000000000000ULL + round;
        const uint64_t pressure_seed = 0x4400000000000000ULL + round;
        uint64_t r0;
        uint64_t r32;
        uint64_t r40;
        uint64_t r8;
        uint64_t noise;
        uint64_t pressure_acc;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(target_paddr, target_seed);
        ai_lqidx_keyword_seed_pressure(pressure_paddr, pressure_seed);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_publish_pressure(pressure_vaddr);
        pressure_acc = ai_lqidx_keyword_pressure_loads(pressure_vaddr);
        ai_lqidx_keyword_publish_line(target_vaddr);
        ai_lqidx_keyword_probe_0_32_noise(target_vaddr,
            pressure_vaddr + 5U * 64U, pressure_vaddr + 17U * 64U,
            &r0, &r32, &r40, &r8, &noise);
        ok = ok && !excpt.triggered &&
            r0 == ai_lqidx_keyword_pattern(target_seed, 0U) &&
            r32 == ai_lqidx_keyword_pattern(target_seed, 4U) &&
            r40 == ai_lqidx_keyword_pattern(target_seed, 5U) &&
            r8 == ai_lqidx_keyword_pattern(target_seed, 1U) &&
            noise == (ai_lqidx_keyword_pattern(pressure_seed, 5U) +
                3U * ai_lqidx_keyword_pattern(pressure_seed, 17U)) &&
            pressure_acc == ai_lqidx_keyword_pressure_signature(pressure_seed);
    }

    TEST_ASSERT("many prior misses that advance load-queue age still keep same-line keyword refill and forward data aligned",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_keyword_dual_line_merge_data_corner()
{
    TEST_START();

    const enum test_page page0 = SWITCH1;
    const enum test_page page1 = SWITCH2;
    const uintptr_t line0_vaddr = hs_page_base(page0) + 0x800U;
    const uintptr_t line0_paddr = phys_page_base(page0) + 0x800U;
    const uintptr_t line1_vaddr = hs_page_base(page1) + 0x800U;
    const uintptr_t line1_paddr = phys_page_base(page1) + 0x800U;
    const uintptr_t noise_vaddr = hs_page_base(page1) + 0xc00U;
    const uintptr_t noise_paddr = phys_page_base(page1) + 0xc00U;
    bool ok = true;

    ai_lqidx_keyword_prepare_hs_env();

    for (unsigned round = 0; round < AI_LQIDX_KEYWORD_ROUNDS; ++round) {
        const uint64_t seed0 = 0x5500000000000000ULL + round;
        const uint64_t seed1 = 0x6600000000000000ULL + round;
        const uint64_t noise_seed = 0x7700000000000000ULL + round;
        uint64_t a0;
        uint64_t a32;
        uint64_t b0;
        uint64_t b32;
        uint64_t noise;

        goto_priv(PRIV_M);
        ai_lqidx_keyword_seed_line(line0_paddr, seed0);
        ai_lqidx_keyword_seed_line(line1_paddr, seed1);
        write64(noise_paddr, ai_lqidx_keyword_pattern(noise_seed, 0U));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_lqidx_keyword_publish_line(line0_vaddr);
        ai_lqidx_keyword_publish_line(line1_vaddr);
        ai_lqidx_keyword_publish_word(noise_vaddr);
        ai_lqidx_keyword_probe_two_lines(line0_vaddr, line1_vaddr,
            noise_vaddr, &a0, &a32, &b0, &b32, &noise);
        ok = ok && !excpt.triggered &&
            a0 == ai_lqidx_keyword_pattern(seed0, 0U) &&
            a32 == ai_lqidx_keyword_pattern(seed0, 4U) &&
            b0 == ai_lqidx_keyword_pattern(seed1, 0U) &&
            b32 == ai_lqidx_keyword_pattern(seed1, 4U) &&
            noise == ai_lqidx_keyword_pattern(noise_seed, 0U);
    }

    TEST_ASSERT("two concurrent same-line merge candidates keep independent refill/forward data aligned under lqIdx noise",
        ok
    );

    ai_lqidx_keyword_reset_hs_env();

    TEST_END();
}
