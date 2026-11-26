#include <rvh_test.h>
#include <page_tables.h>
#include <csrs.h> 

#define ADDR_UNALIGNED_EXPECT_CAUSE_IF(test_msg, op, expected_cause, extra_cond) do { \
    TEST_SETUP_EXCEPT(); \
    op; \
    TEST_ASSERT(test_msg, \
        excpt.triggered == true && \
        excpt.cause == (expected_cause) && \
        (extra_cond) \
    ); \
} while (0)

#define ADDR_UNALIGNED_RUN_HS_PBMT_MISALIGNED_CHECKS(extra_cond) do { \
    uintptr_t nc_paddr = addr_unaligned_pbmt_backing_paddr(); \
    uintptr_t source_paddr = phys_page_base(SWITCH1); \
    uintptr_t source_vaddr = hs_page_base(SWITCH1); \
    goto_priv(PRIV_M); \
    write64(nc_paddr, 0x1122334455667788ULL); \
    write64(nc_paddr + sizeof(uint64_t), 0x99aabbccddeeff00ULL); \
    write64(source_paddr, 0x0123456789abcdefULL); \
    write64(source_paddr + sizeof(uint64_t), 0xfedcba9876543210ULL); \
    uintptr_t nc_base = addr_unaligned_prepare_pbmt_alias(VSRWXPbmt1_GURWXPbmt0); \
    goto_priv(PRIV_HS); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=NC scalar misaligned ld reports load address misaligned", \
        (void)ld(nc_base + 7U), CAUSE_LAM, (extra_cond)); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=NC scalar misaligned sd reports store address misaligned", \
        sd(nc_base + 7U, 0xdeadbeefcafef00dULL), CAUSE_SAM, (extra_cond)); \
    TEST_SETUP_EXCEPT(); \
    vle64_to_v6((const uint64_t *)source_vaddr, 2); \
    TEST_ASSERT("HS normal vector source load succeeds before PBMT misaligned vector probes", \
        excpt.triggered == false \
    ); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=NC vector misaligned vle64 reports load access fault", \
        vle64_to_v6((const uint64_t *)(nc_base + 1U), 2), CAUSE_LAF, (extra_cond)); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=NC vector misaligned vse64 reports store access fault", \
        vse64_from_v6((uint64_t *)(nc_base + 1U), 2), CAUSE_SAF, (extra_cond)); \
    uintptr_t io_base = addr_unaligned_prepare_pbmt_io_alias(VSRWXPbmt2_GURWXPbmt0); \
    goto_priv(PRIV_HS); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=IO device-style scalar misaligned ld reports load access fault", \
        (void)ld(io_base + 7U), CAUSE_LAF, (extra_cond)); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=IO device-style scalar misaligned sd reports store access fault", \
        sd(io_base + 7U, 0x13579bdf2468ace0ULL), CAUSE_SAF, (extra_cond)); \
    TEST_SETUP_EXCEPT(); \
    vle64_to_v6((const uint64_t *)source_vaddr, 2); \
    TEST_ASSERT("HS normal vector source reload succeeds before PBMT=IO device-style store probe", \
        excpt.triggered == false \
    ); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=IO device-style vector misaligned vle64 reports load access fault", \
        vle64_to_v6((const uint64_t *)(io_base + 1U), 2), CAUSE_LAF, (extra_cond)); \
    ADDR_UNALIGNED_EXPECT_CAUSE_IF("HS PBMT=IO device-style vector misaligned vse64 reports store access fault", \
        vse64_from_v6((uint64_t *)(io_base + 1U), 2), CAUSE_SAF, (extra_cond)); \
    addr_unaligned_reset_pbmt_env(); \
} while (0)

static inline uintptr_t addr_unaligned_pbmt_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t addr_unaligned_prepare_pbmt_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t addr_unaligned_prepare_pbmt_io_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline void addr_unaligned_reset_pbmt_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

bool addr_unaligned_1() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    
    goto_priv(PRIV_HS);     
    hspt_init();        

    uintptr_t vaddr_f = hs_page_base(VSRWX_GURWX) + 1;      
    uint64_t value = 0xdeadbeef;
    //load byte地址不会发生未对齐
    TEST_SETUP_EXCEPT();        
    value = lb(vaddr_f);
    TEST_ASSERT("load byte address is not aligned successful(mmu open)",         
        excpt.triggered == false
    );

    // Spike --misaligned supports scalar misaligned load/store.
    TEST_SETUP_EXCEPT();       
    vaddr_f = hs_page_base(VSRWX_GURWX) + 3;
    value = lh(vaddr_f);
    TEST_ASSERT("load half misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );
    
    TEST_SETUP_EXCEPT();        
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5;
    value = lw(vaddr_f);
    TEST_ASSERT("load word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();        
    vaddr_f = hs_page_base(VSRWX_GURWX) + 7;
    value = ld(vaddr_f);
    TEST_ASSERT("load double word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    //store byte地址不会发生未对齐
    vaddr_f = hs_page_base(VSRWX_GURWX) + 1;
    TEST_SETUP_EXCEPT();        
    sb(vaddr_f,value);
    TEST_ASSERT("store byte address is not aligned successful(mmu open)",         
        excpt.triggered == false
    );

    vaddr_f = hs_page_base(VSRWX_GURWX) + 3;
    TEST_SETUP_EXCEPT();        
    sh(vaddr_f,value);
    TEST_ASSERT("store half misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );
    
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5;
    TEST_SETUP_EXCEPT();      
    sw(vaddr_f,value);
    TEST_ASSERT("store word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    vaddr_f = hs_page_base(VSRWX_GURWX) + 7;
    TEST_SETUP_EXCEPT();        
    sd(vaddr_f,value);
    TEST_ASSERT("store double word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    
    // Misaligned atomic instructions are classified as access faults.
    TEST_SETUP_EXCEPT();      
    vaddr_f = hs_page_base(VSRWX_GURWX) + 1;
    amoor_w(vaddr_f, 0xdeadbeef);
    TEST_ASSERT("amo.w misaligned reports store/AMO access fault(mmu open)",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5;   
    sc_w(vaddr_f,value);
    TEST_ASSERT("sc.w misaligned reports store/AMO access fault(mmu open)",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = hs_page_base(VSRWX_GURWX) + 7;   
    sc_d(vaddr_f,value);
    TEST_ASSERT("sc.d misaligned reports store/AMO access fault(mmu open)",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF
    );

    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5; 
    lr_w(vaddr_f);
    TEST_ASSERT("lr.w address is not aligned result in a laf(mmu open)",         
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF
    );
    
    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = hs_page_base(VSRWX_GURWX) + 7; 
    lr_d(vaddr_f);
    TEST_ASSERT("lr.d address is not aligned result in a laf(mmu open)",         
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF
    );

    ADDR_UNALIGNED_RUN_HS_PBMT_MISALIGNED_CHECKS(true);

    TEST_END();

}

bool addr_unaligned_2() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    //开启异常代理
    CSRW(CSR_MEDELEG,
         (1ULL << CAUSE_LAM) | (1ULL << CAUSE_LAF) |
         (1ULL << CAUSE_SAM) | (1ULL << CAUSE_SAF));
    
    hspt_init();        
    goto_priv(PRIV_HS);     
    uintptr_t vaddr_f = hs_page_base(VSRWX_GURWX) + 1;      
    uint64_t value = 0xdeadbeef;
    //load byte地址不会发生未对齐
    TEST_SETUP_EXCEPT();        
    value = lb(vaddr_f);
    TEST_ASSERT("load byte address is not aligned successful(mmu open)",         
        excpt.triggered == false
    );

    // Spike --misaligned supports scalar misaligned load/store.
    TEST_SETUP_EXCEPT();       
    vaddr_f = hs_page_base(VSRWX_GURWX) + 3;
    value = lh(vaddr_f);
    TEST_ASSERT("load half misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );
    
    TEST_SETUP_EXCEPT();        
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5;
    value = lw(vaddr_f);
    TEST_ASSERT("load word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();        
    vaddr_f = hs_page_base(VSRWX_GURWX) + 7;
    value = ld(vaddr_f);
    TEST_ASSERT("load double word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    //store byte地址不会发生未对齐
    vaddr_f = hs_page_base(VSRWX_GURWX) + 1;
    TEST_SETUP_EXCEPT();        
    sb(vaddr_f,value);
    TEST_ASSERT("store byte address is not aligned successful(mmu open)",         
        excpt.triggered == false
    );

    vaddr_f = hs_page_base(VSRWX_GURWX) + 3;
    TEST_SETUP_EXCEPT();        
    sh(vaddr_f,value);
    TEST_ASSERT("store half misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );
    
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5;
    TEST_SETUP_EXCEPT();      
    sw(vaddr_f,value);
    TEST_ASSERT("store word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    vaddr_f = hs_page_base(VSRWX_GURWX) + 7;
    TEST_SETUP_EXCEPT();        
    sd(vaddr_f,value);
    TEST_ASSERT("store double word misaligned succeeds when misaligned scalar access is supported(mmu open)",
        excpt.triggered == false
    );

    
    // Misaligned atomic instructions are classified as access faults.
    TEST_SETUP_EXCEPT();      
    vaddr_f = hs_page_base(VSRWX_GURWX) + 1;
    amoor_w(vaddr_f, 0xdeadbeef);
    TEST_ASSERT("amo.w misaligned reports store/AMO access fault(mmu open)",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF&&
        excpt.priv == PRIV_HS
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5;   
    sc_w(vaddr_f,value);
    TEST_ASSERT("sc.w misaligned reports store/AMO access fault(mmu open)",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF&&
        excpt.priv == PRIV_HS
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = hs_page_base(VSRWX_GURWX) + 7;   
    sc_d(vaddr_f,value);
    TEST_ASSERT("sc.d misaligned reports store/AMO access fault(mmu open)",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF&&
        excpt.priv == PRIV_HS
    );

    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = hs_page_base(VSRWX_GURWX) + 5; 
    lr_w(vaddr_f);
    TEST_ASSERT("lr.w address is not aligned result in a laf(mmu open)",         
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF&&
        excpt.priv == PRIV_HS
    );
    
    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = hs_page_base(VSRWX_GURWX) + 7; 
    lr_d(vaddr_f);
    TEST_ASSERT("lr.d address is not aligned result in a laf(mmu open)",         
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF&&
        excpt.priv == PRIV_HS
    );

    ADDR_UNALIGNED_RUN_HS_PBMT_MISALIGNED_CHECKS(excpt.priv == PRIV_HS);

    TEST_END();
}

bool addr_unaligned_3() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    
    goto_priv(PRIV_M);     

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t vaddr_f = paddr + 1;
    uint64_t value = 0xdeadbeef;
    //load byte地址不会发生未对齐
    TEST_SETUP_EXCEPT();        
    value = lb(vaddr_f);
    TEST_ASSERT("m-mode load byte address is not aligned successful",         
        excpt.triggered == false
    );

    // Spike --misaligned supports scalar misaligned load/store.
    TEST_SETUP_EXCEPT();       
    vaddr_f = paddr + 3;
    value = lh(vaddr_f);
    TEST_ASSERT("m-mode load half misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );
    
    TEST_SETUP_EXCEPT();        
    vaddr_f = paddr + 5;
    value = lw(vaddr_f);
    TEST_ASSERT("m-mode load word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();        
    vaddr_f = paddr + 7;
    value = ld(vaddr_f);
    TEST_ASSERT("m-mode load double word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    //store byte地址不会发生未对齐
    vaddr_f = paddr + 1;
    TEST_SETUP_EXCEPT();        
    sb(vaddr_f,value);
    TEST_ASSERT("m-mode store byte address is not aligned successful",         
        excpt.triggered == false
    );

    vaddr_f = paddr + 3;
    TEST_SETUP_EXCEPT();        
    sh(vaddr_f,value);
    TEST_ASSERT("m-mode store half misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );
    
    vaddr_f = paddr + 5;
    TEST_SETUP_EXCEPT();      
    sw(vaddr_f,value);
    TEST_ASSERT("m-mode store word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    vaddr_f = paddr + 7;
    TEST_SETUP_EXCEPT();        
    sd(vaddr_f,value);
    TEST_ASSERT("m-mode store double word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    
    // Misaligned atomic instructions are classified as access faults.
    TEST_SETUP_EXCEPT();      
    vaddr_f = paddr + 1;
    amoor_w(vaddr_f, 0xdeadbeef);
    TEST_ASSERT("m-mode amo.w misaligned reports store/AMO access fault",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = paddr + 5;
    sc_w(vaddr_f,value);
    TEST_ASSERT("m-mode sc.w misaligned reports store/AMO access fault",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = paddr + 7;
    sc_d(vaddr_f,value);
    TEST_ASSERT("m-mode sc.d misaligned reports store/AMO access fault",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF
    );

    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = paddr + 5;
    lr_w(vaddr_f);
    TEST_ASSERT("m-mode lr.w address is not aligned result in a laf",
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF
    );
    
    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = paddr + 7;
    lr_d(vaddr_f);
    TEST_ASSERT("m-mode lr.d address is not aligned result in a laf",
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF
    );


    TEST_END();

}

bool addr_unaligned_4() {

    TEST_START();

    TEST_SETUP_EXCEPT();
    //开启异常代理
    CSRW(CSR_MEDELEG,
         (1ULL << CAUSE_LAM) | (1ULL << CAUSE_LAF) |
         (1ULL << CAUSE_SAM) | (1ULL << CAUSE_SAF));
    
    goto_priv(PRIV_M);
    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t vaddr_f = paddr + 1;
    uint64_t value = 0xdeadbeef;
    //load byte地址不会发生未对齐
    TEST_SETUP_EXCEPT();        
    value = lb(vaddr_f);
    TEST_ASSERT("m-mode load byte address is not aligned successful",         
        excpt.triggered == false
    );

    // Spike --misaligned supports scalar misaligned load/store.
    TEST_SETUP_EXCEPT();       
    vaddr_f = paddr + 3;
    value = lh(vaddr_f);
    TEST_ASSERT("m-mode load half misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );
    
    TEST_SETUP_EXCEPT();        
    vaddr_f = paddr + 5;
    value = lw(vaddr_f);
    TEST_ASSERT("m-mode load word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();        
    vaddr_f = paddr + 7;
    value = ld(vaddr_f);
    TEST_ASSERT("m-mode load double word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    //store byte地址不会发生未对齐
    vaddr_f = paddr + 1;
    TEST_SETUP_EXCEPT();        
    sb(vaddr_f,value);
    TEST_ASSERT("m-mode store byte address is not aligned successful",         
        excpt.triggered == false
    );

    vaddr_f = paddr + 3;
    TEST_SETUP_EXCEPT();        
    sh(vaddr_f,value);
    TEST_ASSERT("m-mode store half misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );
    
    vaddr_f = paddr + 5;
    TEST_SETUP_EXCEPT();      
    sw(vaddr_f,value);
    TEST_ASSERT("m-mode store word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    vaddr_f = paddr + 7;
    TEST_SETUP_EXCEPT();        
    sd(vaddr_f,value);
    TEST_ASSERT("m-mode store double word misaligned succeeds when misaligned scalar access is supported",
        excpt.triggered == false
    );

    
    // Misaligned atomic instructions are classified as access faults.
    TEST_SETUP_EXCEPT();      
    vaddr_f = paddr + 1;
    amoor_w(vaddr_f, 0xdeadbeef);
    TEST_ASSERT("m-mode amo.w misaligned reports store/AMO access fault",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF&&
        excpt.priv == PRIV_M
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = paddr + 5;
    sc_w(vaddr_f,value);
    TEST_ASSERT("m-mode sc.w misaligned reports store/AMO access fault",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF&&
        excpt.priv == PRIV_M
    );

    // Misaligned SC is a store/AMO access fault.
    TEST_SETUP_EXCEPT();
    vaddr_f = paddr + 7;
    sc_d(vaddr_f,value);
    TEST_ASSERT("m-mode sc.d misaligned reports store/AMO access fault",
        excpt.triggered == true&&
        excpt.cause == CAUSE_SAF&&
        excpt.priv == PRIV_M
    );

    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = paddr + 5;
    lr_w(vaddr_f);
    TEST_ASSERT("m-mode lr.w address is not aligned result in a laf",
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF&&
        excpt.priv == PRIV_M
    );
    
    // Spike still reports misaligned LR as load access fault.
    TEST_SETUP_EXCEPT();       
    vaddr_f = paddr + 7;
    lr_d(vaddr_f);
    TEST_ASSERT("m-mode lr.d address is not aligned result in a laf",
        excpt.triggered == true&&
        excpt.cause == CAUSE_LAF&&
        excpt.priv == PRIV_M
    );

    TEST_END();
}



// 额外的L2页表，用于映射高位虚拟地址 0x7FFFFFFFFFFB
static pte_t fullva_l2[PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));
static pte_t fullva_l2_2[PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

bool addr_unaligned_5() {

    //构造fullva检测错误结合地址非对齐的场景
    TEST_START();

    TEST_SETUP_EXCEPT();
    
    goto_priv(PRIV_M);     
    uintptr_t vaddr = 0x7ffffffffffb;       

    goto_priv(PRIV_HS);
    hspt_init();

    // 构造 Sv48 页表映射
    for (int i = 0; i < PAGE_SIZE/sizeof(pte_t); i++)
        fullva_l2[i] = 0;
    // L2[0]: 1GB superpage → 0x80000000 (给 0xFFFF800000000000 用, L3=256,L2=0)
    fullva_l2[0]   = PTE_V | PTE_AD | PTE_RWX | (0x80000000ULL >> 2);
    // L2[511]: 1GB superpage → 0x80000000 (给 0x7FFFFFFFFFFB 用, L3=255,L2=511)
    fullva_l2[511] = PTE_V | PTE_AD | PTE_RWX | (0x80000000ULL >> 2);
    hspt[0][255] = PTE_V | (((uintptr_t)fullva_l2) >> 2);
    hspt[0][256] = PTE_V | (((uintptr_t)fullva_l2) >> 2);
    asm volatile("sfence.vma" ::: "memory");

    // 测试1: bit47=1 的规范地址 0xFFFF800000000000 (L3=256, L2=0)
    // Sv48规范: bit47=1 → bits[63:48]=全1, 已映射, 应无异常
    uintptr_t vaddr_hi = 0xFFFF800000000000ULL;
    TEST_SETUP_EXCEPT();
    lb(vaddr_hi);
    TEST_ASSERT("s-mode lb canonical addr with bit47=1 (0xFFFF800000000000) no exception",
        excpt.triggered == false
    );

    // 测试2: bit47=1 的非规范地址 0x0000800000000000 (bit47=1但bits[63:48]=0)
    // fullva检查失败 → 应产生 page fault
    uintptr_t vaddr_bad = 0x0000800000000000ULL;
    TEST_SETUP_EXCEPT();
    lb(vaddr_bad);
    TEST_ASSERT("s-mode lb non-canonical addr with bit47=1 (0x0000800000000000) triggers PF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    uintptr_t Misalign_vaddr = 0x00007FFFFFFFFFFFULL;

    TEST_SETUP_EXCEPT();
    sd(Misalign_vaddr,0xdeadbeef);
    TEST_ASSERT("s-mode sd non-canonical addr with bit47=1 (0x00007FFFFFFFFFFF) triggers PF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF
    );
    excpt_info();

    TEST_SETUP_EXCEPT();
    ld(Misalign_vaddr);
    TEST_ASSERT("s-mode ld non-canonical addr with bit47=1 (0x00007FFFFFFFFFFF) triggers PF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );
    excpt_info();
    TEST_END();
}
