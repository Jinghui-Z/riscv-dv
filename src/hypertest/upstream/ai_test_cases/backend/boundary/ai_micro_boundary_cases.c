#include "boundary_suite.h"

/* Boundary IDs are document group * 100 + item. Each ELF has one case. */

bool ai_micro_boundary_301_flush_addi(void)
{
    TEST_START();
    bool correct=boundary_run(301,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_301_flush_addi");
}

bool ai_micro_boundary_302_violation_replay_load(void)
{
    TEST_START();
    bool correct=boundary_run(302,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_302_violation_replay_load");
}

bool ai_micro_boundary_303_flush_illegal(void)
{
    TEST_START();
    bool correct=boundary_run(303,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_303_flush_illegal");
}

bool ai_micro_boundary_304_flush_noncanonical(void)
{
    TEST_START();
    bool correct=boundary_run(304,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_304_flush_noncanonical");
}

bool ai_micro_boundary_305_flush_fetch_pmp(void)
{
    TEST_START();
    bool correct=boundary_run(305,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_305_flush_fetch_pmp");
}

bool ai_micro_boundary_306_flush_old_fetch_error(void)
{
    TEST_START();
    bool correct=boundary_run(306,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_306_flush_old_fetch_error");
}

bool ai_micro_boundary_401_flush_first_load(void)
{
    TEST_START();
    bool correct=boundary_run(401,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_401_flush_first_load");
}

bool ai_micro_boundary_402_flush_first_store(void)
{
    TEST_START();
    bool correct=boundary_run(402,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_402_flush_first_store");
}

bool ai_micro_boundary_403_flush_first_amo(void)
{
    TEST_START();
    bool correct=boundary_run(403,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_403_flush_first_amo");
}

bool ai_micro_boundary_404_flush_first_load_pf(void)
{
    TEST_START();
    bool correct=boundary_run(404,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_404_flush_first_load_pf");
}

bool ai_micro_boundary_405_flush_first_store_pf(void)
{
    TEST_START();
    bool correct=boundary_run(405,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_405_flush_first_store_pf");
}

bool ai_micro_boundary_406_flush_squashed_store(void)
{
    TEST_START();
    bool correct=boundary_run(406,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_406_flush_squashed_store");
}

bool ai_micro_boundary_501_handler_first_addi(void)
{
    TEST_START();
    bool correct=boundary_run(501,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_501_handler_first_addi");
}

bool ai_micro_boundary_502_handler_first_jal(void)
{
    TEST_START();
    bool correct=boundary_run(502,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_502_handler_first_jal");
}

bool ai_micro_boundary_503_handler_first_illegal(void)
{
    TEST_START();
    bool correct=boundary_run(503,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_503_handler_first_illegal");
}

bool ai_micro_boundary_504_handler_first_fetch_pmp(void)
{
    TEST_START();
    bool correct=boundary_run(504,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_504_handler_first_fetch_pmp");
}

bool ai_micro_boundary_505_handler_first_fetch_pf(void)
{
    TEST_START();
    bool correct=boundary_run(505,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_505_handler_first_fetch_pf");
}

bool ai_micro_boundary_506_handler_masked_irq(void)
{
    TEST_START();
    bool correct=boundary_run(506,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_506_handler_masked_irq");
}

bool ai_micro_boundary_601_handler_first_load(void)
{
    TEST_START();
    bool correct=boundary_run(601,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_601_handler_first_load");
}

bool ai_micro_boundary_602_handler_first_store(void)
{
    TEST_START();
    bool correct=boundary_run(602,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_602_handler_first_store");
}

bool ai_micro_boundary_603_handler_first_unaligned_load(void)
{
    TEST_START();
    bool correct=boundary_run(603,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_603_handler_first_unaligned_load");
}

bool ai_micro_boundary_604_handler_first_load_pf(void)
{
    TEST_START();
    bool correct=boundary_run(604,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_604_handler_first_load_pf");
}

bool ai_micro_boundary_605_handler_first_store_pmp(void)
{
    TEST_START();
    bool correct=boundary_run(605,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_605_handler_first_store_pmp");
}

bool ai_micro_boundary_606_handler_first_load_pf_masked_irq(void)
{
    TEST_START();
    bool correct=boundary_run(606,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_606_handler_first_load_pf_masked_irq");
}

bool ai_micro_boundary_701_mret_first_addi(void)
{
    TEST_START();
    bool correct=boundary_run(701,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_701_mret_first_addi");
}

bool ai_micro_boundary_703_mret_first_illegal(void)
{
    TEST_START();
    bool correct=boundary_run(703,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_703_mret_first_illegal");
}

bool ai_micro_boundary_704_mret_first_noncanonical(void)
{
    TEST_START();
    bool correct=boundary_run(704,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_704_mret_first_noncanonical");
}

bool ai_micro_boundary_705_mret_first_fetch_pmp(void)
{
    TEST_START();
    bool correct=boundary_run(705,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_705_mret_first_fetch_pmp");
}

bool ai_micro_boundary_706_sret_pending_irq(void)
{
    TEST_START();
    bool correct=boundary_run(706,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_706_sret_pending_irq");
}

bool ai_micro_boundary_801_mret_first_load_mprv(void)
{
    TEST_START();
    bool correct=boundary_run(801,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_801_mret_first_load_mprv");
}

bool ai_micro_boundary_804_mret_first_load_pf(void)
{
    TEST_START();
    bool correct=boundary_run(804,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_804_mret_first_load_pf");
}

bool ai_micro_boundary_805_mret_first_store_pf(void)
{
    TEST_START();
    bool correct=boundary_run(805,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_805_mret_first_store_pf");
}

bool ai_micro_boundary_806_xret_irq_before_store(void)
{
    TEST_START();
    bool correct=boundary_run(806,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_806_xret_irq_before_store");
}

bool ai_micro_boundary_901_bare_to_sv39_first_fetch(void)
{
    TEST_START();
    bool correct=boundary_run(901,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_901_bare_to_sv39_first_fetch");
}

bool ai_micro_boundary_902_bare_to_sv48_first_fetch(void)
{
    TEST_START();
    bool correct=boundary_run(902,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_902_bare_to_sv48_first_fetch");
}

bool ai_micro_boundary_903_sv39_to_sv48_first_fetch(void)
{
    TEST_START();
    bool correct=boundary_run(903,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_903_sv39_to_sv48_first_fetch");
}

bool ai_micro_boundary_904_sv48_to_sv39_noncanonical(void)
{
    TEST_START();
    bool correct=boundary_run(904,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_904_sv48_to_sv39_noncanonical");
}

bool ai_micro_boundary_906_satp_first_fetch_no_x(void)
{
    TEST_START();
    bool correct=boundary_run(906,true);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_906_satp_first_fetch_no_x");
}

bool ai_micro_boundary_1001_satp_first_load(void)
{
    TEST_START();
    bool correct=boundary_run(1001,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_1001_satp_first_load");
}

bool ai_micro_boundary_1002_satp_first_store(void)
{
    TEST_START();
    bool correct=boundary_run(1002,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_1002_satp_first_store");
}

bool ai_micro_boundary_1003_satp_first_load_pf(void)
{
    TEST_START();
    bool correct=boundary_run(1003,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_1003_satp_first_load_pf");
}

bool ai_micro_boundary_1004_satp_first_store_pf(void)
{
    TEST_START();
    bool correct=boundary_run(1004,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_1004_satp_first_store_pf");
}

bool ai_micro_boundary_1005_satp_first_load_pmp(void)
{
    TEST_START();
    bool correct=boundary_run(1005,false);
    TEST_ASSERT("the documented boundary has the expected trap, data and required IT witness",correct);
    TEST_END("ai_micro_boundary_1005_satp_first_load_pmp");
}

