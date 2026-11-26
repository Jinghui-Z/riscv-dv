#include <rvh_test.h>
#include <page_tables.h>
#include <csrs.h> 

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

bool instruction_page_fault_1(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    uintptr_t vaddr;
    uintptr_t paddr ;

    //s mode执行取指指令时，pte.v=1 x=1 u=0 
    goto_priv(PRIV_HS);
    vaddr = hs_page_base(VSRWX_GRWX);
    paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)vaddr = 0x00008067;  // ret指令
    sfence_vma();
    TEST_SETUP_EXCEPT();
    
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction when x=1 v=1 successful",
        excpt.triggered == false
    );

    //s mode 执行取指指令时，pte有保留位被设置
    goto_priv(PRIV_M);
    hspt_add_reserved_bits();
    sfence_vma();
    goto_priv(PRIV_HS);
    
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 

    TEST_ASSERT("hs mode fetche instruction when pte has reserved bits leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //执行取指指令时，pte.v=0
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);
    vaddr = vs_page_base(VSI_GI);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 

    TEST_ASSERT("hs mode fetche instruction when pte.v=0 leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //执行取指指令时，pte.x=0
    goto_priv(PRIV_M);
    sfence_vma();

    goto_priv(PRIV_HS);
    vaddr = vs_page_base(VSRW_GRW);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 

    TEST_ASSERT("hs mode fetche instruction when pte.x=0 leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //m mode(mmu open)执行取指指令时，pte.v=1 x=1 u=0
    vaddr = hs_page_base(VSRWX_GRWX);
    paddr = phys_page_base(VSRWX_GRWX);
    goto_priv(PRIV_M);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_HS);
    ((void(*)(void))vaddr)();  
    TEST_ASSERT("hs mode fetche instruction when x=1 v=1 successful",
        excpt.triggered == false
    );

    TEST_END();
}

bool instruction_page_fault_2_1(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    uintptr_t vaddr;
    uintptr_t paddr ;

    //s mode执行取指指令时，pte.v=1 x=1 u=0
    goto_priv(PRIV_HS);
    vaddr = hs_page_base(VSRWX_GRWX);
    paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)vaddr = 0x00008067;  // ret指令

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hu mode fetche instruction when x=1 v=1 successful",
        excpt.triggered == false
    );

}

bool instruction_page_fault_2_2(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    uintptr_t vaddr;
    uintptr_t paddr ;

    //u mode执行取指指令时，pte.v=0 x=0 u=1
    goto_priv(PRIV_HS);
    vaddr = hs_page_base(VSRWX_GRWX);
    paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)vaddr = 0x00008067;  // ret指令

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    //执行取指指令时，pte.v=0
    vaddr = vs_page_base(VSI_GI);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 

    TEST_ASSERT("hu mode fetche instruction when pte.v=0 leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}

bool instruction_page_fault_2_3(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    uintptr_t vaddr;
    uintptr_t paddr ;

    //u mode执行取指指令时，pte.v=1 x=0 u=1
    goto_priv(PRIV_HS);
    vaddr = hs_page_base(VSRWX_GRWX);
    paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)vaddr = 0x00008067;  // ret指令

    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    //执行取指指令时，pte.x=0
    vaddr = vs_page_base(VSRW_GRW);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 

    TEST_ASSERT("hu mode fetche instruction when pte.x=0 leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}


bool instruction_page_fault_3(){

    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    uintptr_t vaddr1 = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr1 = phys_page_base(VSRWX_GRWX);

    uintptr_t vaddr2 = hs_page_base(VSURWX_GRWX);
    uintptr_t paddr2 = phys_page_base(VSURWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    *(uint32_t*)paddr1 = 0x00008067;  // ret指令
    *(uint32_t*)paddr2 = 0x00008067;  // ret指令
    sfence_vma();

    //HS mode 执行取指指令时，在HS模式下访问a=1的页
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr1)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction when a=1 successful",
        excpt.triggered == false
    );

    //S mode 执行取指指令时，pte.a从1变为0 pte.rwx=1，不执行sfence.vma
    goto_priv(PRIV_M);
    hspt_del_A();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr1)();   // 使用函数调用，编译器自动处理返回地址 

    TEST_ASSERT("hs mode fetche instruction when a=0 successful without sfence",
        excpt.triggered == false
    );


    //S mode 执行取指指令时，pte.a从1变为0 pte.rwx=1，执行sfence.vma
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr1); 

    TEST_ASSERT("hs mode fetche instruction when a=0 cause to IPF with sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}

bool instruction_page_fault_4(){

    TEST_START();
    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);
    uintptr_t paddr = phys_page_base(VSURWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    hspt_u_mode_allow();
    sfence_vma();

    //U mode 执行取指指令时，pte.a=1 pte.ruwx=1，不产生异常
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hu mode fetche instruction when a=1 successful",
        excpt.triggered == false
    );

}

bool instruction_page_fault_5(){

    TEST_START();
    uintptr_t vaddr = hs_page_base(VSURWX_GRWX);
    uintptr_t paddr = phys_page_base(VSURWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    hspt_u_mode_allow();

    //U mode 执行取指指令时，pte.a=0 pte.ruwx=1，引发IPF
    goto_priv(PRIV_M);
    hspt_del_A();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 

    TEST_ASSERT("hu mode fetche instruction when a=0 cause to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}

bool instruction_page_fault_6(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    //执行取指指令时，在HS模式下开启mmu，产生prepf
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    TEST_SETUP_EXCEPT();

#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    TEST_EXEC_EXCEPT(bad_vaddr);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    TEST_EXEC_EXCEPT(bad_vaddr);
#endif


    TEST_ASSERT("hs mode instr fetch with prepf fault leads to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    TEST_END();
}

bool instruction_page_fault_7(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    //执行取指指令时，在HU模式下开启mmu，产生prepf
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    TEST_EXEC_EXCEPT(bad_vaddr);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    TEST_EXEC_EXCEPT(bad_vaddr);
#endif


    TEST_ASSERT("hu mode instr fetch with prepf fault leads to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

}


bool instruction_page_fault_8(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    //执行取指指令时，在S模式下访问 pte.u=1 的页表项
    uintptr_t vaddr = hs_page_base(VSURWX_GURWX);
    uintptr_t paddr = phys_page_base(VSURWX_GURWX);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr); 
    TEST_ASSERT("hs mode fetche instruction of u mode leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //执行取指指令时，在U模式下访问 pte.u=1 的页表项
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址

    TEST_ASSERT("hu mode fetche instruction of u mode successful",
        excpt.triggered == false
    );
}

bool instruction_page_fault_9(){

    TEST_START();
    uintptr_t vaddr;
    uintptr_t paddr ;
    vaddr = hs_page_base(VSX_GX);
    paddr = phys_page_base(VSX_GX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    //s mode执行取指指令时，pte.v=1 x=1 u=0 
    goto_priv(PRIV_HS);
    hspt_init();
    TEST_SETUP_EXCEPT();    
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    
    TEST_ASSERT("hs mode fetche instruction when x=1 v=1 successful",
        excpt.triggered == false
    );

   //u mode执行取指指令时，pte.v=1 x=1 u=1 
    goto_priv(PRIV_M);
    vaddr = hs_page_base(VSUX_GX);
    paddr = phys_page_base(VSUX_GX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();    
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    
    TEST_ASSERT("hu mode fetche instruction when x=1 v=1 successful",
        excpt.triggered == false
    );

}

bool instruction_page_fault_10(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    //执行取指指令时，在S模式下访问pte.g=1的页表项，不产生异常
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)(); 
    TEST_ASSERT("hs mode fetche instruction successful with G bit page",
        excpt.triggered == false
    );

    //执行取指指令时，在U模式下访问pte.u=1的页表项，不产生异常
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址

    TEST_ASSERT("hu mode fetche instruction of successful with G bit page",
        excpt.triggered == false
    );
}

bool instruction_page_fault_11(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    sfence_vma();

    //执行取指指令时，在S模式下访问pte.g=1 pte.x=0的页表项，引发IPF
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetche instruction successful with G bit page and x=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //执行取指指令时，在U模式下访问pte.u=1 pte.x=0的页表项，引发IPF
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hu mode fetche instruction successful with G bit page and x=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );
}

bool instruction_page_fault_12(){

    TEST_START();


    goto_priv(PRIV_M);
    uintptr_t vaddr = hs_page_base(VSRW_GURWX);

    CSRS(CSR_MEDELEG,1ULL << CAUSE_IPF);
    //执行取指指令时，在S模式下pte.x=0的页表项，引发IPF，代理到S-mode
    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetche instruction cause to ipf with x=0 and medeleg.ipf=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.priv == PRIV_HS
    );
}

bool instruction_page_fault_13(){

    TEST_START();

    //测试大页地址非对齐访问导致的IPF（sv48中的2MB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x140000000;      //sv48中2MB大页基物理地址:hspt[3][0]

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    uintptr_t paddr = SUPERPAGE_SIZE(1) * 4;
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction with 2MB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetche instruction with 2MB superpage misalign leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_align_boundary_setup();
    paddr = SUPERPAGE_SIZE(1) * 4 + 0x200000;
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode fetche instruction with 2MB superpage successful(boundary align)",
        excpt.triggered == false
    );

}

bool instruction_page_fault_14(){

    TEST_START();

    //测试大页地址非对齐访问导致的IPF（sv48中的1GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0xc0000000;      //sv48中1GB大页基物理地址:hspt[1][3]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, (uint64_t)-1);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    uintptr_t paddr = SUPERPAGE_SIZE(1) * 3;
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HS);
    
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction with 1GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_1GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetche instruction with 1GB superpage misalign leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    hspt_1GB_superpage_align_boundary_setup();
    paddr = SUPERPAGE_SIZE(1) * 3 + 0x40000000;
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction with 1GB superpage successful(boundary align)",
        excpt.triggered == false
    );


    TEST_END();
}

bool instruction_page_fault_15(){

    TEST_START();

    #ifdef sv48

    //测试大页地址非对齐访问导致的IPF（sv48中的512GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x8000000000;      //sv48中512GB大页基物理地址:hspt[0][1]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, (uint64_t)-1);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    set_superpage_512G();
    uintptr_t paddr = SUPERPAGE_SIZE(0);
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction with 512GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetche instruction with 512GB superpage misalign leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_align_boundary_setup();
    paddr = SUPERPAGE_SIZE(0) + 0x8000000000;
    *(uint32_t*)paddr = 0x00008067;  // ret指令
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetche instruction with 512GB superpage successful(boundary align)",
        excpt.triggered == false
    );

    TEST_END();
    #endif
}

bool instruction_page_fault_16(){

    TEST_START();

    //执行取指指令时，1、2、3级页表项分别 v=0 页面中执行(sv48)
    goto_priv(PRIV_M);
    hspt_init();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //执行取指指令时，三级页表项无效，pte.v=0
    hspt[2][0] &= ~PTE_V;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode fetche instruction cause to IPF when third level pte.v=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //执行取指指令时，二级页表项无效，pte.v=0
    goto_priv(PRIV_M);
    hspt_init();
    
    hspt[1][4] &= ~PTE_V;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode fetche instruction cause to IPF when second level pte.v=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //执行取指指令时，一级页表项无效，pte.v=0
    goto_priv(PRIV_M);
    hspt_init();

    hspt[0][0] &= ~PTE_V;
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    CSRW(CSR_SATP,0);

    TEST_ASSERT("hs mode fetche instruction cause to IPF when first level pte.v=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}

bool instruction_page_fault_17(){

    TEST_START();

    //执行取指指令时，1、2、3级页表项分别 pte.reserved != 0 页面中执行(sv48)
    goto_priv(PRIV_M);
    hspt_init();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    //执行取指指令时，三级页表项 pte.reserved != 0
    hspt[2][0] |=  (0x3FULL << 54);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode fetche instruction cause to IPF when third level pte.reserved != 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //执行取指指令时，二级页表项 pte.reserved != 0
    goto_priv(PRIV_M);
    hspt_init();
    
    hspt[1][4] |=  (0x3FULL << 54);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("hs mode fetche instruction cause to IPF when second level pte.reserved != 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


    //执行取指指令时，一级页表项 pte.reserved != 0
    goto_priv(PRIV_M);
    hspt_init();

    hspt[0][0] |=  (0x3FULL << 54);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    CSRW(CSR_SATP,0);

    TEST_ASSERT("hs mode fetche instruction cause to IPF when first level pte.reserved != 0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}

bool instruction_page_fault_18(){
    //取指指令，将pte.x=1改为0(进程不切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction when pte.x=1 successful",
        excpt.triggered == false
    );

    //取指指令，将pte.x=1改为0(进程不切换)，不执行sfence.vma，不产生异常
    goto_priv(PRIV_M);
    hspt_del_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 successful without sfence",
        excpt.triggered == false
    );

    //取指指令，将pte.x=1改为0(进程不切换)，执行sfence.vma，引发IPF
    //rs1为对应vaddr，rs2为对应ASID，执行SFECNE.VMA
    sfence_vma_rs(vaddr,0);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 leads to IPF with sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}

bool instruction_page_fault_19(){
    //取指指令，将pte.x=1改为0(进程不切换),pte为global页
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();
    hspt_add_G();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction when pte.x=1 successful",
        excpt.triggered == false
    );

    //s mode执行取指指令，将pte.g=1不变 pte.x=1改为0(进程不切换)，不执行sfence.vma，不产生异常
    goto_priv(PRIV_M);
    hspt_del_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    //不执行sfence.vma，仍然可以成功执行
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=1 successful without sfence",
        excpt.triggered == false
    );

    //s mode执行取指指令，将pte.g=1不变 pte.x=1改为0(进程不切换)，执行sfence.vma（rs1为对应vaddr，rs2为非x0且非对应ASID），不产生异常
    sfence_vma_rs(vaddr,1234);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=1 successful with sfence and non-asid",
        excpt.triggered == false
    );


}

bool instruction_page_fault_20(){
    //取指指令，将pte.x=0改为1(进程不切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRW_GRW);
    uintptr_t paddr = phys_page_base(VSRW_GRW);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //s mode执行取指指令，将pte.x=0改为1(进程不切换)，不执行sfence.vma，引发IPF
    goto_priv(PRIV_M);
    hspt_add_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    //不执行sfence.vma
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x from 0 to 1 without sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //s mode执行取指指令，将pte.x=0改为1(进程不切换)，执行sfence.vma（rs1为对应vaddr，rs2为对应ASID）
    sfence_vma_rs(vaddr,0);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction successful when pte.x=0 from 0 to 1 with sfence",
        excpt.triggered == false
    );


}

bool instruction_page_fault_21(){
    //取指指令，将pte.x=0改为1(进程不切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRW_GRW);
    uintptr_t paddr = phys_page_base(VSRW_GRW);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();
    hspt_add_G();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    hspt_add_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    //不执行sfence.vma，仍然可以成功执行
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=1 successful without sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //rs1为对应vaddr，rs2为非x0且非对应ASID，执行SFECNE.VMA ，仍然可以成功执行
    sfence_vma_rs(vaddr,1234);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=1 successful with sfence and non-asid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}

bool instruction_page_fault_22(){
    //取指指令，将pte.x=1改为0(进程切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction when pte.x=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);

    //修改satp.asid为新asid
    uint64_t satp = CSRR(CSR_SATP);
    satp = (satp & ~0x0ffff00000000000) | (0x1234ULL << 44);
    CSRW(CSR_SATP, satp);

    hspt_del_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    //不执行SFENCE.VMA，再访存同一个地址
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 and satp.asid change to new value without sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //rs1任意值，rs2任意值，执行SFECNE.VMA，再访存同一个地址
    sfence_vma_rs(0xdeadbeef,0xdead);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 and satp.asid change to new value with sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}

bool instruction_page_fault_23(){
    //取指指令，pte.g=1 将pte.x=1改为0(进程切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();
    hspt_add_G();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction when pte.x=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    //修改satp.asid为新asid
    uint64_t satp = CSRR(CSR_SATP);
    satp = (satp & ~0x0ffff00000000000) | (0x1234ULL << 44);
    CSRW(CSR_SATP, satp);

    hspt_del_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    //不执行sfence.vma，仍然可以成功执行
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=0 and satp.asid change to new value successful without sfence",
        excpt.triggered == false
    );

    //对应遍历路径G位为1，rs1为对应vaddr，rs2=x0，执行SFENCE.VMA，再访存同一地址
    sfence_vma_rs(vaddr,(uint64_t)-1);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 pte.g=0 and satp.asid change to new value with sfence(rs2=x0 rs1=vaddr)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}

bool instruction_page_fault_24(){
    //取指指令，将pte.x=0改为1(进程切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRW_GRW);
    uintptr_t paddr = phys_page_base(VSRW_GRW);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    //修改satp.asid为新asid
    uint64_t satp = CSRR(CSR_SATP);
    satp = (satp & ~0x0ffff00000000000) | (0x1234ULL << 44);
    CSRW(CSR_SATP, satp);
    hspt_add_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    //不执行sfence.vma
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x from 0 to 1 and satp.asid change to new value without sfence",
        excpt.triggered == false
    );

    //rs1为对应vaddr，rs2为对应ASID，执行SFECNE.VMA
    sfence_vma_rs(vaddr,0);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();   // 使用函数调用，编译器自动处理返回地址
    TEST_ASSERT("hs mode fetch instruction successful when pte.x=0 from 0 to 1 and satp.asid change to new value with sfence",
        excpt.triggered == false
    );


}

bool instruction_page_fault_25(){
    //取指指令，pte.g=1，将pte.x=0改为1(进程切换)
    TEST_START();
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    uintptr_t vaddr = hs_page_base(VSRW_GRW);
    uintptr_t paddr = phys_page_base(VSRW_GRW);
    *(uint32_t*)paddr = 0x00008067;  // ret指令

    goto_priv(PRIV_HS);
    hspt_init();
    hspt_add_G();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction cause to IPF when pte.x=0 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    //修改satp.asid为新asid
    uint64_t satp = CSRR(CSR_SATP);
    satp = (satp & ~0x0ffff00000000000) | (0x1234ULL << 44);
    CSRW(CSR_SATP, satp);
    hspt_add_X();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    //不执行sfence.vma
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=0 and satp.asid change to new value successful without sfence",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    //对应遍历路径G位为1，rs1为对应vaddr，rs2非x0，执行SFENCE.VMA，再访存同一地址
    sfence_vma_rs(vaddr,0x1234ULL);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("hs mode fetch instruction when pte.x=0 pte.g=0 and satp.asid change to new value successful with sfence and non-asid",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );


}



bool load_page_fault_1(){

    TEST_START();

    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    goto_priv(PRIV_HS); 
    hspt_init();


    uintptr_t vaddr;
    uintptr_t addr;

    goto_priv(PRIV_HS);
    sfence_vma();

    //执行load指令时，pte.v=0
    TEST_SETUP_EXCEPT();

    vaddr = vs_page_base(VSI_GI);
    lb(vaddr);
    TEST_ASSERT("hs mode load when pte.v=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();

    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d when pte.v=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    //执行load、lr指令时，叶子页表项可写但不可读，pte.r=0&&pte.w=1
    vaddr = vs_page_base(VSW_GI);
    printf("hspt[4][VSW_GI] pte: 0x%lx\n", (hspt[4][VSW_GI]));
    TEST_SETUP_EXCEPT();
    lb(vaddr);
    TEST_ASSERT("hs mode load when pte.r=0 and pte.w=1 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr when pte.r=0 and pte.w=1 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    //s mode 执行load指令时，pte有保留位被设置
    vaddr = vs_page_base(VSR_GRWX);

    goto_priv(PRIV_M);
    hspt_add_reserved_bits();
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ld(vaddr); 

    TEST_ASSERT("hs mode ld when pte has reserved bits leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr); 

    TEST_ASSERT("hs mode lr_d when pte has reserved bits leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );


    //执行ld指令时，设置了sstatus.MXR=0，只能从读页面读取，如果此时pte.r=0
    vaddr = vs_page_base(VSX_GRWX);
    hspt_init();

    CSRC(CSR_SSTATUS, SSTATUS_MXR);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ld(vaddr);   
    TEST_ASSERT("hs mode load when sstatus.mxr=0 and pte.r=0 and pte.x=1 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    vaddr = vs_page_base(VSV_GV);
    CSRC(CSR_SSTATUS, SSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);   
    TEST_ASSERT("hs mode load when sstatus.mxr=0 and pte.r=0 and pte.x=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    vaddr = vs_page_base(VSR_GURWX);

    CSRC(CSR_SSTATUS, SSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr); 
    TEST_ASSERT("hs mode load success when sstatus.mxr=0 and pte.r=1 and pte.x=0",
        excpt.triggered == false
    );

    vaddr = vs_page_base(VSRWX_GURWX);

    CSRC(CSR_SSTATUS, SSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr); 
    TEST_ASSERT("hs mode load success when sstatus.mxr=0 and pte.r=1 and pte.x=1",
        excpt.triggered == false
    );

    //执行ld指令时，设置了sstatus.MXR=1，能从可读或可执行页面读取，

    goto_priv(PRIV_HS);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vaddr = vs_page_base(VSV_GV);

    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    ld(vaddr);
    TEST_ASSERT("hs mode load when sstatus.mxr=1 and pte.r=0 and pte.x=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    vaddr = vs_page_base(VSR_GUR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode load success when sstatus.mxr=1 and pte.r=1 and pte.x=0",
        excpt.triggered == false
    );

    vaddr = vs_page_base(VSX_GUX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode load success when sstatus.mxr=1 and pte.r=0 and pte.x=1",
        excpt.triggered == false
    );

    vaddr = vs_page_base(VSRWX_GUX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode load success when sstatus.mxr=1 and pte.r=1 and pte.x=1",
        excpt.triggered == false
    );

    CSRC(CSR_SSTATUS, SSTATUS_MXR);



    TEST_END();

}

bool load_page_fault_2(){

    TEST_START();


    goto_priv(PRIV_HS);
    hspt_init();


    goto_priv(PRIV_M);
    CSRC(CSR_SSTATUS, SSTATUS_SUM);
    sfence_vma();

    goto_priv(PRIV_HS);

    //s mode执行ld指令时，设置了sstatus.SUM=0，在S模式下访问pte.u=1的页表项
    TEST_SETUP_EXCEPT();
    uintptr_t addr = hs_page_base(VSURWX_GURWX);

    
    ld(addr);
    TEST_ASSERT("hs mode ld u mode page when vsstatus.sum=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_END();
}

bool load_page_fault_3(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    sfence_vma();

    //s mode执行ld指令时，设置了sstatus.SUM=1，在S模式下访问pte.u=1的页表项，不产生异常

    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_VS);
    
    uintptr_t addr = hs_page_base(VSURWX_GURWX);
    TEST_SETUP_EXCEPT();
    ld(addr);

    TEST_ASSERT("hs mode ld u mode page when sstatus.sum=1 successful",
        excpt.triggered == false
    );

    //执行ld指令时，设置了sstatus.SUM=1，在m模式下访存(MPP=S)
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);

    TEST_ASSERT("m mode ld(mpp=S) when PTE.U=0 mstatus.mprv=1 and mstatus.sum=1 successful",
        excpt.triggered == false
    );

    vaddr = hs_page_base(VSURWX_GRWX);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);

    TEST_ASSERT("m mode ld(mpp=S) when PTE.U=1 mstatus.mprv=1 and mstatus.sum=1 successful",
        excpt.triggered == false
    );

    //执行ld指令时，设置了sstatus.SUM=0，在m模式下访存(MPP=S)
    vaddr = hs_page_base(VSRWX_GRWX);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    TEST_ASSERT("m mode ld(mpp=S) when PTE.U=0 mstatus.mprv=1 and mstatus.sum=0 successful",
        excpt.triggered == false
    );

    vaddr = hs_page_base(VSURWX_GRWX);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    TEST_ASSERT("m mode ld(mpp=S) when PTE.U=1 mstatus.mprv=1 and mstatus.sum=0 cause to lpf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

   //执行ld指令时，设置了sstatus.SUM=1，在m模式下访存(MPP=U)
    vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);
    hspt_u_mode_allow_no_leaf();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    TEST_ASSERT("m mode ld(mpp=U) when PTE.U=0 mstatus.mprv=1 and mstatus.sum=1 cause to lpf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    vaddr = hs_page_base(VSURWX_GRWX);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);

    TEST_ASSERT("m mode ld(mpp=U) when PTE.U=1 mstatus.mprv=1 and mstatus.sum=1 successful",
        excpt.triggered == false
    );

    //执行ld指令时，设置了sstatus.SUM=0，在m模式下访存(MPP=U)
    vaddr = hs_page_base(VSRWX_GRWX);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    TEST_ASSERT("m mode ld(mpp=U) when PTE.U=0 mstatus.mprv=1 and mstatus.sum=0 cause to lpf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    vaddr = hs_page_base(VSURWX_GRWX);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_SUM);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    TEST_ASSERT("m mode ld(mpp=U) when PTE.U=1 mstatus.mprv=1 and mstatus.sum=0 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    hspt_init();
    sfence_vma();

    TEST_END();
}


bool load_page_fault_4(){

    TEST_START();


    goto_priv(PRIV_HS);
    hspt_init();

    //执行ld指令时，在HS模式下开启mmu，产生prepf
    TEST_SETUP_EXCEPT();
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    ld(bad_vaddr);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    ld(bad_vaddr);
#endif



    TEST_ASSERT("hs mode ld with prepf fault leads to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_END();
}

bool load_page_fault_5(){

    TEST_START();


    goto_priv(PRIV_HS);
    hspt_init();

    //执行ld指令时，在HU模式下开启mmu，产生prepf
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    ld(bad_vaddr);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    ld(bad_vaddr);
#endif


    TEST_ASSERT("hu mode ld with prepf fault leads to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

}

bool load_page_fault_6(){
    TEST_START();

    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HS);
    hspt_init();
    uintptr_t vaddr = hs_page_base(VSR_GR);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs ld when pte.a=1 successful without sfence.vma",
        excpt.triggered == false
    );

    //hs执行ld指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs ld when pte.a=0 successful without sfence.vma",
        excpt.triggered == false
    );

    //hs执行ld指令时，PTE.A=0
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs ld when pte.a=0 leads to LPF with sfence.vma",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    vaddr = hs_page_base(VSUR_GR);

    //hu执行ld指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hu ld when pte.a=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );
    

}

bool load_page_fault_7(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSUR_GR);

    //hu执行ld指令时，PTE.A=1
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hu ld when pte.a=1 successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hu lr_d when pte.a=1 successful",
        excpt.triggered == false
    );

}

bool load_page_fault_8(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSR_GR);

    //m执行ld指令时，PTE.A=1(mpp=S)
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    // hspt_del_A();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("m ld when pte.a=1 successful(mmu open)",
        excpt.triggered == false
    );
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    
    //m执行ld指令时，PTE.A=1,PTE.X=1,PTE.R=0,mstatus.mxr=1
    vaddr = hs_page_base(VSX_GX);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=1,PTE.R=0,mstatus.mxr=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行ld指令时，PTE.X=1,PTE.R=1,mstatus.mxr=1
    vaddr = hs_page_base(VSX_GX);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=1,PTE.R=1,mstatus.mxr=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行ld指令时，PTE.A=1,PTE.X=0,PTE.R=1,mstatus.mxr=1
    vaddr = hs_page_base(VSX_GX);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=0,PTE.R=1,mstatus.mxr=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行ld指令时，PTE.A=1,PTE.X=0,PTE.R=0,mstatus.mxr=1
    vaddr = hs_page_base(VSV_GV);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=0,PTE.R=0,mstatus.mxr=1 cause to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );


    //m执行ld指令时，PTE.A=1,PTE.X=0,PTE.R=1,mstatus.mxr=0
    vaddr = hs_page_base(VSR_GX);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=0,PTE.R=1,mstatus.mxr=0 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行ld指令时，PTE.X=1,PTE.R=1,mstatus.mxr=0
    vaddr = hs_page_base(VSRX_GX);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=1,PTE.R=1,mstatus.mxr=0 successful(mmu open)",
        excpt.triggered == false
    );


    //m执行ld指令时，PTE.A=1,PTE.X=1,PTE.R=0,mstatus.mxr=0
    vaddr = hs_page_base(VSX_GX);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=1,PTE.R=0,mstatus.mxr=0 cause to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    //m执行ld指令时，PTE.A=1,PTE.X=0,PTE.R=0,mstatus.mxr=0
    vaddr = hs_page_base(VSV_GV);
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRC(CSR_MSTATUS,MSTATUS_MXR);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPP);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m ld when pte.a=1,PTE.X=0,PTE.R=0,mstatus.mxr=0 cause to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

}

bool load_page_fault_9(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSR_GR);

    goto_priv(PRIV_HS);
    hspt_init();

    //m执行ld指令时，PTE.A=1
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m ld when pte.a=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行ld指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m ld when pte.a=0 lead to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    //


}


bool load_page_fault_10(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSR_GURWX);
    uintptr_t paddr = phys_page_base(VSR_GURWX);
    sfence_vma();

    //s mode执行load指令时，PTE.G=1 PTE.R=1，不产生异常
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld successful with G bit page",
        excpt.triggered == false
    );

    //u mode执行load指令时，PTE.G=1 PTE.R=1，不产生异常
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hu mode ld successful with G bit page",
        excpt.triggered == false
    );
}

bool load_page_fault_11(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    sfence_vma();

    //s mode执行load指令时，PTE.G=1 PTE.R=0，引发LPF
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld leads to lpf with G bit page and r=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    //u mode执行load指令时，PTE.G=1 PTE.R=0，引发LPF
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    ld(vaddr);

    TEST_ASSERT("hu mode ld leads to lpf with G bit page and r=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );
}

bool load_page_fault_12(){

    TEST_START();


    goto_priv(PRIV_M);
    uintptr_t vaddr = hs_page_base(VSX_GURWX);

    CSRS(CSR_MEDELEG,1ULL << CAUSE_LPF);
    //执行load指令时，在HS模式下发生LPF代理到S-mode下
    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld cause to lpf with r=0 and medeleg.lpf=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.priv == PRIV_HS
    );
}

bool load_page_fault_13(){

    TEST_START();

    //测试大页地址非对齐访问导致的LPF（sv48中的2MB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x140000000;      //sv48中2MB大页基物理地址:hspt[3][0]

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 2MB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 2MB superpage misalign leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 2MB superpage successful(boundary align)",
        excpt.triggered == false
    );

}

bool load_page_fault_14(){

    TEST_START();

    //测试大页地址非对齐访问导致的LPF（sv48中的1GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0xc0000000;      //sv48中1GB大页基物理地址:hspt[1][3]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    linknan_pma_cfg0_tor_entry1_encoded(0x0, (uint64_t)-1,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X |
        LINKNAN_PMA_A_TOR | LINKNAN_PMA_C);

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 1GB superpage successful",
        excpt.triggered == false
    );

    // goto_priv(PRIV_M);
    // hspt_1GB_superpage_misalign_setup();
    // sfence_vma();

    // goto_priv(PRIV_HS);
    // TEST_SETUP_EXCEPT();
    // ld(vaddr);
    // TEST_ASSERT("hs mode ld with 1GB superpage misalign leads to LPF",
    //     excpt.triggered == true &&
    //     excpt.cause == CAUSE_LPF
    // );

    // goto_priv(PRIV_M);
    // hspt_1GB_superpage_align_boundary_setup();
    // sfence_vma();

    // goto_priv(PRIV_HS);
    // TEST_SETUP_EXCEPT();
    // ld(vaddr);
    // TEST_ASSERT("hs mode ld with 1GB superpage successful(boundary align)",
    //     excpt.triggered == false
    // );


    // TEST_END();
}

bool load_page_fault_15(){

    #ifdef sv48
    TEST_START();

    //测试大页地址非对齐访问导致的LPF（sv48中的512GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x8000000000;      //sv48中512GB大页基物理地址:hspt[0][1]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, (uint64_t)-1);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    set_superpage_512G();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 512GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 512GB superpage misalign leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("hs mode ld with 512GB superpage successful(boundary align)",
        excpt.triggered == false
    );


    TEST_END();
    #endif
}


bool store_page_fault_1(){

    TEST_START();

    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HS);
    hspt_init();


    uintptr_t vaddr;
    uintptr_t addr;

    goto_priv(PRIV_HS);
    sfence_vma();

    //执行store指令时，pte.v=0
    TEST_SETUP_EXCEPT();

    addr = hs_page_base(VSI_GI);
    sd(addr, 0xdeadbeef);
    TEST_ASSERT("hs mode sd when pte.v=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //执行store/sc指令时，叶子页表项可写但不可读，pte.r=0&&pte.w=1
    vaddr = vs_page_base(VSW_GI);
    printf("hspt[4][VSW_GI] pte: 0x%lx\n", (hspt[4][VSW_GI]));
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode store when pte.r=0 and pte.w=1 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc when pte.r=0 and pte.w=1 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //s mode 执行取指指令时，pte有保留位被设置
    vaddr = vs_page_base(VSRW_GRWX);

    goto_priv(PRIV_M);
    hspt_add_reserved_bits();
    sfence_vma();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef); 

    TEST_ASSERT("hs mode sd when pte has reserved bits leads to IPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );


    //s mode执行store指令时，pte.w=0，引发SPF
    hspt_init();
    sfence_vma();
    TEST_SETUP_EXCEPT();
    addr = hs_page_base(VSRX_GRWX);

    sd(addr, 0xdeadbeef);
    TEST_ASSERT("hs mode sd when and pte.w=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //s mode执行store指令时，pte.w=1，不产生异常
    sfence_vma();
    TEST_SETUP_EXCEPT();
    addr = hs_page_base(VSRW_GURW);

    sd(addr, 0xdeadbeef);
    TEST_ASSERT("hs mode sd successful when pte.w=1",
        excpt.triggered == false
    );

    TEST_END();
}

bool store_page_fault_2(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    print_page_table_type();    //看当前页表类型是sv48还是sv39
    CSRC(CSR_SSTATUS, SSTATUS_SUM);
    sfence_vma();

    goto_priv(PRIV_HS);

    //执行sd指令时，设置了sstatus.SUM=0，在S模式下访问U模式可以访问的页表项
    TEST_SETUP_EXCEPT();
    uintptr_t vaddr = hs_page_base(VSURWX_GURWX);

    
    sd(vaddr, 0xdeadbeef);
    TEST_ASSERT("hs mode sd u mode page when sstatus.sum=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );


    TEST_END();
}

bool store_page_fault_3(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    //执行sd指令时，设置了sstatus.SUM=1，在S模式下访问U模式可以访问的页表项
    goto_priv(PRIV_M);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    sfence_vma();

    goto_priv(PRIV_HS);
    uintptr_t addr = hs_page_base(VSURWX_GURWX);
    
    TEST_SETUP_EXCEPT();
    sd(addr, 0xdeadbeef);
    TEST_ASSERT("hs mode sd u mode page when sstatus.sum=1 successful",
        excpt.triggered == false
    );

    TEST_END();
}

bool store_page_fault_4(){

    TEST_START();


    goto_priv(PRIV_HS);
    hspt_init();

    //执行sd指令时，在HS模式下开启mmu，产生prepf
    TEST_SETUP_EXCEPT();
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    sd(bad_vaddr, 0xdeadbeef);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    sd(bad_vaddr, 0xdeadbeef);
#endif

    TEST_ASSERT("hs mode sd with prepf fault leads to SPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_END();
}

bool store_page_fault_5(){

    TEST_START();


    goto_priv(PRIV_HS);
    hspt_init();

    //执行sd指令时，在HU模式下开启mmu，产生prepf
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    sd(bad_vaddr, 0xdeadbeef);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    sd(bad_vaddr, 0xdeadbeef);
#endif

    TEST_ASSERT("hu mode sd with prepf fault leads to SPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool store_page_fault_6(){
    TEST_START();
    goto_priv(PRIV_HS);
    hspt_init();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs sd when pte.a=1 successful",
        excpt.triggered == false
    );

    //hs执行sd指令时，PTE.A=0,不执行sfence.vma
    goto_priv(PRIV_M);
    hspt_del_A();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs sd when pte.a=0 successful without sfence.vma",
        excpt.triggered == false
    );

    //hs执行sd指令时，PTE.A=0,执行sfence.vma

    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs sd when pte.a=0 leads to SPF with sfence.vma",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    vaddr = hs_page_base(VSURW_GR);

    //hu执行sd指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu sd when pte.a=0 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool store_page_fault_7(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSURW_GR);

    //hu执行sd指令时，PTE.A=1
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu sd when pte.a=1 successful",
        excpt.triggered == false
    );

}

bool store_page_fault_8(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    //m执行sd指令时，PTE.A=1(mpp=s)
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    // hspt_del_A();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("m sd when pte.a=1 successful(mmu open)",
        excpt.triggered == false
    );

}

bool store_page_fault_9(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    goto_priv(PRIV_HS);
    hspt_init();

    //m执行sd指令时，PTE.A=1
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m sd when pte.a=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行sd指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();
    sfence_vma();

    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m sd when pte.a=0 lead to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool store_page_fault_10(){
    TEST_START();
    goto_priv(PRIV_HS);
    hspt_init();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    //hs执行sd指令时，PTE.D=1
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs sd when pte.D=1 successful",
        excpt.triggered == false
    );

    //hs执行sd指令时，PTE.D=0，不执行sfence.vma
    goto_priv(PRIV_M);
    hspt_del_D();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs sd when pte.D=0 leads to SPF without sfence.vma",
        excpt.triggered == false
    );

    //hs执行sd指令时，PTE.D=0,执行sfence.vma

    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs sd when pte.D=0 leads to SPF with sfence.vma",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    vaddr = hs_page_base(VSURW_GR);

    //hu执行sd指令时，PTE.D=0
    goto_priv(PRIV_M);
    hspt_del_D();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu sd when pte.a=0 leads to SPF with ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //hu执行sd指令，pte.u=0
    vaddr = hs_page_base(VSRWX_GR);

    goto_priv(PRIV_M);
    hspt_u_mode_allow_no_leaf();
    hspt_init();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu sd when pte.u=0 leads to SPF with ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );
}

bool store_page_fault_11(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSURW_GR);

    //hu执行sd指令时，PTE.D=1
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu sd when pte.D=1 successful",
        excpt.triggered == false
    );

}

bool store_page_fault_12(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    //m执行sd指令时，PTE.D=1
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("m sd when pte.D=1 successful(mmu open)",
        excpt.triggered == false
    );

}

bool store_page_fault_13(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    goto_priv(PRIV_HS);
    hspt_init();

    //m执行sd指令时，PTE.D=1
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m sd when pte.D=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m mode执行store指令时，PTE.D=1，引发SPF（mpp=S）
    goto_priv(PRIV_M);
    hspt_del_D();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m sd when pte.D=0 lead to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool store_page_fault_14(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    sfence_vma();

    //s mode执行store指令时，PTE.G=1，不产生异常
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd successful with G bit page",
        excpt.triggered == false
    );

    //u mode执行store指令时，PTE.G=1，不产生异常
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hu mode sd successful with G bit page",
        excpt.triggered == false
    );
}

bool store_page_fault_15(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSR_GURWX);
    uintptr_t paddr = phys_page_base(VSR_GURWX);
    sfence_vma();

    //s mode执行store指令时，PTE.G=1 PTE.W=0，引发SPF
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd leads to spf with G bit page and w=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //u mode执行store指令时，PTE.G=1 PTE.W=0，引发SPF
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode sd leads to spf with G bit page and w=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );
}

bool store_page_fault_16(){

    TEST_START();


    goto_priv(PRIV_M);
    uintptr_t vaddr = hs_page_base(VSX_GURWX);

    CSRS(CSR_MEDELEG,1ULL << CAUSE_SPF);
    //执行store指令时，在HS模式下发生SPF代理到S-mode下
    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd cause to spf with r=0 and medeleg.spf=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.priv == PRIV_HS
    );
}

bool store_page_fault_17(){

    TEST_START();

    //测试大页地址非对齐访问导致的SPF（sv48中的2MB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x140000000;      //sv48中2MB大页基物理地址:hspt[3][0]

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 2MB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 2MB superpage misalign leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 2MB superpage successful(boundary align)",
        excpt.triggered == false
    );

}

bool store_page_fault_18(){

    TEST_START();

    //测试大页地址非对齐访问导致的SPF（sv48中的1GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0xc0000000;      //sv48中1GB大页基物理地址:hspt[1][3]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, (uint64_t)-1);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 1GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_1GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 1GB superpage misalign leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    goto_priv(PRIV_M);
    hspt_1GB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 1GB superpage successful(boundary align)",
        excpt.triggered == false
    );


    TEST_END();
}

bool store_page_fault_19(){

    TEST_START();

    #ifdef sv48

    //测试大页地址非对齐访问导致的SPF（sv48中的512GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x8000000000;      //sv48中512GB大页基物理地址:hspt[0][1]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    linknan_pma_cfg0_tor_entry1_encoded(0x0, (uint64_t)-1,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X |
        LINKNAN_PMA_A_TOR | LINKNAN_PMA_C);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    set_superpage_512G();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 512GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 512GB superpage misalign leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sd with 512GB superpage successful(boundary align)",
        excpt.triggered == false
    );

    TEST_END();
    #endif

}

bool amo_page_fault_1(){

    TEST_START();

    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HS);
    hspt_init();


    uintptr_t vaddr;
    uintptr_t addr;

    uint64_t value = 0xdeadbeef;  
    goto_priv(PRIV_HS);
    sfence_vma();

    //s mode执行原子指令时，pte.v=0，引发SPF
    TEST_SETUP_EXCEPT();

    addr = hs_page_base(VSI_GI);
    value = amoadd_w(addr,value);
    TEST_ASSERT("hs mode execute amoadd_w when pte.v=0 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );


    //s mode执行原子指令时，pte.w=0，引发SPF
    sfence_vma();

    TEST_SETUP_EXCEPT();
    addr = hs_page_base(VSRX_GRWX);

    value = amoand_d(addr,value);
    TEST_ASSERT("hs mode execute amoand_d when pte.w=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //s mode执行原子指令时，pte.w=1，不产生异常
    sfence_vma();

    TEST_SETUP_EXCEPT();
    addr = hs_page_base(VSRWX_GURWX);

    value = amomax_d(addr,value);
    TEST_ASSERT("hs mode execute amomax_d successful when pte.w=0",
        excpt.triggered == false
    );

    TEST_END();
}

bool amo_page_fault_2(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, SSTATUS_SUM);
    sfence_vma();

    goto_priv(PRIV_HS);
    //执行原子指令时，设置了mstatus.SUM=0，在S模式下访问PTE.U=1的页
    TEST_SETUP_EXCEPT();
    uintptr_t addr = hs_page_base(VSURWX_GURWX);

    uint64_t value = amomin_d(addr,value);
    TEST_ASSERT("hs mode execute amomin_d of u mode page when mstatus.sum=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_END();
}

bool amo_page_fault_3(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
 

    //执行原子指令时，设置了mstatus.SUM=1，在S模式下访问PTE.U=1的页
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_SUM);
    sfence_vma();

    uintptr_t addr = hs_page_base(VSURWX_GURWX);

    goto_priv(PRIV_HS);
    
    TEST_SETUP_EXCEPT();
    uint64_t value = amoadd_w(addr,value);
    TEST_ASSERT("hs mode execute amoadd_w of u mode page when sstatus.sum=1 successful",
        excpt.triggered == false
    );

    TEST_END();
}


bool amo_page_fault_4(){

    TEST_START();


    goto_priv(PRIV_HS);
    hspt_init();

    //执行原子指令时，在S模式下开启mmu，产生prepf
    TEST_SETUP_EXCEPT();
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    uint64_t value = amoadd_w(bad_vaddr, value);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    uint64_t value = amoadd_w(bad_vaddr, value);
#endif

    TEST_ASSERT("hs mode amo with prepf fault leads to SPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_END();
}

bool amo_page_fault_5(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();

    //执行原子指令时，在U模式下开启mmu，产生prepf
    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
#ifdef sv48
    uint64_t bad_vaddr = vaddr;
    uint64_t bit47 = (vaddr >> 47) & 1ULL;

    if (bit47)
        bad_vaddr &= ~(1ULL << 48);   // 如果 bit47=1, 就把 bit48=0
    else
        bad_vaddr |= (1ULL << 48);    // 如果 bit47=0, 就把 bit48=1

    uint64_t value = amoadd_w(bad_vaddr, value);
#endif


#ifdef sv39
    uint64_t bad_vaddr = vaddr;
    uint64_t bit38 = (vaddr >> 38) & 1ULL;

    if (bit38)
        bad_vaddr &= ~(1ULL << 39);   // 如果 bit38=1, 就把 bit39=0
    else
        bad_vaddr |= (1ULL << 39);    // 如果 bit38=0, 就把 bit39=1

    uint64_t value = amoadd_w(bad_vaddr, value);
#endif

    TEST_ASSERT("hu mode amo with prepf fault leads to SPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}


bool amo_page_fault_6(){
    TEST_START();
    goto_priv(PRIV_HS);
    hspt_init();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    //s mode执行原子指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();
    sfence_vma();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs amoadd_w when pte.a=0 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    vaddr = hs_page_base(VSURW_GR);

    //u mode执行原子指令时，PTE.A=0
    goto_priv(PRIV_M);
    hspt_del_A();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hu amoadd_w when pte.a=0 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool amo_page_fault_7(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSURW_GR);

    //u mode执行原子指令时，PTE.A=1
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hu amoadd_w when pte.a=1 successful",
        excpt.triggered == false
    );

}

bool amo_page_fault_8(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    //m mode执行原子指令时，PTE.A=0(MPP=S)
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    // hspt_del_A();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("m amoadd_w when pte.a=1 successful(mmu open)",
        excpt.triggered == false
    );

}

bool amo_page_fault_9(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    goto_priv(PRIV_HS);
    hspt_init();

    //m mode执行原子指令时，PTE.A=1(MPP=S)
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m amoadd_w when pte.a=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m mode执行原子指令时，PTE.A=1改为0，执行sfence.vma(MPP=S)
    goto_priv(PRIV_M);
    hspt_del_A();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m amoadd_w when pte.a=0 lead to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool amo_page_fault_10(){
    TEST_START();
    goto_priv(PRIV_HS);
    hspt_init();
    uintptr_t vaddr = hs_page_base(VSRW_GR);
    //s mode执行原子指令时，PTE.D=1
    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs amoadd_w when pte.D=1 successful",
        excpt.triggered == false
    );  

    //s mode执行原子指令时，PTE.D=0改为1,不执行sfence.vma
    goto_priv(PRIV_M);
    hspt_del_D();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs amoadd_w when pte.D=0 leads to SPF without sfence.vma",
        excpt.triggered == false
    );

    //s mode执行原子指令时，PTE.D=0改为1，执行sfence.vma
    sfence_vma();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs amoadd_w when pte.D=0 leads to SPF with sfence.vma",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    vaddr = hs_page_base(VSURW_GR);

    //u mode执行原子指令时，PTE.D=0
    goto_priv(PRIV_M);
    hspt_del_D();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hu amoadd_w when pte.D=0 leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}

bool amo_page_fault_11(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSURW_GR);

    //u mode执行原子指令时，PTE.D=1
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hu amoadd_w when pte.D=1 successful",
        excpt.triggered == false
    );

}

bool amo_page_fault_12(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    //m mode执行原子指令时，PTE.D=1(MPP=S)
    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    // hspt_del_D();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("m amoadd_w when pte.D=1 successful(mmu open)",
        excpt.triggered == false
    );

}

bool amo_page_fault_13(){
    TEST_START();
    uintptr_t vaddr = hs_page_base(VSRW_GR);

    goto_priv(PRIV_HS);
    hspt_init();

    //m执行amoadd_w指令时，PTE.D=1
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m amoadd_w when pte.D=1 successful(mmu open)",
        excpt.triggered == false
    );

    //m执行amoadd_w指令时，PTE.D=0
    goto_priv(PRIV_M);
    hspt_del_D();
    CSRS(CSR_MSTATUS,MSTATUS_MPRV);
    CSRS(CSR_MSTATUS,MSTATUS_MPP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    CSRC(CSR_MSTATUS,MSTATUS_MPRV);
    TEST_ASSERT("m amoadd_w when pte.D=0 lead to LPF(mmu open)",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

}


bool amo_page_fault_14(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    sfence_vma();

    //s mode执行原子指令时，PTE.G=1，不产生异常
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode amoadd_w successful with G bit page",
        excpt.triggered == false
    );

    //u mode执行原子指令时，PTE.G=1，不产生异常
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hu mode amoadd_w successful with G bit page",
        excpt.triggered == false
    );
}

bool amo_page_fault_15(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_add_G();
    uintptr_t vaddr = hs_page_base(VSR_GURWX);
    uintptr_t paddr = phys_page_base(VSR_GURWX);
    sfence_vma();

    //s mode执行原子指令时，PTE.G=1 PTE.W=0
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode amoadd_w leads to spf with G bit page and w=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    //u mode执行原子指令时，PTE.G=1 PTE.W=0
    goto_priv(PRIV_M);
    hspt_add_G();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);

    TEST_ASSERT("hu mode amoadd_w leads to spf with G bit page and w=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );
}

bool amo_page_fault_16(){

    TEST_START();


    goto_priv(PRIV_M);
    uintptr_t vaddr = hs_page_base(VSX_GURWX);

    CSRS(CSR_MEDELEG,1ULL << CAUSE_SPF);
    //s mode执行原子指令时，发生SPF代理到S-mode下
    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    amoadd_w(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode amoadd_w cause to spf with r=0 and medeleg.spf=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.priv == PRIV_HS
    );
}

bool amo_page_fault_17(){

    TEST_START();

    //测试大页地址非对齐访问导致的SPF（sv48中的2MB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x140000000;      //sv48中2MB大页基物理地址:hspt[3][0]

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 2MB superpage successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 2MB superpage successful",
        excpt.triggered == false
    );


    goto_priv(PRIV_M);
    hspt_2MB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 2MB superpage misalign leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 2MB superpage misalign leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    goto_priv(PRIV_M);
    hspt_2MB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 2MB superpage successful(boundary align)",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 2MB superpage successful(boundary align)",
        excpt.triggered == false
    );

}

bool amo_page_fault_18(){

    TEST_START();

    //测试大页地址非对齐访问导致的SPF（sv48中的1GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0xc0000000;      //sv48中1GB大页基物理地址:hspt[1][3]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, (uint64_t)-1);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.Atomic

    goto_priv(PRIV_HS);
    hspt_init();

    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 1GB superpage successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 1GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_1GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 1GB superpage misalign leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 1GB superpage misalign leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    goto_priv(PRIV_M);
    hspt_1GB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 1GB superpage successful(boundary align)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 1GB superpage successful(boundary align)",
        excpt.triggered == false
    );

    TEST_END();
}

bool amo_page_fault_19(){

    TEST_START();

    #ifdef sv48

    //测试大页地址非对齐访问导致的SPF（sv48中的512GB）
    goto_priv(PRIV_M);
    uintptr_t vaddr = 0x8000000000;      //sv48中512GB大页基物理地址:hspt[0][1]

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置防止af
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, (uint64_t)-1);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    set_superpage_512G();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 512GB superpage successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 512GB superpage successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_misalign_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 512GB superpage misalign leads to SPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 512GB superpage misalign leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    goto_priv(PRIV_M);
    hspt_512GB_superpage_align_boundary_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_d(vaddr,0xdeadbeef);
    TEST_ASSERT("hs mode sc_d with 512GB superpage successful(boundary align)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);
    TEST_ASSERT("hs mode lr_d with 512GB superpage successful(boundary align)",
        excpt.triggered == false
    );

    TEST_END();
    #endif

}
