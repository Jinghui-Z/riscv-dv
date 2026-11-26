#include <rvh_test.h>
#include <page_tables.h>
#include <csrs.h> 

bool ebreak_tests_1() {

    TEST_START();

    //m模式下执行ebreak指令
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    
    ebreak();

    TEST_ASSERT("m mode ebreak cause to Breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    ); 

    //HS模式下执行ebreak指令
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    ebreak();

    TEST_ASSERT("hs mode ebreak cause to Breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    ); 

    //HU模式下执行ebreak指令
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    ebreak();

    TEST_ASSERT("hu mode ebreak cause to Breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    ); 

    TEST_END();

}



bool ebreak_tests_2() {

    TEST_START();

    //m模式下执行ebreak指令
    goto_priv(PRIV_M);
    CSRW(CSR_MEDELEG,(uint64_t)-1);
    TEST_SETUP_EXCEPT();
    
    ebreak();

    TEST_ASSERT("m mode ebreak cause to Breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.priv == PRIV_M
    ); 

    //HS模式下执行ebreak指令
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    ebreak();

    TEST_ASSERT("hs mode ebreak cause to Breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.priv == PRIV_HS
    ); 

    //HU模式下执行ebreak指令
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    ebreak();

    TEST_ASSERT("hu mode ebreak cause to Breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.priv == PRIV_HS
    ); 


}