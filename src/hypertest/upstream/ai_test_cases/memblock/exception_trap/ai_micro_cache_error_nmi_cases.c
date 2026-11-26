#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#ifndef CSR_SCEC
#define CSR_SCEC 0x5da
#endif

#ifndef CSR_SCEIC
#define CSR_SCEIC 0x5db
#endif

#ifndef CSR_SFETCHCTL
#define CSR_SFETCHCTL 0x9e0
#endif

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_CACHE_ERROR_IDATA_SRC 1ULL
#define AI_MICRO_CACHE_ERROR_DTAG_SRC 2ULL
#define AI_MICRO_CACHE_ERROR_DDATA_SRC 3ULL
#define AI_MICRO_CACHE_ERROR_L2TLB_L1_SRC 6ULL
#define AI_MICRO_CACHE_ERROR_NONE_SRC 7ULL
#define AI_MICRO_CACHE_ERROR_NMI_31_CAUSE 31ULL
#define AI_MICRO_CACHE_ERROR_HOT_PAGE VSRWX_GRWX
#define AI_MICRO_CACHE_ERROR_HOT_OFFSET 0x180U
#define AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET 0x1c0U
#define AI_MICRO_CACHE_ERROR_VERIFY_OFFSET 0x200U
#define AI_MICRO_CACHE_ERROR_MARKER_OFFSET 0x240U
#define AI_MICRO_CACHE_ERROR_TLB_OFFSET 0x280U
#define AI_MICRO_CACHE_ERROR_ROUNDS 2U
#define AI_MICRO_CACHE_ERROR_DENSE_ROUNDS 1U
#define AI_MICRO_CACHE_ERROR_L2TLB_DEBUG_ROUNDS 16U
#define AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS 2U
#define AI_MICRO_CACHE_ERROR_SCEC_EPA_MASK ((1ULL << 60) - 1ULL)
#define AI_MICRO_CACHE_ERROR_ICACHE_LINE_BYTES 64ULL
#define AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ENTRY_OFFSET 48ULL
#define AI_MICRO_CACHE_ERROR_SFETCHCTL_ICACHE_PARITY_ENABLE 1ULL

static const enum test_page ai_micro_cache_error_tlb_pages[] = {
    VSRWX_GRW,
    VSRW_GRWX,
    VSRX_GRWX,
    VSR_GRWX,
};

static inline uint64_t ai_micro_cache_error_make_sceic(uint64_t source,
    bool inject_two_bits)
{
    return (source << 2) | ((uint64_t)inject_two_bits << 1) | 1ULL;
}

static inline bool ai_micro_cache_error_scec_valid(uint64_t scec)
{
    return ((scec >> 63) & 0x1ULL) != 0ULL;
}

static inline uint64_t ai_micro_cache_error_scec_source(uint64_t scec)
{
    return (scec >> 60) & 0x7ULL;
}

static inline uint64_t ai_micro_cache_error_scec_epa(uint64_t scec)
{
    return scec & AI_MICRO_CACHE_ERROR_SCEC_EPA_MASK;
}

static inline bool ai_micro_cache_error_scec_matches(uint64_t scec,
    uint64_t source, uint64_t expected_epa)
{
    return ai_micro_cache_error_scec_valid(scec) &&
        ai_micro_cache_error_scec_source(scec) == source &&
        ai_micro_cache_error_scec_epa(scec) ==
            (expected_epa & AI_MICRO_CACHE_ERROR_SCEC_EPA_MASK);
}

static inline bool ai_micro_cache_error_same_icache_line(uint64_t lhs_epa,
    uint64_t rhs_epa)
{
    const uint64_t line_mask = AI_MICRO_CACHE_ERROR_SCEC_EPA_MASK &
        ~(AI_MICRO_CACHE_ERROR_ICACHE_LINE_BYTES - 1ULL);

    return ((lhs_epa ^ rhs_epa) & line_mask) == 0ULL;
}

static inline void ai_micro_cache_error_note_scec_match(uint64_t scec,
    uint64_t source, uint64_t expected_epa, bool *exact_seen,
    bool *unexpected_seen)
{
    if (!ai_micro_cache_error_scec_valid(scec)) {
        return;
    }

    bool exact_match = ai_micro_cache_error_scec_matches(
        scec, source, expected_epa);
    *exact_seen = *exact_seen || exact_match;
    *unexpected_seen = *unexpected_seen || !exact_match;
}

static inline bool ai_micro_cache_error_is_interrupt_cause(uint64_t cause)
{
    return ((cause >> 63) & 0x1ULL) != 0ULL;
}

static inline uint64_t ai_micro_cache_error_cause_code(uint64_t cause)
{
    return cause & ~(1ULL << 63);
}

static inline bool ai_micro_cache_error_is_nmi31(uint64_t cause)
{
    return ai_micro_cache_error_is_interrupt_cause(cause) &&
        ai_micro_cache_error_cause_code(cause) ==
            AI_MICRO_CACHE_ERROR_NMI_31_CAUSE;
}

static inline uint64_t ai_micro_cache_error_round_mix(uint64_t acc,
    uint64_t tlb_value, uint64_t hot_value, uint64_t follower_value,
    unsigned round)
{
    return ((acc << 9) ^ (acc >> 7)) ^ tlb_value ^ hot_value ^ follower_value ^
        (0x9e3779b97f4a7c15ULL + (uint64_t)round * 0x100000001b3ULL);
}

static inline uint64_t ai_micro_cache_error_seeded_tlb_value(unsigned index)
{
    return 0x1100000000000000ULL + (uint64_t)index * 0x0101010101010101ULL;
}

static inline uint64_t ai_micro_cache_error_ld64(uintptr_t addr)
{
    uint64_t value;

    asm volatile("ld %0, 0(%1)\n\t"
        : "=r"(value) : "r"(addr) : "memory");
    return value;
}

extern void ai_micro_cache_error_icache_data_probe(
    volatile uint64_t *marker, uint64_t value);
extern void ai_micro_cache_error_guarded_inject_icache_data_probe(
    volatile uint64_t *marker, uint64_t value,
    volatile uint64_t *fault_inst_ptr, uintptr_t *jump_pc,
    uintptr_t *after_jump_pc, uint64_t sceic_value);
extern void ai_micro_cache_error_icache_data_doubleline_probe(
    volatile uint64_t *marker, uint64_t value);
extern void ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe(
    volatile uint64_t *marker, uint64_t value,
    volatile uint64_t *fault_inst_ptr, uintptr_t *jump_pc,
    uintptr_t *after_jump_pc, uint64_t sceic_value);
extern void ai_micro_cache_error_icache_data_doubleline_inject_target(void);

asm(
    ".pushsection .text\n"
    ".option push\n"
    ".option norvc\n"
    ".balign 64\n"
    ".global ai_micro_cache_error_icache_data_probe\n"
    ".type ai_micro_cache_error_icache_data_probe, @function\n"
    "ai_micro_cache_error_icache_data_probe:\n"
    "  sd a1, 0(a0)\n"
    "  addi a1, a1, 1\n"
    "  sd a1, 8(a0)\n"
    "  ret\n"
    ".size ai_micro_cache_error_icache_data_probe, .-ai_micro_cache_error_icache_data_probe\n"
    ".balign 64\n"
    ".global ai_micro_cache_error_guarded_inject_icache_data_probe\n"
    ".type ai_micro_cache_error_guarded_inject_icache_data_probe, @function\n"
    "ai_micro_cache_error_guarded_inject_icache_data_probe:\n"
    "  addi sp, sp, -16\n"
    "  sd ra, 0(sp)\n"
    "  la t2, ai_micro_cache_error_icache_data_probe\n"
    "  la t0, 1f\n"
    "  sd t0, 0(a2)\n"
    "  sd t0, 0(a3)\n"
    "  la t1, 2f\n"
    "  sd t1, 0(a4)\n"
    "  csrw 0x5da, zero\n"
    "  csrw 0x5db, a5\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "  addi zero, zero, 0\n"
    "1:\n"
    "  jalr ra, 0(t2)\n"
    "2:\n"
    "  sd zero, 0(a2)\n"
    "  ld ra, 0(sp)\n"
    "  addi sp, sp, 16\n"
    "  ret\n"
    ".size ai_micro_cache_error_guarded_inject_icache_data_probe, .-ai_micro_cache_error_guarded_inject_icache_data_probe\n"
    ".balign 64\n"
    ".space 48\n"
    ".global ai_micro_cache_error_icache_data_doubleline_probe\n"
    ".type ai_micro_cache_error_icache_data_doubleline_probe, @function\n"
    "ai_micro_cache_error_icache_data_doubleline_probe:\n"
    "  sd a1, 0(a0)\n"
    "  addi a1, a1, 1\n"
    "  sd a1, 8(a0)\n"
    "  addi a1, a1, 1\n"
    "  sd a1, 16(a0)\n"
    "  ret\n"
    ".size ai_micro_cache_error_icache_data_doubleline_probe, .-ai_micro_cache_error_icache_data_doubleline_probe\n"
    ".balign 64\n"
    ".global ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe\n"
    ".type ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe, @function\n"
    "ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe:\n"
    "  addi sp, sp, -16\n"
    "  sd ra, 0(sp)\n"
    "  la t0, ai_micro_cache_error_icache_data_doubleline_inject_target\n"
    "  sd t0, 0(a2)\n"
    "  sd t0, 0(a3)\n"
    "  la t1, 1f\n"
    "  sd t1, 0(a4)\n"
    "  csrw 0x5db, a5\n"
    "  addi t3, zero, 64\n"
    "  fence.i\n"
    ".global ai_micro_cache_error_icache_data_doubleline_inject_target\n"
    "ai_micro_cache_error_icache_data_doubleline_inject_target:\n"
    "  sd a1, 0(a0)\n"
    "  addi t4, a1, 1\n"
    "  sd t4, 8(a0)\n"
    "  addi t4, a1, 2\n"
    "  sd t4, 16(a0)\n"
    "  addi t3, t3, -1\n"
    "  bnez t3, ai_micro_cache_error_icache_data_doubleline_inject_target\n"
    "1:\n"
    "  sd zero, 0(a2)\n"
    "  ld ra, 0(sp)\n"
    "  addi sp, sp, 16\n"
    "  ret\n"
    ".size ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe, .-ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe\n"
    ".option pop\n"
    ".popsection\n"
);

struct ai_micro_cache_error_load_site_result {
    uint64_t value;
    uintptr_t load_pc;
    uintptr_t after_load_pc;
};

struct ai_micro_cache_error_csr_observation {
    uint64_t scec;
    uint64_t sceic;
    uint64_t mncause;
    uint64_t mnepc;
};

struct ai_micro_cache_error_marker_site_result {
    uint64_t load_value;
    uintptr_t load_pc;
    uintptr_t after_load_pc;
    uintptr_t marker_store_pc;
    uintptr_t after_marker_store_pc;
};

static volatile uint64_t ai_micro_cache_error_rnmi_hits;
static volatile uint64_t ai_micro_cache_error_rnmi_last_mncause;
static volatile uint64_t ai_micro_cache_error_rnmi_last_mnepc;
static volatile uint64_t ai_micro_cache_error_rnmi_last_mnstatus;
static volatile uint64_t ai_micro_cache_error_rnmi_last_scec;
static volatile uint64_t ai_micro_cache_error_rnmi_last_sceic;

static inline void ai_micro_cache_error_reset_rnmi_state(void)
{
    ai_micro_cache_error_rnmi_hits = 0ULL;
    ai_micro_cache_error_rnmi_last_mncause = 0ULL;
    ai_micro_cache_error_rnmi_last_mnepc = 0ULL;
    ai_micro_cache_error_rnmi_last_mnstatus = 0ULL;
    ai_micro_cache_error_rnmi_last_scec = 0ULL;
    ai_micro_cache_error_rnmi_last_sceic = 0ULL;
}

static inline void ai_micro_cache_error_enable_default_rnmi_return(void)
{
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
}

static inline void ai_micro_cache_error_restore_default_rnmi_return(
    uint64_t original_mnstatus)
{
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, original_mnstatus);
}

extern void ai_micro_cache_error_rnmi_entry(void);

asm(
    ".balign 4\n"
    ".global ai_micro_cache_error_rnmi_entry\n"
    "ai_micro_cache_error_rnmi_entry:\n"
    "  addi sp, sp, -248\n"
    "  sd x1, 0(sp)\n"
    "  sd x3, 16(sp)\n"
    "  sd x4, 24(sp)\n"
    "  sd x5, 32(sp)\n"
    "  sd x6, 40(sp)\n"
    "  sd x7, 48(sp)\n"
    "  sd x8, 56(sp)\n"
    "  sd x9, 64(sp)\n"
    "  sd x10, 72(sp)\n"
    "  sd x11, 80(sp)\n"
    "  sd x12, 88(sp)\n"
    "  sd x13, 96(sp)\n"
    "  sd x14, 104(sp)\n"
    "  sd x15, 112(sp)\n"
    "  sd x16, 120(sp)\n"
    "  sd x17, 128(sp)\n"
    "  sd x18, 136(sp)\n"
    "  sd x19, 144(sp)\n"
    "  sd x20, 152(sp)\n"
    "  sd x21, 160(sp)\n"
    "  sd x22, 168(sp)\n"
    "  sd x23, 176(sp)\n"
    "  sd x24, 184(sp)\n"
    "  sd x25, 192(sp)\n"
    "  sd x26, 200(sp)\n"
    "  sd x27, 208(sp)\n"
    "  sd x28, 216(sp)\n"
    "  sd x29, 224(sp)\n"
    "  sd x30, 232(sp)\n"
    "  sd x31, 240(sp)\n"
    "  csrr x5, 0x742\n"
    "  la x6, ai_micro_cache_error_rnmi_last_mncause\n"
    "  sd x5, 0(x6)\n"
    "  csrr x5, 0x741\n"
    "  la x6, ai_micro_cache_error_rnmi_last_mnepc\n"
    "  sd x5, 0(x6)\n"
    "  csrr x5, 0x744\n"
    "  la x6, ai_micro_cache_error_rnmi_last_mnstatus\n"
    "  sd x5, 0(x6)\n"
    "  csrr x5, 0x5da\n"
    "  la x6, ai_micro_cache_error_rnmi_last_scec\n"
    "  sd x5, 0(x6)\n"
    "  csrr x5, 0x5db\n"
    "  la x6, ai_micro_cache_error_rnmi_last_sceic\n"
    "  sd x5, 0(x6)\n"
    "  la x6, ai_micro_cache_error_rnmi_hits\n"
    "  ld x5, 0(x6)\n"
    "  addi x5, x5, 1\n"
    "  sd x5, 0(x6)\n"
    "  ld x1, 0(sp)\n"
    "  ld x3, 16(sp)\n"
    "  ld x4, 24(sp)\n"
    "  ld x5, 32(sp)\n"
    "  ld x6, 40(sp)\n"
    "  ld x7, 48(sp)\n"
    "  ld x8, 56(sp)\n"
    "  ld x9, 64(sp)\n"
    "  ld x10, 72(sp)\n"
    "  ld x11, 80(sp)\n"
    "  ld x12, 88(sp)\n"
    "  ld x13, 96(sp)\n"
    "  ld x14, 104(sp)\n"
    "  ld x15, 112(sp)\n"
    "  ld x16, 120(sp)\n"
    "  ld x17, 128(sp)\n"
    "  ld x18, 136(sp)\n"
    "  ld x19, 144(sp)\n"
    "  ld x20, 152(sp)\n"
    "  ld x21, 160(sp)\n"
    "  ld x22, 168(sp)\n"
    "  ld x23, 176(sp)\n"
    "  ld x24, 184(sp)\n"
    "  ld x25, 192(sp)\n"
    "  ld x26, 200(sp)\n"
    "  ld x27, 208(sp)\n"
    "  ld x28, 216(sp)\n"
    "  ld x29, 224(sp)\n"
    "  ld x30, 232(sp)\n"
    "  ld x31, 240(sp)\n"
    "  addi sp, sp, 248\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
);

static inline void ai_micro_cache_error_csr_to_load_delay(void)
{
    asm volatile(
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        ::: "memory");
}

static inline void ai_micro_cache_error_quiesce_injection(void)
{
    CSRW(CSR_SCEIC, 0ULL);
    CSRW(CSR_SCEC, 0ULL);
    ai_micro_cache_error_csr_to_load_delay();
}

static inline uint64_t ai_micro_cache_error_inject_then_load_hot(
    uintptr_t hot_paddr, uint64_t sceic_value)
{
    uint64_t value;

    asm volatile(
        "csrw %[scec], zero\n\t"
        "csrw %[sceic], %[ceic]\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "ld %[value], 0(%[addr])\n\t"
        : [value] "=r"(value)
        : [scec] "i"(CSR_SCEC),
          [sceic] "i"(CSR_SCEIC),
          [ceic] "r"(sceic_value),
          [addr] "r"(hot_paddr)
        : "memory");
    return value;
}

static inline struct ai_micro_cache_error_load_site_result
ai_micro_cache_error_inject_then_load_hot_with_site(
    uintptr_t hot_paddr, uint64_t sceic_value)
{
    struct ai_micro_cache_error_load_site_result result;

    asm volatile(
        "la %[load_pc], 1f\n\t"
        "la %[after_load_pc], 2f\n\t"
        "csrw %[scec], zero\n\t"
        "csrw %[sceic], %[ceic]\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "1:\n\t"
        "ld %[value], 0(%[addr])\n\t"
        "2:\n\t"
        : [value] "=r"(result.value),
          [load_pc] "=&r"(result.load_pc),
          [after_load_pc] "=&r"(result.after_load_pc)
        : [scec] "i"(CSR_SCEC),
          [sceic] "i"(CSR_SCEIC),
          [ceic] "r"(sceic_value),
          [addr] "r"(hot_paddr)
        : "memory");
    return result;
}

static inline struct ai_micro_cache_error_marker_site_result
ai_micro_cache_error_inject_load_then_store_marker_with_site(
    uintptr_t hot_paddr, uintptr_t marker_paddr, uint64_t marker_value,
    uint64_t sceic_value)
{
    struct ai_micro_cache_error_marker_site_result result;

    asm volatile(
        "la %[load_pc], 1f\n\t"
        "la %[after_load_pc], 2f\n\t"
        "la %[marker_store_pc], 3f\n\t"
        "la %[after_marker_store_pc], 4f\n\t"
        "csrw %[scec], zero\n\t"
        "csrw %[sceic], %[ceic]\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "1:\n\t"
        "ld %[load_value], 0(%[hot_addr])\n\t"
        "2:\n\t"
        "addi zero, zero, 0\n\t"
        "3:\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "4:\n\t"
        : [load_value] "=r"(result.load_value),
          [load_pc] "=&r"(result.load_pc),
          [after_load_pc] "=&r"(result.after_load_pc),
          [marker_store_pc] "=&r"(result.marker_store_pc),
          [after_marker_store_pc] "=&r"(result.after_marker_store_pc)
        : [scec] "i"(CSR_SCEC),
          [sceic] "i"(CSR_SCEIC),
          [ceic] "r"(sceic_value),
          [hot_addr] "r"(hot_paddr),
          [marker_addr] "r"(marker_paddr),
          [marker_value] "r"(marker_value)
        : "memory");
    return result;
}

static inline struct ai_micro_cache_error_load_site_result
ai_micro_cache_error_inject_then_cbo_flush_hot_with_site(
    uintptr_t hot_paddr, uint64_t sceic_value)
{
    struct ai_micro_cache_error_load_site_result result;

    asm volatile(
        "la %[load_pc], 1f\n\t"
        "la %[after_load_pc], 2f\n\t"
        "csrw %[scec], zero\n\t"
        "csrw %[sceic], %[ceic]\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "addi zero, zero, 0\n\t"
        "1:\n\t"
        ".insn i 0x0f, 0x2, x0, %[addr], 2\n\t"
        "2:\n\t"
        "ld %[value], 0(%[addr])\n\t"
        : [value] "=r"(result.value),
          [load_pc] "=&r"(result.load_pc),
          [after_load_pc] "=&r"(result.after_load_pc)
        : [scec] "i"(CSR_SCEC),
          [sceic] "i"(CSR_SCEIC),
          [ceic] "r"(sceic_value),
          [addr] "r"(hot_paddr)
        : "memory");
    return result;
}

static inline struct ai_micro_cache_error_csr_observation
ai_micro_cache_error_sample_csrs(void)
{
    struct ai_micro_cache_error_csr_observation obs;

    obs.scec = CSRR(CSR_SCEC);
    obs.sceic = CSRR(CSR_SCEIC);
    obs.mncause = CSRR(CSR_MNCAUSE);
    obs.mnepc = CSRR(CSR_MNEPC);
    return obs;
}

static inline struct ai_micro_cache_error_csr_observation
ai_micro_cache_error_wait_after_expected_consume(void)
{
    struct ai_micro_cache_error_csr_observation obs =
        ai_micro_cache_error_sample_csrs();
    bool consumed_seen = ((obs.sceic & 0x1ULL) == 0ULL);
    unsigned settle_rounds = consumed_seen ? 1U : 0U;

    for (unsigned poll = 0; poll < 16U; ++poll) {
        if (consumed_seen && settle_rounds >= 3U) {
            break;
        }
        ai_micro_cache_error_csr_to_load_delay();
        obs = ai_micro_cache_error_sample_csrs();
        if ((obs.sceic & 0x1ULL) == 0ULL) {
            consumed_seen = true;
            ++settle_rounds;
        }
    }

    return obs;
}

static inline bool ai_micro_cache_error_icache_fetch_window_line(uint64_t addr,
    uintptr_t wrapper_pc, uintptr_t jump_pc, uintptr_t after_jump_pc,
    uintptr_t probe_pc)
{
    return ai_micro_cache_error_same_icache_line(addr, wrapper_pc) ||
        ai_micro_cache_error_same_icache_line(addr, jump_pc) ||
        ai_micro_cache_error_same_icache_line(addr, after_jump_pc) ||
        ai_micro_cache_error_same_icache_line(addr, probe_pc);
}

static inline bool
ai_micro_cache_error_icache_doubleline_fetch_window_line(uint64_t addr,
    uintptr_t wrapper_pc, uintptr_t jump_pc, uintptr_t after_jump_pc,
    uintptr_t probe_pc)
{
    return ai_micro_cache_error_icache_fetch_window_line(addr, wrapper_pc,
        jump_pc, after_jump_pc, probe_pc) ||
        ai_micro_cache_error_same_icache_line(
            addr, probe_pc + AI_MICRO_CACHE_ERROR_ICACHE_LINE_BYTES);
}

// Requires LinkNan/Nanhu RTL with the frontend ICache data-ECC injection path.
// The public oracle is SCEIC one-shot consume plus an SCEC ICACHE_DATA record;
// the exact poisoned instruction line is intentionally treated as diagnostic
// because the frontend may already have prefetched part of the wrapper window.
bool ai_micro_m_icache_data_ecc_inject_fetch_progress_reports_scec_corner()
{
    TEST_START();

    static volatile uint64_t marker[2] __attribute__((aligned(64)));
    const uint64_t wrapper_warm_value = 0x1ca0000000000001ULL;
    const uint64_t direct_warm_value = 0x1ca0000000001001ULL;
    const uint64_t inject_value = 0x1ca0000000002001ULL;
    const uint64_t recovery_value = 0x1ca0000000003001ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_IDATA_SRC, true);
    const uintptr_t wrapper_pc =
        (uintptr_t)ai_micro_cache_error_guarded_inject_icache_data_probe;
    const uintptr_t probe_pc =
        (uintptr_t)ai_micro_cache_error_icache_data_probe;

    uint64_t original_mnstatus;
    uint64_t original_sfetchctl;
    uint64_t original_scec;
    uint64_t original_sceic;
    uintptr_t warm_jump_pc = 0ULL;
    uintptr_t warm_after_jump_pc = 0ULL;
    uintptr_t inject_jump_pc = 0ULL;
    uintptr_t inject_after_jump_pc = 0ULL;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_sfetchctl = CSRR(CSR_SFETCHCTL);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);

    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEIC, 0ULL);
    CSRW(CSR_SCEC, 0ULL);
    CSRS(CSR_SFETCHCTL, AI_MICRO_CACHE_ERROR_SFETCHCTL_ICACHE_PARITY_ENABLE);
    ai_micro_cache_error_csr_to_load_delay();

    marker[0] = 0ULL;
    marker[1] = 0ULL;

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_guarded_inject_icache_data_probe(marker,
        wrapper_warm_value, (volatile uint64_t *)&excpt.fault_inst,
        &warm_jump_pc, &warm_after_jump_pc, 0ULL);
    AI_ASSERT_NO_EXCEPT_AND(
        "with SCEIC disabled, the guarded ICache fetch probe warms the wrapper/probe code path and updates both marker doublewords",
        marker[0] == wrapper_warm_value &&
            marker[1] == wrapper_warm_value + 1ULL &&
            warm_jump_pc != 0ULL &&
            warm_after_jump_pc != 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_icache_data_probe(marker, direct_warm_value);
    AI_ASSERT_NO_EXCEPT_AND(
        "the direct aligned fetch probe executes normally before arming ICache-data ECC injection",
        marker[0] == direct_warm_value &&
            marker[1] == direct_warm_value + 1ULL
    );

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_guarded_inject_icache_data_probe(marker,
        inject_value, (volatile uint64_t *)&excpt.fault_inst,
        &inject_jump_pc, &inject_after_jump_pc, sceic_value);
    struct ai_micro_cache_error_csr_observation obs =
        ai_micro_cache_error_wait_after_expected_consume();
    const uint64_t scec_after = obs.scec;
    const uint64_t sceic_after = obs.sceic;
    const uint64_t scec_epa = ai_micro_cache_error_scec_epa(scec_after);
    const bool inject_consumed = (sceic_after & 0x1ULL) == 0ULL;
    const bool scec_icache_data = ai_micro_cache_error_scec_valid(scec_after) &&
        ai_micro_cache_error_scec_source(scec_after) ==
            AI_MICRO_CACHE_ERROR_IDATA_SRC;
    const bool scec_expected_line =
        ai_micro_cache_error_icache_fetch_window_line(scec_epa, wrapper_pc,
            inject_jump_pc, inject_after_jump_pc, probe_pc);
    const bool inject_excpt_triggered = excpt.triggered;
    const uint64_t inject_excpt_cause = excpt.cause;
    const uint64_t inject_excpt_tval = excpt.tval;
    const bool sync_exception_ok = !inject_excpt_triggered ||
        (inject_excpt_cause == CAUSE_IAF &&
            (inject_excpt_tval == 0ULL ||
                ai_micro_cache_error_icache_fetch_window_line(
                    inject_excpt_tval, wrapper_pc, inject_jump_pc,
                    inject_after_jump_pc, probe_pc)));

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_icache_data_probe(marker, recovery_value);
    const uint64_t recovery_marker0 = marker[0];
    const uint64_t recovery_marker1 = marker[1];
    AI_ASSERT_NO_EXCEPT_AND(
        "after the one-shot ICache-data ECC injection window, a normal fetch of the same probe still reaches the marker stores exactly once",
        recovery_marker0 == recovery_value &&
            recovery_marker1 == recovery_value + 1ULL
    );

    TEST_ASSERT(
        "the ICACHE_DATA SCEIC request is consumed and auto-cleared by a frontend fetch-data ECC check",
        inject_consumed,
        "sceic_after=0x%llx scec_after=0x%llx jump_pc=0x%llx after_jump_pc=0x%llx probe_pc=0x%llx",
        (unsigned long long)sceic_after,
        (unsigned long long)scec_after,
        (unsigned long long)inject_jump_pc,
        (unsigned long long)inject_after_jump_pc,
        (unsigned long long)probe_pc
    );

    TEST_ASSERT(
        "the consumed frontend ECC injection records an SCEC event sourced from ICACHE_DATA rather than a DCache/L2TLB source",
        scec_icache_data,
        "scec_after=0x%llx source=%llu epa=0x%llx expected_line=%d wrapper_pc=0x%llx jump_pc=0x%llx after_jump_pc=0x%llx probe_pc=0x%llx",
        (unsigned long long)scec_after,
        (unsigned long long)ai_micro_cache_error_scec_source(scec_after),
        (unsigned long long)scec_epa,
        (int)scec_expected_line,
        (unsigned long long)wrapper_pc,
        (unsigned long long)inject_jump_pc,
        (unsigned long long)inject_after_jump_pc,
        (unsigned long long)probe_pc
    );

    TEST_ASSERT(
        "if the injected ICache data corrupt also becomes a synchronous fetch exception, it is reported as instruction access fault in the guarded fetch window",
        sync_exception_ok,
        "triggered=%d cause=0x%llx tval=0x%llx wrapper_pc=0x%llx jump_pc=0x%llx after_jump_pc=0x%llx probe_pc=0x%llx",
        (int)inject_excpt_triggered,
        (unsigned long long)inject_excpt_cause,
        (unsigned long long)inject_excpt_tval,
        (unsigned long long)wrapper_pc,
        (unsigned long long)inject_jump_pc,
        (unsigned long long)inject_after_jump_pc,
        (unsigned long long)probe_pc
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    CSRW(CSR_SFETCHCTL, original_sfetchctl);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Re-arms ICACHE_DATA SCEIC several times, then uses FENCE.I to force a fresh
// fetch of an inline probe placed at offset 0x30 in a 64B line. The target loops
// after refill so injection is detected by a subsequent double-line SRAM hit.
bool ai_micro_m_icache_data_ecc_multi_rearm_doubleline_fetch_reports_scec_corner()
{
    TEST_START();

    static volatile uint64_t marker[4] __attribute__((aligned(64)));
    const uint64_t wrapper_warm_value = 0x1cd0000000000001ULL;
    const uint64_t direct_warm_value = 0x1cd0000000001001ULL;
    const uint64_t recovery_value = 0x1cd0000000005001ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_IDATA_SRC, true);
    const uintptr_t wrapper_pc =
        (uintptr_t)ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe;
    const uintptr_t probe_pc =
        (uintptr_t)ai_micro_cache_error_icache_data_doubleline_inject_target;
    const uintptr_t probe_second_line_pc =
        probe_pc + AI_MICRO_CACHE_ERROR_ICACHE_LINE_BYTES;
    const uintptr_t probe_entry_offset =
        probe_pc & (AI_MICRO_CACHE_ERROR_ICACHE_LINE_BYTES - 1ULL);
    const bool probe_layout_doubleline =
        probe_entry_offset ==
            AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ENTRY_OFFSET;

    uint64_t original_mnstatus;
    uint64_t original_sfetchctl;
    uint64_t original_scec;
    uint64_t original_sceic;
    uintptr_t warm_jump_pc = 0ULL;
    uintptr_t warm_after_jump_pc = 0ULL;
    uintptr_t last_jump_pc = 0ULL;
    uintptr_t last_after_jump_pc = 0ULL;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_scec_epa = 0ULL;
    uint64_t last_excpt_cause = 0ULL;
    uint64_t last_excpt_tval = 0ULL;
    unsigned consumed_rounds = 0U;
    unsigned icache_data_rounds = 0U;
    unsigned expected_line_rounds = 0U;
    unsigned target_doubleline_rounds = 0U;
    bool sync_exception_ok_each_round = true;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_sfetchctl = CSRR(CSR_SFETCHCTL);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);

    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEIC, 0ULL);
    CSRW(CSR_SCEC, 0ULL);
    CSRS(CSR_SFETCHCTL, AI_MICRO_CACHE_ERROR_SFETCHCTL_ICACHE_PARITY_ENABLE);
    ai_micro_cache_error_csr_to_load_delay();

    marker[0] = 0ULL;
    marker[1] = 0ULL;
    marker[2] = 0ULL;
    marker[3] = 0ULL;

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe(marker,
        wrapper_warm_value, (volatile uint64_t *)&excpt.fault_inst,
        &warm_jump_pc, &warm_after_jump_pc, 0ULL);
    AI_ASSERT_NO_EXCEPT_AND(
        "with SCEIC disabled, the guarded double-line ICache fetch probe warms the wrapper/probe path and reaches all marker stores",
        marker[0] == wrapper_warm_value &&
            marker[1] == wrapper_warm_value + 1ULL &&
            marker[2] == wrapper_warm_value + 2ULL &&
            warm_jump_pc != 0ULL &&
            warm_after_jump_pc != 0ULL
    );

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_icache_data_doubleline_probe(marker,
        direct_warm_value);
    AI_ASSERT_NO_EXCEPT_AND(
        "the direct double-line fetch probe executes normally before arming repeated ICache-data ECC injection",
        marker[0] == direct_warm_value &&
            marker[1] == direct_warm_value + 1ULL &&
            marker[2] == direct_warm_value + 2ULL
    );

    TEST_ASSERT(
        "the double-line probe entry is placed at 0x30 within a 64B ICache line so LinkNan fetches line0 and line1 for the same request window",
        probe_layout_doubleline,
        "probe_pc=0x%llx offset=0x%llx expected_offset=0x%llx second_line_pc=0x%llx",
        (unsigned long long)probe_pc,
        (unsigned long long)probe_entry_offset,
        (unsigned long long)AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ENTRY_OFFSET,
        (unsigned long long)probe_second_line_pc
    );

    for (unsigned round = 0U;
        round < AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS; ++round) {
        const uint64_t inject_value =
            0x1cd0000000002001ULL + (uint64_t)round * 0x100ULL;
        uintptr_t round_jump_pc = 0ULL;
        uintptr_t round_after_jump_pc = 0ULL;

        CSRW(CSR_SCEC, 0ULL);
        TEST_SETUP_EXCEPT();
        ai_micro_cache_error_guarded_inject_icache_data_doubleline_probe(marker,
            inject_value, (volatile uint64_t *)&excpt.fault_inst,
            &round_jump_pc, &round_after_jump_pc, sceic_value);
        struct ai_micro_cache_error_csr_observation obs =
            ai_micro_cache_error_wait_after_expected_consume();
        const uint64_t scec_after = obs.scec;
        const uint64_t sceic_after = obs.sceic;
        const uint64_t scec_epa = ai_micro_cache_error_scec_epa(scec_after);
        const bool inject_consumed = (sceic_after & 0x1ULL) == 0ULL;
        const bool scec_icache_data =
            ai_micro_cache_error_scec_valid(scec_after) &&
            ai_micro_cache_error_scec_source(scec_after) ==
                AI_MICRO_CACHE_ERROR_IDATA_SRC;
        const bool scec_expected_line =
            ai_micro_cache_error_icache_doubleline_fetch_window_line(
                scec_epa, wrapper_pc, round_jump_pc, round_after_jump_pc,
                probe_pc);
        const bool scec_target_doubleline =
            ai_micro_cache_error_same_icache_line(scec_epa, probe_pc) ||
            ai_micro_cache_error_same_icache_line(scec_epa,
                probe_second_line_pc);
        const bool sync_exception_ok = !excpt.triggered ||
            (excpt.cause == CAUSE_IAF &&
                (excpt.tval == 0ULL ||
                    ai_micro_cache_error_icache_doubleline_fetch_window_line(
                        excpt.tval, wrapper_pc, round_jump_pc,
                        round_after_jump_pc, probe_pc)));

        consumed_rounds += inject_consumed ? 1U : 0U;
        icache_data_rounds += scec_icache_data ? 1U : 0U;
        expected_line_rounds += (scec_icache_data && scec_expected_line) ?
            1U : 0U;
        target_doubleline_rounds +=
            (scec_icache_data && scec_target_doubleline) ? 1U : 0U;
        sync_exception_ok_each_round =
            sync_exception_ok_each_round && sync_exception_ok;
        last_jump_pc = round_jump_pc;
        last_after_jump_pc = round_after_jump_pc;
        last_scec = scec_after;
        last_sceic = sceic_after;
        last_scec_epa = scec_epa;
        last_excpt_cause = excpt.cause;
        last_excpt_tval = excpt.tval;
    }

    TEST_SETUP_EXCEPT();
    ai_micro_cache_error_icache_data_doubleline_probe(marker, recovery_value);
    const uint64_t recovery_marker0 = marker[0];
    const uint64_t recovery_marker1 = marker[1];
    const uint64_t recovery_marker2 = marker[2];
    AI_ASSERT_NO_EXCEPT_AND(
        "after repeated one-shot ICache-data ECC injection, the same double-line probe fetches normally and updates all marker doublewords",
        recovery_marker0 == recovery_value &&
            recovery_marker1 == recovery_value + 1ULL &&
            recovery_marker2 == recovery_value + 2ULL
    );

    TEST_ASSERT(
        "each re-armed ICACHE_DATA SCEIC request is consumed and auto-cleared by frontend fetch-data ECC checking",
        consumed_rounds == AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS,
        "consumed_rounds=%u expected=%u last_sceic=0x%llx last_scec=0x%llx last_jump_pc=0x%llx last_after_jump_pc=0x%llx probe_pc=0x%llx",
        consumed_rounds,
        AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS,
        (unsigned long long)last_sceic,
        (unsigned long long)last_scec,
        (unsigned long long)last_jump_pc,
        (unsigned long long)last_after_jump_pc,
        (unsigned long long)probe_pc
    );

    TEST_ASSERT(
        "each repeated frontend ECC injection records an SCEC event sourced from ICACHE_DATA",
        icache_data_rounds == AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS,
        "icache_data_rounds=%u expected=%u last_scec=0x%llx source=%llu epa=0x%llx",
        icache_data_rounds,
        AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS,
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_source(last_scec),
        (unsigned long long)last_scec_epa
    );

    TEST_ASSERT(
        "all repeated SCEC ICACHE_DATA records point at the guarded double-line fetch window rather than an unrelated cache-error source",
        expected_line_rounds == AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS,
        "expected_line_rounds=%u expected=%u last_scec=0x%llx last_epa=0x%llx wrapper_pc=0x%llx jump_pc=0x%llx after_jump_pc=0x%llx probe_pc=0x%llx second_line_pc=0x%llx",
        expected_line_rounds,
        AI_MICRO_CACHE_ERROR_ICACHE_DOUBLELINE_ROUNDS,
        (unsigned long long)last_scec,
        (unsigned long long)last_scec_epa,
        (unsigned long long)wrapper_pc,
        (unsigned long long)last_jump_pc,
        (unsigned long long)last_after_jump_pc,
        (unsigned long long)probe_pc,
        (unsigned long long)probe_second_line_pc
    );

    TEST_ASSERT(
        "at least one repeated ICACHE_DATA SCEC record is attributed to the intended double-line probe cacheline pair",
        target_doubleline_rounds >= 1U,
        "target_doubleline_rounds=%u last_scec=0x%llx last_epa=0x%llx probe_pc=0x%llx second_line_pc=0x%llx wrapper_pc=0x%llx",
        target_doubleline_rounds,
        (unsigned long long)last_scec,
        (unsigned long long)last_scec_epa,
        (unsigned long long)probe_pc,
        (unsigned long long)probe_second_line_pc,
        (unsigned long long)wrapper_pc
    );

    TEST_ASSERT(
        "if any repeated double-line fetch injection also becomes a synchronous exception, it is an instruction access fault in the guarded two-line fetch window",
        sync_exception_ok_each_round,
        "last_cause=0x%llx last_tval=0x%llx last_scec=0x%llx wrapper_pc=0x%llx jump_pc=0x%llx after_jump_pc=0x%llx probe_pc=0x%llx second_line_pc=0x%llx",
        (unsigned long long)last_excpt_cause,
        (unsigned long long)last_excpt_tval,
        (unsigned long long)last_scec,
        (unsigned long long)wrapper_pc,
        (unsigned long long)last_jump_pc,
        (unsigned long long)last_after_jump_pc,
        (unsigned long long)probe_pc,
        (unsigned long long)probe_second_line_pc
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    CSRW(CSR_SFETCHCTL, original_sfetchctl);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Requires Xiangshan RTL with DCacheParameters.enableDataEcc = true.
// Under the current default Nanhu-V5.1 parameter file, tag/data ECC codes are configured
// but DCache injection itself is still locally gated by enableDataEcc/enableTagEcc.
static bool
ai_micro_cache_error_run_dcache_data_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
    const char *case_name, bool inject_two_bits)
{
    const char* __test_name = case_name;
    bool test_status = true;
    if (LOG_LEVEL >= LOG_INFO) {
        printf(CBLU "%-70s" CDFLT, __test_name);
    }
    if (LOG_LEVEL >= LOG_DETAIL) {
        printf("\n");
    }

    const uintptr_t hot_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t follower_paddr =
        phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uint64_t hot_seed = 0x0123456789abcdefULL;
    const uint64_t follower_seed = 0xfedcba9876543210ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, inject_two_bits);

    uint64_t original_mnstatus;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t progress = 0x6f5e4d3c2b1a0918ULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    bool tlb_match = true;
    bool dcache_match = true;
    bool inject_consumed_each_round = true;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);

    write64(hot_paddr, hot_seed);
    write64(follower_paddr, follower_seed);
    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t warm_hot = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t warm_follower = ai_micro_cache_error_ld64(follower_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the interleaved HS-TLB and M-mode DCache ECC injection window, both target physical lines warm in DCache with their seeded payloads",
        warm_hot == hot_seed && warm_follower == follower_seed
    );

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_ROUNDS; ++round) {
        unsigned page_index = round % (sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]));
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(
            page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_observed = ai_micro_cache_error_ld64(tlb_vaddr);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_observed == expected_tlb;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        uint64_t hot_observed = ai_micro_cache_error_inject_then_load_hot(
            hot_paddr, sceic_value);
        ai_micro_cache_error_csr_to_load_delay();
        uint64_t follower_observed = ai_micro_cache_error_ld64(follower_paddr);
        uint64_t scec_after = CSRR(CSR_SCEC);
        uint64_t sceic_after = CSRR(CSR_SCEIC);
        uint64_t mncause_after = CSRR(CSR_MNCAUSE);
        uint64_t mnepc_after = CSRR(CSR_MNEPC);

        dcache_match = dcache_match &&
            excpt.triggered == false &&
            hot_observed == hot_seed &&
            follower_observed == follower_seed;
        inject_consumed_each_round = inject_consumed_each_round &&
            ((sceic_after & 0x1ULL) == 0ULL);
        ai_micro_cache_error_note_scec_match(scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);

        progress = ai_micro_cache_error_round_mix(progress, tlb_observed,
            hot_observed, follower_observed, round);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, hot_seed, follower_seed, round);

        last_scec = scec_after;
        last_sceic = sceic_after;
        last_mncause = mncause_after;
        last_mnepc = mnepc_after;
    }

    ai_micro_cache_error_quiesce_injection();

    TEST_ASSERT(
        "with repeated full sfence.vma before each HS probe, the interleaved virtual-address stream still returns every seeded payload so TLB activity stays live throughout the mixed cache-error window",
        tlb_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "after each one-shot DCache-data CEIC programming, the immediately following hot physical load and the later follower physical load both retire with their seeded payloads instead of stalling or corrupting the visible data path",
        dcache_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "the CEIC enable bit is consumed and auto-cleared in every round, so the stress window really reaches the DCache injection sink instead of silently bypassing it",
        inject_consumed_each_round,
        "last_sceic=0x%llx last_scec=0x%llx",
        (unsigned long long)last_sceic,
        (unsigned long long)last_scec
    );

    TEST_ASSERT(
        "whenever the active build surfaces a DCache-data cache-error record in SCEC during the mixed sink-reachability loop, the record names DCache-data and the injected hot-line physical address",
        !scec_dcache_data_unexpected,
        "last_scec=0x%llx last_scec_epa=0x%llx last_sceic=0x%llx exact_seen=%d",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_sceic,
        (int)scec_dcache_data_exact_seen
    );

    TEST_ASSERT(
        "the interleaved HS-TLB and M-mode DCache ECC-injection sequence reaches the deterministic final progress signature instead of hanging mid-window",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_mncause=0x%llx",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Legacy unsuffixed name keeps the historical 2-bit DCache-data profile.
bool ai_micro_m_dcache_data_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_data_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
        __func__, true);
}

bool ai_micro_m_dcache_data_ecc_1bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_data_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
        __func__, false);
}

bool ai_micro_m_dcache_data_ecc_2bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_data_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
        __func__, true);
}

// Requires Xiangshan RTL with DCacheParameters.enableDataEcc = true.
// This case targets the current XSCore path where memBlock.io.error.valid is OR-ed
// into backend externalInterrupt.nmi.nmi_31, so a DCache load ECC error should be
// observable through MNCAUSE while the mixed TLB-plus-DCache stream still makes progress.
bool ai_micro_m_dcache_data_ecc_nmi31_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    TEST_START();

    const uintptr_t hot_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t follower_paddr =
        phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uint64_t hot_seed = 0x2468ace013579bdfULL;
    const uint64_t follower_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, true);

    uint64_t original_mnstatus;
    uint64_t original_mtvec;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t baseline_mncause;
    uint64_t baseline_mnepc;
    uint64_t progress = 0x55aa12ef34cd78b9ULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    uint64_t last_rnmi_hits = 0ULL;
    uint64_t last_rnmi_mncause = 0ULL;
    uint64_t last_rnmi_mnepc = 0ULL;
    uint64_t last_rnmi_scec = 0ULL;
    uint64_t last_rnmi_sceic = 0ULL;
    bool tlb_match = true;
    bool dcache_match = true;
    bool inject_consumed_each_round = true;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;
    bool nmi31_seen = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_mtvec = CSRR(CSR_MTVEC);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);
    baseline_mncause = CSRR(CSR_MNCAUSE);
    baseline_mnepc = CSRR(CSR_MNEPC);

    write64(hot_paddr, hot_seed);
    write64(follower_paddr, follower_seed);
    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    ai_micro_cache_error_reset_rnmi_state();
    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t warm_hot = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t warm_follower = ai_micro_cache_error_ld64(follower_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the DCache-data-ECC NMI window, both target physical lines warm in DCache with their seeded payloads",
        warm_hot == hot_seed && warm_follower == follower_seed
    );

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_ROUNDS; ++round) {
        unsigned page_index = round % (sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]));
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(
            page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_observed = ai_micro_cache_error_ld64(tlb_vaddr);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_observed == expected_tlb;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
        struct ai_micro_cache_error_load_site_result hot_load =
            ai_micro_cache_error_inject_then_load_hot_with_site(
                hot_paddr, sceic_value);
        uint64_t hot_observed = hot_load.value;
        ai_micro_cache_error_csr_to_load_delay();
        uint64_t follower_observed = ai_micro_cache_error_ld64(follower_paddr);
        uint64_t scec_after = CSRR(CSR_SCEC);
        uint64_t sceic_after = CSRR(CSR_SCEIC);
        uint64_t mncause_after = CSRR(CSR_MNCAUSE);
        uint64_t mnepc_after = CSRR(CSR_MNEPC);
        uint64_t rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
        uint64_t rnmi_mncause_after =
            ai_micro_cache_error_rnmi_last_mncause;
        uint64_t rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
        uint64_t rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
        uint64_t rnmi_sceic_after = ai_micro_cache_error_rnmi_last_sceic;
        CSRW(CSR_MTVEC, original_mtvec);

        dcache_match = dcache_match &&
            excpt.triggered == false &&
            hot_observed == hot_seed &&
            follower_observed == follower_seed;
        inject_consumed_each_round = inject_consumed_each_round &&
            ((sceic_after & 0x1ULL) == 0ULL);
        ai_micro_cache_error_note_scec_match(scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
        ai_micro_cache_error_note_scec_match(rnmi_scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
        nmi31_seen = nmi31_seen ||
            (rnmi_hits_after > 0ULL &&
                ai_micro_cache_error_is_nmi31(rnmi_mncause_after));

        progress = ai_micro_cache_error_round_mix(progress, tlb_observed,
            hot_observed, follower_observed, round);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, hot_seed, follower_seed, round);

        last_scec = scec_after;
        last_sceic = sceic_after;
        last_mncause = mncause_after;
        last_mnepc = mnepc_after;
        last_rnmi_hits = rnmi_hits_after;
        last_rnmi_mncause = rnmi_mncause_after;
        last_rnmi_mnepc = rnmi_mnepc_after;
        last_rnmi_scec = rnmi_scec_after;
        last_rnmi_sceic = rnmi_sceic_after;
    }

    TEST_ASSERT(
        "with repeated full sfence.vma before each HS probe, the interleaved virtual-address stream still returns every seeded payload so TLB activity stays live throughout the DCache-error NMI window",
        tlb_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "after each one-shot DCache-data CEIC programming, the immediately following hot physical load and the later follower physical load both retire with their seeded payloads instead of stalling in the presence of the cache-error path",
        dcache_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "the CEIC enable bit is consumed and auto-cleared in every round, so the stress window really reaches the DCache data injection sink instead of silently bypassing it",
        inject_consumed_each_round,
        "last_sceic=0x%llx last_scec=0x%llx",
        (unsigned long long)last_sceic,
        (unsigned long long)last_scec
    );

    TEST_ASSERT(
        "at least one round latches a DCache-data cache-error record with the injected hot-line physical address in SCEC while the mixed TLB-plus-DCache loop still reaches completion",
        scec_dcache_data_exact_seen,
        "last_scec=0x%llx last_scec_epa=0x%llx last_rnmi_scec=0x%llx last_rnmi_scec_epa=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_rnmi_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_rnmi_scec)
    );

    TEST_ASSERT(
        "whenever the mixed DCache-data NMI loop surfaces a valid SCEC record, the record stays pinned to the DCache-data source and the injected hot-line physical address",
        !scec_dcache_data_unexpected,
        "last_scec=0x%llx last_scec_epa=0x%llx last_rnmi_scec=0x%llx last_rnmi_scec_epa=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_rnmi_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_rnmi_scec)
    );

    TEST_ASSERT(
        "the local mtvec RNMI hook records injected DCache-data activity as RNMI cause NMI_31 in MNCAUSE, matching the current XSCore path that ORs memBlock.io.error.valid into externalInterrupt.nmi.nmi_31",
        nmi31_seen,
        "baseline_mncause=0x%llx baseline_mnepc=0x%llx last_rnmi_hits=%llu last_rnmi_mncause=0x%llx last_rnmi_mnepc=0x%llx",
        (unsigned long long)baseline_mncause,
        (unsigned long long)baseline_mnepc,
        (unsigned long long)last_rnmi_hits,
        (unsigned long long)last_rnmi_mncause,
        (unsigned long long)last_rnmi_mnepc
    );

    TEST_ASSERT(
        "the interleaved HS-TLB and M-mode DCache-data-ECC NMI sequence reaches the deterministic final progress signature instead of hanging mid-window",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_mncause=0x%llx",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, original_mtvec);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

bool ai_micro_m_dcache_data_ecc_nmi31_head_load_younger_marker_progress_corner()
{
    TEST_START();

    const uintptr_t hot_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t marker_paddr =
        phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_MARKER_OFFSET;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t hot_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_seed = 0xfeedfacecafebeefULL;
    const uint64_t marker_guard_seed = 0x0badf00d55aa33ccULL;
    const uint64_t marker_update = 0x1122aabb3344ccddULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, true);

    uint64_t original_mnstatus;
    uint64_t original_mtvec;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t baseline_mncause;
    uint64_t baseline_mnepc;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_mtvec = CSRR(CSR_MTVEC);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);
    baseline_mncause = CSRR(CSR_MNCAUSE);
    baseline_mnepc = CSRR(CSR_MNEPC);

    write64(hot_paddr, hot_seed);
    write64(marker_paddr, marker_seed);
    write64(marker_guard_paddr, marker_guard_seed);

    ai_micro_cache_error_reset_rnmi_state();
    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t warm_hot = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t warm_marker = ai_micro_cache_error_ld64(marker_paddr);
    uint64_t warm_guard = ai_micro_cache_error_ld64(marker_guard_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the AP-24 head-load plus younger-marker window, the DCache hot line, marker doubleword, and marker guard all warm with their seeded payloads",
        warm_hot == hot_seed &&
            warm_marker == marker_seed &&
            warm_guard == marker_guard_seed
    );

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
    struct ai_micro_cache_error_marker_site_result site =
        ai_micro_cache_error_inject_load_then_store_marker_with_site(
            hot_paddr, marker_paddr, marker_update, sceic_value);
    ai_micro_cache_error_csr_to_load_delay();
    fence_iorw_iorw();
    uint64_t marker_after = read64(marker_paddr);
    uint64_t marker_guard_after = read64(marker_guard_paddr);
    uint64_t hot_after = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t scec_after = CSRR(CSR_SCEC);
    uint64_t sceic_after = CSRR(CSR_SCEIC);
    uint64_t rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
    uint64_t rnmi_mncause_after = ai_micro_cache_error_rnmi_last_mncause;
    uint64_t rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
    uint64_t rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
    CSRW(CSR_MTVEC, original_mtvec);

    ai_micro_cache_error_note_scec_match(scec_after,
        AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
        &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
    ai_micro_cache_error_note_scec_match(rnmi_scec_after,
        AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
        &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);

    TEST_ASSERT(
        "the AP-24 head load that consumes DCache-data CEIC still returns its seeded payload and does not leave a regular synchronous exception while NMI_31 is observed around the same window",
        excpt.triggered == false &&
            site.load_value == hot_seed &&
            hot_after == hot_seed,
        "load_pc=0x%llx after_load_pc=0x%llx scec=0x%llx sceic=0x%llx",
        (unsigned long long)site.load_pc,
        (unsigned long long)site.after_load_pc,
        (unsigned long long)scec_after,
        (unsigned long long)sceic_after
    );

    TEST_ASSERT(
        "the younger marker store behind the DCache-error head load becomes visible exactly once after the RNMI window returns, proving the ROB/NMI boundary neither loses nor duplicates the younger side effect",
        marker_after == marker_update &&
            marker_guard_after == marker_guard_seed,
        "marker_after=0x%llx marker_update=0x%llx marker_guard_after=0x%llx marker_guard_seed=0x%llx store_pc=0x%llx after_store_pc=0x%llx",
        (unsigned long long)marker_after,
        (unsigned long long)marker_update,
        (unsigned long long)marker_guard_after,
        (unsigned long long)marker_guard_seed,
        (unsigned long long)site.marker_store_pc,
        (unsigned long long)site.after_marker_store_pc
    );

    TEST_ASSERT(
        "the DCache-data head-load plus younger-marker window consumes and auto-clears the one-shot CEIC request",
        (sceic_after & 0x1ULL) == 0ULL,
        "sceic_after=0x%llx scec_after=0x%llx rnmi_hits=%llu",
        (unsigned long long)sceic_after,
        (unsigned long long)scec_after,
        (unsigned long long)rnmi_hits_after
    );

    TEST_ASSERT(
        "the AP-24 head-load marker smoke records the DCache-data source and injected hot-line physical address in SCEC/RNMI state",
        scec_dcache_data_exact_seen && !scec_dcache_data_unexpected,
        "scec_after=0x%llx rnmi_scec=0x%llx expected_epa=0x%llx",
        (unsigned long long)scec_after,
        (unsigned long long)rnmi_scec_after,
        (unsigned long long)hot_paddr
    );

    TEST_ASSERT(
        "the local RNMI hook observes the head-load DCache-data ECC event as NMI_31 instead of leaving a pending async NMI blocked behind the memory head",
        rnmi_hits_after > 0ULL &&
            ai_micro_cache_error_is_nmi31(rnmi_mncause_after),
        "baseline_mncause=0x%llx baseline_mnepc=0x%llx rnmi_hits=%llu rnmi_mncause=0x%llx rnmi_mnepc=0x%llx",
        (unsigned long long)baseline_mncause,
        (unsigned long long)baseline_mnepc,
        (unsigned long long)rnmi_hits_after,
        (unsigned long long)rnmi_mncause_after,
        (unsigned long long)rnmi_mnepc_after
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, original_mtvec);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// This case prefers Xiangshan RTL with DCacheParameters.enableTagEcc = true.
// On data-only builds, it falls back to proving the tag CEIC request stays
// predictably pending instead of falsely claiming the tag sink was reached.
static bool
ai_micro_cache_error_run_dcache_tag_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
    const char *case_name, bool inject_two_bits)
{
    const char* __test_name = case_name;
    bool test_status = true;
    if (LOG_LEVEL >= LOG_INFO) {
        printf(CBLU "%-70s" CDFLT, __test_name);
    }
    if (LOG_LEVEL >= LOG_DETAIL) {
        printf("\n");
    }

    const uintptr_t hot_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t follower_paddr =
        phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uint64_t hot_seed = 0x89abcdef01234567ULL;
    const uint64_t follower_seed = 0x76543210fedcba98ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DTAG_SRC, inject_two_bits);

    uint64_t original_mnstatus;
    uint64_t original_mtvec;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t baseline_mncause;
    uint64_t baseline_mnepc;
    uint64_t progress = 0x18293a4b5c6d7e8fULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    uint64_t tag_preflight_scec = 0ULL;
    uint64_t tag_preflight_sceic = 0ULL;
    uint64_t last_rnmi_hits = 0ULL;
    uint64_t last_rnmi_mncause = 0ULL;
    uint64_t last_rnmi_mnepc = 0ULL;
    uint64_t last_rnmi_scec = 0ULL;
    uint64_t last_rnmi_sceic = 0ULL;
    struct ai_micro_cache_error_csr_observation obs;
    bool tlb_match = true;
    bool dcache_match = true;
    bool tag_sink_supported = false;
    bool tag_sink_missing_expected = false;
    bool inject_consumed_each_round = true;
    bool scec_dcache_tag_exact_seen = false;
    bool scec_dcache_tag_unexpected = false;
    bool nmi31_seen = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_mtvec = CSRR(CSR_MTVEC);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);
    baseline_mncause = CSRR(CSR_MNCAUSE);
    baseline_mnepc = CSRR(CSR_MNEPC);

    write64(hot_paddr, hot_seed);
    write64(follower_paddr, follower_seed);
    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    ai_micro_cache_error_reset_rnmi_state();
    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t warm_hot = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t warm_follower = ai_micro_cache_error_ld64(follower_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the interleaved HS-TLB and M-mode DCache tag-ECC injection window, both target physical lines warm in DCache with their seeded payloads",
        warm_hot == hot_seed && warm_follower == follower_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t tag_preflight_hot = ai_micro_cache_error_inject_then_load_hot(
        hot_paddr, sceic_value);
    obs = ai_micro_cache_error_wait_after_expected_consume();
    tag_preflight_scec = obs.scec;
    tag_preflight_sceic = obs.sceic;
    tag_sink_supported =
        excpt.triggered == false &&
        tag_preflight_hot == hot_seed &&
        ((tag_preflight_sceic & 0x1ULL) == 0ULL);
    tag_sink_missing_expected =
        excpt.triggered == false &&
        tag_preflight_hot == hot_seed &&
        ((tag_preflight_sceic & 0x1ULL) != 0ULL) &&
        !ai_micro_cache_error_scec_valid(tag_preflight_scec);
    ai_micro_cache_error_note_scec_match(tag_preflight_scec,
        AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_paddr,
        &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
    if (!tag_sink_supported) {
        CSRW(CSR_SCEC, 0ULL);
        CSRW(CSR_SCEIC, 0ULL);
    }

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_ROUNDS; ++round) {
        unsigned page_index = round % (sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]));
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(
            page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_observed = ai_micro_cache_error_ld64(tlb_vaddr);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_observed == expected_tlb;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        struct ai_micro_cache_error_load_site_result hot_load = {0};
        uint64_t hot_observed;
        uint64_t follower_observed;
        uint64_t scec_after;
        uint64_t sceic_after;
        uint64_t mncause_after;
        uint64_t mnepc_after;
        uint64_t rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
        uint64_t rnmi_mncause_after = ai_micro_cache_error_rnmi_last_mncause;
        uint64_t rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
        uint64_t rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
        uint64_t rnmi_sceic_after = ai_micro_cache_error_rnmi_last_sceic;

        if (tag_sink_supported) {
            CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
            hot_load = ai_micro_cache_error_inject_then_load_hot_with_site(
                hot_paddr, sceic_value);
            hot_observed = hot_load.value;
            ai_micro_cache_error_csr_to_load_delay();
            follower_observed = ai_micro_cache_error_ld64(follower_paddr);
            scec_after = CSRR(CSR_SCEC);
            sceic_after = CSRR(CSR_SCEIC);
            mncause_after = CSRR(CSR_MNCAUSE);
            mnepc_after = CSRR(CSR_MNEPC);
            rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
            rnmi_mncause_after = ai_micro_cache_error_rnmi_last_mncause;
            rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
            rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
            rnmi_sceic_after = ai_micro_cache_error_rnmi_last_sceic;
            CSRW(CSR_MTVEC, original_mtvec);

            inject_consumed_each_round = inject_consumed_each_round &&
                ((sceic_after & 0x1ULL) == 0ULL);
            ai_micro_cache_error_note_scec_match(scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
            ai_micro_cache_error_note_scec_match(rnmi_scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
            nmi31_seen = nmi31_seen ||
                (rnmi_hits_after > 0ULL &&
                    ai_micro_cache_error_is_nmi31(rnmi_mncause_after));
        } else {
            hot_observed = ai_micro_cache_error_ld64(hot_paddr);
            follower_observed = ai_micro_cache_error_ld64(follower_paddr);
            scec_after = CSRR(CSR_SCEC);
            sceic_after = CSRR(CSR_SCEIC);
            mncause_after = CSRR(CSR_MNCAUSE);
            mnepc_after = CSRR(CSR_MNEPC);
            ai_micro_cache_error_note_scec_match(scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
        }

        dcache_match = dcache_match &&
            excpt.triggered == false &&
            hot_observed == hot_seed &&
            follower_observed == follower_seed;

        progress = ai_micro_cache_error_round_mix(progress, tlb_observed,
            hot_observed, follower_observed, round);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, hot_seed, follower_seed, round);

        last_scec = scec_after;
        last_sceic = sceic_after;
        last_mncause = mncause_after;
        last_mnepc = mnepc_after;
        last_rnmi_hits = rnmi_hits_after;
        last_rnmi_mncause = rnmi_mncause_after;
        last_rnmi_mnepc = rnmi_mnepc_after;
        last_rnmi_scec = rnmi_scec_after;
        last_rnmi_sceic = rnmi_sceic_after;
    }

    ai_micro_cache_error_quiesce_injection();

    TEST_ASSERT(
        "with repeated full sfence.vma before each HS probe, the interleaved virtual-address stream still returns every seeded payload so TLB activity stays live throughout the mixed cache-error window",
        tlb_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "after each one-shot DCache-tag CEIC programming, the immediately following hot physical load and the later follower physical load both retire with their seeded payloads instead of stalling or poisoning the visible load-return path",
        dcache_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "whenever the mixed tag-ECC case surfaces a valid SCEC record, the record stays pinned to the DCache-tag source and the injected hot-line physical address",
        !scec_dcache_tag_unexpected,
        "last_scec=0x%llx last_scec_epa=0x%llx last_rnmi_scec=0x%llx last_rnmi_scec_epa=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_rnmi_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_rnmi_scec)
    );

    if (tag_sink_supported) {
        TEST_ASSERT(
            "the CEIC enable bit is consumed and auto-cleared in every round, so the stress window really reaches the DCache tag injection sink instead of silently bypassing it",
            inject_consumed_each_round,
            "last_sceic=0x%llx last_scec=0x%llx",
            (unsigned long long)last_sceic,
            (unsigned long long)last_scec
        );

        TEST_ASSERT(
            "at least one round latches a DCache-tag cache-error record with the injected hot-line physical address in SCEC while the mixed TLB-plus-DCache stress loop still reaches completion",
            scec_dcache_tag_exact_seen,
            "last_scec=0x%llx last_scec_epa=0x%llx last_rnmi_scec=0x%llx last_rnmi_scec_epa=0x%llx",
            (unsigned long long)last_scec,
            (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
            (unsigned long long)last_rnmi_scec,
            (unsigned long long)ai_micro_cache_error_scec_epa(last_rnmi_scec)
        );

        TEST_ASSERT(
            "the local mtvec RNMI hook records injected DCache-tag activity as RNMI cause NMI_31 in MNCAUSE, matching the current XSCore path that ORs memBlock.io.error.valid into externalInterrupt.nmi.nmi_31",
            nmi31_seen,
            "baseline_mncause=0x%llx baseline_mnepc=0x%llx last_rnmi_hits=%llu last_rnmi_mncause=0x%llx last_rnmi_mnepc=0x%llx",
            (unsigned long long)baseline_mncause,
            (unsigned long long)baseline_mnepc,
            (unsigned long long)last_rnmi_hits,
            (unsigned long long)last_rnmi_mncause,
            (unsigned long long)last_rnmi_mnepc
        );
    } else {
        TEST_ASSERT(
            "when the build does not enable the DCache-tag ECC sink, the one-shot tag preflight should leave CEIC pending while the tagged hot line still returns its seeded payload, matching the current NanhuV5_3Config data-only capability",
            tag_sink_missing_expected,
            "tag_preflight_sceic=0x%llx tag_preflight_scec=0x%llx last_sceic=0x%llx",
            (unsigned long long)tag_preflight_sceic,
            (unsigned long long)tag_preflight_scec,
            (unsigned long long)last_sceic
        );
    }

    TEST_ASSERT(
        "the interleaved HS-TLB and M-mode DCache tag-ECC injection sequence reaches the deterministic final progress signature instead of hanging mid-window",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_mncause=0x%llx tag_supported=%d",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (int)tag_sink_supported
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, original_mtvec);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Legacy unsuffixed name keeps the historical 2-bit DCache-tag profile.
bool ai_micro_m_dcache_tag_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_tag_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
        __func__, true);
}

bool ai_micro_m_dcache_tag_ecc_1bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_tag_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
        __func__, false);
}

bool ai_micro_m_dcache_tag_ecc_2bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_tag_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner(
        __func__, true);
}

// This case always requires Xiangshan RTL with DCache data-ECC injection enabled.
// If the current build also enables DCache tag-ECC injection, the case validates
// full dense dual-sink observability. If the build is NanhuV5_3Config-style data-only
// (enableDataEcc = true, enableTagEcc = false), the case falls back to proving that
// data injection is still observable while tag CEIC predictably remains pending.
static bool
ai_micro_cache_error_run_dcache_data_tag_ecc_dense_dual_inject_interleaved_hs_tlb_progress_no_stuck_corner(
    const char *case_name, bool inject_two_bits)
{
    const char* __test_name = case_name;
    bool test_status = true;
    if (LOG_LEVEL >= LOG_INFO) {
        printf(CBLU "%-70s" CDFLT, __test_name);
    }
    if (LOG_LEVEL >= LOG_DETAIL) {
        printf("\n");
    }

    const uintptr_t hot_data_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t hot_tag_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uintptr_t verify_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_VERIFY_OFFSET;
    const uint64_t hot_data_seed = 0x13579bdf2468ace0ULL;
    const uint64_t hot_tag_seed = 0x0eca8642fdb97531ULL;
    const uint64_t verify_seed = 0x55aa33cc77ee11ffULL;
    const uint64_t data_sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, inject_two_bits);
    const uint64_t tag_sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DTAG_SRC, inject_two_bits);

    uint64_t original_mnstatus;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t progress = 0x3141592653589793ULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    uint64_t data_preflight_scec = 0ULL;
    uint64_t data_preflight_sceic = 0ULL;
    uint64_t tag_preflight_scec = 0ULL;
    uint64_t tag_preflight_sceic = 0ULL;
    struct ai_micro_cache_error_csr_observation obs;
    bool tlb_match = true;
    bool data_path_match = true;
    bool tag_path_match = true;
    bool data_inject_consumed_each_window = true;
    bool tag_inject_consumed_each_window = true;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;
    bool scec_dcache_tag_exact_seen = false;
    bool scec_dcache_tag_unexpected = false;
    bool data_sink_supported = false;
    bool tag_sink_supported = false;
    bool tag_sink_missing_expected = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);

    write64(hot_data_paddr, hot_data_seed);
    write64(hot_tag_paddr, hot_tag_seed);
    write64(verify_paddr, verify_seed);
    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t warm_hot_data = ai_micro_cache_error_ld64(hot_data_paddr);
    uint64_t warm_hot_tag = ai_micro_cache_error_ld64(hot_tag_paddr);
    uint64_t warm_verify = ai_micro_cache_error_ld64(verify_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the dense dual-injection window, both hot lines and the verifier line warm in DCache with their seeded payloads",
        warm_hot_data == hot_data_seed &&
            warm_hot_tag == hot_tag_seed &&
            warm_verify == verify_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t data_preflight_hot = ai_micro_cache_error_inject_then_load_hot(
        hot_data_paddr, data_sceic_value);
    obs = ai_micro_cache_error_wait_after_expected_consume();
    data_preflight_scec = obs.scec;
    data_preflight_sceic = obs.sceic;
    data_sink_supported =
        excpt.triggered == false &&
        data_preflight_hot == hot_data_seed &&
        ((data_preflight_sceic & 0x1ULL) == 0ULL);
    ai_micro_cache_error_note_scec_match(data_preflight_scec,
        AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_data_paddr,
        &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);

    TEST_SETUP_EXCEPT();
    uint64_t tag_preflight_hot = ai_micro_cache_error_inject_then_load_hot(
        hot_tag_paddr, tag_sceic_value);
    obs = ai_micro_cache_error_wait_after_expected_consume();
    tag_preflight_scec = obs.scec;
    tag_preflight_sceic = obs.sceic;
    tag_sink_supported =
        excpt.triggered == false &&
        tag_preflight_hot == hot_tag_seed &&
        ((tag_preflight_sceic & 0x1ULL) == 0ULL);
    tag_sink_missing_expected =
        excpt.triggered == false &&
        tag_preflight_hot == hot_tag_seed &&
        ((tag_preflight_sceic & 0x1ULL) != 0ULL) &&
        !ai_micro_cache_error_scec_valid(tag_preflight_scec);
    ai_micro_cache_error_note_scec_match(tag_preflight_scec,
        AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_tag_paddr,
        &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
    if (!tag_sink_supported) {
        CSRW(CSR_SCEC, 0ULL);
        CSRW(CSR_SCEIC, 0ULL);
    }

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_DENSE_ROUNDS; ++round) {
        const unsigned page_count = sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]);
        unsigned first_page_index = round % page_count;
        unsigned second_page_index = (round + 1U) % page_count;
        uintptr_t tlb_vaddr_first = hs_page_base(ai_micro_cache_error_tlb_pages[
            first_page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uintptr_t tlb_vaddr_second = hs_page_base(ai_micro_cache_error_tlb_pages[
            second_page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb_first = ai_micro_cache_error_seeded_tlb_value(
            first_page_index);
        uint64_t expected_tlb_second = ai_micro_cache_error_seeded_tlb_value(
            second_page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_first = ai_micro_cache_error_ld64(tlb_vaddr_first);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_first == expected_tlb_first;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        uint64_t data_hot_observed = ai_micro_cache_error_inject_then_load_hot(
            hot_data_paddr, data_sceic_value);
        uint64_t data_verify_observed = ai_micro_cache_error_ld64(verify_paddr);
        obs = ai_micro_cache_error_wait_after_expected_consume();
        uint64_t data_scec_after = obs.scec;
        uint64_t data_sceic_after = obs.sceic;
        uint64_t data_mncause_after = obs.mncause;
        uint64_t data_mnepc_after = obs.mnepc;

        data_path_match = data_path_match &&
            excpt.triggered == false &&
            data_hot_observed == hot_data_seed &&
            data_verify_observed == verify_seed;
        data_inject_consumed_each_window = data_inject_consumed_each_window &&
            ((data_sceic_after & 0x1ULL) == 0ULL);
        ai_micro_cache_error_note_scec_match(data_scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_data_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_second = ai_micro_cache_error_ld64(tlb_vaddr_second);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_second == expected_tlb_second;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        uint64_t tag_hot_observed;
        if (tag_sink_supported) {
            tag_hot_observed = ai_micro_cache_error_inject_then_load_hot(
                hot_tag_paddr, tag_sceic_value);
        } else {
            tag_hot_observed = ai_micro_cache_error_ld64(hot_tag_paddr);
        }
        uint64_t tag_verify_observed = ai_micro_cache_error_ld64(verify_paddr);
        uint64_t hot_data_reload = ai_micro_cache_error_ld64(hot_data_paddr);
        obs = ai_micro_cache_error_wait_after_expected_consume();
        uint64_t tag_scec_after = obs.scec;
        uint64_t tag_sceic_after = obs.sceic;
        uint64_t tag_mncause_after = obs.mncause;
        uint64_t tag_mnepc_after = obs.mnepc;

        tag_path_match = tag_path_match &&
            excpt.triggered == false &&
            tag_hot_observed == hot_tag_seed &&
            tag_verify_observed == verify_seed &&
            hot_data_reload == hot_data_seed;
        if (tag_sink_supported) {
            tag_inject_consumed_each_window = tag_inject_consumed_each_window &&
                ((tag_sceic_after & 0x1ULL) == 0ULL);
            ai_micro_cache_error_note_scec_match(tag_scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_tag_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
        }

        progress = ai_micro_cache_error_round_mix(progress, tlb_first,
            data_hot_observed, data_verify_observed, round * 2U);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb_first, hot_data_seed, verify_seed, round * 2U);
        progress = ai_micro_cache_error_round_mix(progress, tlb_second,
            tag_hot_observed, tag_verify_observed ^ hot_data_reload,
            round * 2U + 1U);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb_second, hot_tag_seed, verify_seed ^ hot_data_seed,
            round * 2U + 1U);

        last_scec = tag_scec_after;
        last_sceic = tag_sceic_after;
        last_mncause = tag_mncause_after ? tag_mncause_after : data_mncause_after;
        last_mnepc = tag_mnepc_after ? tag_mnepc_after : data_mnepc_after;
    }

    ai_micro_cache_error_quiesce_injection();

    TEST_ASSERT(
        "with two sfence.vma-forced HS probes in every outer round, the dense mixed stream keeps returning the seeded payloads so the page-walk side stays live while the DCache injection side is under pressure",
        tlb_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "in every data-ECC sub-window, the injected hot line and the separate verifier line both retire with their seeded payloads instead of hanging or corrupting the visible data path",
        data_path_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "in every tag-ECC sub-window, the injected hot-tag line, the verifier line, and the previously data-stressed line all retire with their seeded payloads instead of stalling or cross-poisoning adjacent hits",
        tag_path_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "the dense mixed window must consume at least the DCache-data CEIC preflight, which is the minimum sink capability required by the current Nanhu build",
        data_sink_supported,
        "data_preflight_sceic=0x%llx data_preflight_scec=0x%llx",
        (unsigned long long)data_preflight_sceic,
        (unsigned long long)data_preflight_scec
    );

    TEST_ASSERT(
        "whenever the dense mixed window surfaces a valid DCache-data SCEC record, the record stays pinned to the injected hot-data physical address",
        !scec_dcache_data_unexpected,
        "data_preflight_scec=0x%llx data_preflight_scec_epa=0x%llx last_scec=0x%llx last_scec_epa=0x%llx",
        (unsigned long long)data_preflight_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(data_preflight_scec),
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec)
    );

    TEST_ASSERT(
        "whenever the dense mixed window surfaces a valid DCache-tag SCEC record, the record stays pinned to the injected hot-tag physical address",
        !scec_dcache_tag_unexpected,
        "tag_preflight_scec=0x%llx tag_preflight_scec_epa=0x%llx last_scec=0x%llx last_scec_epa=0x%llx",
        (unsigned long long)tag_preflight_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(tag_preflight_scec),
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec)
    );

    if (tag_sink_supported) {
        TEST_ASSERT(
            "when the build really enables both DCache data and tag ECC sinks, every dense round consumes both CEIC windows and observes both exact source-plus-EPA SCEC records",
            data_inject_consumed_each_window &&
                tag_inject_consumed_each_window &&
                scec_dcache_data_exact_seen &&
                scec_dcache_tag_exact_seen &&
                !scec_dcache_tag_unexpected,
            "last_sceic=0x%llx last_scec=0x%llx last_scec_epa=0x%llx tag_preflight_sceic=0x%llx",
            (unsigned long long)last_sceic,
            (unsigned long long)last_scec,
            (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
            (unsigned long long)tag_preflight_sceic
        );
    } else {
        TEST_ASSERT(
            "when the build does not enable the DCache-tag ECC sink, the one-shot tag preflight should leave CEIC pending while the tagged hot line still returns its seeded payload, matching the current NanhuV5_3Config data-only capability",
            tag_sink_missing_expected,
            "tag_preflight_sceic=0x%llx tag_preflight_scec=0x%llx last_sceic=0x%llx",
            (unsigned long long)tag_preflight_sceic,
            (unsigned long long)tag_preflight_scec,
            (unsigned long long)last_sceic
        );
    }

    TEST_ASSERT(
        "the dense dual-injection plus HS page-walk sequence reaches the deterministic final progress signature instead of hanging partway through the mixed window",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_mncause=0x%llx tag_supported=%d",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (int)tag_sink_supported
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Legacy unsuffixed name keeps the historical 2-bit dense dual-injection profile.
bool ai_micro_m_dcache_data_tag_ecc_dense_dual_inject_interleaved_hs_tlb_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_data_tag_ecc_dense_dual_inject_interleaved_hs_tlb_progress_no_stuck_corner(
        __func__, true);
}

bool ai_micro_m_dcache_data_tag_ecc_dense_dual_1bit_inject_interleaved_hs_tlb_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_data_tag_ecc_dense_dual_inject_interleaved_hs_tlb_progress_no_stuck_corner(
        __func__, false);
}

bool ai_micro_m_dcache_data_tag_ecc_dense_dual_2bit_inject_interleaved_hs_tlb_progress_no_stuck_corner()
{
    return ai_micro_cache_error_run_dcache_data_tag_ecc_dense_dual_inject_interleaved_hs_tlb_progress_no_stuck_corner(
        __func__, true);
}

// Requires Xiangshan RTL with both DCacheParameters.enableDataEcc = true and
// DCacheParameters.enableTagEcc = true. This extends the dense dual-injection
// stress window with explicit RNMI observation for both the data-ECC and tag-ECC
// sub-windows.
bool ai_micro_m_dcache_data_tag_dense_dual_nmi31_interleaved_hs_tlb_progress_no_stuck_corner()
{
    TEST_START();

    const uintptr_t hot_data_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t hot_tag_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uintptr_t verify_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_VERIFY_OFFSET;
    const uint64_t hot_data_seed = 0x57e8a14c2396bdf0ULL;
    const uint64_t hot_tag_seed = 0x93cb61f0a47ed258ULL;
    const uint64_t verify_seed = 0x0badf00ddeadc0deULL;
    const uint64_t data_sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, true);
    const uint64_t tag_sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DTAG_SRC, true);

    uint64_t original_mnstatus;
    uint64_t original_mtvec;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t baseline_mncause;
    uint64_t baseline_mnepc;
    uint64_t progress = 0xc13fa9b5724de680ULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    uint64_t tag_preflight_scec = 0ULL;
    uint64_t tag_preflight_sceic = 0ULL;
    uint64_t last_data_rnmi_hits = 0ULL;
    uint64_t last_data_rnmi_mncause = 0ULL;
    uint64_t last_data_rnmi_mnepc = 0ULL;
    uint64_t last_tag_rnmi_hits = 0ULL;
    uint64_t last_tag_rnmi_mncause = 0ULL;
    uint64_t last_tag_rnmi_mnepc = 0ULL;
    struct ai_micro_cache_error_csr_observation obs;
    bool tlb_match = true;
    bool data_path_match = true;
    bool tag_path_match = true;
    bool data_inject_consumed_each_window = true;
    bool tag_inject_consumed_each_window = true;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;
    bool scec_dcache_tag_exact_seen = false;
    bool scec_dcache_tag_unexpected = false;
    bool tag_sink_supported = false;
    bool tag_sink_missing_expected = false;
    bool data_nmi31_seen = false;
    bool tag_nmi31_seen = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_mtvec = CSRR(CSR_MTVEC);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);
    baseline_mncause = CSRR(CSR_MNCAUSE);
    baseline_mnepc = CSRR(CSR_MNEPC);

    write64(hot_data_paddr, hot_data_seed);
    write64(hot_tag_paddr, hot_tag_seed);
    write64(verify_paddr, verify_seed);
    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    ai_micro_cache_error_reset_rnmi_state();
    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t warm_hot_data = ai_micro_cache_error_ld64(hot_data_paddr);
    uint64_t warm_hot_tag = ai_micro_cache_error_ld64(hot_tag_paddr);
    uint64_t warm_verify = ai_micro_cache_error_ld64(verify_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the dense dual-injection NMI window, both hot lines and the verifier line warm in DCache with their seeded payloads",
        warm_hot_data == hot_data_seed &&
            warm_hot_tag == hot_tag_seed &&
            warm_verify == verify_seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t tag_preflight_hot = ai_micro_cache_error_inject_then_load_hot(
        hot_tag_paddr, tag_sceic_value);
    obs = ai_micro_cache_error_wait_after_expected_consume();
    tag_preflight_scec = obs.scec;
    tag_preflight_sceic = obs.sceic;
    tag_sink_supported =
        excpt.triggered == false &&
        tag_preflight_hot == hot_tag_seed &&
        ((tag_preflight_sceic & 0x1ULL) == 0ULL);
    tag_sink_missing_expected =
        excpt.triggered == false &&
        tag_preflight_hot == hot_tag_seed &&
        ((tag_preflight_sceic & 0x1ULL) != 0ULL) &&
        !ai_micro_cache_error_scec_valid(tag_preflight_scec);
    ai_micro_cache_error_note_scec_match(tag_preflight_scec,
        AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_tag_paddr,
        &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
    if (!tag_sink_supported) {
        CSRW(CSR_SCEC, 0ULL);
        CSRW(CSR_SCEIC, 0ULL);
    }

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_DENSE_ROUNDS; ++round) {
        const unsigned page_count = sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]);
        unsigned first_page_index = round % page_count;
        unsigned second_page_index = (round + 1U) % page_count;
        uintptr_t tlb_vaddr_first = hs_page_base(ai_micro_cache_error_tlb_pages[
            first_page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uintptr_t tlb_vaddr_second = hs_page_base(ai_micro_cache_error_tlb_pages[
            second_page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb_first = ai_micro_cache_error_seeded_tlb_value(
            first_page_index);
        uint64_t expected_tlb_second = ai_micro_cache_error_seeded_tlb_value(
            second_page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_first = ai_micro_cache_error_ld64(tlb_vaddr_first);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_first == expected_tlb_first;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
        struct ai_micro_cache_error_load_site_result data_hot_load =
            ai_micro_cache_error_inject_then_load_hot_with_site(
                hot_data_paddr, data_sceic_value);
        uint64_t data_hot_observed = data_hot_load.value;
        ai_micro_cache_error_csr_to_load_delay();
        uint64_t data_verify_observed = ai_micro_cache_error_ld64(verify_paddr);
        uint64_t data_scec_after = CSRR(CSR_SCEC);
        uint64_t data_sceic_after = CSRR(CSR_SCEIC);
        uint64_t data_mncause_after = CSRR(CSR_MNCAUSE);
        uint64_t data_mnepc_after = CSRR(CSR_MNEPC);
        uint64_t data_rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
        uint64_t data_rnmi_mncause_after =
            ai_micro_cache_error_rnmi_last_mncause;
        uint64_t data_rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
        uint64_t data_rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
        CSRW(CSR_MTVEC, original_mtvec);

        data_path_match = data_path_match &&
            excpt.triggered == false &&
            data_hot_observed == hot_data_seed &&
            data_verify_observed == verify_seed;
        data_inject_consumed_each_window = data_inject_consumed_each_window &&
            ((data_sceic_after & 0x1ULL) == 0ULL);
        ai_micro_cache_error_note_scec_match(data_scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_data_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
        ai_micro_cache_error_note_scec_match(data_rnmi_scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_data_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
        data_nmi31_seen = data_nmi31_seen ||
            (data_rnmi_hits_after > 0ULL &&
                ai_micro_cache_error_is_nmi31(data_rnmi_mncause_after));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t tlb_second = ai_micro_cache_error_ld64(tlb_vaddr_second);
        tlb_match = tlb_match &&
            excpt.triggered == false &&
            tlb_second == expected_tlb_second;

        goto_priv(PRIV_M);
        TEST_SETUP_EXCEPT();
        struct ai_micro_cache_error_load_site_result tag_hot_load = {0};
        uint64_t tag_hot_observed;
        uint64_t tag_verify_observed;
        uint64_t hot_data_reload;
        uint64_t tag_scec_after;
        uint64_t tag_sceic_after;
        uint64_t tag_mncause_after;
        uint64_t tag_mnepc_after;
        uint64_t tag_rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
        uint64_t tag_rnmi_mncause_after =
            ai_micro_cache_error_rnmi_last_mncause;
        uint64_t tag_rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
        uint64_t tag_rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;

        if (tag_sink_supported) {
            CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
            tag_hot_load = ai_micro_cache_error_inject_then_load_hot_with_site(
                hot_tag_paddr, tag_sceic_value);
            tag_hot_observed = tag_hot_load.value;
            ai_micro_cache_error_csr_to_load_delay();
            tag_verify_observed = ai_micro_cache_error_ld64(verify_paddr);
            hot_data_reload = ai_micro_cache_error_ld64(hot_data_paddr);
            tag_scec_after = CSRR(CSR_SCEC);
            tag_sceic_after = CSRR(CSR_SCEIC);
            tag_mncause_after = CSRR(CSR_MNCAUSE);
            tag_mnepc_after = CSRR(CSR_MNEPC);
            tag_rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
            tag_rnmi_mncause_after =
                ai_micro_cache_error_rnmi_last_mncause;
            tag_rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
            tag_rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
            CSRW(CSR_MTVEC, original_mtvec);
        } else {
            tag_hot_observed = ai_micro_cache_error_ld64(hot_tag_paddr);
            tag_verify_observed = ai_micro_cache_error_ld64(verify_paddr);
            hot_data_reload = ai_micro_cache_error_ld64(hot_data_paddr);
            tag_scec_after = CSRR(CSR_SCEC);
            tag_sceic_after = CSRR(CSR_SCEIC);
            tag_mncause_after = CSRR(CSR_MNCAUSE);
            tag_mnepc_after = CSRR(CSR_MNEPC);
            ai_micro_cache_error_note_scec_match(tag_scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_tag_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
        }

        tag_path_match = tag_path_match &&
            excpt.triggered == false &&
            tag_hot_observed == hot_tag_seed &&
            tag_verify_observed == verify_seed &&
            hot_data_reload == hot_data_seed;
        if (tag_sink_supported) {
            tag_inject_consumed_each_window = tag_inject_consumed_each_window &&
                ((tag_sceic_after & 0x1ULL) == 0ULL);
            ai_micro_cache_error_note_scec_match(tag_scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_tag_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
            ai_micro_cache_error_note_scec_match(tag_rnmi_scec_after,
                AI_MICRO_CACHE_ERROR_DTAG_SRC, hot_tag_paddr,
                &scec_dcache_tag_exact_seen, &scec_dcache_tag_unexpected);
            tag_nmi31_seen = tag_nmi31_seen ||
                (tag_rnmi_hits_after > 0ULL &&
                    ai_micro_cache_error_is_nmi31(tag_rnmi_mncause_after));
        }

        progress = ai_micro_cache_error_round_mix(progress, tlb_first,
            data_hot_observed, data_verify_observed, round * 2U);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb_first, hot_data_seed, verify_seed, round * 2U);
        progress = ai_micro_cache_error_round_mix(progress, tlb_second,
            tag_hot_observed, tag_verify_observed ^ hot_data_reload,
            round * 2U + 1U);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb_second, hot_tag_seed, verify_seed ^ hot_data_seed,
            round * 2U + 1U);

        last_scec = tag_scec_after;
        last_sceic = tag_sceic_after;
        last_mncause = tag_mncause_after ? tag_mncause_after : data_mncause_after;
        last_mnepc = tag_mnepc_after ? tag_mnepc_after : data_mnepc_after;
        last_data_rnmi_hits = data_rnmi_hits_after;
        last_data_rnmi_mncause = data_rnmi_mncause_after;
        last_data_rnmi_mnepc = data_rnmi_mnepc_after;
        last_tag_rnmi_hits = tag_rnmi_hits_after;
        last_tag_rnmi_mncause = tag_rnmi_mncause_after;
        last_tag_rnmi_mnepc = tag_rnmi_mnepc_after;
    }

    ai_micro_cache_error_quiesce_injection();

    TEST_ASSERT(
        "with two sfence.vma-forced HS probes in every outer round, the dense mixed stream keeps returning the seeded payloads so the page-walk side stays live while the dual DCache NMI window is under pressure",
        tlb_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "in every data-ECC sub-window, the injected hot line and the separate verifier line both retire with their seeded payloads instead of hanging or corrupting the visible data path",
        data_path_match,
        "last_scec=0x%llx last_data_rnmi_mncause=0x%llx last_data_rnmi_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_data_rnmi_mncause,
        (unsigned long long)last_data_rnmi_mnepc
    );

    TEST_ASSERT(
        "in every tag-ECC sub-window, the injected hot-tag line, the verifier line, and the previously data-stressed line all retire with their seeded payloads instead of stalling or cross-poisoning adjacent hits",
        tag_path_match,
        "last_scec=0x%llx last_tag_rnmi_mncause=0x%llx last_tag_rnmi_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_tag_rnmi_mncause,
        (unsigned long long)last_tag_rnmi_mnepc
    );

    TEST_ASSERT(
        "every data-ECC window consumes and auto-clears its CEIC request, so the dense NMI case really reaches the DCache data sink instead of silently bypassing it",
        data_inject_consumed_each_window,
        "last_sceic=0x%llx last_scec=0x%llx",
        (unsigned long long)last_sceic,
        (unsigned long long)last_scec
    );

    TEST_ASSERT(
        "across the dense dual-injection run, at least one data-ECC sub-window records an exact DCache-data source-plus-EPA SCEC entry while the mixed stream still reaches completion",
        scec_dcache_data_exact_seen,
        "last_scec=0x%llx last_scec_epa=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "whenever the dense dual-injection run surfaces a valid DCache-data SCEC record, the record stays pinned to the injected hot-data physical address",
        !scec_dcache_data_unexpected,
        "last_scec=0x%llx last_scec_epa=0x%llx last_data_rnmi_mncause=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_data_rnmi_mncause
    );

    TEST_ASSERT(
        "whenever the dense dual-injection run surfaces a valid DCache-tag SCEC record, the record stays pinned to the injected hot-tag physical address",
        !scec_dcache_tag_unexpected,
        "tag_preflight_scec=0x%llx tag_preflight_scec_epa=0x%llx last_scec=0x%llx last_scec_epa=0x%llx",
        (unsigned long long)tag_preflight_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(tag_preflight_scec),
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec)
    );

    TEST_ASSERT(
        "at least one data-ECC sub-window is reflected as RNMI cause NMI_31, matching the current XSCore path that ORs memBlock.io.error.valid into externalInterrupt.nmi.nmi_31",
        data_nmi31_seen,
        "last_data_hits=%llu last_data_mncause=0x%llx last_data_mnepc=0x%llx",
        (unsigned long long)last_data_rnmi_hits,
        (unsigned long long)last_data_rnmi_mncause,
        (unsigned long long)last_data_rnmi_mnepc
    );

    if (tag_sink_supported) {
        TEST_ASSERT(
            "every tag-ECC window consumes and auto-clears its CEIC request when the build really enables the DCache tag sink",
            tag_inject_consumed_each_window,
            "last_sceic=0x%llx tag_preflight_sceic=0x%llx last_scec=0x%llx",
            (unsigned long long)last_sceic,
            (unsigned long long)tag_preflight_sceic,
            (unsigned long long)last_scec
        );

        TEST_ASSERT(
            "when the build really enables the DCache tag sink, at least one tag-ECC sub-window records an exact DCache-tag source-plus-EPA SCEC entry",
            scec_dcache_tag_exact_seen && !scec_dcache_tag_unexpected,
            "last_scec=0x%llx last_scec_epa=0x%llx tag_preflight_scec=0x%llx tag_preflight_scec_epa=0x%llx",
            (unsigned long long)last_scec,
            (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
            (unsigned long long)tag_preflight_scec,
            (unsigned long long)ai_micro_cache_error_scec_epa(tag_preflight_scec)
        );

        TEST_ASSERT(
            "at least one tag-ECC sub-window is reflected as RNMI cause NMI_31 when the build really enables the DCache tag sink",
            tag_nmi31_seen,
            "last_tag_hits=%llu last_tag_mncause=0x%llx last_tag_mnepc=0x%llx",
            (unsigned long long)last_tag_rnmi_hits,
            (unsigned long long)last_tag_rnmi_mncause,
            (unsigned long long)last_tag_rnmi_mnepc
        );
    } else {
        TEST_ASSERT(
            "when the build does not enable the DCache-tag ECC sink, the one-shot tag preflight should leave CEIC pending while the tagged hot line still returns its seeded payload, matching the current NanhuV5_3Config data-only capability",
            tag_sink_missing_expected,
            "tag_preflight_sceic=0x%llx tag_preflight_scec=0x%llx last_sceic=0x%llx",
            (unsigned long long)tag_preflight_sceic,
            (unsigned long long)tag_preflight_scec,
            (unsigned long long)last_sceic
        );
    }

    TEST_ASSERT(
        "the dense dual-injection plus HS page-walk NMI sequence reaches the deterministic final progress signature instead of hanging partway through the mixed window",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_mncause=0x%llx tag_supported=%d",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (int)tag_sink_supported
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, original_mtvec);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Requires Xiangshan RTL with DCacheParameters.enableDataEcc = true.
// This isolates repeated one-shot data-ECC rearm on the same hot line, and
// checks that RNMI_31 can be observed in multiple rounds without relying on
// the HS page-walk side to keep progress moving.
bool ai_micro_m_dcache_data_ecc_nmi31_rearm_same_line_multi_hit_progress_corner()
{
    TEST_START();

    const uintptr_t hot_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t follower_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uintptr_t verify_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_VERIFY_OFFSET;
    const uint64_t hot_seed = 0x61d4c3b2a5908f7eULL;
    const uint64_t follower_seed = 0x7e8f90a5b2c3d461ULL;
    const uint64_t verify_seed = 0x1122334455667788ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, true);

    uint64_t original_mnstatus;
    uint64_t original_mtvec;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t baseline_mncause;
    uint64_t baseline_mnepc;
    uint64_t progress = 0x4b3a291807f6e5d4ULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    uint64_t last_rnmi_hits = 0ULL;
    uint64_t last_rnmi_mncause = 0ULL;
    uint64_t last_rnmi_mnepc = 0ULL;
    unsigned nmi31_rounds = 0U;
    bool dcache_match = true;
    bool inject_consumed_each_round = true;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;
    bool nmi31_seen = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_mtvec = CSRR(CSR_MTVEC);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);
    baseline_mncause = CSRR(CSR_MNCAUSE);
    baseline_mnepc = CSRR(CSR_MNEPC);

    write64(hot_paddr, hot_seed);
    write64(follower_paddr, follower_seed);
    write64(verify_paddr, verify_seed);

    ai_micro_cache_error_reset_rnmi_state();
    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t warm_hot = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t warm_follower = ai_micro_cache_error_ld64(follower_paddr);
    uint64_t warm_verify = ai_micro_cache_error_ld64(verify_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the same-line multi-hit data-ECC window, the hot line, follower line, and verifier line all warm in DCache with their seeded payloads",
        warm_hot == hot_seed &&
            warm_follower == follower_seed &&
            warm_verify == verify_seed
    );

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_ROUNDS; ++round) {
        TEST_SETUP_EXCEPT();
        CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
        struct ai_micro_cache_error_load_site_result hot_load =
            ai_micro_cache_error_inject_then_load_hot_with_site(
                hot_paddr, sceic_value);
        uint64_t hot_observed = hot_load.value;
        ai_micro_cache_error_csr_to_load_delay();
        uint64_t follower_observed = ai_micro_cache_error_ld64(follower_paddr);
        uint64_t verify_observed = ai_micro_cache_error_ld64(verify_paddr);
        uint64_t scec_after = CSRR(CSR_SCEC);
        uint64_t sceic_after = CSRR(CSR_SCEIC);
        uint64_t mncause_after = CSRR(CSR_MNCAUSE);
        uint64_t mnepc_after = CSRR(CSR_MNEPC);
        uint64_t rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
        uint64_t rnmi_mncause_after =
            ai_micro_cache_error_rnmi_last_mncause;
        uint64_t rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
        uint64_t rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
        CSRW(CSR_MTVEC, original_mtvec);

        dcache_match = dcache_match &&
            excpt.triggered == false &&
            hot_observed == hot_seed &&
            follower_observed == follower_seed &&
            verify_observed == verify_seed;
        inject_consumed_each_round = inject_consumed_each_round &&
            ((sceic_after & 0x1ULL) == 0ULL);
        ai_micro_cache_error_note_scec_match(scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
        ai_micro_cache_error_note_scec_match(rnmi_scec_after,
            AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
            &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
        if (rnmi_hits_after > 0ULL &&
            ai_micro_cache_error_is_nmi31(rnmi_mncause_after)) {
            nmi31_seen = true;
            nmi31_rounds = (unsigned)rnmi_hits_after;
        }
        progress = ai_micro_cache_error_round_mix(progress, hot_observed,
            follower_observed, verify_observed, round);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            hot_seed, follower_seed, verify_seed, round);

        last_scec = scec_after;
        last_sceic = sceic_after;
        last_mncause = mncause_after;
        last_mnepc = mnepc_after;
        last_rnmi_hits = rnmi_hits_after;
        last_rnmi_mncause = rnmi_mncause_after;
        last_rnmi_mnepc = rnmi_mnepc_after;
    }

    TEST_ASSERT(
        "throughout the same-line multi-hit data-ECC run, the hot line, follower line, and verifier line all retire with their seeded payloads instead of hanging or poisoning adjacent reads",
        dcache_match,
        "last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "the CEIC enable bit is consumed and auto-cleared in every rearm round, so the same-line stress window really reaches the DCache data sink every time",
        inject_consumed_each_round,
        "last_sceic=0x%llx last_scec=0x%llx",
        (unsigned long long)last_sceic,
        (unsigned long long)last_scec
    );

    TEST_ASSERT(
        "at least one round latches a DCache-data cache-error record with the injected hot-line physical address in SCEC during the same-line multi-hit run",
        scec_dcache_data_exact_seen,
        "last_scec=0x%llx last_scec_epa=0x%llx last_rnmi_hits=%llu",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_rnmi_hits
    );

    TEST_ASSERT(
        "whenever the same-line rearm run surfaces a valid SCEC record, the record stays pinned to the DCache-data source and the injected hot-line physical address",
        !scec_dcache_data_unexpected,
        "last_scec=0x%llx last_scec_epa=0x%llx last_rnmi_mncause=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (unsigned long long)last_rnmi_mncause
    );

    TEST_ASSERT(
        "the local mtvec RNMI hook records DCache-data RNMI cause NMI_31 during the same-line rearm run",
        nmi31_seen,
        "baseline_mncause=0x%llx baseline_mnepc=0x%llx last_rnmi_hits=%llu last_rnmi_mncause=0x%llx last_rnmi_mnepc=0x%llx",
        (unsigned long long)baseline_mncause,
        (unsigned long long)baseline_mnepc,
        (unsigned long long)last_rnmi_hits,
        (unsigned long long)last_rnmi_mncause,
        (unsigned long long)last_rnmi_mnepc
    );

    TEST_ASSERT(
        "the same-line rearm run records more than one RNMI hook hit while the last recorded cause remains NMI_31, so this is not just a one-shot first-hit coincidence",
        nmi31_rounds >= 2U,
        "nmi31_rounds=%u last_rnmi_hits=%llu last_rnmi_mncause=0x%llx",
        nmi31_rounds,
        (unsigned long long)last_rnmi_hits,
        (unsigned long long)last_rnmi_mncause
    );

    TEST_ASSERT(
        "the same-line multi-hit data-ECC sequence reaches the deterministic final progress signature instead of hanging partway through the repeated rearm window",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_mncause=0x%llx",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, original_mtvec);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

bool ai_micro_m_cbo_flush_dirty_line_dcache_data_ecc_nmi31_converges_corner()
{
    TEST_START();

    const uintptr_t hot_paddr = phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_HOT_OFFSET;
    const uintptr_t follower_paddr =
        phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_FOLLOWER_OFFSET;
    const uintptr_t verify_paddr =
        phys_page_base(AI_MICRO_CACHE_ERROR_HOT_PAGE) +
        AI_MICRO_CACHE_ERROR_VERIFY_OFFSET;
    const uint64_t clean_seed = 0x1122334455667788ULL;
    const uint64_t dirty_seed = 0xa55a6cc6d33d5aa5ULL;
    const uint64_t follower_seed = 0x7e8f90a5b2c3d461ULL;
    const uint64_t verify_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_DDATA_SRC, true);

    uint64_t original_mnstatus;
    uint64_t original_mtvec;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t baseline_mncause;
    uint64_t baseline_mnepc;
    bool scec_dcache_data_exact_seen = false;
    bool scec_dcache_data_unexpected = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_mtvec = CSRR(CSR_MTVEC);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);
    baseline_mncause = CSRR(CSR_MNCAUSE);
    baseline_mnepc = CSRR(CSR_MNEPC);

    write64(hot_paddr, clean_seed);
    write64(follower_paddr, follower_seed);
    write64(verify_paddr, verify_seed);

    ai_micro_cache_error_reset_rnmi_state();
    ai_micro_cache_error_enable_default_rnmi_return();
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    TEST_SETUP_EXCEPT();
    uint64_t warm_hot = ai_micro_cache_error_ld64(hot_paddr);
    uint64_t warm_follower = ai_micro_cache_error_ld64(follower_paddr);
    uint64_t warm_verify = ai_micro_cache_error_ld64(verify_paddr);
    AI_ASSERT_NO_EXCEPT_AND(
        "before the CBO.FLUSH dirty-line ECC window, the hot line, follower line, and verifier line all warm in DCache with their seeded payloads",
        warm_hot == clean_seed &&
            warm_follower == follower_seed &&
            warm_verify == verify_seed
    );

    write64(hot_paddr, dirty_seed);
    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MTVEC, (uintptr_t)ai_micro_cache_error_rnmi_entry);
    struct ai_micro_cache_error_load_site_result flush_site =
        ai_micro_cache_error_inject_then_cbo_flush_hot_with_site(
            hot_paddr, sceic_value);
    ai_micro_cache_error_csr_to_load_delay();
    uint64_t follower_after = ai_micro_cache_error_ld64(follower_paddr);
    uint64_t verify_after = ai_micro_cache_error_ld64(verify_paddr);
    uint64_t scec_after = CSRR(CSR_SCEC);
    uint64_t sceic_after = CSRR(CSR_SCEIC);
    uint64_t rnmi_hits_after = ai_micro_cache_error_rnmi_hits;
    uint64_t rnmi_mncause_after = ai_micro_cache_error_rnmi_last_mncause;
    uint64_t rnmi_mnepc_after = ai_micro_cache_error_rnmi_last_mnepc;
    uint64_t rnmi_scec_after = ai_micro_cache_error_rnmi_last_scec;
    CSRW(CSR_MTVEC, original_mtvec);

    ai_micro_cache_error_note_scec_match(scec_after,
        AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
        &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);
    ai_micro_cache_error_note_scec_match(rnmi_scec_after,
        AI_MICRO_CACHE_ERROR_DDATA_SRC, hot_paddr,
        &scec_dcache_data_exact_seen, &scec_dcache_data_unexpected);

    TEST_ASSERT(
        "the dirty-line CBO.FLUSH plus DCache-data ECC injection window retires without a regular synchronous exception and reloads the dirty payload instead of losing the line",
        excpt.triggered == false &&
            flush_site.value == dirty_seed,
        "flush_pc=0x%llx after_flush_pc=0x%llx scec=0x%llx sceic=0x%llx",
        (unsigned long long)flush_site.load_pc,
        (unsigned long long)flush_site.after_load_pc,
        (unsigned long long)scec_after,
        (unsigned long long)sceic_after
    );

    TEST_ASSERT(
        "the same dirty-line CBO.FLUSH ECC window preserves nearby DCache lines and continues forward progress after the NMI observation point",
        follower_after == follower_seed &&
            verify_after == verify_seed,
        "follower_after=0x%llx verify_after=0x%llx",
        (unsigned long long)follower_after,
        (unsigned long long)verify_after
    );

    TEST_ASSERT(
        "the CBO.FLUSH dirty-line probe consumes the one-shot CEIC request, proving the flush path reached the DCache data ECC sink",
        (sceic_after & 0x1ULL) == 0ULL,
        "sceic_after=0x%llx scec_after=0x%llx rnmi_hits=%llu",
        (unsigned long long)sceic_after,
        (unsigned long long)scec_after,
        (unsigned long long)rnmi_hits_after
    );

    TEST_ASSERT(
        "the CBO.FLUSH dirty-line probe records a DCache-data SCEC entry pinned to the flushed dirty cache block",
        scec_dcache_data_exact_seen && !scec_dcache_data_unexpected,
        "scec_after=0x%llx rnmi_scec=0x%llx expected_epa=0x%llx",
        (unsigned long long)scec_after,
        (unsigned long long)rnmi_scec_after,
        (unsigned long long)hot_paddr
    );

    TEST_ASSERT(
        "the RNMI hook observes the dirty-line CBO.FLUSH DCache-data ECC event as NMI_31",
        rnmi_hits_after > 0ULL &&
            ai_micro_cache_error_is_nmi31(rnmi_mncause_after),
        "baseline_mncause=0x%llx baseline_mnepc=0x%llx rnmi_hits=%llu rnmi_mncause=0x%llx rnmi_mnepc=0x%llx",
        (unsigned long long)baseline_mncause,
        (unsigned long long)baseline_mnepc,
        (unsigned long long)rnmi_hits_after,
        (unsigned long long)rnmi_mncause_after,
        (unsigned long long)rnmi_mnepc_after
    );

    goto_priv(PRIV_M);
    CSRW(CSR_MTVEC, original_mtvec);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    ai_micro_cache_error_restore_default_rnmi_return(original_mnstatus);

    TEST_END();
}

// Debug-only reproducer. This depends on L2TLB/PTW ECC being enabled, which is true in the
// current MMUConst default, but current RTL may terminate simulation on the intended hit with
// PageTableCache XSError("Should not happen at sim stage") before TEST_END is reached.
// The design document models L2TLB l1 as parity-protected, so only 1-bit injection is a
// meaningful observable target; 2-bit parity injection is architecturally undetectable.
bool ai_micro_m_l2tlb_l1_ecc_inject_repeated_hs_page_walk_debug_manual_corner()
{
    TEST_START();

    const uint64_t l2tlb_sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_L2TLB_L1_SRC, false);

    uint64_t original_mnstatus;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t progress = 0x9a78563412efcdabULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    bool warmup_match = true;
    bool walk_match = true;
    bool inject_consumed = false;
    bool scec_l2tlb_exact_seen = false;
    bool scec_l2tlb_unexpected = false;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);

    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned warm = 0; warm < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++warm) {
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            warm]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(warm);

        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t observed = ai_micro_cache_error_ld64(tlb_vaddr);
        warmup_match = warmup_match &&
            excpt.triggered == false &&
            observed == expected_tlb;
        progress = ai_micro_cache_error_round_mix(progress, observed,
            expected_tlb, 0x1111000000000000ULL | (uint64_t)warm, warm);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, expected_tlb, 0x1111000000000000ULL | (uint64_t)warm,
            warm);
    }

    goto_priv(PRIV_M);
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, l2tlb_sceic_value);
    ai_micro_cache_error_csr_to_load_delay();

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_L2TLB_DEBUG_ROUNDS; ++round) {
        const unsigned page_count = sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]);
        unsigned page_index = round % page_count;
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(
            page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t observed = ai_micro_cache_error_ld64(tlb_vaddr);
        walk_match = walk_match &&
            excpt.triggered == false &&
            observed == expected_tlb;

        goto_priv(PRIV_M);
        uint64_t scec_after = CSRR(CSR_SCEC);
        uint64_t sceic_after = CSRR(CSR_SCEIC);
        uint64_t mncause_after = CSRR(CSR_MNCAUSE);
        uint64_t mnepc_after = CSRR(CSR_MNEPC);

        inject_consumed = inject_consumed || ((sceic_after & 0x1ULL) == 0ULL);
        ai_micro_cache_error_note_scec_match(scec_after,
            AI_MICRO_CACHE_ERROR_L2TLB_L1_SRC, 0ULL,
            &scec_l2tlb_exact_seen, &scec_l2tlb_unexpected);

        progress = ai_micro_cache_error_round_mix(progress, observed,
            expected_tlb, sceic_after ^ scec_after, round + 0x40U);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, expected_tlb, sceic_after ^ scec_after, round + 0x40U);

        last_scec = scec_after;
        last_sceic = sceic_after;
        last_mncause = mncause_after;
        last_mnepc = mnepc_after;
    }

    TEST_ASSERT(
        "before arming L2TLB-L1 injection, the repeated HS sfence.vma plus reload warmup populates the page-walk path and still returns every seeded payload",
        warmup_match,
        "last_scec=0x%llx last_sceic=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_sceic
    );

    TEST_ASSERT(
        "after arming the pending L2TLB-L1 injection, the repeated HS sfence.vma plus reload debug pattern still makes forward progress as long as the sim does not terminate early on the known PageTableCache XSError path",
        walk_match,
        "last_scec=0x%llx last_sceic=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_sceic,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "the debug pattern should eventually consume the pending L2TLB-L1 CEIC request; on current RTL, a true hit may instead stop simulation first with the expected PageTableCache sim-stage XSError",
        inject_consumed,
        "last_sceic=0x%llx last_scec=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_sceic,
        (unsigned long long)last_scec,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "if the repeated page-walk debug run surfaces a valid L2TLB SCEC record before TEST_END, the record must stay pinned to the L2TLB source and the architecturally defined EPA=0 image",
        !scec_l2tlb_unexpected,
        "last_scec=0x%llx last_scec_epa=0x%llx exact_seen=%d",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec),
        (int)scec_l2tlb_exact_seen
    );

    TEST_ASSERT(
        "if the run reaches TEST_END instead of stopping earlier in sim, the repeated page-walk debug sequence still reaches the deterministic final progress signature",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx l2tlb_exact_seen=%d",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (int)scec_l2tlb_exact_seen
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    CSRW(CSR_MNSTATUS, original_mnstatus);

    TEST_END();
}

// AP-107D manual smoke. This does not prove the internal one-cycle L2TLB_L1
// request/response poison alignment, but it does prove a non-L2TLB source is
// not consumed by repeated PageTableCache/L2TLB page-walk activity.
bool ai_micro_m_l2tlb_l1_ecc_non_l2tlb_source_hs_page_walk_not_consumed_manual_corner()
{
    TEST_START();

    const uint64_t non_l2tlb_sceic_value = ai_micro_cache_error_make_sceic(
        AI_MICRO_CACHE_ERROR_NONE_SRC, false);

    uint64_t original_mnstatus;
    uint64_t original_scec;
    uint64_t original_sceic;
    uint64_t progress = 0x5e1cec107d13579bULL;
    uint64_t expected_progress = progress;
    uint64_t last_scec = 0ULL;
    uint64_t last_sceic = 0ULL;
    uint64_t last_mncause = 0ULL;
    uint64_t last_mnepc = 0ULL;
    bool warmup_match = true;
    bool walk_match = true;
    bool non_l2tlb_source_stays_pending = true;
    bool no_scec_error_seen = true;

    goto_priv(PRIV_M);
    original_mnstatus = CSRR(CSR_MNSTATUS);
    original_scec = CSRR(CSR_SCEC);
    original_sceic = CSRR(CSR_SCEIC);

    for (unsigned i = 0; i < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++i) {
        uintptr_t tlb_paddr = phys_page_base(ai_micro_cache_error_tlb_pages[i]) +
            AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        write64(tlb_paddr, ai_micro_cache_error_seeded_tlb_value(i));
    }

    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned warm = 0; warm < sizeof(ai_micro_cache_error_tlb_pages) /
        sizeof(ai_micro_cache_error_tlb_pages[0]); ++warm) {
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            warm]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(warm);

        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t observed = ai_micro_cache_error_ld64(tlb_vaddr);
        warmup_match = warmup_match &&
            excpt.triggered == false &&
            observed == expected_tlb;
        progress = ai_micro_cache_error_round_mix(progress, observed,
            expected_tlb, 0x2222000000000000ULL | (uint64_t)warm, warm);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, expected_tlb, 0x2222000000000000ULL | (uint64_t)warm,
            warm);
    }

    goto_priv(PRIV_M);
    CSRW(CSR_SCEC, 0ULL);
    CSRW(CSR_SCEIC, non_l2tlb_sceic_value);
    ai_micro_cache_error_csr_to_load_delay();

    for (unsigned round = 0; round < AI_MICRO_CACHE_ERROR_L2TLB_DEBUG_ROUNDS; ++round) {
        const unsigned page_count = sizeof(ai_micro_cache_error_tlb_pages) /
            sizeof(ai_micro_cache_error_tlb_pages[0]);
        unsigned page_index = round % page_count;
        uintptr_t tlb_vaddr = hs_page_base(ai_micro_cache_error_tlb_pages[
            page_index]) + AI_MICRO_CACHE_ERROR_TLB_OFFSET;
        uint64_t expected_tlb = ai_micro_cache_error_seeded_tlb_value(
            page_index);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sfence_vma();
        uint64_t observed = ai_micro_cache_error_ld64(tlb_vaddr);
        walk_match = walk_match &&
            excpt.triggered == false &&
            observed == expected_tlb;

        goto_priv(PRIV_M);
        uint64_t scec_after = CSRR(CSR_SCEC);
        uint64_t sceic_after = CSRR(CSR_SCEIC);
        uint64_t mncause_after = CSRR(CSR_MNCAUSE);
        uint64_t mnepc_after = CSRR(CSR_MNEPC);

        non_l2tlb_source_stays_pending = non_l2tlb_source_stays_pending &&
            ((sceic_after & 0x1fULL) == non_l2tlb_sceic_value);
        no_scec_error_seen = no_scec_error_seen &&
            !ai_micro_cache_error_scec_valid(scec_after);

        progress = ai_micro_cache_error_round_mix(progress, observed,
            expected_tlb, sceic_after ^ scec_after, round + 0x80U);
        expected_progress = ai_micro_cache_error_round_mix(expected_progress,
            expected_tlb, expected_tlb, sceic_after ^ scec_after, round + 0x80U);

        last_scec = scec_after;
        last_sceic = sceic_after;
        last_mncause = mncause_after;
        last_mnepc = mnepc_after;
    }

    TEST_ASSERT(
        "before arming the non-L2TLB CEIC source, repeated HS sfence.vma plus reload warmup populates the page-walk path and still returns every seeded payload",
        warmup_match,
        "last_scec=0x%llx last_sceic=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_sceic
    );

    TEST_ASSERT(
        "with a non-L2TLB CEIC source armed, repeated HS sfence.vma plus reload page walks still return every seeded payload",
        walk_match,
        "last_scec=0x%llx last_sceic=0x%llx last_mncause=0x%llx last_mnepc=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)last_sceic,
        (unsigned long long)last_mncause,
        (unsigned long long)last_mnepc
    );

    TEST_ASSERT(
        "the non-L2TLB CEIC source remains pending across repeated PageTableCache/L2TLB activity, so the L2TLB_L1 source filter does not falsely consume it",
        non_l2tlb_source_stays_pending,
        "last_sceic=0x%llx expected_sceic=0x%llx last_scec=0x%llx",
        (unsigned long long)last_sceic,
        (unsigned long long)non_l2tlb_sceic_value,
        (unsigned long long)last_scec
    );

    TEST_ASSERT(
        "the same non-L2TLB source-filter run must not create an L2TLB or cache-error SCEC record while only legal HS page walks are executed",
        no_scec_error_seen,
        "last_scec=0x%llx last_scec_src=%llu last_scec_epa=0x%llx",
        (unsigned long long)last_scec,
        (unsigned long long)ai_micro_cache_error_scec_source(last_scec),
        (unsigned long long)ai_micro_cache_error_scec_epa(last_scec)
    );

    TEST_ASSERT(
        "the non-L2TLB source-filter page-walk sequence reaches the deterministic final progress signature",
        progress == expected_progress,
        "progress=0x%llx expected=0x%llx last_scec=0x%llx last_sceic=0x%llx",
        (unsigned long long)progress,
        (unsigned long long)expected_progress,
        (unsigned long long)last_scec,
        (unsigned long long)last_sceic
    );

    goto_priv(PRIV_M);
    CSRW(CSR_SCEIC, original_sceic);
    CSRW(CSR_SCEC, original_scec);
    CSRW(CSR_MNSTATUS, original_mnstatus);

    TEST_END();
}
