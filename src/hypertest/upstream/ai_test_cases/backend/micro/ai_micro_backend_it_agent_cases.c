#include "backend_it_0911.h"

/* Monitor/agent acceptance for the WFI debug and FTQ-reset scenarios. These are not
 * substitutes for the complete WFI and reset matrices. No ARCH_SMOKE build. */
extern uint64_t be_it_agent_wfi(uint64_t);
asm(".pushsection .text.be_it_agent,\"ax\",@progbits\n" BE_ASM_BEGIN
    ".balign 64\n.global be_it_agent_wfi\nbe_it_agent_wfi:\n"
    "wfi\naddi a0,a0,1\nret\n" BE_ASM_END ".popsection\n");

bool ai_micro_backend_it_debug_agent_smoke(void)
{
    TEST_START();
    be_it_enable();
    uint64_t status = CSRR(CSR_MSTATUS), mie = CSRR(CSR_MIE);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0);
    for (unsigned row = 756; row <= 757; ++row) {
        backend_it_control.target_pc = (uintptr_t)be_it_agent_wfi;
        unsigned scenario = row == 756 ? BE_IT_SC_DEBUG_WFI_HALTREQ :
            BE_IT_SC_DEBUG_WFI_TRIGGER;
        if (!be_it_begin_scenario(row, scenario, 0)) return false;
        uint64_t result = be_it_agent_wfi(40);
        TEST_ASSERT("debug resume executes the post-WFI marker once", result == 41);
        TEST_ASSERT("debugger checked DPC, cause, trigger and resume", be_it_finish());
    }
    CSRW(CSR_MIE, mie);
    CSRW(CSR_MSTATUS, status);
    TEST_END("ai_micro_backend_it_debug_agent_smoke");
}

static volatile struct {
    uint64_t magic, row, armed, observed_row, errors;
} agent_reset_cookie __attribute__((aligned(64))) = {0x4245495452535431ULL, 874, 0, 0, 0};

bool ai_micro_backend_it_reset_agent_smoke(void)
{
    TEST_START();
    be_it_enable();
    if (agent_reset_cookie.armed) {
        TEST_ASSERT("coordinated reset retained DRAM and the agent validated reboot",
            agent_reset_cookie.magic == 0x4245495452535431ULL &&
            agent_reset_cookie.observed_row == 874 && agent_reset_cookie.errors == 0);
    } else {
        backend_it_control.auxiliary = (uintptr_t)&agent_reset_cookie;
        if (!be_it_begin_scenario(874, BE_IT_SC_RESET_FTQ, 0)) return false;
        agent_reset_cookie.armed = 1;
        asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(&agent_reset_cookie) : "memory");
        for (unsigned i = 0; i < 1000000; ++i)
            asm volatile("nop\nnop\nnop\nnop" ::: "memory");
        TEST_ASSERT("the reset agent must interrupt this boot", false);
    }
    TEST_END("ai_micro_backend_it_reset_agent_smoke");
}
