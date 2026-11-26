#include <rvh_test.h>
#include <page_tables.h>
#include <csrs.h> 

bool illegal_except_1() {

    TEST_START();

    TEST_SETUP_EXCEPT();

    //译码过程中decode table查表，若未查询到匹配项，赋予INVALID_INSTR
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    INVALID_INSTR();
    TEST_ASSERT("decode do not match leads to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    // 访问有效，CSRoptype非跳转行为，且访问CSR地址不在CSR mapping地址范围内
    
    TEST_SETUP_EXCEPT();
    CSRR(0xf16);
    TEST_ASSERT("CSR addr not match the csr mapping leads to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    //当前权限满足最低CSR访问权限，CSR写行为，但访问的CSR只读
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRW(CSR_MCONFIGPTR,0xfff);
    TEST_ASSERT("write csr which is read-only leads to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI 
    ); 


    //当前权限不满足最低CSR访问权限
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_MIE);
    TEST_ASSERT("low priviliege mode access high priviliege csr leads to illegal instruction interrupt",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();

}



bool illegal_except_2() {

    TEST_START();

    TEST_SETUP_EXCEPT();

    // 当前实现对WLRL字段写入非支持值做合法化/忽略，不产生非法指令异常。
    CSRW(CSR_MCAUSE,0xffffffffff);     
    TEST_ASSERT("write unsupported value into WLRL csr field does not trap in current implementation",
        excpt.triggered == false
    ); 


    TEST_END();
}



bool illegal_except_3() {

    TEST_START();

    //M模式下可以访问任何CSR
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_MCAUSE);
    CSRR(CSR_SCAUSE);
    TEST_ASSERT("m mode access any csr successful",
        excpt.triggered == false
    ); 

    reset_state();

    //尝试访问高特权级别的任何CSR
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_MCAUSE);
    TEST_ASSERT("hs mode access higher privilege csr leads to ili",
        excpt.triggered == true &&
        excpt.cause==CAUSE_ILI
    ); 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_MIDELEG);
    TEST_ASSERT("hu mode access higher privilege csr leads to ili",
        excpt.triggered == true &&
        excpt.cause==CAUSE_ILI
    );

    TEST_END();
}

//当寄存器为32位时才做该测试
/*
    //当V=1且XLEN>32时，尝试访问高半部分监管级CSR
    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_SSTATUS,1ULL << 35);
    TEST_ASSERT("When V=1 and XLEN>32, try to access the higher half of the regulatory CSR results in an illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause==CAUSE_ILI
    );

    //当V=1且XLEN>32时，尝试访问高半部分虚拟化扩展CSR
    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_HSTATUS,1ULL << 35);
    TEST_ASSERT("When V=1 and XLEN>32, try to access the higher half of the Virtualization extension CSR results in an illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause==CAUSE_ILI
    );

    //当V=1且XLEN>32时，尝试访问高半部分VS CSR
    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_VSSTATUS,1ULL << 35);
    TEST_ASSERT("When V=1 and XLEN>32, try to access the higher half of the vs CSR results in an illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause==CAUSE_ILI
    );

    //当V=1且XLEN>32时，尝试访问高半部分非特权CSR
    goto_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_CYCLEH,1ULL << 35);
    TEST_ASSERT("When V=1 and XLEN>32, try to access the higher half of the non-privileged CSR results in an illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause==CAUSE_ILI
    );
*/


bool illegal_except_4() {

    TEST_START();
    //TVM=1，在S模式下，读写satp 寄存器
    goto_priv(PRIV_M);
    CSRW(satp, 0x0);
    CSRS(CSR_MSTATUS, MSTATUS_TVM);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(satp);
    TEST_ASSERT("The Satp register was accessed in S mode without virtual memory enabled leads to ili when tvm=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();
}

bool illegal_except_5() {

    TEST_START();

    //TVM=1，在S模式下，执行SFENCE.VMA或SINVAL.VMA指令
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HS); 
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("s mode sfence.vma leads to ili when tvm=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("s mode sinval.vma leads to ili when tvm=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    //TVM=0，在S模式下，执行SFENCE.VMA或SINVAL.VMA指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HS); 
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("s mode sfence.vma leads to ili when tvm=1",
        excpt.triggered == false
    ); 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("s mode sinval.vma leads to ili when tvm=1",
        excpt.triggered == false
    ); 

    TEST_END();
}



bool illegal_except_6() {

    TEST_START();
    
    //mstatus.TVM=1，尝试在S模式下执行SINVAL.VMA
    goto_priv(PRIV_M);
    reset_state();
    CSRS(CSR_MSTATUS,MSTATUS_TVM);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("hs mode sinval.vma successful when mstatus.tvm=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 


    TEST_END();
}



bool illegal_except_7() {

    TEST_START();

    //在U模式下执行SINVAL.VMA中的任何一条
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    reset_state();
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("hu mode sinval.vma cause to illegal instruction interrupt",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();
}


bool illegal_except_8() {

    TEST_START();
    // 当前实现中satp.mode不是只读0，satp=0时HS执行sfence.vma合法。
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HS);
    CSRW(CSR_SATP,0ULL);
    sfence_vma();
    TEST_ASSERT("hs mode sfence.vma when satp.mode=0 succeeds in current implementation",
        excpt.triggered == false
    ); 

    TEST_END();
}




    // uintptr_t vaddr_f = hs_page_base(VSI_GI);  


    // //当V=1时，设置vsstatus.FS=0，HS级别的sstatus.FS=1时，尝试执行浮点指令（vsstatus.FS和HS级别的sstatus.FS同时生效才可以执行浮点指令）
    // goto_priv(PRIV_M);
    // TEST_SETUP_EXCEPT();
    // CSRS(CSR_MISA,1ULL << 7);        //开启h扩展(v=1)
    // CSRS(CSR_MISA,1ULL << 5 | 1ULL << 3);        //开启float和double
    // CSRC(CSR_VSSTATUS, (1ULL << 13) | (1ULL << 14));
    // CSRS(CSR_SSTATUS, (1ULL << 13) | (1ULL << 14));
    // flw(vaddr_f);
    // TEST_ASSERT("Execute floating-point instruction leads to ili when v=1 and vsstatus.fs=0 and sstatus.fs=1",
    //     excpt.triggered == true &&
    //     excpt.cause == CAUSE_ILI
    // ); 


    //当V=1时，设置vsstatus.FS=1，HS级别的sstatus.FS=0时，尝试执行浮点指令
    //当V=1时，设置vsstatus.FS=0，HS级别的sstatus.FS=0时，尝试执行浮点指令



    // //当V=1时，设置vsstatus.VS=0，HS级别的sstatus.VS=1时，尝试执行向量指令（vsstatus.VS和HS级别的sstatus.VS同时生效才可以执行向量指令）
    // goto_priv(PRIV_M);
    // TEST_SETUP_EXCEPT();
    // CSRS(CSR_MISA,1ULL << 7);        //开启h扩展(v=1)
    // CSRS(CSR_MISA,1ULL << 5 | 1ULL << 3);        //开启float和double
    // CSRC(CSR_VSSTATUS, (1ULL << 9) | (1ULL << 10));
    // CSRS(CSR_SSTATUS, (1ULL << 9) | (1ULL << 10));
    // vle32_v();
    // TEST_ASSERT("Execution vector instruction leads to ili when v=1 and vsstatus.fs=0 and sstatus.fs=1",
    //     excpt.triggered == true &&
    //     excpt.cause == CAUSE_ILI
    // ); 


    //当V=1时，设置vsstatus.VS=1，HS级别的sstatus.VS=0时，尝试执行向量指令（vsstatus.VS和HS级别的sstatus.VS同时生效才可以执行向量指令）
    //当V=1时，设置vsstatus.VS=0，HS级别的sstatus.VS=0时，尝试执行向量指令（vsstatus.VS和HS级别的sstatus.VS同时生效才可以执行向量指令）
    //扩展FS的状态设置为0时，尝试读取或者写入浮点对应的状态指令
    //扩展VS的状态设置为0时，尝试读取或者写入向量对应的状态指令
    //扩展XS的状态设置为0时，尝试读取或者写入其他扩展对应的状态指令
    //设置HU=0时，在U模式执行超级虚拟机指令

bool illegal_except_9() {

    TEST_START();
    //mstatus.TSR=1时，执行sret指令
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,1ULL << 22);    //TSR位

    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_HU);

    //TEST_EXEC_SRET();
    sret();

    TEST_ASSERT("hs mode sret cause to ili when mstatus.TSR=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();

}

bool illegal_except_10() {

    TEST_START();
    //mstatus.TSR=0时，执行sret指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,1ULL << 22);    //TSR位

    goto_priv(PRIV_HU);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);


    TEST_ASSERT("hs mode sret successful when mstatus.TSR=0",
        excpt.triggered == false
    ); 
    TEST_END();

}

bool illegal_except_11() {

    TEST_START();
    //m模式下执行dret指令，产生非法指令异常
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,1ULL << 22);    //TSR位

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    dret();

    TEST_ASSERT("m mode dret cause to ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 



    //HU模式下执行dret指令，产生非法指令异常
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,1ULL << 22);    //TSR位

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    dret();

    TEST_ASSERT("HU mode dret cause to ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 



    //hs模式下执行dret指令，产生非法指令异常
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,1ULL << 22);    //TSR位

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    dret();

    TEST_ASSERT("hs mode dret cause to ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 



    // TEST_END();

}


bool illegal_except_12() {

    TEST_START();
    //当mcounteren寄存器中的CY、TM、IR或HPMn位被清除时，在S模式或U模式下执行时尝试读取cycle、time、instret或hpmcountern寄存器
    goto_priv(PRIV_M);
    bool cond1,cond2,cond3,cond4;

    CSRW(CSR_MCOUNTEREN,0);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_CYCLE);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;
    TEST_SETUP_EXCEPT();
    CSRR(CSR_TIME);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;
    TEST_SETUP_EXCEPT();
    CSRR(CSR_INSTRET);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;
    TEST_SETUP_EXCEPT();
    CSRR(CSR_HPMCOUNTER10);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI; 

    TEST_ASSERT("hu mode read cycle/time/instret/hpmcounteren when mcounteren.cy/tm/ir/hpmn=0 cause to ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_CYCLE);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;
    TEST_SETUP_EXCEPT();
    CSRR(CSR_TIME);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;
    TEST_SETUP_EXCEPT();
    CSRR(CSR_INSTRET);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;
    TEST_SETUP_EXCEPT();
    CSRR(CSR_HPMCOUNTER10);
    cond1 = excpt.triggered == true && excpt.cause == CAUSE_ILI;  

    TEST_ASSERT("hs mode read cycle/time/instret/hpmcounteren when mcounteren.cy/tm/ir/hpmn=0 cause to ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();


}

bool illegal_except_13() {

    TEST_START();

    //TVM=任意，在u模式下，执行SFENCE.VMA或SINVAL.VMA指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HU); 
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("u mode sfence.vma leads to ili when tvm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("u mode sinval.vma leads to ili when tvm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_HU); 
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("u mode sfence.vma leads to ili when tvm=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("u mode sinval.vma leads to ili when tvm=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();
}


bool illegal_except_14() {

    TEST_START();

    //TVM=任意，在M模式下，执行SFENCE.VMA或SINVAL.VMA指令
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_M); 
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("m mode sfence.vma successful when tvm=0",
        excpt.triggered == false
    ); 

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("m mode sinval.vma successful when tvm=0",
        excpt.triggered == false
    ); 

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,MSTATUS_TVM);

    goto_priv(PRIV_M); 
    TEST_SETUP_EXCEPT();
    sfence_vma();
    TEST_ASSERT("m mode sfence.vma successful when tvm=1",
        excpt.triggered == false
    ); 

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    Sinval_vma();
    TEST_ASSERT("m mode sinval.vma successful when tvm=1",
        excpt.triggered == false
    ); 

    TEST_END();
}


bool illegal_except_15() {

    TEST_START();

    //在非debug模式下，访问debug csr
    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(0x7B0);
    TEST_ASSERT("m mode access dcsr cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    CSRR(0x7B1);
    TEST_ASSERT("s mode access dpc cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(0x7B2);
    TEST_ASSERT("u mode access dscratch cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 




    TEST_END();
}

bool illegal_except_16() {
    TEST_START();

    //在hs mode下执行mnret指令
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    mnret();
    TEST_ASSERT("hs mode mnret cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    //在hu mode下执行mnret指令
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    mnret();
    TEST_ASSERT("hu mode mnret cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    //在m mode下执行mnret指令
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    // Set mnepc to point to the next instruction after mnret
    asm volatile (
        "la t0, 1f\n"           // Load address of label 1 into t0
        "csrw 0x741, t0\n"      // Write to mnepc (CSR 0x741)
        ".insn r 0x73, 0, 0x38, x0, x0, x2\n"               // Execute mnret, should return to label 1
        "1:\n"                  // Label for return address
        ::: "t0"
    );
    TEST_ASSERT("m mode mnret successful",
        excpt.triggered == false
    );
}

bool illegal_except_17() {
    //S/U-mode访问pmacfg、pmaaddr
    TEST_START();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMACFG0); //pmacfg
    TEST_ASSERT("hs mode access pmacfg cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMAADDR0); //pmaaddr
    TEST_ASSERT("hs mode access pmaaddr cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMPCFG0); //pmpcfg
    TEST_ASSERT("hs mode access pmpcfg cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMAADDR0); //pmpaddr
    TEST_ASSERT("hs mode access pmpaddr cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );
    
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMACFG0); //pmacfg
    TEST_ASSERT("hu mode access pmacfg cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMAADDR0); //pmaaddr
    TEST_ASSERT("hu mode access pmaaddr cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMACFG0); //pmpcfg
    TEST_ASSERT("hu mode access pmpcfg cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRR(CSR_PMPADDR0); //pmpaddr
    TEST_ASSERT("hu mode access pmpaddr cause to illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

}
