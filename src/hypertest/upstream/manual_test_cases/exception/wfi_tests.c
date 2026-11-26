#include <rvh_test.h>
#include <csrs.h> 
#include <stdio.h>

bool wfi_exception_tests_1() {

    TEST_START();


    //mstatus.TW=0时，在u模式下，中断未被禁用，且代理没打开，执行wfi指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("U-mode wfi causes illegal instruction exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=0",
        excpt.triggered == true &&  
        excpt.cause == CAUSE_ILI
    );   

    TEST_END();
}


bool wfi_exception_tests_2() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    /* Keep an interrupt pending so we don't get stuck when wfi succeeds */
    //mstatus.TW=0时，在m模式下，中断使能未拉高，中断未被禁用，且代理没打开，执行wfi指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(medeleg,0);
    CSRW(mideleg,0);     
    CSRC(mie, 0b0100);
    CSRS(mip, 0b0100);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("machine mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=0 and mie=0",
        excpt.triggered == false
    );  

   
    TEST_END();
}

bool wfi_exception_tests_3() {

    TEST_START();
    
    TEST_SETUP_EXCEPT();

    //mstatus.TW=0时，在HS模式下，中断未被禁用，且代理没打开，执行wfi指令
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(medeleg,0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("S-mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=0",
        excpt.triggered == false
    );   

    TEST_END();
}

bool wfi_exception_tests_4() {

    TEST_START();
    
    TEST_SETUP_EXCEPT();
    
    //mstatus.TW=0时，在M模式下，中断未被禁用，且代理打开，执行wfi指令

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(medeleg,0);
    CSRW(mideleg,0);   
    CSRS(mie, 0b0100);
    CSRS(mip, 0b0100);
    CSRS(mideleg, 0b0100);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("machine mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=0",
        excpt.triggered == false
    );  

    /** 
     * Delegate the pending interrupt to hs mode so it doesnt immediately 
     * trigger a trap to machine when we jump to hs.
     */
    TEST_END();
}




bool wfi_exception_tests_5() {

    TEST_START();
    //mstatus.TW=0时，在HS模式下，发生中断，中断使能未拉高，中断未被代理到低特权模式
    
    goto_priv(PRIV_M); 
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRS(CSR_SIE, 0x0);
    CSRS(mideleg, 0x0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("S-mode wfi is not awakened when mstatus.TW=0 and mideleg=0 and xie=0",
        excpt.triggered == false
    );   

    TEST_END();
}

bool wfi_exception_tests_6() {

    TEST_START();
    //mstatus.TW=0时，在M模式下，发生中断，中断使能未拉高，中断未被代理到低特权模式
    
    goto_priv(PRIV_M); 
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRS(CSR_MIE, 0x0);
    CSRS(mideleg, 0x0);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("M-mode wfi is not awakened when mstatus.TW=0 and mideleg=0 and xie=0",
        excpt.triggered == false
    );   

    TEST_END();
}


bool wfi_exception_tests_7() {

    TEST_START();
    //mstatus.TW=0时，在HS模式下，发生中断，中断使能未拉高，中断被代理到低特权模式
    
    goto_priv(PRIV_M); 
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(CSR_SIE, 0x0);
    CSRW(mideleg, (uint64_t)-1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("HS-mode wfi is not awakened when mstatus.TW=0 and mideleg=1 and xie=0",
        excpt.triggered == false
    );   

    TEST_END();
}

bool wfi_exception_tests_8() {

    TEST_START();
    //mstatus.TW=0时，在M模式下，发生中断，中断使能未拉高，中断被代理到低特权模式
    
    goto_priv(PRIV_M); 
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(CSR_MIE, 0x0);
    CSRW(mideleg, (uint64_t)-1);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("M-mode wfi is not awakened when mstatus.TW=0 and mideleg=1 and xie=0",
        excpt.triggered == false
    );   

    TEST_END();
}




bool wfi_exception_tests_9() {

    //mstatus.TW=1时，在HS模式下执行WFI指令，没有在一个约定时间内产生中断
    TEST_START();
    goto_priv(PRIV_M);
    CSRS(mstatus, MSTATUS_TW);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("S-mode wfi triggers illegal instructions exception when mstatus.tw = 1",
        excpt.triggered == true &&  
        excpt.cause == CAUSE_ILI
    );   

    TEST_END();
}
    /** 
     * Delegate the pending interrupt to hs mode so it doesnt immediately 
     * trigger a trap to machine when we jump to vs.
     */




bool wfi_exception_tests_10() {

    //mstatus.TW=1时，在HU模式下执行WFI指令，没有在一个约定时间内产生中断
    TEST_START();
    goto_priv(PRIV_M);
    CSRS(mstatus, MSTATUS_TW);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("HU-mode wfi triggers illegal instructions exception when mstatus.tw = 1",
        excpt.triggered == true &&  
        excpt.cause == CAUSE_ILI
    );   

    TEST_END();
}
    /** 
     * Delegate the pending interrupt to hs mode so it doesnt immediately 
     * trigger a trap to machine when we jump to vs.
     */












// bool wfi_exception_tests_21() {

//     TEST_START();

//     //V=0&mstatus.TW=1时，且实现了S/HS模式，在U模式下执行WFI指令，并在一个约定时间内产生中断,wfi指令被唤醒，且中断会发生在下一条指令上，xepc=pc+4
//     TEST_SETUP_EXCEPT();
//     goto_priv(PRIV_M);
//     CSRW(CSR_HIE, 0x2);
    
//     // CSRS(mstatus, MSTATUS_TW);
//     CSRS(hstatus, HSTATUS_VTW);

//     goto_priv(PRIV_VS);
//     wfi();
//     // CSRW(CSR_HIP,0x2);
// excpt_info();
//     TEST_ASSERT("HU-mode wfi causes illegal instruction exception when mstatus.tw=1",
//         excpt.triggered == true &&  
//         excpt.cause == CAUSE_ILI
//     ); 
    

//     TEST_END();
// }


// bool wfi_exception_tests_22() {

//     TEST_START();

//     //V=0&mstatus.TW=1时，如果实现了S/HS模式，在U模式下执行WFI指令，没有在一个约定时间内产生中断
//     TEST_SETUP_EXCEPT();
//     goto_priv(PRIV_M);
//     CSRW(CSR_HIE, 0);
//     CSRW(CSR_HIP,0);
//     CSRS(mstatus, MSTATUS_TW);

//     goto_priv(PRIV_HU);
//     wfi();
    
//     TEST_ASSERT("HU-mode wfi causes illegal instruction exception when mstatus.tw=1",
//         excpt.triggered == true &&  
//         excpt.cause == CAUSE_ILI
//     ); 
    

//     TEST_END();
// }


bool wfi_exception_tests_11() {

    TEST_START();
     //mstatus.TW=0时，在u模式下，中断未被禁用，且代理打开，执行wfi指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);

    TEST_SETUP_EXCEPT();
    CSRS(mideleg, 0b0100);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("U-mode wfi causes illegal instruction exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=1",
        excpt.triggered == true &&  
        excpt.cause == CAUSE_ILI
    );   


    TEST_END();
}

    


bool wfi_exception_tests_12() {

    TEST_START();
    
    TEST_SETUP_EXCEPT();
    //mstatus.TW=0时，在HS模式下，中断未被禁用，且代理打开，执行wfi指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);

    CSRS(mideleg, 0b0100);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("S-mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=1",
        excpt.triggered == false
    );   

    TEST_END();
}


bool wfi_exception_tests_13() {

    TEST_START();
    
    TEST_SETUP_EXCEPT();
    
    //mstatus.TW=0时，在M模式下，中断未被禁用，且代理打开，执行wfi指令

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(mideleg,0b0100);   
    CSRS(mie, 0b0100);
    CSRS(mip, 0b0100);
    CSRS(mideleg, 0b0100);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();
    goto_priv(PRIV_HS);

    TEST_ASSERT("machine mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=1",
        excpt.triggered == false
    );  

    
    /** 
     * Delegate the pending interrupt to hs mode so it doesnt immediately 
     * trigger a trap to machine when we jump to hs.
     */
    TEST_END();
}

bool wfi_exception_tests_14() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    /* Keep an interrupt pending so we don't get stuck when wfi succeeds */
    //mstatus.TW=0时，在m模式下，中断使能未拉高，中断未被禁用，且代理没打开，执行wfi指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TW);
    CSRW(mideleg,0b0100);     
    CSRC(mie, 0b0100);
    CSRS(mip, 0b0100);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("machine mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=0 and mideleg=1 and mie=0",
        excpt.triggered == false
    );  

   
    TEST_END();
}

bool wfi_exception_tests_15() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    /* Keep an interrupt pending so we don't get stuck when wfi succeeds */
    //mstatus.TW=1时，在m模式下，中断使能拉高，中断未被禁用，且代理没打开，执行wfi指令
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_TW);
    CSRW(mideleg,0b0100);     
    CSRS(mie, 0b0100);
    CSRS(mip, 0b0100);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();
    TEST_ASSERT("machine mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=1 and mideleg=1 and mie=0",
        excpt.triggered == false
    );  

   
    TEST_END();
}

bool wfi_exception_tests_16() {

    TEST_START();
    
    TEST_SETUP_EXCEPT();
    
    //mstatus.TW=1时，在M模式下，中断未被禁用，且代理打开，执行wfi指令

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_TW);
    CSRW(mideleg,0b0100);   
    CSRS(mie, 0b0100);
    CSRS(mip, 0b0100);
    CSRS(mideleg, 0b0100);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();
    goto_priv(PRIV_HS);

    TEST_ASSERT("machine mode wfi does not trigger exception when Interrupt is not disabled and mstatus.TW=1 and mideleg=1",
        excpt.triggered == false
    );  

    
    /** 
     * Delegate the pending interrupt to hs mode so it doesnt immediately 
     * trigger a trap to machine when we jump to hs.
     */
    TEST_END();
}

    //mstatus.TW=0时，在M模式下，中断未被禁用，且代理打开，执行wfi指令

/*

    //mstatus.TW=0时，在VS模式下，发生中断，中断使能拉高，中断未被代理到低特权模式
    reset_state();
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_M); 

    CSRS(CSR_SIE, 0b0100);

    CSRC(mideleg, 0b0100);

    wfi();


    goto_priv(PRIV_VS);

    CSRS(CSR_SIP, 0b0100);

    TEST_ASSERT("VS-mode wfi is awakened and the interrupt occurs on the next instruction, sepc=pc+4 when mstatus.TW=0 and mideleg=0",
        excpt.triggered == true
    );   




    //mstatus.TW=0时，在M模式下，发生中断，中断使能拉高，中断未被代理到低特权模式

    reset_state();
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_M); 

    CSRS(CSR_MIE, (uint64_t)-1);

    CSRC(mideleg, (uint64_t)-1);

    CSRS(CSR_MIP,0b0100);

    goto_priv(PRIV_M);

    wfi();


    TEST_ASSERT("M-mode wfi is awakened and the interrupt occurs on the next instruction, mepc=pc+4 when mstatus.TW=0 and mideleg=0",
        excpt.triggered == true
    );   

    //mstatus.TW=0时，在VS模式下，发生中断，中断使能拉高，中断被代理到低特权模式
    
    reset_state();
    goto_priv(PRIV_M); 
    CSRS(CSR_SIE, 0b0100);
    CSRS(mideleg, 0b0100);
    CSRS(CSR_SIP, 0b0100);

    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("VS-mode wfi is awakened and the interrupt occurs on the next instruction, sepc=pc+4 when mstatus.TW=0 and mideleg=1",
        excpt.triggered == true
    );   

    //mstatus.TW=0时，在HS模式下，发生中断，中断使能拉高，中断被代理到低特权模式
    
    goto_priv(PRIV_M); 
    CSRS(CSR_SIE, 0b0100);
    CSRS(mideleg, 0b0100);
    CSRS(CSR_SIP, 0b0100);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    wfi();

    TEST_ASSERT("HS-mode wfi is awakened and the interrupt occurs on the next instruction, sepc=pc+4 when mstatus.TW=0 and mideleg=1",
        excpt.triggered == true
    );   

    //mstatus.TW=0时，在M模式下，发生中断，中断使能拉高，中断被代理到低特权模式
    
    goto_priv(PRIV_M); 
    CSRS(CSR_MIE, (uint64_t)-1);
    CSRS(mideleg, (uint64_t)-1);
    CSRS(CSR_MIP,0b0100);
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    wfi();

    //printf("%lld %lld %lld",x,y,CSRR(CSR_MEPC));
    TEST_ASSERT("M-mode wfi is awakened and the interrupt occurs on the next instruction, mepc=pc+4 when mstatus.TW=0 and mideleg=1",
        excpt.triggered == true
    );   
*/
