#include "backend_it_vmodule.h"

/* AP-IT injection acceptance only. These are not substitutes for the 218
 * precise ROB/cache/queue checkpoints and do not acknowledge their mailbox. */
bool ai_micro_backend_vmodule_mei_smoke(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    const unsigned delays[] = {0, 32, 256, 2048};
    for (unsigned i = 0; i < sizeof(delays)/sizeof(delays[0]); ++i)
        TEST_ASSERT("VModule MEI arrives once and handler clears its physical source",
            be_it_vmodule_irq_probe(delays[i], false), "delay=%u", delays[i]);
    TEST_END("ai_micro_backend_vmodule_mei_smoke");
}

bool ai_micro_backend_vmodule_wfi_smoke(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    const unsigned delays[] = {0, 2048, 8192};
    for (unsigned i = 0; i < sizeof(delays)/sizeof(delays[0]); ++i)
        TEST_ASSERT("locally enabled MEI wakes WFI and is serviced once after global enable",
            be_it_vmodule_irq_probe(delays[i], true), "delay=%u", delays[i]);
    TEST_END("ai_micro_backend_vmodule_wfi_smoke");
}

bool ai_micro_backend_vmodule_ecc_smoke(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    TEST_ASSERT("VModule sources initially clear", be_vm_clear());
    be_vm_watchdog(2000000);
    bool ok = be_it_vmodule_ecc_probe();
    be_vm_watchdog(UINT32_MAX);
    TEST_ASSERT("ICache parity injection, hardware consumption, clean refetch and re-arm", ok);
    TEST_END("ai_micro_backend_vmodule_ecc_smoke");
}
