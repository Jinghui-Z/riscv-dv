#include <rvh_test.h>
#include <csrs.h> 
#include <page_tables.h>

//本c文件基于sv48，若改变，则相应代码需改变

#define SVPBMT_IO_MEM_BASE  0x30000000000ULL
#define SVPBMT_IO_MEM_LIMIT 0x40000000000ULL
#define SVPBMT_DRAM_PMA_BASE 0x80000000ULL
#define SVPBMT_DRAM_PMA_LIMIT 0x2000000000ULL

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

static inline void svpbmt_prepare_mem_3t_4t_window(void)
{
    goto_priv(PRIV_M);
    /*
     * svpbmt_test_2 is a PBMT=IO test, so its backing must remain in the
     * LinkNan 3T device/uncache window rather than default DRAM.
     */
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    linknan_pma_cfg0_tor_entry1_encoded(SVPBMT_IO_MEM_BASE >> 2,
        SVPBMT_IO_MEM_LIMIT >> 2,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X | LINKNAN_PMA_A_TOR);
    fence_iorw_iorw();
}

bool svpbmt_test_1() {

    //PTE 的 PBMT 位设置为 1（NC），访问主存,数据不可缓存
    TEST_START();    
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    CSRS(CSR_MENVCFG,MENVCFG_CBIE01);

    uintptr_t addr1 = phys_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t addr2 = phys_page_base(VSRWXPbmt1_GURWXPbmt1);
    uintptr_t addr3 = phys_page_base(X);
    uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t vaddr2 = vs_page_base(VSRWXPbmt1_GURWXPbmt1);
    uintptr_t vaddr3 = vs_page_base(X);

    write64(addr1, 0x11);
    write64(addr2, 0x22);
    write64(addr3, 0x33);

    goto_priv(PRIV_HS);
    hspt_init();

    //让两个vaddr指向同一个addr，但是pbmt不同
    pbmt_hspt_to_x(VSRWXPbmt0_GURWXPbmt0);
    pbmt_hspt_to_x(VSRWXPbmt1_GURWXPbmt1);
    sfence_vma();

    sd(vaddr2, 0xdeadbeef);    //绕过cache


    bool check1 = ld(vaddr1) == 0x33;
    bool check2 = ld(vaddr2) == 0xdeadbeef;
    TEST_ASSERT("before fence and cbo_inval, gets right values(pbmt0)", check1);
    TEST_ASSERT("before fence and cbo_inval, gets right values(pbmt1)", check2);

    
    fence_iorw_iorw();
    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr3);
    TEST_ASSERT("cbo.inval completes under MENVCFG.CBIE=Flush; do not require cacheable alias convergence",
        excpt.triggered == false
    );

    TEST_END(); 

}

bool svpbmt_test_2() {

    //PTE 的 PBMT 位设置为 2（IO），访问 LinkNan 3T responsive IO/uncache 窗口
    TEST_START();    
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    CSRS(CSR_MENVCFG,MENVCFG_CBIE01);
    svpbmt_prepare_mem_3t_4t_window();
    const uintptr_t pbmt_test_mem_base = SVPBMT_IO_MEM_BASE;

    uintptr_t addr1 = phys_page_base_offset(VSRWXPbmt0_GURWXPbmt0,
        pbmt_test_mem_base);
    uintptr_t addr2 = phys_page_base_offset(VSRWXPbmt2_GURWXPbmt1,
        pbmt_test_mem_base);
    uintptr_t addr3 = phys_page_base_offset(X, pbmt_test_mem_base);
    uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t vaddr2 = vs_page_base(VSRWXPbmt2_GURWXPbmt1);
    uintptr_t vaddr3 = vs_page_base(X);

    write64(addr1, 0x11);
    write64(addr2, 0x22);
    write64(addr3, 0x33);

    goto_priv(PRIV_HS);
    hspt_init();

    //让两个vaddr指向同一个addr，但是pbmt不同
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(X, pbmt_test_mem_base);
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt0_GURWXPbmt0, pbmt_test_mem_base);
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt2_GURWXPbmt1, pbmt_test_mem_base);
    sfence_vma();

    goto_priv(PRIV_HS);
    sd(vaddr2, 0xdeadbeef);    //绕过cache

    bool check1 = ld(vaddr1) == 0x33;
    bool check2 = ld(vaddr2) == 0xdeadbeef;
    TEST_ASSERT("before fence and cbo_inval, gets right values(pbmt0)", check1);
    TEST_ASSERT("before fence and cbo_inval, gets right values(pbmt2)", check2);
    fence_iorw_iorw();
    TEST_SETUP_EXCEPT();
    cbo_inval(vaddr3);
    TEST_ASSERT("cbo.inval completes under MENVCFG.CBIE=Flush; do not require cacheable alias convergence for PBMT=IO",
        excpt.triggered == false
    );


    TEST_END(); 
}


bool svpbmt_test_3() {

    //PTE 的 PBMT 位设置为 3（Reserved），访问主存，引发PF
    TEST_START();    
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    
    goto_priv(PRIV_HS);
    hspt_init();

    uintptr_t vaddr;
    uintptr_t addr;

    TEST_SETUP_EXCEPT();
    vaddr = vs_page_base(VSRWXPbmt3_GURWXPbmt1);
    sd(vaddr, 0xdeadbeef);

    TEST_ASSERT("sd cause to SPF when PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr, 0xdeadbeef);

    TEST_ASSERT("sc_d cause to SPF when PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );

    TEST_SETUP_EXCEPT();
    lb(vaddr);


    TEST_ASSERT("lb cause to LPF when PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);


    TEST_ASSERT("lr_d cause to LPF when PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("instr fetch casue to IPF when PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    TEST_END(); 
}



bool svpbmt_test_4() {

    //PTE 62-61 位在非叶 PTE 设置非 0，引发PF
    TEST_START();    
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);

    goto_priv(PRIV_HS);
    hspt_init();

#ifdef sv48
    hspt[1][4] = 
        PTE_V |  PTE_Pbmt1 | (((uintptr_t)&hspt[2][0]) >> 2);
#endif

#ifdef sv39
    hspt[1][0] = 
        PTE_V |  PTE_Pbmt2 | (((uintptr_t)&hspt[2][0]) >> 2);
#endif

    uintptr_t vaddr;
    uintptr_t addr;

    sfence_vma();

    TEST_SETUP_EXCEPT();

    vaddr = vs_page_base(VSRWXPbmt1_GURWXPbmt1);
    sd(vaddr, 0xdeadbeef);

    TEST_ASSERT("SPF when noleaf PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );


    TEST_SETUP_EXCEPT();

    ld(vaddr);

    TEST_ASSERT("LPF when noleaf PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_SETUP_EXCEPT();

    TEST_EXEC_EXCEPT(vaddr);

    TEST_ASSERT("IPF when noleaf PTE.PBMT=3",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    TEST_END(); 
}

bool svpbmt_test_5() {
    TEST_START();

    //PMA=IO,PBMT=None,非对齐访存0x0-2G范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置0~2G范围内的物理地址为PMA=IO
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, 0x20000000);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    
    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x0);
    sfence_vma();
    goto_priv(PRIV_HS);


    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt0_GURWXPbmt0, (hspt[4][VSRWXPbmt0_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt0_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(0,2G)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(0,2G)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(0,2G)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr, 0xdeadbeef);   //对齐访存
    TEST_ASSERT("hs mode sc_d (addr(0,2G)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(0,2G)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    lr_d(vaddr);   //对齐访存
    TEST_ASSERT("hs mode lr_d (addr(0,2G)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool svpbmt_test_6() {
    TEST_START();

    //PMA=IO,PBMT=None,非对齐访存3T-4T范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=IO
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt0_GURWXPbmt0, (hspt[4][VSRWXPbmt0_GURWXPbmt0]));
    
    uintptr_t vaddr = vs_page_base(VSRWXPbmt0_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(3T,4T)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(3T,4T)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(3T,4T)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(3T,4T)) when pte.pbmt=None and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}

bool svpbmt_test_7() {
    TEST_START();

    //PMA=IO,PBMT=IO,非对齐访存0x0-2G范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置0~2G范围内的物理地址为PMA=IO
    CSRW(CSR_PMAADDR0, 0x0);
    CSRW(CSR_PMAADDR1, 0x20000000);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x0);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt2_GURWXPbmt0, (hspt[4][VSRWXPbmt2_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt2_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(0,2G)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(0,2G)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(0,2G)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(0,2G)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool svpbmt_test_8() {
    TEST_START();
    
    //PMA=IO,PBMT=IO,非对齐访存3T-4T范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=IO
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt2_GURWXPbmt0, (hspt[4][VSRWXPbmt2_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt2_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=IO leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}

bool svpbmt_test_9() {
    TEST_START();

    //PMA=IO,PBMT=NC,非对齐访存3T-4T范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=IO
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt1_GURWXPbmt0, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt1_GURWXPbmt0, (hspt[4][VSRWXPbmt1_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt1_GURWXPbmt0);  
    uintptr_t paddr = phys_page_base(VSRWXPbmt1_GURWXPbmt0);

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=IO leads to SAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=IO leads to LAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=IO leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=IO leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}

bool svpbmt_test_10() {
    TEST_START();

    //PMA=MEM,PBMT=None,非对齐访存2G~10G范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置2G-10G范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x80000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x280000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    CSRS(CSR_PMACFG0, 1ULL << 13); //pma1cfg.ATOMIC

    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);

    goto_priv(PRIV_HS);
    hspt_init();

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt0_GURWXPbmt0, (hspt[4][VSRWXPbmt0_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt0_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode scalar sd misaligned(addr(2G,10G)) when pte.pbmt=None and pma=MEM/cacheable does not raise exception",
        excpt.triggered == false
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode scalar ld misaligned(addr(2G,10G)) when pte.pbmt=None and pma=MEM/cacheable does not raise exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(2G,10G)) when pte.pbmt=None and pma=MEM leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(2G,10G)) when pte.pbmt=None and pma=MEM leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool svpbmt_test_11() {
    TEST_START();

    //PMA=MEM,PBMT=None,非对齐访存3T-4T范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt0_GURWXPbmt0, (hspt[4][VSRWXPbmt0_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt0_GURWXPbmt0);  
    uintptr_t paddr = phys_page_base(VSRWXPbmt0_GURWXPbmt0);

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode scalar sd misaligned(addr(3T,4T)) when pte.pbmt=None and pma=MEM/cacheable does not raise exception",
        excpt.triggered == false
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode scalar ld misaligned(addr(3T,4T)) when pte.pbmt=None and pma=MEM/cacheable does not raise exception",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(3T,4T)) when pte.pbmt=None and pma=MEM leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(3T,4T)) when pte.pbmt=None and pma=MEM leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool svpbmt_test_12() {
    TEST_START();

    //PMA=MEM,PBMT=IO,非对齐访存3T-4T范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A


    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    hspt_leaf_change_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt2_GURWXPbmt0, (hspt[4][VSRWXPbmt2_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt2_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=MEM leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=MEM leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=MEM leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(3T,4T)) when pte.pbmt=IO and pma=MEM leads to AF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();
}


bool svpbmt_test_13() {
    TEST_START();

    //PMA=MEM,PBMT=NC,非对齐访存2G~10G范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置2G-10G范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x80000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x280000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A
    
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);

    goto_priv(PRIV_HS);
    hspt_init();

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt1_GURWXPbmt0, (hspt[4][VSRWXPbmt1_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt1_GURWXPbmt0);  

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(2G,10G)) when pte.pbmt=NC and pma=MEM leads to SAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(2G,10G)) when pte.pbmt=NC and pma=MEM leads to LAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(2G,10G)) when pte.pbmt=NC and pma=MEM leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(2G,10G)) when pte.pbmt=NC and pma=MEM leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool svpbmt_test_14() {
    TEST_START();

    //PMA=MEM,PBMT=NC,非对齐访存3T-4T范围内

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);

    goto_priv(PRIV_HS);
    hspt_init();
    
    hspt_leaf_change_base_paddr(VSRWXPbmt1_GURWXPbmt0, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //打印PTE.ppn值
    printf("hspt[4][%d] pte: 0x%lx\n", VSRWXPbmt1_GURWXPbmt0, (hspt[4][VSRWXPbmt1_GURWXPbmt0]));

    uintptr_t vaddr = vs_page_base(VSRWXPbmt1_GURWXPbmt0);  
    uintptr_t paddr = phys_page_base(VSRWXPbmt1_GURWXPbmt0);

    TEST_SETUP_EXCEPT();
    sd(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sd misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=MEM leads to SAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );
    
    TEST_SETUP_EXCEPT();
    ld(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode ld misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=MEM leads to LAM",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM
    );

    TEST_SETUP_EXCEPT();
    sc_d(vaddr + 7, 0xdeadbeef);   //非对齐访存
    TEST_ASSERT("hs mode sc_d misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=MEM leads to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );
    
    TEST_SETUP_EXCEPT();
    lr_d(vaddr + 7);   //非对齐访存
    TEST_ASSERT("hs mode lr_d misaligned(addr(3T,4T)) when pte.pbmt=NC and pma=MEM leads to LAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF
    );

    TEST_END();
}

bool svpbmt_test_15(){
    TEST_START();

    //在3T~4T(pma=mem)内，PBMT=0 访问同一页，切换到 PBMT=NC 访问，再切换到PBMT=0 访问（切换时不执行cbo.inval）
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    uintptr_t addr1 = phys_page_base_offset(VSRWXPbmt0_GURWXPbmt0,0x30000000000);
    uintptr_t addr2= phys_page_base_offset(VSRWXPbmt2_GURWXPbmt0,0x30000000000);
    uintptr_t addr3 = phys_page_base_offset(X,0x30000000000);
    uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t vaddr2 = vs_page_base(VSRWXPbmt2_GURWXPbmt0);
    uintptr_t vaddr3 = vs_page_base(X);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=MEM
    linknan_pma_cfg0_tor_entry1_encoded(0x30000000000ULL >> 2,
        0x40000000000ULL >> 2,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X |
        LINKNAN_PMA_A_TOR | LINKNAN_PMA_C);

    write64(addr3, 0x33);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x30000000000);
    hspt_leaf_change_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x30000000000);
    hspt_leaf_change_base_paddr(X, 0x30000000000);
    sfence_vma();
    goto_priv(PRIV_HS);

    //让两个vaddr指向同一个addr，但是pbmt不同
    pbmt_hspt_to_x(VSRWXPbmt0_GURWXPbmt0);
    pbmt_hspt_to_x(VSRWXPbmt2_GURWXPbmt0);
    sfence_vma();


    sd(vaddr1, 0x11);    
    sd(vaddr2, 0xdeadbeef);    //绕过cache
    ld(vaddr1);    

    bool check = ld(vaddr1) == 0x11;
    TEST_ASSERT("gets fault values when pbmt change (without cbo instr)", check);


    TEST_END(); 

}

bool svpbmt_test_16(){
    TEST_START();

    //在3T~4T(pma=mem)内，PBMT=0 访问同一页，切换到 PBMT=NC 访问，再切换到PBMT=0 访问（切换时执行cbo.inval）
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);

    uintptr_t addr1 = phys_page_base_offset(VSRWXPbmt0_GURWXPbmt0,0x30000000000);
    uintptr_t addr2= phys_page_base_offset(VSRWXPbmt2_GURWXPbmt0,0x30000000000);
    uintptr_t addr3 = phys_page_base_offset(X,0x30000000000);
    uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t vaddr2 = vs_page_base(VSRWXPbmt2_GURWXPbmt0);
    uintptr_t vaddr3 = vs_page_base(X);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    write64(addr3, 0x33);

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(X, 0x30000000000);
    sfence_vma();

    //让两个vaddr指向同一个addr，但是pbmt不同
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x30000000000);
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x30000000000);
    sfence_vma();

    goto_priv(PRIV_HS);
    sd(vaddr1, 0x11);    
    
    fence_iorw_iorw();
    cbo_inval(vaddr3);
    fence_iorw_iorw();
    sd(vaddr2, 0xdeadbeef);    //绕过cache

    fence_iorw_iorw();
    cbo_inval(vaddr3);
    fence_iorw_iorw();

    bool check = ld(vaddr1) == 0xdeadbeef;
    TEST_ASSERT("gets right values when pbmt change (cbo instr)", check);
    printf("values = %llx \n ", ld(vaddr1));

    TEST_END(); 

}


bool svpbmt_test_17(){
    TEST_START();

    //M模式下访问对应地址，切到S/U模式访问同一个地址（uncache）
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    CSRS(CSR_MENVCFG,MENVCFG_CBIE01);

    uintptr_t addr1 = phys_page_base_offset(VSRWXPbmt0_GURWXPbmt0,0x30000000000);
    uintptr_t addr2= phys_page_base_offset(VSRWXPbmt2_GURWXPbmt0,0x30000000000);
    uintptr_t addr3 = phys_page_base_offset(X,0x30000000000);
    uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t vaddr2 = vs_page_base(VSRWXPbmt2_GURWXPbmt0);
    uintptr_t vaddr3 = vs_page_base(X);

    //适配linknan的pma环境
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);

    //设置3T~4T范围内的物理地址为PMA=MEM
    CSRW(CSR_PMAADDR0, 0x30000000000 >> 2);
    CSRW(CSR_PMAADDR1, 0x40000000000 >> 2);  
    CSRS(CSR_PMACFG0, 1ULL << 8); //pma1cfg.R
    CSRS(CSR_PMACFG0, 1ULL << 9); //pma1cfg.W
    CSRS(CSR_PMACFG0, 1ULL << 10); //pma1cfg.X
    CSRS(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C
    CSRS(CSR_PMACFG0, 1ULL << 11); //pma1cfg.A

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    hspt_leaf_change_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x30000000000);
    hspt_leaf_change_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x30000000000);
    hspt_leaf_change_base_paddr(X, 0x30000000000);
    sfence_vma();
    sd(addr3, 0x11);

    goto_priv(PRIV_HS);

    //让两个vaddr指向同一个addr，但是pbmt不同
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt0_GURWXPbmt0, 0x30000000000);
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt2_GURWXPbmt0, 0x30000000000);
    sfence_vma();

    sd(vaddr3, 0x11);
    sd(vaddr2, 0xdeadbeef);    //绕过cache

    goto_priv(PRIV_M);

    bool check = ld(addr2) == 0x11;
    TEST_ASSERT("gets cache values when pbmt change from S mode to M ", check);

    goto_priv(PRIV_HS);
    check = ld(vaddr2) == 0xdeadbeef;
    TEST_ASSERT("gets mem values when change from M mode to S(pbmt=2)", check);

    TEST_END(); 

}


bool svpbmt_test_18(){
    //缓存中有副本且数据和内存不一致，M模式下修改PMA.C(1->0），S/U模式持续访问修改后的区域

    TEST_START();

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    CSRS(CSR_MENVCFG,MENVCFG_CBIE01);

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

    uintptr_t vaddr = hs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t vaddr2 = hs_page_base(VSRWXPbmt2_GURWXPbmt0);

    //范围为叶子pte的物理地址范围
    uintptr_t addr_start = (uintptr_t)&hspt[4][X];
    uintptr_t addr_end = ((uintptr_t)&hspt[4][X] + 0x1000);
    printf("addr_start=%lx, addr_end=%lx\n", vaddr, addr_start, addr_end);

    //设置目标地址范围权限
    linknan_pma_cfg0_tor_entry1_encoded(addr_start >> 2, addr_end >> 2,
        LINKNAN_PMA_R | LINKNAN_PMA_W | LINKNAN_PMA_X |
        LINKNAN_PMA_A_TOR | LINKNAN_PMA_C);

    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt0_GURWXPbmt0);
    pbmt_hspt_to_x(VSRWXPbmt2_GURWXPbmt0);
    sfence_vma();

    goto_priv(PRIV_HS);

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

    TEST_SETUP_EXCEPT();
    sd(vaddr,0xdeadbeef);
    TEST_ASSERT("sd successful",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("ld successful",
        excpt.triggered == false
    );

    sd(vaddr2,0xaaaaaaaa); //绕过cache，修改内存数据

    //修改目标地址范围权限C位拉低
    goto_priv(PRIV_M);
    CSRC(CSR_PMACFG0, 1ULL << 14); //pma1cfg.C

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    int64_t val = ld(vaddr2);
    //直接访问内存而不使用缓存中的数据,得到0xaaaaaaaa
    TEST_ASSERT("ld get mem value direct(no cache) after pma.c change 1->0",
        excpt.triggered == false &&
        val == 0xaaaaaaaa
    );

}


// bool svpbmt_test_5() {

//     //PBMT=0 访问同一页，切换到 PBMT=NC 访问,不执行fence
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);


//     uintptr_t addr1 = phys_page_base(VSRWXPbmt0_GURWXPbmt0);
//     uintptr_t addr2 = phys_page_base(VSRWXPbmt0_GURWXPbmt1);
//     uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
//     uintptr_t vaddr2 = vs_page_base(VSRWXPbmt0_GURWXPbmt1);
//     write64(addr1, 0x11);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t addr0 = TEST_PPAGE_BASE;

//     for(int i = 0; i < TEST_PAGE_MAX; i++){     //使得两个pte指向同一个页面
//         if(i == VSRWXPbmt0_GURWXPbmt1){
//             addr0 -=  PAGE_SIZE;
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX | PTE_Pbmt1; 
//             addr0 +=  PAGE_SIZE;
//         }
//         addr0 +=  PAGE_SIZE;
//     }

//     goto_priv(PRIV_VS);

//     bool check2 = read64(vaddr2) == 0x11;
//     bool check1 = read64(vaddr1) == 0x11;


//     TEST_ASSERT("gets right values", check1 && check2);


    
//     goto_priv(PRIV_VS);
//     sd(vaddr1,0x22);

//     check2 = read64(vaddr2) == 0x11;   
//     TEST_ASSERT("Access the same page with PBMT=0, then switch to PBMT=NC cause to get old value if no fence", check2);

//     TEST_END(); 
// }


// bool svpbmt_test_6() {

//     //PBMT=0 访问同一页，切换到 PBMT=NC 访问,执行fence

//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);


//     uintptr_t addr1 = phys_page_base(VSRWXPbmt0_GURWXPbmt0);
//     uintptr_t addr2 = phys_page_base(VSRWXPbmt0_GURWXPbmt1);
//     uintptr_t vaddr1 = vs_page_base(VSRWXPbmt0_GURWXPbmt0);
//     uintptr_t vaddr2 = vs_page_base(VSRWXPbmt0_GURWXPbmt1);
//     write64(addr1, 0x11);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t addr0 = TEST_PPAGE_BASE;

//     for(int i = 0; i < TEST_PAGE_MAX; i++){     //使得两个pte指向同一个页面
//         if(i == VSRWXPbmt0_GURWXPbmt1){
//             addr0 -=  PAGE_SIZE;
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX | PTE_Pbmt1; 
//             addr0 +=  PAGE_SIZE;
//         }
//         addr0 +=  PAGE_SIZE;
//     }


//     goto_priv(PRIV_VS);


//     bool check2 = read64(vaddr2) == 0x11;
//     bool check1 = read64(vaddr1) == 0x11;


//     TEST_ASSERT("gets right values", check1 && check2);


    
//     goto_priv(PRIV_VS);
//     sd(vaddr1,0x22);

//     fence_iorw_iorw();
//     // cbo_fush()          //后续修改base
//     fence_iorw_iorw();

//     check2 = read64(vaddr2) == 0x22;   
//     TEST_ASSERT("Access the same page with PBMT=0, then switch to PBMT=NC cause to get new value if fence", check2);

//     TEST_END(); 
// }

// bool svpbmt_test_7() {

//     TEST_START();    
//     //PBMT=NC 进行 misaligned 访问

//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t vaddr;
//     uintptr_t addr;

//     goto_priv(PRIV_VS);

//     TEST_SETUP_EXCEPT();

//     vaddr = vs_page_base(VSRWXPbmt1_GURWXPbmt1);
//     sd(vaddr + 1, 0xdeadbeef);

//     TEST_ASSERT("sd misalign when PTE.PBMT=1 cause to SAM",
//         excpt.triggered == true &&
//         excpt.cause == CAUSE_SAM
//     );


//     TEST_END(); 
// }

// bool svpbmt_test_8() {

//     TEST_START();    
//     //PBMT=NC 进行 misaligned 访问

//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t vaddr;
//     uintptr_t addr;

//     goto_priv(PRIV_VS);

//     TEST_SETUP_EXCEPT();

//     vaddr = vs_page_base(VSRWXPbmt2_GURWXPbmt2);
//     sd(vaddr + 1, 0xdeadbeef);

//     excpt_info();
//     TEST_ASSERT("sd misalign when PTE.PBMT=2 cause to SAM",
//         excpt.triggered == true &&
//         excpt.cause == CAUSE_SAM
//     );


//     TEST_END(); 
// }



// bool svpbmt_test_9() {


//     //两阶段地址访问页表，G-stage PBMT 有效但 VS-stage PBMT=None,访问遵循 G-stage PBMT 设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt1_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt1_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt1_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt1_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();


//     uintptr_t addr0 = TEST_VPAGE_BASE;
//     for(int i = 0; i < TEST_PAGE_MAX; i++){     //使得VS-stage pbmt=None
//         if(i == Pbmt1_SWITCH1){
//             vspt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_RWX ; 
//         }
//         if(i == Pbmt1_SWITCH2){
//             vspt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_RWX ; 
//         }
//         addr0 +=  PAGE_SIZE;
//     }


//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt1)", check1 && check2);

//     goto_priv(PRIV_VS);
//     pbmt1_hpt_switch();
//     bool check3 = read64(vaddr1) == 0x22;
//     bool check4 = read64(vaddr2) == 0x11;
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt1)", check3 && check4);

//     TEST_ASSERT("两阶段地址访问页表,G-stage PBMT 有效但 VS-stage PBMT=None,访问遵循 G-stage PBMT 设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }


// bool svpbmt_test_10() {


//     //两阶段地址访问页表，G-stage PBMT=None 但 VS-stage PBMT 有效,访问遵循 VS-stage PBMT 设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt1_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt1_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt1_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt1_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t addr0 = TEST_PPAGE_BASE;
//     for(int i = 0; i < TEST_PAGE_MAX; i++){     //使得G-stage pbmt=None
//         if(i == Pbmt1_SWITCH1){
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX ; 
//         }
//         if(i == Pbmt1_SWITCH2){
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX ; 
//         }
//         addr0 +=  PAGE_SIZE;
//     }

//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt1)", check1 && check2);

//     goto_priv(PRIV_VS);
//     pbmt1_hpt_switch();
//     bool check3 = read64(vaddr1) == 0x22;
//     bool check4 = read64(vaddr2) == 0x11;   
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt1)", check3 && check4);

//     TEST_ASSERT("两阶段地址访问页表,G-stage PBMT=None 但 VS-stage PBMT 有效,访问遵循 VS-stage PBMT 设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }




// bool svpbmt_test_11() {

//     //vsatp.mode !=0 & hgatp.mode =0,G-stage PBMT 任意值，但 VS-stage PBMT=None,访问遵循原PMA设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt2_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt2_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt2_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt2_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     vspt_init();

//     uintptr_t addr0 = TEST_VPAGE_BASE;
//     for(int i = 0; i < TEST_PAGE_MAX; i++){     //使得VS-stage pbmt=None
//         if(i == Pbmt1_SWITCH1){
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_RWX ; 
//         }
//         if(i == Pbmt1_SWITCH2){
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_RWX ; 
//         }
//         addr0 +=  PAGE_SIZE;
//     }


//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt2)", check1 && check2);

//     goto_priv(PRIV_VS);
//     pbmt1_hpt_switch();
//     bool check3 = read64(vaddr1) == 0x11;
//     bool check4 = read64(vaddr2) == 0x22;   
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt2)", check1 && check2);

//     TEST_ASSERT("vsatp.mode !=0 & hgatp.mode =0,G-stage PBMT 任意值，但 VS-stage PBMT=None,访问遵循原PMA设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }


// bool svpbmt_test_12() {

//     //vsatp.mode !=0 & hgatp.mode =0,G-stage PBMT 任意值，但 VS-stage PBMT !=None,访问遵循 VS-stage PBMT 设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt2_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt2_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt2_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt2_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     vspt_init();

    
//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt2)", check1 && check2);

//     goto_priv(PRIV_VS);

//     pbmt2_vspt_switch();
//     bool check3 = ld(vaddr1) == 0x22;
//     bool check4 = ld(vaddr2) == 0x11;   
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt2)", check3 && check4);

//     TEST_ASSERT("vsatp.mode !=0 & hgatp.mode =0,G-stage PBMT 任意值，但 VS-stage PBMT!=None,访问遵循 VS-stage PBMT 设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }



// bool svpbmt_test_13() {

//     //vsatp.mode =0 & hgatp.mode !=0,G-stage PBMT=None，但 VS-stage PBMT 任意值,访问遵循原PMA设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt2_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt2_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt2_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt2_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();

//     uintptr_t addr0 = TEST_PPAGE_BASE;
//     for(int i = 0; i < TEST_PAGE_MAX; i++){     //使得G-stage pbmt=None
//         if(i == Pbmt1_SWITCH1){
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX ; 
//         }
//         if(i == Pbmt1_SWITCH2){
//             hpt[5][i] = (addr0 >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX ; 
//         }
//         addr0 +=  PAGE_SIZE;
//     }


//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt2)", check1 && check2);

//     goto_priv(PRIV_VS);
//     pbmt2_hpt_switch();
//     bool check3 = read64(vaddr1) == 0x11;
//     bool check4 = read64(vaddr2) == 0x22;   
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt2)", check1 && check2);

//     TEST_ASSERT("vsatp.mode =0 & hgatp.mode !=0,G-stage PBMT=None,但 VS-stage PBMT 任意值,访问遵循原PMA设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }


// bool svpbmt_test_14() {

//     //vsatp.mode =0 & hgatp.mode !=0,G-stage PBMT != None，但 VS-stage PBMT 任意值,访问遵循 G-stage PBMT 设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt2_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt2_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt2_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt2_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();

//     goto_priv(PRIV_VS);
    
//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt2)", check1 && check2);

//     pbmt2_hpt_switch();
//     bool check3 = read64(vaddr1) == 0x22;
//     bool check4 = read64(vaddr2) == 0x11;   
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt2)", check3 && check4);

//     TEST_ASSERT("vsatp.mode =0 & hgatp.mode !=0,G-stage PBMT != None,但 VS-stage PBMT 任意值,访问遵循 G-stage PBMT 设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }

// bool svpbmt_test_15() {         //没写好，要修改

//     //vsatp.mode !=0 & hgatp.mode !=0,G-stage PBMT != None，且 VS-stage PBMT != None,访问遵循 VS-stage PBMT 设定
//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     uintptr_t addr1 = phys_page_base(Pbmt2_SWITCH1);
//     uintptr_t addr2 = phys_page_base(Pbmt2_SWITCH2);
//     uintptr_t vaddr1 = vs_page_base(Pbmt2_SWITCH1);
//     uintptr_t vaddr2 = vs_page_base(Pbmt2_SWITCH2);
//     write64(addr1, 0x11);
//     write64(addr2, 0x22);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     vspt_init();
//     hpt_init();

//     goto_priv(PRIV_VS);
    
//     bool check1 = read64(vaddr1) == 0x11;
//     bool check2 = read64(vaddr2) == 0x22;
//     TEST_ASSERT("gets right values(pbmt2)", check1 && check2);

//     pbmt2_hpt_switch();
//     bool check3 = read64(vaddr1) == 0x22;
//     bool check4 = read64(vaddr2) == 0x11;   
//     TEST_ASSERT("gets right values after changing pt without fence(pbmt2)", check3 && check4);

//     TEST_ASSERT("vsatp.mode !=0 & hgatp.mode !=0,G-stage PBMT != None,且 VS-stage PBMT != None,访问遵循 VS-stage PBMT 设定", check1 && check2 && check3 && check4);

//     TEST_END(); 
// }




// bool svpbmt_test_111() {

//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t vaddr;
//     uintptr_t addr;

//     goto_priv(PRIV_VS);

//     TEST_SETUP_EXCEPT();

//     addr = vs_page_base(VSRWXPbmt1_GURWXPbmt1);
//     sd(addr, 0xdeadbeef);

//     TEST_ASSERT("PTE.PBMT=1",
//         excpt.triggered == false
//     );


//     TEST_END(); 
// }

// bool svpbmt_test_113() {

//     TEST_START();    
//     goto_priv(PRIV_M);
//     CSRC(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRC(CSR_HENVCFG,HENVCFG_PBMTE);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t vaddr;
//     uintptr_t addr;

//     goto_priv(PRIV_VS);
//     vaddr = vs_page_base(VSRWXPbmt1_GURWXPbmt1);

//     TEST_SETUP_EXCEPT();
//     sd(vaddr, 0xdeadbeef);

//     TEST_ASSERT("PTE.PBMT=1 when menvcfg.pbmte=0、henvcfg.pbmte=0",
//         excpt.triggered == true &&
//         excpt.cause == CAUSE_SPF
//     );

//     TEST_SETUP_EXCEPT();
//     lb(vaddr);

//     TEST_ASSERT("PTE.PBMT=1",
//         excpt.triggered == true &&
//         excpt.cause == CAUSE_LPF
//     );


//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRC(CSR_HENVCFG,HENVCFG_PBMTE);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     goto_priv(PRIV_VS);
//     vaddr = vs_page_base(VSRWXPbmt2_GURWXPbmt2);

//     TEST_SETUP_EXCEPT();
//     sd(vaddr, 0xdeadbeef);

//     TEST_ASSERT("PTE.PBMT=1",
//         excpt.triggered == true &&
//         excpt.cause == CAUSE_SPF
//     );

//     TEST_SETUP_EXCEPT();
//     lb(vaddr);

//     TEST_ASSERT("PTE.PBMT=1",
//         excpt.triggered == true &&
//         excpt.cause == CAUSE_LPF
//     );


//     TEST_END(); 
// }

// bool svpbmt_test_114() {

//     TEST_START();    
//     //PTE 的 PBMT 位设置为 1（NC），访问主存，测数据是否不可缓存

//     goto_priv(PRIV_M);
//     CSRS(CSR_MENVCFG,MENVCFG_PBMTE);
//     CSRS(CSR_HENVCFG,HENVCFG_PBMTE);

//     goto_priv(PRIV_HS);
//     hspt_init();
//     hpt_init();
//     vspt_init();

//     uintptr_t vaddr;
//     uintptr_t addr;

//     goto_priv(PRIV_VS);

//     TEST_SETUP_EXCEPT();

//     addr = vs_page_base(VSRWXPbmt1_GURWXPbmt1);
//     sd(addr, 0xdeadbeef);

//     TEST_ASSERT("PTE.PBMT=1",
//         excpt.triggered == false
//     );


//     TEST_END(); 
// }
