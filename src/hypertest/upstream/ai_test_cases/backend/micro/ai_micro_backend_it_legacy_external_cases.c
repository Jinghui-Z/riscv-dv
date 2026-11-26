#include "backend_it_0911.h"

/*
 * AP-IT-only cases. The monitor observes the internal completion/reset
 * contract; software supplies an architectural transaction and a retained
 * DRAM cookie. Keep these registrations disabled in ordinary Spike runs.
 */

static volatile uint64_t be_it_cmo_line[16]
    __attribute__((aligned(64)));

bool ai_micro_backend_it_cmo_completion_baseline(void)
{
    TEST_START();
    be_it_enable();

    be_it_cmo_line[0] = 0x273273273273273ULL;
    be_it_cmo_line[8] = 0x5a5aa5a55a5aa5a5ULL;
    backend_it_control.target_addr = (uintptr_t)be_it_cmo_line;
    if (!be_it_begin_scenario(273, BE_IT_SC_CMO_COMPLETION, 0)) return false;

    uint64_t observed;
    asm volatile(
        BE_ASM_BEGIN
        "cbo.flush 0(%[line])\n"
        /* Keep an ordinary memory uop waiting behind the CMO completion. */
        "ld %[out],64(%[line])\n"
        ".rept 16\n"
        "ld t0,64(%[line])\n"
        "add %[out],%[out],t0\n"
        ".endr\n"
        BE_ASM_END
        : [out] "=&r"(observed)
        : [line] "r"(be_it_cmo_line)
        : "t0", "memory");

    TEST_ASSERT("the memory stream following CMO completion observes the published line",
        observed == 17ULL * be_it_cmo_line[8]);
    TEST_ASSERT("IT observed cmofinish, the eight-cycle memory block tail, and release",
        be_it_finish());
    TEST_END("ai_micro_backend_it_cmo_completion_baseline");
}

bool ai_micro_backend_it_boot_reset_contract(void)
{
    TEST_START();
    be_it_enable();

    static const unsigned rows[] = {563, 564, 565};
    static const unsigned scenarios[] = {
        BE_IT_SC_BOOT_BPU_VECTOR,
        BE_IT_SC_BOOT_ICACHE_RESET,
        BE_IT_SC_BOOT_CSR_RESET
    };
    for (unsigned i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        if (!be_it_begin_scenario(rows[i], scenarios[i], 0)) return false;
        TEST_ASSERT("the sticky power-on monitor recorded the row-specific reset contract",
            be_it_finish(), "row=%u scenario=0x%x", rows[i], scenarios[i]);
    }

    TEST_END("ai_micro_backend_it_boot_reset_contract");
}

extern void be_it_reset_fetch_stream(void);
asm(
    ".pushsection .text.be_it_reset_fetch_stream,\"ax\",@progbits\n"
    BE_ASM_BEGIN
    ".balign 4096\n"
    ".global be_it_reset_fetch_stream\n"
    "be_it_reset_fetch_stream:\n"
    ".rept 1536\n"
    "addi t0,t0,1\n"
    ".endr\n"
    "ret\n"
    BE_ASM_END
    ".popsection\n");

static volatile struct {
    uint64_t magic;
    uint64_t row;
    uint64_t armed;
    uint64_t observed_row;
    uint64_t errors;
} be_it_frontend_reset_cookie __attribute__((aligned(64))) = {
    0x4245495452535431ULL, 567, 0, 0, 0
};

bool ai_micro_backend_it_warm_reset_frontend_contract(void)
{
    TEST_START();
    be_it_enable();

    TEST_ASSERT("the frontend warm-reset cookie has the expected format",
        be_it_frontend_reset_cookie.magic == 0x4245495452535431ULL);
    if (be_it_frontend_reset_cookie.armed) {
        TEST_ASSERT("warm reset retained DRAM and the reset agent validated this row",
            be_it_frontend_reset_cookie.observed_row ==
                be_it_frontend_reset_cookie.row &&
            be_it_frontend_reset_cookie.errors == 0,
            "row=%lu observed=%lu errors=%lu",
            be_it_frontend_reset_cookie.row,
            be_it_frontend_reset_cookie.observed_row,
            be_it_frontend_reset_cookie.errors);
        be_it_frontend_reset_cookie.armed = 0;
        be_it_frontend_reset_cookie.row++;
    }

    const unsigned row = (unsigned)be_it_frontend_reset_cookie.row;
    if (row <= 568) {
        const unsigned scenario = row == 567 ?
            BE_IT_SC_RESET_FTQ : BE_IT_SC_RESET_ICACHE_MSHR;
        backend_it_control.target_pc = (uintptr_t)be_it_reset_fetch_stream;
        backend_it_control.auxiliary =
            (uintptr_t)&be_it_frontend_reset_cookie;
        if (!be_it_begin_scenario(row, scenario, 0)) return false;

        be_it_frontend_reset_cookie.armed = 1;
        asm volatile("cbo.flush 0(%0)\nfence rw,rw"
            :: "r"(&be_it_frontend_reset_cookie) : "memory");
        asm volatile("fence.i" ::: "memory");
        for (unsigned attempt = 0; attempt < 1000000; ++attempt)
            be_it_reset_fetch_stream();
        TEST_ASSERT("the reset agent must interrupt the active frontend window",
            false, "row=%u scenario=0x%x", row, scenario);
    }

    TEST_END("ai_micro_backend_it_warm_reset_frontend_contract");
}
