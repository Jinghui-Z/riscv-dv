#include <rvh_test.h>
#include <page_tables.h> 

static inline void touchread(uintptr_t addr){
    asm volatile("" ::: "memory");
    volatile uint64_t x = *(volatile uint64_t *)addr;
}

static inline void touchwrite(uintptr_t addr){
    *(volatile uint64_t *)addr = 0xdeadbeef;
}

static inline void touch(uintptr_t addr){
    touchwrite(addr);
}

bool two_stage_translation_1(){
    
    TEST_START();

    uintptr_t addr1 = phys_page_base(SWITCH1);
    uintptr_t addr2 = phys_page_base(SWITCH2);
    uintptr_t vaddr1 = vs_page_base(SWITCH1);
    uintptr_t vaddr2 = vs_page_base(SWITCH2);
    write64(addr1, 0x11);
    write64(addr2, 0x22);

    /**
     * Setup hyp page_tables.
     */
    goto_priv(PRIV_HS);
    hspt_init();


    bool check1 = read64(vaddr1) == 0x11;
    bool check2 = read64(vaddr2) == 0x22;
    TEST_ASSERT("hs gets right values", check1 && check2);
    
    hspt_switch();
    check1 = read64(vaddr1) == 0x11;
    check2 = read64(vaddr2) == 0x22;   
    TEST_ASSERT("hs gets old values after changing pt without sfence", check1 && check2);

    sfence_vma();
    check1 = read64(vaddr1) == 0x22;
    check2 = read64(vaddr2) == 0x11;   
    TEST_ASSERT("hs gets right values after sfence", check1 && check2);


    TEST_END();
}

bool fence_test_1(){

    TEST_START();

    //hs load test with fence when pte.v changed

    uintptr_t vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();


    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_V();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault when not executing sfence(pte.v from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load with page fault when executing sfence(pte.v from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    //hs store test with fence when pte.v changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();


    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_V();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault when not executing sfence(pte.v from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store with page fault when executing sfence(pte.v from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //hs instr fetch test with fence when pte.v changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    *(uint32_t*)vaddr = 0x00008067;  // ret指令
    sfence_vma();

    TEST_SETUP_EXCEPT();
    
    
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_V();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault when not executing sfence(pte.v from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs instr fetch with page fault when executing sfence(pte.v from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}



bool fence_test_2(){

    TEST_START();

    //hs store test with fence when pte.w changed

    uintptr_t vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();


    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_W();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault when not executing sfence(pte.w from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store with page fault when executing sfence(pte.w from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );


    //hs load test with fence when pte.r changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();


    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_R();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault when not executing sfence(pte.r from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load with page fault when executing sfence(pte.r from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );


    //hs instr fetch test with fence when pte.x changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();

    *(uint32_t*)vaddr = 0x00008067;  // ret指令
    sfence_vma();

    TEST_SETUP_EXCEPT();
    
    
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_X();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault when not executing sfence(pte.x from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs instr fetch with page fault when executing sfence(pte.x from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}

bool fence_test_3(){

    TEST_START();

    //hs instr fetch test with fence when pte.w changed

    uintptr_t vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();

    *(uint32_t*)vaddr = 0x00008067;  // ret指令
    sfence_vma();

    TEST_SETUP_EXCEPT();
    
    
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_W();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault when not executing sfence(pte.w from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault when executing sfence(pte.w from 1 to 0)",
        excpt.triggered == false
    );

    //hs instr fetch test with fence when pte.r changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();

    *(uint32_t*)vaddr = 0x00008067;  // ret指令
    sfence_vma();

    TEST_SETUP_EXCEPT();
    
    
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_R();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs instr fetch without page fault when not executing sfence(pte.r from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs instr fetch with page fault when executing sfence(pte.r from 1 to 0)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //hs store test with fence when pte.x changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_X();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault when not executing sfence(pte.x from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs store without page fault when executing sfence(pte.x from 1 to 0)",
        excpt.triggered == false
    );


    //hs load test with fence when pte.W changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_W();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault when not executing sfence(pte.w from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault when executing sfence(pte.w from 1 to 0)",
        excpt.triggered == false
    );

    //hs load test with fence when pte.x changed

    vaddr = vs_page_base(VSRWX_GX);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_del_X();


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault when not executing sfence(pte.x from 1 to 0)",
        excpt.triggered == false
    );


    sfence_vma();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs load without page fault when executing sfence(pte.x from 1 to 0)",
        excpt.triggered == false
    );

}


bool fence_test_4(){
    TEST_START();

    //rs1的值不是一个有效的虚拟地址，执行SFENCE.VMA指令
    uintptr_t vaddr = vs_page_base(VSRWX_GX);

    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HS);
    hspt_init();

    sd(vaddr, 0xdeadbeef);

    goto_priv(PRIV_M);
    hspt_del_V();

    goto_priv(PRIV_HS);
    sfence_vma_rs(0x80000000, 0);  // 刷新特定虚拟地址的TLB

    TEST_SETUP_EXCEPT();
    uint64_t val = ld(vaddr);
    TEST_ASSERT("sfence with invalid rs1 does not affect other addresses",
        val == 0xdeadbeef &&
        excpt.triggered == false
    );

    sfence_vma_rs(-1, -1);

    TEST_SETUP_EXCEPT();
    ld(vaddr);


    TEST_ASSERT("sfence with rs1=x0 affects all addresses",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    
}