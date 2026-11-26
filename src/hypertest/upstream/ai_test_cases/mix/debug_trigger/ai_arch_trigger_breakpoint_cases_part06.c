#include "ai_arch_trigger_breakpoint_common.h"

struct ai_arch_load_width_trigger_case {
    const char *name;
    unsigned width_code;
    bool unsigned_load;
    uintptr_t offset;
};

static bool ai_arch_check_load_width_trigger_af_priority(
    const struct ai_arch_load_width_trigger_case *cfg)
{
    uintptr_t addr = AI_ARCH_TRIGGER_BAD_PADDR + cfg->offset;
    uint64_t saved_mstatus = 0ULL;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_LOAD);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    (void)ai_arch_trigger_scalar_load_width_once(addr, cfg->width_code,
        cfg->unsigned_load);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result(cfg->name);
    }
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_restore_machine_status(saved_mstatus);
        ai_arch_clear_mcontrol6_0();
        return false;
    }

    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    (void)ai_arch_trigger_scalar_load_width_once(addr, cfg->width_code,
        cfg->unsigned_load);
    if (!(excpt.triggered == true &&
          excpt.cause == CAUSE_LAF &&
          excpt.tval == addr)) {
        ai_arch_log_unexpected_trigger_result(cfg->name);
        return false;
    }

    return true;
}

bool ai_arch_load_widths_trigger_access_fault_priority_manual_corner()
{
    TEST_START();

    static const struct ai_arch_load_width_trigger_case cases[] = {
        { "AP-64 lb address-trigger plus load-access-fault priority", 1U, false, 0U },
        { "AP-64 lbu address-trigger plus load-access-fault priority", 1U, true, 1U },
        { "AP-64 lh address-trigger plus load-access-fault priority", 2U, false, 2U },
        { "AP-64 lhu address-trigger plus load-access-fault priority", 2U, true, 4U },
        { "AP-64 lw address-trigger plus load-access-fault priority", 4U, false, 8U },
        { "AP-64 lwu address-trigger plus load-access-fault priority", 4U, true, 12U },
        { "AP-64 ld address-trigger plus load-access-fault priority", 8U, false, 16U },
    };

    for (unsigned i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        TEST_ASSERT("AP-64 scalar load width matrix keeps address-trigger breakpoint priority over load access fault, then exposes LAF after clearing the trigger",
            ai_arch_check_load_width_trigger_af_priority(&cases[i])
        );
    }

    TEST_END();
}
