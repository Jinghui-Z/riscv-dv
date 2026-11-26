#include "backend_it_0911.h"
#include <stddef.h>

/* nhv5_1_ap, row882/scenario0x2009, external coordinated warm reset.
 * The agent alone proves simultaneous FTQ/IBuffer/Rename/IQ/LSU/ROB activity
 * and reset cleanup. Ordinary cacheable DRAM; no internal state forcing.
 * Official Spike cannot gate this scenario. No pressure_cases.c dependency.
 */
#pragma GCC optimize ("no-tree-vectorize")

#define RFP_MAGIC 0x4245495452535431ULL
#define RFP_ROW 882U
#define RFP_LINES 32U
#define RFP_GUARD 0x739b52e18a04d6cfULL

struct rfp_cookie {
    uint64_t magic, row, armed, observed_row, errors;
};
_Static_assert(sizeof(struct rfp_cookie) == 5 * sizeof(uint64_t), "reset cookie ABI size");
_Static_assert(offsetof(struct rfp_cookie, observed_row) == 24, "TB observed_row offset");
_Static_assert(offsetof(struct rfp_cookie, errors) == 32, "TB errors offset");
_Static_assert(BE_IT_SC_RESET_FULL_PIPE == 0x2009, "reset scenario ABI");

/* boot.S clears .bss only. armed: 0=cold boot, 1=await reboot, 2=done.
 * Only the reset agent may write observed_row/errors; never reload this ELF.
 */
static volatile struct rfp_cookie rfp_cookie
    __attribute__((section(".data"), aligned(64))) = {RFP_MAGIC, RFP_ROW, 0, 0, 0};
static volatile uint64_t rfp_data[RFP_LINES + 1][8] __attribute__((aligned(4096)));

static void rfp_publish_cookie(void)
{
    asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(&rfp_cookie) : "memory");
}

__attribute__((noinline)) static bool rfp_pressure(void)
{
    uint64_t sum, quotient;
    /* Older DIVs delay retirement; younger loads have independent addresses,
     * while store data waits on DIV. The straight-line body also feeds fetch
     * and rename. Actual occupancy, including 1..15 FTQ entries, is TB evidence.
     */
    asm volatile(BE_ASM_BEGIN
        "mv t3,%[p]\nli t2,0\n"
        BE_DELAY
        ".rept 32\n"
        "ld t0,0(t3)\nadd t1,t5,t0\nsd t1,8(t3)\n"
        "add t2,t2,t0\naddi t3,t3,64\n"
        ".endr\n"
        "mv %[sum],t2\nmv %[quotient],t5\n"
        BE_ASM_END
        : [sum] "=&r"(sum), [quotient] "=&r"(quotient)
        : [p] "r"(rfp_data)
        : "t0", "t1", "t2", "t3", "t5", "t6", "memory");
    const uint64_t expected_div = UINT64_MAX / 6561ULL;
    bool ok = sum == 32U * 17U + 31U * 32U / 2U && quotient == expected_div;
    for (unsigned i = 0; i < RFP_LINES; ++i)
        ok &= rfp_data[i][0] == 17U + i &&
            rfp_data[i][1] == expected_div + 17U + i && rfp_data[i][7] == RFP_GUARD;
    return ok && rfp_data[RFP_LINES][0] == RFP_GUARD;
}

bool ai_micro_backend_it_reset_full_pipe(void)
{
    TEST_START();
    be_it_enable();
    bool format_ok = rfp_cookie.magic == RFP_MAGIC && rfp_cookie.row == RFP_ROW;
    TEST_ASSERT("row882 retained cookie has the exact reset ABI", format_ok);
    if (!format_ok) return false;
    bool expected_boot = rfp_cookie.armed <= 1;
    TEST_ASSERT("row882 permits one cold boot and one warm reboot, never a third boot", expected_boot);
    if (!expected_boot) return false;

    if (rfp_cookie.armed == 1) {
        bool checked = rfp_cookie.observed_row == RFP_ROW && rfp_cookie.errors == 0;
        TEST_ASSERT("row882 agent checked real full-pipe reset, retained RAM, reset vector and clean state",
            checked, "observed=%lu errors=%lx", rfp_cookie.observed_row, rfp_cookie.errors);
        if (!checked) return false;
        rfp_cookie.armed = 2;
        rfp_publish_cookie();
        /* The reset agent auto-completes the monitor window. boot.S has
         * cleared the descriptor in .bss: be_it_finish() would use epoch 0
         * and cannot serve as the reboot oracle. Keep the TB cookie intact.
         */
    } else {
        bool pristine = rfp_cookie.observed_row == 0 && rfp_cookie.errors == 0;
        TEST_ASSERT("row882 cold boot has no fabricated or stale TB result", pristine);
        if (!pristine) return false;
        uint64_t status = CSRR(CSR_MSTATUS), mie = CSRR(CSR_MIE);
        CSRC(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE | MSTATUS_MPRV);
        CSRW(CSR_MIE, 0);
        for (unsigned i = 0; i < RFP_LINES; ++i) {
            rfp_data[i][0] = 17U + i;
            rfp_data[i][1] = 0;
            rfp_data[i][7] = RFP_GUARD;
            asm volatile("cbo.flush 0(%0)" :: "r"(&rfp_data[i][0]) : "memory");
        }
        rfp_data[RFP_LINES][0] = RFP_GUARD;
        asm volatile("fence rw,rw" ::: "memory");
        backend_it_control.target_pc = (uintptr_t)rfp_pressure;
        backend_it_control.target_addr = (uintptr_t)rfp_data;
        backend_it_control.auxiliary = (uintptr_t)&rfp_cookie;
        if (!be_it_begin_scenario(RFP_ROW, BE_IT_SC_RESET_FULL_PIPE, 0)) {
            CSRW(CSR_MIE, mie);
            CSRW(CSR_MSTATUS, status);
            return false;
        }
        rfp_cookie.armed = 1;
        rfp_publish_cookie();
        for (unsigned round = 0; round < 256; ++round) {
            asm volatile("fence.i" ::: "memory");
            if (!rfp_pressure()) {
                TEST_ASSERT("row882 unfinished reset stress preserves arithmetic and memory guards", false);
                break;
            }
        }
        /* Only a real warm reset may leave this boot successfully. */
        (void)be_it_finish_timeout(1024);
        CSRW(CSR_MIE, mie);
        CSRW(CSR_MSTATUS, status);
        TEST_ASSERT("row882 coordinated reset must interrupt the bounded full-pipe stress", false);
    }
    TEST_END("ai_micro_backend_it_reset_full_pipe");
}
