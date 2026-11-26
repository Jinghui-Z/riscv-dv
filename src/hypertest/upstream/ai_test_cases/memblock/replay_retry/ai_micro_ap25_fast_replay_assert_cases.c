#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_AP25_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_AP25_VEC_BYTES 16U
#define AI_AP25_PRESSURE_LOADS 16U
#define AI_AP25_REDIRECT_TRAINING_ROUNDS 31U

static inline void ai_ap25_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline void ai_ap25_write_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_ap25_bytes_match(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_ap25_scalar_vector_window(uintptr_t cold_vaddr,
    uintptr_t vec_src_vaddr, uintptr_t vec_dst_vaddr, uintptr_t marker_vaddr,
    uint64_t marker_value, uint64_t *cold_out)
{
    uint64_t cold_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[cold_value], 0(%[cold])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[vec_src])\n\t"
        "vse8.v v8, (%[vec_dst])\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        ".option pop\n\t"
        : [cold_value] "=&r"(cold_value)
        : [cold] "r"(cold_vaddr),
          [vec_src] "r"(vec_src_vaddr),
          [vec_dst] "r"(vec_dst_vaddr),
          [marker] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [vl] "r"((int)AI_AP25_VEC_BYTES)
        : "t0", "memory");

    *cold_out = cold_value;
}

static inline void ai_ap25_prefetch_pressure_window(uintptr_t cold_vaddr,
    uintptr_t prefetch_vaddr, uintptr_t pressure_vaddr,
    uintptr_t marker_vaddr, uint64_t marker_value, uint64_t *cold_out,
    uint64_t *pressure_mix_out)
{
    uint64_t cold_value = 0ULL;
    uint64_t pressure_mix = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[cold_value], 0(%[cold])\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch], 1\n\t"
        "ld t0, 0(%[pressure])\n\t"
        "ld t1, 8(%[pressure])\n\t"
        "xor %[pressure_mix], t0, t1\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        ".option pop\n\t"
        : [cold_value] "=&r"(cold_value),
          [pressure_mix] "=&r"(pressure_mix)
        : [cold] "r"(cold_vaddr),
          [prefetch] "r"(prefetch_vaddr),
          [pressure] "r"(pressure_vaddr),
          [marker] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "t0", "t1", "memory");

    *cold_out = cold_value;
    *pressure_mix_out = pressure_mix;
}

static __attribute__((noinline)) void
ai_ap25_fast_replay_redirect_branch_window(uintptr_t wrong_vaddr,
    uintptr_t taken_vaddr, uintptr_t pressure_vaddr, uintptr_t marker_vaddr,
    uint64_t skipped_seed, uintptr_t do_wrong_path)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t0, %[skipped_seed]\n\t"
        "beqz %[do_wrong_path], 1f\n\t"
        "ld t0, 0(%[wrong_addr])\n\t"
        "1:\n\t"
        "sd t0, 0(%[marker_addr])\n\t"
        "ld t1, 0(%[taken_addr])\n\t"
        "sd t1, 8(%[marker_addr])\n\t"
        "ld t2, 0(%[pressure_addr])\n\t"
        "ld t3, 8(%[pressure_addr])\n\t"
        "xor t2, t2, t3\n\t"
        "sd t2, 24(%[marker_addr])\n\t"
        "sd t0, 16(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [wrong_addr] "r"(wrong_vaddr),
          [taken_addr] "r"(taken_vaddr),
          [pressure_addr] "r"(pressure_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [skipped_seed] "r"(skipped_seed),
          [do_wrong_path] "r"(do_wrong_path)
        : "t0", "t1", "t2", "t3", "memory");
}

bool ai_micro_ap25_fast_replay_vector_issue_progress_manual_smoke_corner()
{
    TEST_START();

    const enum test_page cold_page = VSRWX_GRWX;
    const enum test_page vec_src_page = VSRW_GRWX;
    const enum test_page vec_dst_page = VSRW_GURWX;
    const uintptr_t cold_vaddr = hs_page_base(cold_page) + 0x680U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x680U;
    const uintptr_t vec_src_vaddr = hs_page_base(vec_src_page) + 0x700U;
    const uintptr_t vec_src_paddr = phys_page_base(vec_src_page) + 0x700U;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_dst_page) + 0x780U;
    const uintptr_t vec_dst_paddr = phys_page_base(vec_dst_page) + 0x780U;
    const uintptr_t marker_vaddr = hs_page_base(cold_page) + 0x7e0U;
    const uintptr_t marker_paddr = phys_page_base(cold_page) + 0x7e0U;
    const uintptr_t guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t cold_initial = 0x0123456789abcdefULL;
    const uint64_t cold_refreshed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x1111222233334444ULL;
    const uint64_t marker_value = 0x5566778899aabbccULL;
    const uint64_t guard_value = 0x99aabbccddeeff00ULL;
    const uint8_t vec_pattern[AI_AP25_VEC_BYTES] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    uint64_t cold_observed = 0ULL;
    uint64_t original_menvcfg = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    write64(cold_paddr, cold_initial);
    write64(marker_paddr, marker_seed);
    write64(guard_paddr, guard_value);
    ai_ap25_write_bytes(vec_src_paddr, vec_pattern, AI_AP25_VEC_BYTES);
    for (unsigned i = 0; i < AI_AP25_VEC_BYTES; ++i) {
        write8(vec_dst_paddr + i, 0x5aU);
    }

    ai_ap25_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(cold_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 vector-issue smoke first warms the scalar replay-like target line",
        warmed == cold_initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(cold_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 vector-issue smoke invalidates the warmed scalar line before the mixed issue window",
        true
    );

    goto_priv(PRIV_M);
    write64(cold_paddr, cold_refreshed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap25_scalar_vector_window(cold_vaddr, vec_src_vaddr, vec_dst_vaddr,
        marker_vaddr, marker_value, &cold_observed);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 vector-issue smoke keeps scalar cold-line replay and vector load/store progress in the same window",
        cold_observed == cold_refreshed
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 vector-issue smoke drains the mixed scalar/vector window before checking memory image",
        true
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-25 vector-issue smoke copies the vector payload, commits the marker exactly once, and preserves the guard word",
        read64(cold_paddr) == cold_refreshed &&
        ai_ap25_bytes_match(vec_dst_paddr, vec_pattern, AI_AP25_VEC_BYTES) &&
        read64(marker_paddr) == marker_value &&
        read64(guard_paddr) == guard_value
    );
    CSRW(CSR_MENVCFG, original_menvcfg);

    TEST_END();
}

bool ai_micro_ap25_fast_replay_prefetch_dcache_pressure_manual_smoke_corner()
{
    TEST_START();

    const enum test_page cold_page = VSRWX_GRWX;
    const enum test_page prefetch_page = VSRW_GRWX;
    const enum test_page pressure_page = VSRW_GURWX;
    const uintptr_t cold_vaddr = hs_page_base(cold_page) + 0x480U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x480U;
    const uintptr_t prefetch_vaddr = hs_page_base(prefetch_page) + 0x4c0U;
    const uintptr_t prefetch_paddr = phys_page_base(prefetch_page) + 0x4c0U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x500U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x500U;
    const uintptr_t marker_vaddr = hs_page_base(cold_page) + 0x560U;
    const uintptr_t marker_paddr = phys_page_base(cold_page) + 0x560U;
    const uintptr_t guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t cold_initial = 0x1021324354657687ULL;
    const uint64_t cold_refreshed = 0x8877665544332211ULL;
    const uint64_t prefetch_payload = 0xa1b2c3d4e5f60718ULL;
    const uint64_t pressure0 = 0x0123456789abcdefULL;
    const uint64_t pressure1 = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x0badf00d2468ace0ULL;
    const uint64_t marker_value = 0x13579bdf02468aceULL;
    const uint64_t guard_value = 0x55aa33cc77ee11ffULL;
    uint64_t cold_observed = 0ULL;
    uint64_t pressure_mix = 0ULL;
    uint64_t original_menvcfg = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    write64(cold_paddr, cold_initial);
    write64(prefetch_paddr, prefetch_payload);
    write64(pressure_paddr, pressure0);
    write64(pressure_paddr + sizeof(uint64_t), pressure1);
    for (unsigned i = 2; i < AI_AP25_PRESSURE_LOADS; ++i) {
        write64(pressure_paddr + i * sizeof(uint64_t),
            pressure0 + (uint64_t)i);
    }
    write64(marker_paddr, marker_seed);
    write64(guard_paddr, guard_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(cold_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 prefetch-pressure smoke first warms the scalar replay-like target line",
        warmed == cold_initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(cold_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 prefetch-pressure smoke invalidates the warmed scalar line before the pressure window",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(prefetch_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 prefetch-pressure smoke also cools the prefetch line so the hint has a cold-cache path",
        true
    );

    goto_priv(PRIV_M);
    write64(cold_paddr, cold_refreshed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap25_prefetch_pressure_window(cold_vaddr, prefetch_vaddr,
        pressure_vaddr, marker_vaddr, marker_value, &cold_observed,
        &pressure_mix);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 prefetch-pressure smoke keeps cold scalar load, software prefetch, pressure loads, and marker progress in one window",
        cold_observed == cold_refreshed &&
        pressure_mix == (pressure0 ^ pressure1)
    );

    TEST_SETUP_EXCEPT();
    uint64_t prefetch_after = ld(prefetch_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 prefetch-pressure smoke leaves the prefetched line readable with the original payload",
        prefetch_after == prefetch_payload
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-25 prefetch-pressure smoke preserves pressure memory and commits the marker without clobbering the guard",
        read64(cold_paddr) == cold_refreshed &&
        read64(prefetch_paddr) == prefetch_payload &&
        read64(pressure_paddr) == pressure0 &&
        read64(pressure_paddr + sizeof(uint64_t)) == pressure1 &&
        read64(marker_paddr) == marker_value &&
        read64(guard_paddr) == guard_value
    );
    CSRW(CSR_MENVCFG, original_menvcfg);

    TEST_END();
}

bool ai_micro_ap25_fast_replay_redirect_kills_wrong_path_cold_load_no_marker_poison_corner()
{
    TEST_START();

    const enum test_page wrong_page = VSRWX_GRWX;
    const enum test_page taken_page = VSRW_GRWX;
    const enum test_page pressure_page = SWITCH1;
    const enum test_page marker_page = VSRW_GURWX;
    const uintptr_t wrong_vaddr = hs_page_base(wrong_page) + 0x880U;
    const uintptr_t wrong_paddr = phys_page_base(wrong_page) + 0x880U;
    const uintptr_t taken_vaddr = hs_page_base(taken_page) + 0x880U;
    const uintptr_t taken_paddr = phys_page_base(taken_page) + 0x880U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x880U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x880U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x880U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x880U;
    const uint64_t wrong_train = 0xa1a2a3a4a5a6a7a8ULL;
    const uint64_t wrong_final = 0xb1b2b3b4b5b6b7b8ULL;
    const uint64_t taken_payload = 0xc1c2c3c4c5c6c7c8ULL;
    const uint64_t skipped_seed = 0xd1d2d3d4d5d6d7d8ULL;
    const uint64_t pressure0 = 0x1021324354657687ULL;
    const uint64_t pressure1 = 0x8877665544332211ULL;
    const uint64_t pressure_mix = pressure0 ^ pressure1;
    const uint64_t marker_seed = 0x0badf00d13579bdfULL;
    const uint64_t marker_guard = 0xfeedfacecafebeefULL;
    uint64_t original_menvcfg = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    write64(wrong_paddr, wrong_train);
    write64(taken_paddr, taken_payload);
    write64(pressure_paddr, pressure0);
    write64(pressure_paddr + sizeof(uint64_t), pressure1);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), marker_seed);
    write64(marker_paddr + 2U * sizeof(uint64_t), marker_seed);
    write64(marker_paddr + 3U * sizeof(uint64_t), marker_seed);
    write64(marker_paddr + 4U * sizeof(uint64_t), marker_guard);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 redirect-quarantine setup exposes wrong-path, taken-path, pressure, and marker lines before branch predictor training",
        ld(wrong_vaddr) == wrong_train &&
        ld(taken_vaddr) == taken_payload &&
        ld(pressure_vaddr) == pressure0 &&
        ld(pressure_vaddr + sizeof(uint64_t)) == pressure1 &&
        ld(marker_vaddr + 4U * sizeof(uint64_t)) == marker_guard
    );

    TEST_SETUP_EXCEPT();
    for (unsigned round = 0; round < AI_AP25_REDIRECT_TRAINING_ROUNDS;
         ++round) {
        ai_ap25_fast_replay_redirect_branch_window(wrong_vaddr,
            taken_vaddr, pressure_vaddr, marker_vaddr, skipped_seed, 1U);
    }
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 redirect-quarantine training keeps the shared branch PC biased toward the fall-through cold-load path",
        ld(marker_vaddr) == wrong_train &&
        ld(marker_vaddr + sizeof(uint64_t)) == taken_payload &&
        ld(marker_vaddr + 2U * sizeof(uint64_t)) == wrong_train &&
        ld(marker_vaddr + 3U * sizeof(uint64_t)) == pressure_mix
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(wrong_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("AP-25 redirect-quarantine final probe cools only the wrong-path load line before the taken redirect",
        true
    );

    goto_priv(PRIV_M);
    write64(wrong_paddr, wrong_final);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), marker_seed);
    write64(marker_paddr + 2U * sizeof(uint64_t), marker_seed);
    write64(marker_paddr + 3U * sizeof(uint64_t), marker_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ap25_fast_replay_redirect_branch_window(wrong_vaddr, taken_vaddr,
        pressure_vaddr, marker_vaddr, skipped_seed, 0U);
    AI_AP25_ASSERT_NO_EXCEPT_AND("taken redirect must quarantine the wrong-path cold-load replay so marker words see only the skipped seed, taken payload, and pressure mix",
        ld(marker_vaddr) == skipped_seed &&
        ld(marker_vaddr + sizeof(uint64_t)) == taken_payload &&
        ld(marker_vaddr + 2U * sizeof(uint64_t)) == skipped_seed &&
        ld(marker_vaddr + 3U * sizeof(uint64_t)) == pressure_mix &&
        ld(marker_vaddr) != wrong_final &&
        ld(marker_vaddr + 2U * sizeof(uint64_t)) != wrong_final
    );

    TEST_SETUP_EXCEPT();
    const uint64_t wrong_after = ld(wrong_vaddr);
    AI_AP25_ASSERT_NO_EXCEPT_AND("after the killed wrong-path cold-load replay candidate, a real same-address load still observes the refreshed payload",
        wrong_after == wrong_final &&
        ld(taken_vaddr) == taken_payload &&
        ld(marker_vaddr) == skipped_seed &&
        ld(marker_vaddr + 2U * sizeof(uint64_t)) == skipped_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory after the AP-25 redirect-quarantine probe contains no wrong-path marker poison and keeps all payload/guard lines intact",
        read64(wrong_paddr) == wrong_final &&
        read64(taken_paddr) == taken_payload &&
        read64(pressure_paddr) == pressure0 &&
        read64(pressure_paddr + sizeof(uint64_t)) == pressure1 &&
        read64(marker_paddr) == skipped_seed &&
        read64(marker_paddr + sizeof(uint64_t)) == taken_payload &&
        read64(marker_paddr + 2U * sizeof(uint64_t)) == skipped_seed &&
        read64(marker_paddr + 3U * sizeof(uint64_t)) == pressure_mix &&
        read64(marker_paddr + 4U * sizeof(uint64_t)) == marker_guard
    );
    CSRW(CSR_MENVCFG, original_menvcfg);

    TEST_END();
}
