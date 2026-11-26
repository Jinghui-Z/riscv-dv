#include <rvh_test.h>
#include <dynamic_page_tables.h>

// 测试用例：动态切换sv39和sv48页表模式

// 测试sv39模式基本功能测试
bool test_sv39_mode() {
    TEST_START();
    
    // 设置为sv39模式
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    
    // 验证模式设置
    TEST_ASSERT("Mode should be SV39",get_page_table_mode() == PAGE_TABLE_MODE_SV39);
    TEST_ASSERT("SV39 should have 3 levels",get_page_table_levels() == 3);
    TEST_ASSERT("SV39 should have 39-bit VA",get_virtual_address_bits() == 39);
    TEST_ASSERT("SV39 should have 38-bit PPN",get_physical_page_number_bits() == 38);
    
    goto_priv(PRIV_HS);
    hspt_init();

    
    // 测试基本访存
    uintptr_t test_addr = vs_page_base(VSRWX_GURWX);
    volatile uint64_t *ptr = (volatile uint64_t *)test_addr;
    
    *ptr = 0xDEADBEEF12345678ULL;
    
    // 读取并验证
    uint64_t val = *ptr;
    TEST_ASSERT("SV39 read/write test successful",
        val == 0xDEADBEEF12345678ULL
    );
    
    TEST_END();
}

// 测试sv48模式基本功能测试
bool test_sv48_mode() {
    TEST_START();
    
    // 设置为sv48模式
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    
    // 验证模式设置
    TEST_ASSERT("Mode should be SV48",get_page_table_mode() == PAGE_TABLE_MODE_SV48);
    TEST_ASSERT("SV48 should have 4 levels",get_page_table_levels() == 4);
    TEST_ASSERT("SV48 should have 48-bit VA",get_virtual_address_bits() == 48);
    TEST_ASSERT("SV48 should have 44-bit PPN",get_physical_page_number_bits() == 44);
    
    goto_priv(PRIV_HS);
    hspt_init();
    
    // 测试基本访存
    uintptr_t test_addr = vs_page_base(VSRWX_GURWX);
    volatile uint64_t *ptr = (volatile uint64_t *)test_addr;
    
    // 写入测试数据
    *ptr = 0xCAFEBABE87654321ULL;
    
    // 读取并验证
    uint64_t val = *ptr;
    TEST_ASSERT("SV48 read/write test successful",
        val == 0xCAFEBABE87654321ULL
    );
    
    TEST_END();
}


bool test_sv39_to_sv48_switch() {
    TEST_START();
    
    // 测试sv39到sv48的模式切换
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    goto_priv(PRIV_HS);
    hspt_init();
    
    uintptr_t test_addr1 = vs_page_base(VSRWX_GURWX);
    volatile uint64_t *ptr1 = (volatile uint64_t *)test_addr1;
    *ptr1 = 0x1111111111111111ULL;
    
    TEST_ASSERT("SV39 write failed before switch",
        *ptr1 == 0x1111111111111111ULL
    );
    
    // 切换到sv48
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    TEST_ASSERT("Mode switch to SV48 successful",get_page_table_mode() == PAGE_TABLE_MODE_SV48);
    
    hspt_init();
    sfence_vma();

    // 在sv48模式下测试
    uintptr_t test_addr2 = vs_page_base(VSRWX_GURW);
    volatile uint64_t *ptr2 = (volatile uint64_t *)test_addr2;
    *ptr2 = 0x4444444444444444ULL;
    
    TEST_ASSERT("SV48 write successful after switch",
        *ptr2 == 0x4444444444444444ULL
    );
    
    TEST_END();
}

bool test_sv48_to_sv39_switch() {
    TEST_START();
    
    // 测试sv48到sv39的模式切换

    // 初始化为sv48
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    goto_priv(PRIV_HS);
    hspt_init();
    
    uintptr_t test_addr1 = vs_page_base(VSRW_GURWX);
    volatile uint64_t *ptr1 = (volatile uint64_t *)test_addr1;
    *ptr1 = 0x8888888888888888ULL;
    
    TEST_ASSERT("SV48 write successful before switch",
        *ptr1 == 0x8888888888888888ULL
    );
    
    // 切换到sv39
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    TEST_ASSERT("Mode switch to SV39 failed",get_page_table_mode() == PAGE_TABLE_MODE_SV39);
    
    hspt_init();
    sfence_vma();

    // 在sv39模式下测试
    uintptr_t test_addr2 = vs_page_base(VSRW_GURW);
    volatile uint64_t *ptr2 = (volatile uint64_t *)test_addr2;
    *ptr2 = 0x9999999999999999ULL;
    
    TEST_ASSERT("SV39 write successful after switch",
        *ptr2 == 0x9999999999999999ULL
    );
    
    TEST_END();
}


bool test_multiple_switches_s_mode() {
    TEST_START();
    
    // 测试多次来回切换

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);

    goto_priv(PRIV_HS);

    for (int i = 0; i < 3; i++) {
        // 切换到sv39
        set_page_table_mode(PAGE_TABLE_MODE_SV39);
        hspt_init();
        sfence_vma();

        uintptr_t addr39 = vs_page_base(SCRATCHPAD);
        volatile uint64_t *ptr39 = (volatile uint64_t *)addr39;
        *ptr39 = 0xAAAA0000 + i;
        TEST_ASSERT("SV39 successful in iteration",
            *ptr39 == (0xAAAA0000 + i)
        );

        //切换到bare模式
        CSRW(CSR_SATP, 0);
        uintptr_t addr1 = phys_page_base(SCRATCHPAD);
        volatile uint64_t *ptr1 = (volatile uint64_t *)addr1;
        *ptr1 = 0xCCCC0000 + i;
        TEST_ASSERT("bare mode successful in iteration",
            *ptr1 == (0xCCCC0000 + i)
        );
        
        // 切换到sv48
        set_page_table_mode(PAGE_TABLE_MODE_SV48);
        hspt_init();
        sfence_vma();

        uintptr_t addr48 = vs_page_base(SCRATCHPAD);
        volatile uint64_t *ptr48 = (volatile uint64_t *)addr48;
        *ptr48 = 0xBBBB0000 + i;
        TEST_ASSERT("SV48 successful in iteration",
            *ptr48 == (0xBBBB0000 + i)
        );

        //切换到bare模式
        CSRW(CSR_SATP, 0);
        uintptr_t addr2 = phys_page_base(SCRATCHPAD);
        volatile uint64_t *ptr2 = (volatile uint64_t *)addr2;
        *ptr2 = 0xCCCC0000 + i;
        TEST_ASSERT("bare mode successful in iteration",
            *ptr2 == (0xCCCC0000 + i)
        );
    }

    for (int i = 0; i < 3; i++) {
        // 切换到sv39
        set_page_table_mode(PAGE_TABLE_MODE_SV39);
        hspt_init();
        sfence_vma();

        uintptr_t addr39 = vs_page_base(SCRATCHPAD);
        volatile uint64_t *ptr39 = (volatile uint64_t *)addr39;
        *ptr39 = 0xAAAA0000 + i;
        TEST_ASSERT("SV39 successful in iteration",
            *ptr39 == (0xAAAA0000 + i)
        );
       
        // 切换到sv48
        set_page_table_mode(PAGE_TABLE_MODE_SV48);
        hspt_init();
        sfence_vma();

        uintptr_t addr48 = vs_page_base(SCRATCHPAD);
        volatile uint64_t *ptr48 = (volatile uint64_t *)addr48;
        *ptr48 = 0xBBBB0000 + i;
        TEST_ASSERT("SV48 successful in iteration",
            *ptr48 == (0xBBBB0000 + i)
        );

    }
    
    TEST_END();
}

bool test_multiple_switches_u_mode() {
    TEST_START();
    
    // 测试多次来回切换
    goto_priv(PRIV_M);
    CSRC(CSR_MEDELEG,1ULL << CAUSE_ECU);
    printf("Medeleg after clearing ECU: 0x%lx\n", CSRR(CSR_MEDELEG));
    for (int i = 0; i < 3; i++) {
        // 切换到sv39
        set_page_table_mode(PAGE_TABLE_MODE_SV39);
        goto_priv(PRIV_M);
        hspt_init();
        hspt_u_mode_allow();
        sfence_vma();

        goto_priv(PRIV_HU);
        uintptr_t addr39 = vs_page_base(SCRATCHPAD);
        TEST_SETUP_EXCEPT();
        ld(addr39);
        TEST_ASSERT("SV39 successful in iteration",
            excpt.triggered == false
        );

        //切换到bare模式
        goto_priv_u_to_m_direct(PRIV_M);
        CSRW(CSR_SATP,0);
        goto_priv(PRIV_HU);
        uintptr_t addr0 = phys_page_base(SCRATCHPAD);
        TEST_SETUP_EXCEPT();
        ld(addr0);
        TEST_ASSERT("bare mode successful in iteration",
            excpt.triggered == false
        );
        
        // 切换到sv48
        goto_priv_u_to_m_direct(PRIV_M);
        set_page_table_mode(PAGE_TABLE_MODE_SV48);
        hspt_init();
        hspt_u_mode_allow();
        sfence_vma();
        goto_priv(PRIV_HU);

        uintptr_t addr48 = vs_page_base(SCRATCHPAD);
        TEST_SETUP_EXCEPT();
        ld(addr48);
        TEST_ASSERT("SV48 successful in iteration",
            excpt.triggered == false
        );

        //切换到bare模式
        goto_priv_u_to_m_direct(PRIV_M);
        CSRW(CSR_SATP,0);
        goto_priv(PRIV_HU);
        addr0 = phys_page_base(SCRATCHPAD);
        TEST_SETUP_EXCEPT();
        ld(addr0);
        TEST_ASSERT("bare mode successful in iteration",
            excpt.triggered == false
        );

        goto_priv_u_to_m_direct(PRIV_M);

    }

    for (int i = 0; i < 3; i++) {
        // 切换到sv39
        goto_priv(PRIV_M);
        set_page_table_mode(PAGE_TABLE_MODE_SV39);
        hspt_init();
        hspt_u_mode_allow();
        sfence_vma();

        goto_priv(PRIV_HU);
        uintptr_t addr39 = vs_page_base(SCRATCHPAD);
        TEST_SETUP_EXCEPT();
        ld(addr39);
        TEST_ASSERT("SV39 successful in iteration",
            excpt.triggered == false
        );
       
        // 切换到sv48
        goto_priv_u_to_m_direct(PRIV_M);
        set_page_table_mode(PAGE_TABLE_MODE_SV48);
        hspt_init();
        hspt_u_mode_allow();
        sfence_vma();

        goto_priv(PRIV_HU);
        uintptr_t addr48 = vs_page_base(SCRATCHPAD);
        TEST_SETUP_EXCEPT();
        ld(addr48);
        TEST_ASSERT("SV48 successful in iteration",
            excpt.triggered == false
        );

        goto_priv_u_to_m_direct(PRIV_M);

    }

    goto_priv(PRIV_M);
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    hspt_init();
    sfence_vma();
    
    TEST_END();
}



bool satp_mode_test() {
    TEST_START();
    
    uintptr_t vaddr = vs_page_base(VSRWX_GURWX);
    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t paddr1 = phys_page_base(X);

    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    sd(paddr,0xdeadbeef);
    sd(paddr1,0xdeadbeef);

    // 初始设置为sv39
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    printf("Initial mode should be SV39\n");
    
    goto_priv(PRIV_HS);
    hspt_init();
    
    TEST_SETUP_EXCEPT();
    uint64_t val = ld(vaddr);
    TEST_ASSERT("Access in SV39 mode",
        val == 0xdeadbeef
    );

    // 切换到sv48
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    printf("SV39 to SV48\n");
    goto_priv(PRIV_M);
    hspt_init();

    //切换 satp.MODE（非bare切换），不执行 SFENCE.VMA，再访存同一地址

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    
    val = ld(vaddr);
    TEST_ASSERT("Access in SV48 mode(without SFENCE.VMA)",     //待修改更复杂
        val == 0xdeadbeef && excpt.triggered == false
    );


    //切换 satp.MODE（非bare切换），rs1=x0 和 rs2=x0，执行 SFENCE.VMA，再访存同一地址
    sfence_vma();

    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access in SV48 mode(after SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    //再切换到sv39
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    printf("SV48 to SV39\n");
    goto_priv(PRIV_M);
    hspt_init();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();

    val = ld(vaddr);
    TEST_ASSERT("Access in SV39 mode(without SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access in SV39 mode(after SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );
    
    //再切换到bare
    printf("SV39 to bare\n");
    CSRW(CSR_SATP, 0);

    val = ld(paddr);
    TEST_ASSERT("Access in bare mode(without SFENCE.VMA)",
        val == 0xdeadbeef
    );

    ld(vaddr);

    //再切换到sv48
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    printf("bare to SV48\n");
    goto_priv(PRIV_M);
    hspt_init();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    
    val = ld(vaddr);
    TEST_ASSERT("Access in SV48 mode(without SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access in SV48 mode(after SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    //再切换到bare
    printf("SV48 to bare\n");
    CSRW(CSR_SATP, 0);

    val = ld(paddr);
    TEST_ASSERT("Access in bare mode(without SFENCE.VMA)",
        val == 0xdeadbeef
    );

    //再切换到sv39
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    printf("bare to SV39\n");
    goto_priv(PRIV_M);
    hspt_init();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();

    val = ld(vaddr);
    TEST_ASSERT("Access in SV39 mode(without SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    sfence_vma();

    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access in SV39 mode(after SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    TEST_END();
}

bool satp_ppn_test() {
    TEST_START();
    
    uintptr_t vaddr = vs_page_base(VSRWX_GURWX);
    uintptr_t paddr1 = phys_page_base(VSRWX_GURWX);

    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    sd(paddr1,0xdeadbeef);

    // 初始设置为sv48
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    printf("Initial mode should be SV48\n");
    
    goto_priv(PRIV_HS);
    hspt_init();
    
    TEST_SETUP_EXCEPT();
    uint64_t val = ld(vaddr);
    TEST_ASSERT("Access with initial PPN successful",
        val == 0xdeadbeef
    );

// 修改satp.PPN指向另一个物理页

    goto_priv(PRIV_M);

    // 取出旧satp.mode
    uint64_t old_satp = CSRR(CSR_SATP);
    uint64_t satp_mode = old_satp & 0xF000000000000000ULL; 
    
    // 创建一个新的页表根地址（使用hspt[1][0]作为新的根）
    uintptr_t new_root_ppn = ((uintptr_t)&hspt[1][0]) >> 12;
    
    // 构造新的satp值
    uint64_t new_satp = satp_mode | (new_root_ppn & 0x00000FFFFFFFFFFFULL);  // SATP64_PPN mask
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.PPN to new physical page (PPN=0x%lx)\n", new_root_ppn);

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    
    val = ld(vaddr);
    TEST_ASSERT("Access after changing PPN(without SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    // 执行 SFENCE.VMA 后再访存
    sfence_vma();
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access after changing PPN(after SFENCE.VMA)",
        val != 0xdeadbeef || excpt.triggered == true
    );

    CSRW(CSR_SATP,0);

    // 设置为sv39
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    printf("Switch mode to SV39\n");
    
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access with initial PPN successful",
        val == 0xdeadbeef
    );

// 修改satp.PPN指向另一个物理页

    goto_priv(PRIV_M);
    uintptr_t addr = 0x00000000;
    for(int i = 0; i < 4; i++){
        hspt[3][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
        addr += SUPERPAGE_SIZE(0);
    }

    // 取出旧satp.mode
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; 
    
    // 创建一个新的页表根地址（使用hspt[3][0]作为新的根）
    new_root_ppn = ((uintptr_t)&hspt[3][0]) >> 12;
    
    // 构造新的satp值
    new_satp = satp_mode | (new_root_ppn & 0x00000FFFFFFFFFFFULL);  // SATP64_PPN mask
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.PPN to new physical page (PPN=0x%lx)\n", new_root_ppn);

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    
    val = ld(vaddr);
    TEST_ASSERT("Access after changing PPN(without SFENCE.VMA)",
        val == 0xdeadbeef && excpt.triggered == false
    );

    // 执行 SFENCE.VMA 后再访存
    sfence_vma();
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);
    TEST_ASSERT("Access after changing PPN(after SFENCE.VMA)",
        val != 0xdeadbeef || excpt.triggered == true
    );


}

bool satp_ppn_asid_limit_test() {
    TEST_START();

    //测试satp.ppn以及satp.asid的max、min值组合

    uintptr_t vaddr = vs_page_base(VSRWX_GURWX);
    uintptr_t paddr1 = phys_page_base(VSRWX_GURWX);

    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    sd(paddr1,0xdeadbeef);

    // 初始设置为sv48，satp.asid=0,satp.ppn=hspt根页表物理页
    set_page_table_mode(PAGE_TABLE_MODE_SV48);
    printf("Initial mode should be SV48, ASID=0\n");
    
    goto_priv(PRIV_HS);
    hspt_init();
    
    TEST_SETUP_EXCEPT();
    uint64_t val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=max值
    goto_priv(PRIV_M);
    uint64_t old_satp = CSRR(CSR_SATP);
    uint64_t satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    uint64_t new_satp = satp_mode | (0xFFFFULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0xFFFF + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to max value\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0x0ULL; // MODE + ASID=0x0 + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to min value 0x0\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0x0ULL; // MODE + ASID=0xFFFF + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to min value 0x0\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0x0 + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to max value\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0xFFFF + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to max value\n"); 
    printf("New satp: 0x%lx\n", new_satp);


    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0x0ULL; // MODE + ASID=0x0 + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to min value 0x0\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0x0ULL; // MODE + ASID=0xFFFF + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to min value 0x0\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0x0 + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to max value\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // satp.mode切换为sv39
    set_page_table_mode(PAGE_TABLE_MODE_SV39);
    printf("change satp.mode to SV39, ASID=0\n"); 
    printf("New satp: 0x%lx\n", new_satp);

    
    // goto_priv(PRIV_HS);
    hspt_init();
    
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0xFFFF + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to max value\n"); 
    printf("New satp: 0x%lx\n", new_satp);


    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0x0ULL; // MODE + ASID=0x0 + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to min value 0x0\n");
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0x0ULL; // MODE + ASID=0xFFFF + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to min value 0x0\n");
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0x0 + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to max value\n");
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);


    //切换satp.mode为bare
    printf("change satp.mode to bare\n");

    CSRW(CSR_SATP, 0);
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0xFFFF + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to max value\n");
    printf("New satp: 0x%lx\n", new_satp);


    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0x0ULL; // MODE + ASID=0x0 + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to min value 0x0\n");
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最大值0xFFFF, satp.ppn=min值0x0
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0xFFFFULL << 44) | 0x0ULL; // MODE + ASID=0xFFFF + PPN=0x0
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to max value 0xFFFF,satp.ppn to min value 0x0\n");
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    // 修改satp.asid为最小值0x0, satp.ppn=max值
    goto_priv(PRIV_M);
    old_satp = CSRR(CSR_SATP);
    satp_mode = old_satp & 0xF000000000000000ULL; // 保留MODE字段
    new_satp = satp_mode | (0x0ULL << 44) | 0xFFFFFFFFFFFULL; // MODE + ASID=0x0 + PPN=max
    CSRW(CSR_SATP, new_satp);
    printf("Change satp.ASID to min value 0x0,satp.ppn to max value\n");
    printf("New satp: 0x%lx\n", new_satp);

    // goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    val = ld(vaddr);

    TEST_END();
}
