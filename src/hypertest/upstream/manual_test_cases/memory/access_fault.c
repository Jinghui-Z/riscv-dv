#include <rvh_test.h>
#include <page_tables.h>

#define LINKNAN_PMA_R      (1ULL << 0)
#define LINKNAN_PMA_W      (1ULL << 1)
#define LINKNAN_PMA_X      (1ULL << 2)
#define LINKNAN_PMA_A_TOR  (1ULL << 3)
#define LINKNAN_PMA_ATOMIC (1ULL << 5)
#define LINKNAN_PMA_C      (1ULL << 6)
#define LINKNAN_PMA_L      (1ULL << 7)

static inline uint64_t linknan_pma_cfg_entry(unsigned entry, uint64_t bits)
{
    return bits << (entry * 8);
}

static inline void linknan_pma_cfg0_tor_entry1_encoded(uint64_t lower, uint64_t upper,
    uint64_t entry1_bits)
{
    CSRW(CSR_PMACFG0, 0x0);
    CSRW(CSR_PMAADDR0, lower);
    CSRW(CSR_PMAADDR1, upper);
    CSRW(CSR_PMACFG0, linknan_pma_cfg_entry(1, entry1_bits));
}




bool load_access_fault_1(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有load权限的PMP区域，pmpcfg.R=0
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位（清除）
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);
    // sfence();
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 
    
    TEST_SETUP_EXCEPT();

    lb(0x80000100ULL << 2);    //访问区域内地址

    CSRW(CSR_PMPADDR0, (uintptr_t)0x82000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x83000000);

    
    TEST_ASSERT("m mode lb when pmpcfg.R=0 and pmpcfg.L=1 leads to LAF",       
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();

    lr_d(0x80000100ULL << 2);    //访问区域内地址

    CSRW(CSR_PMPADDR0, (uintptr_t)0x82000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x83000000);

    
    TEST_ASSERT("m mode lr_d when pmpcfg.R=0 and pmpcfg.L=1 leads to LAF",       
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}



bool load_access_fault_2(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有load权限的PMP区域，pmpcfg.R=0

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, 0x88000000UL);
    CSRW(CSR_PMPADDR1, 0x89000000UL);


    CSRS(CSR_PMPCFG0,1ULL << 7 );       //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );      //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    lb(0x88100000ULL << 2);

    TEST_ASSERT("hs mode lb when pmpcfg.R=0 and pmpcfg.L=1 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();    

    lr_d(0x88100000ULL << 2);

    TEST_ASSERT("hs mode lr_d when pmpcfg.R=0 and pmpcfg.L=1 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}



bool load_access_fault_3(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HU，访问没有load权限的PMP区域，pmpcfg.R=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    lb(0x80000100ULL << 2);

    TEST_ASSERT("HU mode lb when pmpcfg.R=0 and pmpcfg.L=1 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();    
    
    lr_d(0x80000100ULL << 2);

    TEST_ASSERT("HU mode lr_d when pmpcfg.R=0 and pmpcfg.L=1 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}

bool load_access_fault_4(){

    TEST_START();

    goto_priv(PRIV_M);
    printf("pmpcfg0=%llx\n",CSRR(CSR_PMPCFG0));
    printf("pmpcfg2=%llx\n",CSRR(CSR_PMPCFG2));

    //pmpcfg.L被设0，当前特权级是M，访问没有load权限的PMP区域，pmpcfg.R=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    
    lb(0x80000100ULL << 2);
    excpt_info();
    TEST_ASSERT("m mode lb successful when pmpcfg.R=0 and pmpcfg.L=0 ",
        excpt.triggered == false
    );

    TEST_END();
}


bool load_access_fault_5(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HS，访问没有load权限的PMP区域，pmpcfg.R=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );           //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );          //pmp1cfg的L位 

    printf("%llx\n",CSRR(CSR_PMPADDR0));
    printf("%llx\n",CSRR(CSR_PMPADDR1));
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    lb(0x80000100ULL << 2);

    TEST_ASSERT("hs mode lb when pmpcfg.R=0 and pmpcfg.L=0 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}


bool load_access_fault_6(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HU，访问没有load权限的PMP区域，pmpcfg.R=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    lb(0x80000100ULL << 2);

    TEST_ASSERT("HU mode lb when pmpcfg.R=0 and pmpcfg.L=0 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}



bool load_access_fault_7(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmpaddr范围内
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    
    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 

    printf("pmpcfg0=%llx \n",CSRR(CSR_PMPCFG0));
    printf("pmpcfg2=%llx \n",CSRR(CSR_PMPCFG2));


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    ld(0x8f000000ULL << 2);

    printf("%d\n",excpt.triggered);
    printf("%d\n",excpt.cause);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmpaddr range leads to laf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}

bool load_access_fault_8(){

    TEST_START();

    goto_priv(PRIV_M);

    //跨越了两个具有不同权限的内存区域，一部分访问成功，一部分失败
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    ld(0x1fffffffeULL << 2);

    TEST_ASSERT("Spanning two memory regions with different permissions, some accessed successfully and some failed leads to laf(ld)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();    
    lr_d(0x1fffffffeULL << 2);

    TEST_ASSERT("Spanning two memory regions with different permissions, some accessed successfully and some failed leads to laf(lr_d)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}


bool load_access_fault_9(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSR_GRWX);
    uintptr_t paddr = phys_page_base(VSR_GRWX);

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有load权限的PMP区域，pmpcfg.R=0(mmu open)

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);


    CSRS(CSR_PMPCFG0,1ULL << 7 );       //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );      //pmp1cfg的L位 

    sfence_vma();
    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();    

    ld(vaddr);

    TEST_ASSERT("hs mode ld when pmpcfg.R=0 and pmpcfg.L=1, llptw leads to LAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}

bool load_access_fault_10(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSURWX_GURWX);
    uintptr_t paddr = phys_page_base(VSURWX_GURWX);

    goto_priv(PRIV_M);

    //pmpcfg.L=0，当前特权级是HU，访问没有load权限的PMP区域，pmpcfg.R=0(mmu open)

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    

    ld(vaddr);

    TEST_ASSERT("hu mode ld when pmpcfg.R=0, llptw leads to LAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_11(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSR_GRWX);
    uintptr_t paddr = phys_page_base(VSR_GRWX);

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有load权限的PMP区域，pmpcfg.R=0(mmu open)

    //模拟linknan的PMA环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);

    //设置目标地址范围权限
    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x800);  
    CSRC(CSR_PMPCFG0, 1ULL << 8); //pmp1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //pmp1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //pmp1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //pmp1cfg.A


    CSRS(CSR_PMPCFG0,1ULL << 15 );      //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,3ULL << 11);   //设置mpp=3
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    prefetch_r(paddr);

    TEST_SETUP_EXCEPT();    

    ld(paddr);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode ld when pmpcfg.R=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    CSRC(CSR_MSTATUS,MSTATUS_MPRV);

    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    CSRC(CSR_MSTATUS,3ULL << 11);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    prefetch_r(vaddr);
    
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRW(CSR_SATP,0);
    TEST_ASSERT("m mode ld when pmpcfg.R=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    hspt_init();
    sfence_vma();
    CSRC(CSR_MSTATUS,3ULL << 11);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    prefetch_r(vaddr);
    TEST_SETUP_EXCEPT();    

    ld(vaddr);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode ld when pmpcfg.R=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=1, llptw leads to LAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool load_access_fault_12(){

    TEST_START();
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式
    CSRW(CSR_PMPADDR0,(uintptr_t)0xffffffff00000);

    //执行ld指令时，在HS模式下未开启mmu，产生preaf
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    ld(0x1fffffffffff8);


    TEST_ASSERT("hs mode ld with preaf fault leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    //执行ld指令时，在M模式下未开启mmu，产生preaf
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();

    ld(0x1fffffffffff8);


    TEST_ASSERT("m mode ld with preaf fault leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    //执行ld指令时，在U模式下未开启mmu，产生preaf
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    ld(0x1fffffffffff8);


    TEST_ASSERT("u mode ld with preaf fault leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );



    TEST_END();
}


bool load_access_fault_13(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);
    sfence_vma();

    //执行ld指令时，在HS模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    
    TEST_ASSERT("hs mode ld,ptw leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool load_access_fault_14(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行ld指令时，在HU模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    
    TEST_ASSERT("hu mode ld,ptw leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_15(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行ld指令时，在M模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    sfence_vma();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("hu mode ld,ptw leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_16(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //s-mode执行ld指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    ld(vaddr);

    TEST_ASSERT("hs mode ld, when ptw takes pte.ppn high bit overflow leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    //s-mode执行ld指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    ld(vaddr);

    TEST_ASSERT("hs mode ld, when llptw takes pte.ppn high bit overflow leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool load_access_fault_17(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行ld指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    ld(vaddr);

    TEST_ASSERT("hu mode ld, when ptw takes pte.ppn high bit overflow leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_18(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行ld指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    ld(vaddr);

    TEST_ASSERT("hu mode ld, when llptw takes pte.ppn high bit overflow leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_19(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //m-mode执行ld指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();

    ld(vaddr); 
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode ld, when ptw takes pte.ppn high bit overflow leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_20(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //m-mode执行ld指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();

    ld(vaddr);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode ld, when llptw takes pte.ppn high bit overflow leads to LAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_21(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(R位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //范围为叶子pte的物理地址范围
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);
    printf("vaddr=%lx, addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    sd(vaddr,0xdeadbeef);

    TEST_SETUP_EXCEPT();
    uint64_t read_data = ld(vaddr);
    TEST_ASSERT("ld successful",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    //修改目标地址范围权限R位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld successful after pma.r removed(without sfence.vma)",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld leads to LAF after pma.r removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_22(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(A位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    //备用pma设置
    CSRW(CSR_PMAADDR2, addr_start >> 2);
    CSRW(CSR_PMAADDR3, addr_end >> 2);  
    CSRC(CSR_PMACFG0, 1ULL << 24); //pma3cfg.R
    CSRC(CSR_PMACFG0, 1ULL << 25); //pma3cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 26); //pma3cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 30); //pma3cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 27); //pma3cfg.A


    goto_priv(PRIV_HS);
    hspt_init();
    sd(vaddr,0xdeadbeef);

    TEST_SETUP_EXCEPT();
    uint64_t read_data = ld(vaddr);
    TEST_ASSERT("ld successful",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld leads to LAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}


bool load_access_fault_23(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(R位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //范围为叶子pte的物理地址范围
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);
    printf("vaddr=%lx, addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    sd(vaddr,0xdeadbeef);

    TEST_SETUP_EXCEPT();
    uint64_t read_data = ld(vaddr);
    TEST_ASSERT("ld successful",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    //修改目标地址范围权限R位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld successful after pma.c removed(without sfence.vma)",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld leads to LAF after pma.c removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}

bool load_access_fault_24(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pmp状态切换(A位)

    //适配linknan的pmp环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMPADDR0, addr_start >> 2);
    CSRW(CSR_PMPADDR1, addr_end >> 2);  
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A

    //备用PMP设置
    CSRW(CSR_PMPADDR2, addr_start >> 2);
    CSRW(CSR_PMPADDR3, addr_end >> 2);  
    CSRC(CSR_PMPCFG0, 1ULL << 24); //PMP3cfg.R
    CSRC(CSR_PMPCFG0, 1ULL << 25); //PMP3cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 26); //PMP3cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 27); //PMP3cfg.A


    goto_priv(PRIV_HS);
    hspt_init();
    sd(vaddr,0xdeadbeef);

    TEST_SETUP_EXCEPT();
    uint64_t read_data = ld(vaddr);
    TEST_ASSERT("ld successful",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld successful after PMP.a change and mismatch (without sfence.vma)",
        excpt.triggered == false &&
        read_data == 0xdeadbeef
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    read_data = ld(vaddr);
    TEST_ASSERT("ld leads to LAF after PMP.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

}


bool load_access_fault_25(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmaaddr范围内
    
    //模拟linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);



    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    ld(0x10000000004ULL << 2);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmaaddr range leads to laf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();    

    ld(0x10000000004ULL << 2);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmaaddr range leads to laf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}


bool load_access_fault_26(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);


    //pmacfg.L被设置，当前特权级是M，访问没有load权限的PMA区域，pmacfg.R=0
    CSRW(CSR_PMACFG0,(uint64_t)0x0);
    
    CSRC(CSR_PMACFG0,1ULL << 8 );      //pma1cfg的R位（清除）
    CSRS(CSR_PMACFG0,1ULL << 9 );      //pma1cfg的W位
    CSRS(CSR_PMACFG0,1ULL << 10 );      //pma1cfg的X位
    CSRS(CSR_PMACFG0,1ULL << 11 );      //pma1cfg的TOR模式

    CSRW(CSR_PMAADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMAADDR1, (uintptr_t)0x81000000);
    // sfence();
    CSRS(CSR_PMACFG0,1ULL << 15 );       //pma1cfg的L位 
    
    TEST_SETUP_EXCEPT();

    lb(0x80000100ULL << 2);    //访问区域内地址

    TEST_ASSERT("m mode lb when pmacfg.R=0 and pmacfg.L=1 leads to LAF",       
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}



bool load_access_fault_27(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的PMA环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //pmacfg.L被设置，当前特权级是HS，访问没有load权限的PMA区域，pmacfg.R=0
    linknan_pma_cfg0_tor_entry1_encoded(0x88000000UL, 0x89000000UL,
        LINKNAN_PMA_W | LINKNAN_PMA_X | LINKNAN_PMA_A_TOR | LINKNAN_PMA_L);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    lb(0x88100000ULL << 2);

    TEST_ASSERT("hs mode lb when pmacfg.R=0 and pmacfg.L=1 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}



bool load_access_fault_28(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的PMA环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //pmacfg.L被设置，当前特权级是HU，访问没有load权限的PMA区域，pmacfg.R=0
    linknan_pma_cfg0_tor_entry1_encoded(0x80000000ULL, 0x81000000ULL,
        LINKNAN_PMA_W | LINKNAN_PMA_X | LINKNAN_PMA_A_TOR | LINKNAN_PMA_L);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    lb(0x80000100ULL << 2);

    TEST_ASSERT("HU mode lb when pmacfg.R=0 and pmacfg.L=1 leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}





bool store_access_fault_1(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    sfence();
    TEST_SETUP_EXCEPT();
    
    sb(0x80000100ULL << 2 , 0x0);

    printf("%d\n",excpt.triggered);
    printf("%d\n",excpt.cause);

    TEST_ASSERT("m mode sb when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool store_access_fault_2(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    sb(0x80000100ULL << 2, 0x0);

    TEST_ASSERT("hs mode sb when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool store_access_fault_3(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HU，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    sb(0x80000100ULL << 2, 0x0);

    TEST_ASSERT("HU mode sb when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool store_access_fault_4(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是M，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    TEST_SETUP_EXCEPT();
    
    sb(0x80000100ULL << 2, 0x0);

    printf("%d\n",excpt.triggered);
    printf("%d\n",excpt.cause);

    TEST_ASSERT("m mode sb successful when pmpcfg.W=0 and pmpcfg.L=0 ",
        excpt.triggered == false
    );


    TEST_END();
}


bool store_access_fault_5(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HS，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    sb(0x80000100ULL << 2, 0x0);

    TEST_ASSERT("hs mode sb when pmpcfg.W=0 and pmpcfg.L=0 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool store_access_fault_6(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HU，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    sb(0x80000100ULL << 2, 0x0);

    TEST_ASSERT("HU mode sb when pmpcfg.W=0 and pmpcfg.L=0 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}



bool store_access_fault_7(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmpaddr范围内
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    sd(0x80300000ULL << 2,0xdeadbeef);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmpaddr range leads to saf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool store_access_fault_8(){

    TEST_START();

    goto_priv(PRIV_M);

    //跨越了两个具有不同权限的内存区域，一部分访问成功，一部分失败
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    sd(0x1fffffffeULL << 2 ,0xdeadbeef);

    TEST_ASSERT("Spanning two memory regions with different permissions, some accessed successfully and some failed leads to saf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}



bool store_access_fault_9(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有store权限的PMP区域，pmpcfg.W=0(mmu open)
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    sfence_vma();
    goto_priv(PRIV_HS);
    hspt_init();
    TEST_SETUP_EXCEPT();    
    
    sw(vaddr, 0x0);

    TEST_ASSERT("hs mode sw when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool store_access_fault_10(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSURWX_GURWX);
    uintptr_t paddr = phys_page_base(VSURWX_GURWX);

    goto_priv(PRIV_M);

    //pmpcfg.L=0，当前特权级是HU，访问没有store权限的PMP区域，pmpcfg.W=0(mmu open)

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    

    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode sd when pmpcfg.W=0, llptw leads to SAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_11(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSRW_GRWX);
    uintptr_t paddr = phys_page_base(VSRW_GRWX);

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有store权限的PMP区域，pmpcfg.W=0(mmu open)


    //适配linknan的PMP环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);

    //设置目标地址范围权限
    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);  
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRC(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A

    
    CSRC(CSR_PMPCFG0,1ULL << 7 );       //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );      //pmp1cfg的L位 

    goto_priv(PRIV_M);
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,3ULL << 11); //mpp设置为M
    TEST_SETUP_EXCEPT();    

    sd(paddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode sd when pmpcfg.W=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=3, llptw leads to SAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);


    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
    CSRC(CSR_MSTATUS,3ULL << 11);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    prefetch_r(vaddr);
    
    TEST_SETUP_EXCEPT();
    sd(paddr,0xdeadbeef);
    CSRW(CSR_SATP,0);
    TEST_ASSERT("m mode sd when pmpcfg.W=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    hspt_init();
    sfence_vma();
    CSRC(CSR_MSTATUS,3ULL << 11);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    TEST_SETUP_EXCEPT();    

    sd(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode sd when pmpcfg.W=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=1, llptw leads to SAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool store_access_fault_12(){

    TEST_START();
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式
    CSRW(CSR_PMPADDR0,(uintptr_t)0xfffffffffffff);

    //执行ld指令时，在HS模式下未开启mmu，产生preaf
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    sd(0x1fffffffffff8, 0xdeadbeef);


    TEST_ASSERT("hs mode sd with preaf fault leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //执行ld指令时，在M模式下未开启mmu，产生preaf
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();

    sd(0x1fffffffffff8, 0xdeadbeef);


    TEST_ASSERT("m mode sd with preaf fault leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //执行ld指令时，在U模式下未开启mmu，产生preaf
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    sd(0x1fffffffffff8, 0xdeadbeef);


    TEST_ASSERT("u mode sd with preaf fault leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool store_access_fault_13(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算store，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);
    sfence_vma();

    //执行sd指令时，在HS模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd,ptw leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}


bool store_access_fault_14(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算store，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行sd指令时，在HU模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu mode sd,ptw leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}



bool store_access_fault_15(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算store，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行sd指令时，在M模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    sfence_vma();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode sd,ptw leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}

bool store_access_fault_16(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //s-mode执行sd指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("hs mode sd, when ptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //s-mode执行sd指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("hs mode sd, when llptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}

bool store_access_fault_17(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行sd指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode sd, when ptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_18(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行ld指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode sd, when llptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_19(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //m-mode执行sd指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();

    sd(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode sd, when ptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_20(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //m-mode执行sd指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();

    sd(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode sd, when llptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_21(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(R位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //范围为最终 store 目标数据页的物理地址范围
    uintptr_t addr_start = phys_page_base(VSRWX_GRWX);
    uintptr_t addr_end = addr_start + PAGE_SIZE;
    printf("vaddr=%lx, addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.Atomic

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("sd successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);

    TEST_ASSERT("sc_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);

    TEST_ASSERT("amoadd_d successful",
        excpt.triggered == false
    );


    //修改目标地址范围权限W位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd leads to SAF after pma.w removed from the final data page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d leads to SAF after pma.w removed from the final data page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d leads to SAF after pma.w removed from the final data page",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_22(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(A位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C 
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.Atomic

    //备用pma设置
    CSRW(CSR_PMAADDR2, addr_start >> 2);
    CSRW(CSR_PMAADDR3, addr_end >> 2);  
    CSRC(CSR_PMACFG0, 1ULL << 24); //pma3cfg.R
    CSRC(CSR_PMACFG0, 1ULL << 25); //pma3cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 26); //pma3cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 30); //pma3cfg.C 
    CSRS(CSR_PMACFG0, 1ULL << 27); //pma3cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 31); //pma3cfg.Atomic


    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful",
        excpt.triggered == false
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd leads to SAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d leads to SAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d leads to SAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool store_access_fault_23(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(C位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t vaddr2 = hs_page_base(VSRWX_GX);
    
    //范围为叶子pte的物理地址范围
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);
    printf("vaddr=%lx, addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.Atomic

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful",
        excpt.triggered == false
    );

    //修改目标地址范围权限C位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful after pma.c removed(without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful after pma.c removed(without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful after pma.c removed(without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd leads to SAF after pma.c removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d leads to SAF after pma.c removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d leads to SAF after pma.c removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}


bool store_access_fault_24(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //PMP状态切换(A位)

    //适配linknan的PMP环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMPADDR0, addr_start >> 2);
    CSRW(CSR_PMPADDR1, addr_end >> 2);  
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A

    //备用PMP设置
    CSRW(CSR_PMPADDR2, addr_start >> 2);
    CSRW(CSR_PMPADDR3, addr_end >> 2);  
    CSRC(CSR_PMPCFG0, 1ULL << 24); //PMP3cfg.R
    CSRC(CSR_PMPCFG0, 1ULL << 25); //PMP3cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 26); //PMP3cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 27); //PMP3cfg.A


    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful",
        excpt.triggered == false
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful after PMP.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful after PMP.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful after PMP.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd leads to SAF after PMP.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d leads to SAF after PMP.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d leads to SAF after PMP.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}



bool store_access_fault_25(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmaaddr范围内
    
    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);



    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    sd(0x10000000004ULL << 2,0xdeadbeef);

    TEST_ASSERT("(S-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to Saf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();    

    sd(0x10000000004ULL << 2,0xdeadbeef);

    TEST_ASSERT("(M-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to Saf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool store_access_fault_26(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);


    //pmacfg.L被设置，当前特权级是M，访问没有store权限的PMA区域，pmacfg.W=0
    CSRW(CSR_PMACFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMACFG0,1ULL << 8 );      //pma1cfg的R位
    CSRC(CSR_PMACFG0,1ULL << 9 );      //pma1cfg的W位（清除）
    CSRS(CSR_PMACFG0,1ULL << 10 );      //pma1cfg的X位
    CSRS(CSR_PMACFG0,1ULL << 11 );      //pma1cfg的TOR模式

    CSRW(CSR_PMAADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMAADDR1, (uintptr_t)0x81000000);
    // sfence();
    CSRS(CSR_PMACFG0,1ULL << 15 );       //pma1cfg的L位 
    
    TEST_SETUP_EXCEPT();

    sd(0x80000100ULL << 2, 0xdeadbeef);    //访问区域内地址

    TEST_ASSERT("m mode sd when pmacfg.W=0 and pmacfg.L=1 leads to SAF",       
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}



bool store_access_fault_27(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //pmacfg.L被设置，当前特权级是HS，访问没有store权限的PMA区域，pmacfg.W=0

    CSRS(CSR_PMACFG0,1ULL << 0 );      //pma0cfg的R位
    CSRS(CSR_PMACFG0,1ULL << 1 );      //pma0cfg的W位
    CSRS(CSR_PMACFG0,1ULL << 2 );      //pma0cfg的X位
    CSRS(CSR_PMACFG0,1ULL << 6 );      //pma0cfg.C位
    CSRS(CSR_PMACFG0,1ULL << 3 );      //pma0cfg的TOR模式

    CSRS(CSR_PMACFG0,1ULL << 8 );      //pma1cfg的R位
    CSRC(CSR_PMACFG0,1ULL << 9 );      //pma1cfg的W位
    CSRS(CSR_PMACFG0,1ULL << 10 );      //pma1cfg的X位
    CSRS(CSR_PMACFG0,1ULL << 14);       //pma1cfg.C位
    CSRS(CSR_PMACFG0,1ULL << 11 );      //pma1cfg的TOR模式

    CSRW(CSR_PMAADDR0, 0x88000000UL);
    CSRW(CSR_PMAADDR1, 0x89000000UL);


    CSRS(CSR_PMACFG0,1ULL << 7 );       //pma0cfg的L位 
    CSRS(CSR_PMACFG0,1ULL << 15 );      //pma1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    sd(0x88100000ULL << 2, 0xdeadbeef);    //访问区域内地址

    TEST_ASSERT("hs mode sd when pmacfg.W=0 and pmacfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}



bool store_access_fault_28(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //pmacfg.L被设置，当前特权级是HU，访问没有store权限的PMA区域，pmacfg.W=0
    linknan_pma_cfg0_tor_entry1_encoded(0x80000000ULL, 0x81000000ULL,
        LINKNAN_PMA_R | LINKNAN_PMA_X | LINKNAN_PMA_A_TOR | LINKNAN_PMA_L);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    sd(0x80000100ULL << 2, 0xdeadbeef);

    TEST_ASSERT("HU mode sd when pmacfg.W=0 and pmacfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool store_access_fault_29(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pmp环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);


    //当前特权级是HS，访问没有store权限的PMA区域，pmpcfg.W=0

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, 0x88000000UL);
    CSRW(CSR_PMPADDR1, 0x89000000UL);


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    sd(0x88100000ULL << 2, 0xdeadbeef);    //访问区域内地址

    TEST_ASSERT("hs mode sd when pmpcfg.W=0 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //当 pmpaddri-1 ≥ pmpaddri 且 pmpcfgi.A=TOR 时,不匹配该项
    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, 0x89000000UL);
    CSRW(CSR_PMPADDR1, 0x88000000UL);
    TEST_SETUP_EXCEPT();    

    sd(0x88100000ULL << 2, 0xdeadbeef);    //访问区域内地址
    TEST_ASSERT("hs mode sd when pmpaddri-1 ≥ pmpaddri  and pmpcfgi.A=TOR leads to no SAF",
        excpt.triggered == false
    );
    TEST_END();
}

bool amo_access_fault_1(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    TEST_SETUP_EXCEPT();
    
    amoadd_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("m mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    
    sc_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("m mode sc_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool amo_access_fault_2(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    amoadd_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("hs mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();    
    
    sc_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("hs mode sc_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_3(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HU，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    amoadd_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("HU mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();    
    
    sc_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("HU mode sc_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool amo_access_fault_4(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是M，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    TEST_SETUP_EXCEPT();
    
    amoadd_d(0x80000100ULL << 2 , 0xdeadbeef);

    printf("%d\n",excpt.triggered);
    printf("%d\n",excpt.cause);

    TEST_ASSERT("m mode amoadd_d successful when pmpcfg.W=0 and pmpcfg.L=0 ",
        excpt.triggered == false
    );


    TEST_END();
}


bool amo_access_fault_5(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HS，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    amoadd_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("hs mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=0 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_6(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HU，访问没有store权限的PMP区域，pmpcfg.W=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    amoadd_d(0x80000100ULL << 2 , 0xdeadbeef);

    TEST_ASSERT("HU mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=0 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool amo_access_fault_7(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmpaddr范围内
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    amoand_d(0x80000000ULL << 2 ,0xdeadbeef);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmpaddr range leads to store guest fault(amo)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();    
    
    sc_d(0x80000000ULL << 2 ,0xdeadbeef);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmpaddr range leads to store guest fault(sc_d)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_8(){

    TEST_START();

    goto_priv(PRIV_M);

    //跨越了两个具有不同权限的内存区域，一部分访问成功，一部分失败
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    amoand_d(0x1fffffffeULL << 2 ,0xdeadbeef);

    TEST_ASSERT("Spanning two memory regions with different permissions, some accessed successfully and some failed leads to saf(amo)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_9(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有store权限的PMP区域，pmpcfg.W=0(mmu open)
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    sfence_vma();
    goto_priv(PRIV_HS);
    hspt_init();
    TEST_SETUP_EXCEPT();    
    
    amoadd_d(vaddr , 0xdeadbeef);

    TEST_ASSERT("hs mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();    
    
    sc_d(vaddr , 0xdeadbeef);

    TEST_ASSERT("hs mode sc_d when pmpcfg.W=0 and pmpcfg.L=1 leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool amo_access_fault_10(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSURWX_GURWX);
    uintptr_t paddr = phys_page_base(VSURWX_GURWX);

    goto_priv(PRIV_M);

    //pmpcfg.L=0，当前特权级是HU，访问没有store权限的PMP区域，pmpcfg.W=0(mmu open)

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    

    amoadd_d(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode amo when pmpcfg.W=0, llptw leads to SAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool amo_access_fault_11(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSRW_GRWX);
    uintptr_t paddr = phys_page_base(VSRW_GRWX);

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有store权限的PMP区域，pmpcfg.W=0(mmu open)

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);


    CSRC(CSR_PMPCFG0,1ULL << 7 );       //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );      //pmp1cfg的L位 


    goto_priv(PRIV_M);
    sfence_vma();


    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,3ULL << 11); //mpp设置为M
    TEST_SETUP_EXCEPT();    

    amoadd_d(paddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=3, llptw leads to SAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    CSRC(CSR_MSTATUS,MSTATUS_MPRV);


    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
    CSRC(CSR_MSTATUS,3ULL << 11);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    prefetch_r(vaddr);
    
    TEST_SETUP_EXCEPT();
    sd(paddr,0xdeadbeef);
    CSRW(CSR_SATP,0);
    TEST_ASSERT("m mode sd when pmpcfg.W=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    hspt_init();
    sfence_vma();
    CSRC(CSR_MSTATUS,3ULL << 11);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    TEST_SETUP_EXCEPT();    

    amoadd_d(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode amoadd_d when pmpcfg.W=0 and pmpcfg.L=1 and mstatus.mprv=1 and mstatus.mpp=1, llptw leads to SAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool amo_access_fault_12(){

    TEST_START();
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式
    CSRW(CSR_PMPADDR0,(uintptr_t)0xfffffffffffff);


    //执行amo指令时，在HS模式下未开启mmu，产生preaf
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    amoadd_d(0x1fffffffffff8, 0xdeadbeef);


    TEST_ASSERT("hs mode amo with preaf fault leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //执行amo指令时，在M模式下未开启mmu，产生preaf
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();

    amoadd_d(0x1fffffffffff8, 0xdeadbeef);


    TEST_ASSERT("m mode amo with preaf fault leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //执行amo指令时，在U模式下未开启mmu，产生preaf
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    amoadd_d(0x1fffffffffff8, 0xdeadbeef);


    TEST_ASSERT("u mode amo with preaf fault leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}


bool amo_access_fault_13(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算amo，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行amo指令时，在HS模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode amo,ptw leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_14(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算amo，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行amo指令时，在HU模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hu mode amo,ptw leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}


bool amo_access_fault_15(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算amo，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行amo指令时，在M模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    sfence_vma();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);
    TEST_ASSERT("m mode amo,ptw leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}

bool amo_access_fault_16(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //s-mode执行amoadd_d指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    amoadd_d(vaddr,0xdeadbeef);

    TEST_ASSERT("hs mode amoadd_d, when ptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //s-mode执行amoadd_d指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    amoadd_d(vaddr,0xdeadbeef);

    TEST_ASSERT("hs mode amoadd_d, when llptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}

bool amo_access_fault_17(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行amoadd_d指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    amoadd_d(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode amoadd_d, when ptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool amo_access_fault_18(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行ld指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    amoadd_d(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode amoadd_d, when llptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool amo_access_fault_19(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //m-mode执行amoadd_d指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();

    amoadd_d(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode amoadd_d, when ptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool amo_access_fault_20(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //m-mode执行amoadd_d指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);

    TEST_SETUP_EXCEPT();

    amoadd_d(vaddr,0xdeadbeef);
    CSRW(CSR_SATP,0);

    TEST_ASSERT("m mode amoadd_d, when llptw takes pte.ppn high bit overflow leads to SAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}


bool amo_access_fault_21(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(atomic位)：当前项目中 PMA Atomic 位不直接生效，AMO/LR/SC 允许性由 cacheability 决定。

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[4][VSRWX_GRWX];
    uintptr_t addr_end = ((uintptr_t)&hspt[4][VSRWX_GRWX] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C 
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.Atomic

    goto_priv(PRIV_HS);
    hspt_init();
    sc_d(vaddr,0xdeadbeef);

    TEST_SETUP_EXCEPT();
    uint64_t read_data = lr_d(vaddr);
    TEST_ASSERT("lr_d successful",
        excpt.triggered == false 
    );

    //仅修改目标地址范围权限 atomic 位拉低，cache 位仍保持开启。
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 13); //pma1cfg.Atomic

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    read_data = lr_d(vaddr);
    TEST_ASSERT("lr_d remains legal after only pma.atomic is removed while pma.cache stays enabled",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    read_data = amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d remains legal after only pma.atomic is removed while pma.cache stays enabled",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    read_data = sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d remains legal after only pma.atomic is removed while pma.cache stays enabled",
        excpt.triggered == false
    );

    TEST_END();

}



bool amo_access_fault_22(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(A位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C 
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.ATOMIC

    //备用pma设置
    CSRW(CSR_PMAADDR2, addr_start >> 2);
    CSRW(CSR_PMAADDR3, addr_end >> 2);  
    CSRC(CSR_PMACFG0, 1ULL << 24); //pma3cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 25); //pma3cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 26); //pma3cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 30); //pma3cfg.C 
    CSRS(CSR_PMACFG0, 1ULL << 27); //pma3cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.ATOMIC


    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("lr_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful",
        excpt.triggered == false
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("lr_d successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("lr_d leads to LAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d leads to SAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d leads to SAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}


bool amo_access_fault_23(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(C位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t vaddr2 = hs_page_base(VSRWX_GX);
    
    //范围为叶子pte的物理地址范围
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);
    printf("vaddr=%lx, addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.ATOMIC

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("lr_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful",
        excpt.triggered == false
    );

    //修改目标地址范围权限C位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("lr_d successful after pma.C removed(without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d successful after pma.C removed(without sfence.vma)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d successful after pma.C removed(without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("lr_d leads to LAF after pma.C removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("sc_d leads to SAF after pma.C removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr,0xdeadbeef);
    TEST_ASSERT("amoadd_d leads to SAF after pma.C removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}



bool amo_access_fault_24(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmaaddr范围内
    
    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);



    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    lr_d(0x10000000004ULL << 2);

    TEST_ASSERT("(S-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();    

    sc_d(0x10000000004ULL << 2,0xdeadbeef);

    TEST_ASSERT("(S-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();    

    amoadd_d(0x10000000004ULL << 2,0xdeadbeef);

    TEST_ASSERT("(S-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();    

    lr_d(0x10000000004ULL << 2);

    TEST_ASSERT("(M-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();    
    sc_d(0x10000000004ULL << 2,0xdeadbeef);

    TEST_ASSERT("(M-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();    

    amoadd_d(0x10000000004ULL << 2,0xdeadbeef);

    TEST_ASSERT("(M-mode)An invalid address range was accessed and is not in the correct pmaaddr range leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_25(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);


    //pmacfg.L被设置，当前特权级是M，访问没有atomic权限的PMA区域，pmacfg.atomic=0
    CSRW(CSR_PMACFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMACFG0,1ULL << 8 );      //pma1cfg的R位
    CSRS(CSR_PMACFG0,1ULL << 9 );      //pma1cfg的W位
    CSRS(CSR_PMACFG0,1ULL << 10 );      //pma1cfg的X位
    CSRS(CSR_PMACFG0,1ULL << 14);       //pma1cfg.C位
    CSRS(CSR_PMACFG0,1ULL << 11 );      //pma1cfg的TOR模式
    CSRC(CSR_PMACFG0,1ULL << 13 );      //pma1cfg的ATOMIC位

    CSRW(CSR_PMAADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMAADDR1, (uintptr_t)0x81000000);
    // sfence();
    CSRS(CSR_PMACFG0,1ULL << 15 );       //pma1cfg的L位 
    
    TEST_SETUP_EXCEPT();

    amoadd_d(0x80000100ULL << 2, 0xdeadbeef);    //访问区域内地址

    TEST_ASSERT("m mode amoadd_d when pmacfg.atomic=0 and pmacfg.L=1 leads to SAF",       
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}



bool amo_access_fault_26(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //pmacfg.L被设置，当前特权级是HS，访问PMA.C=0/atomic=0区域
    linknan_pma_cfg0_tor_entry1_encoded(0x88000000UL, 0x89000000UL,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X |
        LINKNAN_PMA_A_TOR | LINKNAN_PMA_L);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    amoadd_d(0x88100000ULL << 2, 0xdeadbeef);    //访问区域内地址

    TEST_ASSERT("hs mode amoadd_d when pmacfg.ATOMIC=0 and pmacfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}



bool amo_access_fault_27(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //pmacfg.L被设置，当前特权级是HU，访问PMA.C=0/atomic=0区域
    linknan_pma_cfg0_tor_entry1_encoded(0x80000000ULL, 0x81000000ULL,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X |
        LINKNAN_PMA_A_TOR | LINKNAN_PMA_L);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    amoadd_d(0x80000100ULL << 2, 0xdeadbeef);

    TEST_ASSERT("HU mode amoadd_d when pmacfg.ATOMIC=0 and pmacfg.L=1 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );


    TEST_END();
}


bool amo_access_fault_28(){

    TEST_START();

    goto_priv(PRIV_M);

    //适配linknan的pmp环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);


    //当前特权级是HS，访问没有store权限的PMA区域，pmpcfg.W=0

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRC(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, 0x88000000UL);
    CSRW(CSR_PMPADDR1, 0x89000000UL);


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    

    amoadd_d(0x88100000ULL << 2, 0xdeadbeef);    //访问区域内地址

    TEST_ASSERT("hs mode amoadd_d when pmpcfg.W=0 leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    //当 pmpaddri-1 ≥ pmpaddri 且 pmpcfgi.A=TOR 时,不匹配该项
    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, 0x89000000UL);
    CSRW(CSR_PMPADDR1, 0x88000000UL);
    TEST_SETUP_EXCEPT();    

    amoadd_d(0x88100000ULL << 2, 0xdeadbeef);    //访问区域内地址
    TEST_ASSERT("hs mode amoadd_d when pmpaddri-1 ≥ pmpaddri  and pmpcfgi.A=TOR leads to no SAF",
        excpt.triggered == false
    );
    TEST_END();
}

bool instruction_access_fault_1(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是M，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    TEST_SETUP_EXCEPT();
    
    TEST_EXEC_EXCEPT(0x80000100ULL << 2);

    TEST_ASSERT("m mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=1 leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );


    TEST_END();
}

bool instruction_access_fault_2(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(0x80000100ULL << 2);

    TEST_ASSERT("hs mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=1 leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );


    TEST_END();
}


bool instruction_access_fault_3(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HU，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(0x80000100ULL << 2);

    TEST_ASSERT("HU mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=1 leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );


    TEST_END();
}


bool instruction_access_fault_4(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是M，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式
    
    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x81000000UL >> 2);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x82000000UL >> 2);

    uintptr_t vaddr = 0x81000100UL;
    *(uint32_t*)vaddr = 0x00008067;  // ret指令

    TEST_SETUP_EXCEPT();
    
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址

    TEST_ASSERT("m mode fetch instruction successful when pmpcfg.X=0 and pmpcfg.L=0 ",
        excpt.triggered == false
    );

    TEST_END();
}


bool instruction_access_fault_5(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HS，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(0x80000100ULL << 2);

    TEST_ASSERT("hs mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=0 leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );


    TEST_END();
}


bool instruction_access_fault_6(){

    TEST_START();

    goto_priv(PRIV_M);

    //pmpcfg.L被设0，当前特权级是HU，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式
    
    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRC(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 


    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(0x80000100ULL << 2);

    TEST_ASSERT("HU mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=0 leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    TEST_END();
}


bool instruction_access_fault_7(){

    TEST_START();

    goto_priv(PRIV_M);

    //访问了无效的地址范围，不在正确的pmpaddr范围内
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x90000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(0x9f000000ULL << 2);

    TEST_ASSERT("An invalid address range was accessed and is not in the correct pmpaddr range leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );


    TEST_END();
}


bool instruction_access_fault_8(){

    TEST_START();

    goto_priv(PRIV_M);

    //跨越了两个具有不同权限的内存区域，一部分访问成功，一部分失败
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, (uintptr_t)0x80000000);
    CSRW(CSR_PMPADDR1, (uintptr_t)0x81000000);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(0x1fffffffeULL << 2);

    TEST_ASSERT("Spanning two memory regions with different permissions, some accessed successfully and some failed leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );


    TEST_END();
}

bool instruction_access_fault_9(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSX_GRWX);
    uintptr_t paddr = phys_page_base(VSX_GRWX);


    goto_priv(PRIV_M);

    //pmpcfg.L=0，当前特权级是HS，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0(mmu open)
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    sfence_vma();
    goto_priv(PRIV_HS);
    hspt_init();
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=0 leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    goto_priv(PRIV_M);

    //pmpcfg.L被设置，当前特权级是HS，访问没有执行权限的PMP区域获取指令，pmpcfg.X=0(mmu open)
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式


    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    CSRS(CSR_PMPCFG0,1ULL << 7 );     //pmp0cfg的L位 
    CSRS(CSR_PMPCFG0,1ULL << 15 );       //pmp1cfg的L位 

    sfence_vma();
    goto_priv(PRIV_HS);
    hspt_init();
    TEST_SETUP_EXCEPT();    
    
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode fetch instruction when pmpcfg.X=0 and pmpcfg.L=1 leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );
    

    TEST_END();
}

bool instruction_access_fault_10(){

    TEST_START();
    uintptr_t vaddr = vs_page_base(VSURWX_GURWX);
    uintptr_t paddr = phys_page_base(VSURWX_GURWX);

    goto_priv(PRIV_M);

    //pmpcfg.L=0，当前特权级是HU，访问没有执行权限的PMP区域获取指令，pmpcfg.W=0(mmu open)

    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRC(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRW(CSR_PMPADDR0, paddr >> 2);
    CSRW(CSR_PMPADDR1, (paddr >> 2) + 0x400);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    

    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hu mode instr fetch when pmpcfg.W=0, llptw leads to IAF(MMU open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}


bool instruction_access_fault_11(){

    TEST_START();
    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式
    CSRW(CSR_PMPADDR0,(uintptr_t)0xfffffffffffff);


    //取指时，在HS模式下未开启mmu，产生preaf
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(0x1ffffffffffff);


    TEST_ASSERT("hs mode instr fetch with preaf fault leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    //取指时，在M模式下未开启mmu，产生preaf
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(0x1ffffffffffff);


    TEST_ASSERT("m mode instr fetch with preaf fault leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    //取指时，在HU模式下未开启mmu，产生preaf
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(0x1ffffffffffff);


    TEST_ASSERT("hu mode instr fetch with preaf fault leads to IAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    TEST_END();
}


bool instruction_access_fault_12(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算instr，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);
    sfence_vma();

    //执行instr fetch，在HS模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode instr fetch,ptw leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    TEST_END();
}



bool instruction_access_fault_13(){

    TEST_START();
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);

    CSRS(CSR_PMPCFG0,1ULL << 0 );      //pmp0cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 1 );      //pmp0cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 2 );      //pmp0cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 3 );      //pmp0cfg的TOR模式

    CSRC(CSR_PMPCFG0,1ULL << 8 );      //pmp1cfg的R位
    CSRS(CSR_PMPCFG0,1ULL << 9 );      //pmp1cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 10 );      //pmp1cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 11 );      //pmp1cfg的TOR模式

    CSRS(CSR_PMPCFG0,1ULL << 16 );      //pmp2cfg的R位(因为就算instr，每个页表都是load出来的)
    CSRS(CSR_PMPCFG0,1ULL << 17 );      //pmp2cfg的W位
    CSRS(CSR_PMPCFG0,1ULL << 18 );      //pmp2cfg的X位
    CSRS(CSR_PMPCFG0,1ULL << 19 );      //pmp2cfg的TOR模式

#ifdef sv39
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[1][0];
#endif

#ifdef sv48
    uintptr_t hspt0_paddr = (uintptr_t)&hspt[2][0];
#endif

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPADDR0, (hspt0_paddr >> 2));
    CSRW(CSR_PMPADDR1, (hspt0_paddr >> 2) + 0x400);
    CSRW(CSR_PMPADDR2, (uint64_t) -1);

    //执行instr fetch，在HU模式下,开启mmu，ptw对应pmp检查引发af
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hu mode instr fetch,ptw leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}



bool instruction_access_fault_14(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //s-mode执行instr fetch指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode instr fetch, when ptw takes pte.ppn high bit overflow leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    //s-mode执行instr fetch指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode instr fetch, when llptw takes pte.ppn high bit overflow leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

    TEST_END();
}

bool instruction_access_fault_15(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行instr fetch指令时，当ptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_ptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hu mode instr fetch, when ptw takes pte.ppn high bit overflow leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}

bool instruction_access_fault_16(){

    TEST_START();

    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);

    //u-mode执行instr fetch指令时，当llptw取的pte.ppn高位溢出(目前只支持36位ppn，高位不能为非0)
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hu mode instr fetch, when llptw takes pte.ppn high bit overflow leads to IAF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}

bool instruction_access_fault_17(){

    TEST_START();

    TEST_ASSERT("invalid testcase disabled: MPRV only changes data access privilege, so M-mode instruction fetch through TEST_EXEC_EXCEPT cannot test S-mode PTW high-bit overflow and may not return for self-check",
        true
    );

    TEST_END();
}

bool instruction_access_fault_18(){

    TEST_START();

    TEST_ASSERT("invalid testcase disabled: MPRV only changes data access privilege, so M-mode instruction fetch through TEST_EXEC_EXCEPT cannot test S-mode LLPTW high-bit overflow and may not return for self-check",
        true
    );

    TEST_END();
}


bool instruction_access_fault_19(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(R位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    //范围为叶子pte的物理地址范围
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);
    printf("vaddr=%lx, addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("instr fetch successful",
        excpt.triggered == false
    );

    //修改目标地址范围权限X位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 8); //pma1cfg.X

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("instr fetch successful after pma.x removed(without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("instr fetch leads to IAF after pma.x removed(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}

bool instruction_access_fault_20(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //pma状态切换(A位)

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMAADDR0, addr_start >> 2);
    CSRW(CSR_PMAADDR1, addr_end >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    //备用pma设置
    CSRW(CSR_PMAADDR2, addr_start >> 2);
    CSRW(CSR_PMAADDR3, addr_end >> 2);  
    CSRC(CSR_PMACFG0, 1ULL << 24); //pma3cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 25); //pma3cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 26); //pma3cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 30); //pma3cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 27); //pma3cfg.A


    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("instr fetch successful",
        excpt.triggered == false
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("instr fetch successful after pma.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("instr fetch leads to IAF after pma.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}


bool instruction_access_fault_21(){

    TEST_START();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    //PMP状态切换(A位)

    //适配linknan的PMP环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t addr_start = (uintptr_t)&hspt[2][0];
    uintptr_t addr_end = ((uintptr_t)&hspt[2][0] + 0x1000);

    //设置目标地址范围权限
    CSRW(CSR_PMPADDR0, addr_start >> 2);
    CSRW(CSR_PMPADDR1, addr_end >> 2);  
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A
    //备用PMP设置
    CSRW(CSR_PMPADDR2, addr_start >> 2);
    CSRW(CSR_PMPADDR3, addr_end >> 2);  
    CSRC(CSR_PMPCFG0, 1ULL << 24); //PMP3cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 25); //PMP3cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 26); //PMP3cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 27); //PMP3cfg.A


    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("instr fetch successful",
        excpt.triggered == false
    );

    //修改目标地址范围匹配模式
    goto_priv(PRIV_M);
    CSRC(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("instr fetch successful after PMP.a change and mismatch (without sfence.vma)",
        excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("instr fetch leads to IAF after PMP.a change and mismatch(with sfence.vma)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF
    );

}


bool pmp_granularity_test(){

    TEST_START();

    goto_priv(PRIV_M);

    //测试pmp细粒度
    
    CSRW(CSR_PMPCFG0,(uint64_t)0x0);
    CSRW(CSR_PMPADDR0,(uint64_t)0x0);


    printf("pmpcfg0=%llx\n",CSRR(CSR_PMPCFG0));
    // printf("pmpaddr0=%llx\n",CSRR(CSR_PMPADDR0));

    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    // CSRS(CSR_PMPADDR0, 1ULL << 9);

    printf("pmpaddr0=%llx\n",CSRR(CSR_PMPADDR0));
    


    TEST_END();
}

bool pma_csr_test(){

    TEST_START();

    goto_priv(PRIV_M);

    //测试pma csr
    CSRW(CSR_PMAADDR1, (uint64_t)0x8000001F);
    CSRW(CSR_PMAADDR2, (uint64_t)0x81000000);

    CSRC(CSR_PMAADDR1, (uint64_t)0x0000001F);

    printf("pmaaddr1=%llx\n",CSRR(CSR_PMAADDR1));
    printf("pmaaddr2=%llx\n",CSRR(CSR_PMAADDR2));

    TEST_END();
}
