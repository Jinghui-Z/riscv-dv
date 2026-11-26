#include <rvh_test.h>
#include <page_tables.h>

bool timecmp_test_1(){

    //当menvcfg.stce=0 mcounteren.tm=0 时 ，hu、hs不可访问stimecmp

    TEST_START();    
    CSRC(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRC(CSR_MCOUNTEREN, HCOUNTEREN_TM); 

    goto_priv(PRIV_HS);


    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hs CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=0 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP, 1);

    TEST_ASSERT("hs CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


    goto_priv(PRIV_HU);


    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hu CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP, 1);

    TEST_ASSERT("hu CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


}

bool timecmp_test_2() {


    //当menvcfg.stce=1 mcounteren.tm=0时，hs/hu不可访问timecmp

    TEST_START();    
    CSRS(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRC(CSR_MCOUNTEREN, HCOUNTEREN_TM); 


    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hu CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("hu CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


    goto_priv(PRIV_HS);


    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hs CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("hs CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


}


bool timecmp_test_3() {


    //当menvcfg.stce=1 mcounteren.tm=1时 ，hu不可访问stimecmp

    TEST_START();    
    CSRS(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRS(CSR_MCOUNTEREN, HCOUNTEREN_TM); 


    goto_priv(PRIV_HU);


    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hu CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=1 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP, 1);

    TEST_ASSERT("hu CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=1 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


}

bool timecmp_test_4() {


    //当menvcfg.stce=1 mcounteren.tm=1 时 ，m、hs可访问timecmp

    TEST_START();    
    CSRS(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRS(CSR_MCOUNTEREN, HCOUNTEREN_TM); 
   
    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("m CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=1",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("m CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=1",
        excpt.triggered == false
    );


    goto_priv(PRIV_HS);


    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hs CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=1",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("hs CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=1 mcounteren.tm=1",
        excpt.triggered == false
    );
}



bool timecmp_test_5() {


    //当menvcfg.stce=0 mcounteren.tm=1 ，hu不可访问timecmp、vstimecmp

    TEST_START();    
    CSRC(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRS(CSR_MCOUNTEREN, HCOUNTEREN_TM); 

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hs CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=1 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("hs CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=1 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("hu CSRR stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=1 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("hu CSRS(imm=1)  stimecmp casuses illegal instruction exception when menvcfg.stce=0 mcounteren.tm=1 ",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

}





bool timecmp_test_6() {


    //当menvcfg.stce和mcounteren.tm=00、01、10 时 ，m可访问stimecmp、vstimecmp

    TEST_START();    
    CSRC(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRC(CSR_MCOUNTEREN, HCOUNTEREN_TM); 

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("M CSRR stimecmp successful when menvcfg.stce=0 mcounteren.tm=0 ",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("M CSRS(imm=1)  stimecmp successful when menvcfg.stce=0 mcounteren.tm=0",
        excpt.triggered == false
    );

    CSRC(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRS(CSR_MCOUNTEREN, HCOUNTEREN_TM); 

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("M CSRR stimecmp successful when menvcfg.stce=0 mcounteren.tm=1",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("M CSRS(imm=1)  stimecmp successful when menvcfg.stce=0 mcounteren.tm=1",
        excpt.triggered == false
    );

    CSRS(CSR_MENVCFG, 1ULL << 63);  //STCE位 
    CSRC(CSR_MCOUNTEREN, HCOUNTEREN_TM); 

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_STIMECMP);

    TEST_ASSERT("M CSRR stimecmp successful when menvcfg.stce=1 mcounteren.tm=0",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    CSRS(CSR_STIMECMP,1ULL << 0);

    TEST_ASSERT("M CSRS(imm=1)  stimecmp successful when menvcfg.stce=1 mcounteren.tm=0",
        excpt.triggered == false
    );

}



bool menvcfg_cbze_tests_1(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("m execute cbo.zero when menvcfg.cbze=0 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("hs execute cbo.zero when menvcfg.cbze=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("u execute cbo.zero when menvcfg.cbze=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


    TEST_END();

}


bool menvcfg_cbze_tests_2(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    CSRC(CSR_SENVCFG, MENVCFG_CBZE);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("m execute cbo.zero when menvcfg.cbze=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("hs execute cbo.zero when menvcfg.cbze=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("u execute cbo.zero when menvcfg.cbze=1 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


    TEST_END();

}

bool senvcfg_cbze_tests_3(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    CSRS(CSR_SENVCFG, MENVCFG_CBZE);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_zero(0x80000000);
    TEST_ASSERT("u execute cbo.zero when menvcfg.cbze=1 and senvcfg.cbze=1 successful",
        excpt.triggered == false
    );


    TEST_END();

}

bool menvcfg_cbzfe_tests_1(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("m execute cbo.clean when menvcfg.cbzfe=0 successful",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("m execute cbo.flush when menvcfg.cbzfe=0 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("hs execute cbo.clean when menvcfg.cbzfe=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("hs execute cbo.flush when menvcfg.cbzfe=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("u execute cbo.clean when menvcfg.cbzfe=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("u execute cbo.flush when menvcfg.cbzfe=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


    TEST_END();

}


bool menvcfg_cbzfe_tests_2(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    CSRC(CSR_SENVCFG, MENVCFG_CBZFE);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("m execute cbo.clean when menvcfg.cbzfe=1 successful",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("m execute cbo.flush when menvcfg.cbzfe=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("hs execute cbo.clean when menvcfg.cbzfe=1 successful",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("hs execute cbo.flush when menvcfg.cbzfe=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("u execute cbo.clean when menvcfg.cbzfe=1 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("u execute cbo.flush when menvcfg.cbzfe=1 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_END();

}

bool senvcfg_cbzfe_tests_3(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    CSRS(CSR_SENVCFG, MENVCFG_CBZFE);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_clean(0x80000000);
    TEST_ASSERT("u execute cbo.clean when menvcfg.cbzfe=1 and senvcfg.cbzfe=1 successful",
        excpt.triggered == false
    );
    TEST_SETUP_EXCEPT();
    cbo_flush(0x80000000);
    TEST_ASSERT("u execute cbo.flush when menvcfg.cbzfe=1 and senvcfg.cbzfe=1 successful",
        excpt.triggered == false
    );


    TEST_END();

}



bool menvcfg_cbie_tests_1(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE11);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("m execute cbo.inval when menvcfg.cbie=0 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("hs execute cbo.inval when menvcfg.cbie=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=0 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );


    TEST_END();

}


bool menvcfg_cbie_tests_2(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    CSRC(CSR_SENVCFG, MENVCFG_CBIE11);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("m execute cbo.inval when menvcfg.cbie=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("hs execute cbo.inval when menvcfg.cbie=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=1 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE11);
    CSRC(CSR_SENVCFG, MENVCFG_CBIE11);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("m execute cbo.inval when menvcfg.cbie=3 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("hs execute cbo.inval when menvcfg.cbie=3 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=3 causes illegal instruction exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    );

    TEST_END();

}

bool senvcfg_cbie_tests_3(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE11);
    CSRS(CSR_SENVCFG, MENVCFG_CBIE11);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=3 and senvcfg.cbie=3 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    CSRS(CSR_SENVCFG, MENVCFG_CBIE01);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=1 and senvcfg.cbie=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE11);
    CSRS(CSR_SENVCFG, MENVCFG_CBIE01);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=3 and senvcfg.cbie=1 successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    CSRS(CSR_SENVCFG, MENVCFG_CBIE11);
    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    cbo_inval(0x80000000);
    TEST_ASSERT("u execute cbo.inval when menvcfg.cbie=1 and senvcfg.cbie=3 successful",
        excpt.triggered == false
    );

    TEST_END();

}

bool pmp_test_1(){
    
    TEST_START();

    //使用R=0且W=1的pmpcfg的配置

    goto_priv(PRIV_M);

    //模拟linknan的PMA环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);

    //配置pmpcfg0的pmp0为R=0,W=1,X=0的配置
    CSRW(CSR_PMPADDR0, 0x800000000 >> 2);
    CSRW(CSR_PMPADDR1, 0x810000000 >> 2);  
    CSRC(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A

    printf("pmpcfg0: %llx\n", CSRR(CSR_PMPCFG0));
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sw(0x800030000, 0x12345678);   //hs模式下执行store指令

    TEST_ASSERT("hs mode store when pmpcfg.R=0 and pmpcfg.W=1 cause to SAF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}

bool pmp_test_2(){
    //PMP.A=1或0，写入非4KB粒度对齐的pmpaddr
    TEST_START();
    goto_priv(PRIV_M);
    //模拟linknan的PMA环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);

    //PMP.A=1，写入非4KB粒度对齐的pmpaddr
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 1ULL << 11); //PMP1cfg.A
    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMPADDR1, 0x81000FF1);
    //验证读出pmpaddr[9:0]位全为0
    TEST_ASSERT("pmpcfg.A=1 write non-4KB aligned pmpaddr causes addr[9:0]=0",
        (CSRR(CSR_PMPADDR1) & 0x3FF) == 0 &&
        (CSRR(CSR_PMPADDR1) & 0x400) != 0
    );

    //PMP.A=3，写入非4KB粒度对齐的pmpaddr
    CSRS(CSR_PMPCFG0, 3ULL << 11); //PMP1cfg.A=3
    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMPADDR1, 0x81000FF1);
    //验证读出pmpaddr[8:0]位全为1
    TEST_ASSERT("pmpcfg.A=3 write non-4KB aligned pmpaddr causes addr[8:0]=1",
        (CSRR(CSR_PMPADDR1) & 0x1FF) == 0x1FF &&
        (CSRR(CSR_PMPADDR1) & 0x200) != 0
    );


}

bool pmp_test_3(){
    TEST_START();

    //模拟linknan的PMA环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);
    CSRW(CSR_PMPADDR0, 0x0);
    CSRW(CSR_PMPADDR1, 0x0);
    CSRW(CSR_PMPADDR2, 0x0);
    CSRW(CSR_PMPADDR3, 0x0);

    //L=1，pmpcfgi.A=NAPOT，对第i条PMP进行写入,第i项PMP条目写入被忽略
    goto_priv(PRIV_M);
    CSRS(CSR_PMPCFG0, 3ULL << 3); //PMP0cfg.A=3(NAPOT)
    CSRS(CSR_PMPCFG0, 1ULL << 7); //PMP0cfg.L=1
    uint64_t origin_value = CSRR(CSR_PMPADDR0);
    uint64_t origin_value_1 = CSRR(CSR_PMPCFG0);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMPADDR0, 0x81000FF1);
    CSRC(CSR_PMPCFG0, 3ULL << 3);
    //验证读出pmpaddr未被写入
    TEST_ASSERT("L=1,pmpcfg.A=NAPOT write pmpaddr ignored",
        CSRR(CSR_PMPADDR0) == origin_value &&
        CSRR(CSR_PMPCFG0) == origin_value_1
    );
    
    //L=1，pmpcfgi.A=TOR，对第i条PMP进行写入,第i项pmpcfg，和第i项和i-1项pmpaddr写入被忽略
    goto_priv(PRIV_M);
    CSRS(CSR_PMPCFG0, 1ULL << 19); //PMP2cfg.A=2(TOR)
    CSRS(CSR_PMPCFG0, 1ULL << 23); //PMP2cfg.L=1

    origin_value = CSRR(CSR_PMPADDR1);
    origin_value_1 = CSRR(CSR_PMPCFG0);
    uint64_t origin_value_2 = CSRR(CSR_PMPADDR2);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMPADDR1, 0x81000FF1);
    CSRW(CSR_PMPADDR2, 0x81000FF1);
    CSRC(CSR_PMPCFG0, 1ULL << 19);

    //验证读出pmpaddr未被写入
    TEST_ASSERT("L=1,pmpcfg.A=TOR write pmpaddr ignored",
        CSRR(CSR_PMPADDR1) == origin_value &&
        CSRR(CSR_PMPCFG0) == origin_value_1 &&        
        CSRR(CSR_PMPADDR2) == origin_value_2 
    );

    //L=1，pmpcfgi.A=OFF，对第i条PMP进行写入,第i 项PMP条目写入被忽略
    goto_priv(PRIV_M);
    CSRC(CSR_PMPCFG0, 3ULL << 27); //PMP3cfg.A=0(OFF)
    CSRS(CSR_PMPCFG0, 1ULL << 31); //PMP3cfg.L=1

    origin_value = CSRR(CSR_PMPADDR3);
    origin_value_1 = CSRR(CSR_PMPCFG0);

    TEST_SETUP_EXCEPT();

    CSRW(CSR_PMPADDR3, 0x81000FF4);
    CSRS(CSR_PMPCFG0, 3ULL << 27);
    //验证读出pmpaddr未被写入

    TEST_ASSERT("L=1,pmpcfg.A=OFF write pmpaddr ignored",
        CSRR(CSR_PMPADDR3) == origin_value &&
        CSRR(CSR_PMPCFG0) == origin_value_1   
    );

}

bool pmp_test_4(){
    //当 pmpcfgi.A=3,napot根据手册Table19要求匹配地址
    TEST_START();
    goto_priv(PRIV_M);

    // //PMA模拟linknan的PMA环境
    // CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    // CSRW(CSR_PMACFG0, 0x6F);

    // CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    // CSRW(CSR_PMAADDR12, 0x20000000);
    // CSRW(CSR_PMAADDR13, 0x120000000);
    // CSRW(CSR_PMAADDR14, 0xC000000000);
    // CSRW(CSR_PMAADDR15, 0x10000000000);

    // CSRW(CSR_PMACFG0, 0x0);
    // CSRW(CSR_PMAADDR1, 0x0);
    // CSRW(CSR_PMAADDR0, 0x0);

    // //pmpcfgi.A=3，napot，根据手册Table19要求匹配地址
    // CSRS(CSR_PMACFG0, 1ULL << 8); //PMA1cfg.R
    // CSRS(CSR_PMACFG0, 1ULL << 9); //PMA1cfg.W
    // CSRS(CSR_PMACFG0, 1ULL << 10); //PMA1cfg.X
    // CSRS(CSR_PMACFG0, 1ULL << 14); //PMA1cfg.C
    // CSRS(CSR_PMACFG0, 3ULL << 11); //PMA1cfg.A=3(NAPOT)
    // CSRW(CSR_PMPADDR1, 0x2F000DFF);


    //PMP模拟linknan的PMA环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);
    CSRW(CSR_PMPADDR1, 0x0);
    CSRW(CSR_PMPADDR0, 0x0);

    //pmpcfgi.A=3，napot，根据手册Table19要求匹配地址
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 3ULL << 11); //PMP1cfg.A=3(NAPOT)
    //配置pmpaddr1为0x80000FFF对应的NAPOT地址范围
    CSRW(CSR_PMPADDR1, 0x2F00FFFF);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();

    sb(0x2F000000ULL << 2, 0x1);   //hs模式下执行store指令，地址在范围内
    TEST_ASSERT("hs mode store when pmpcfg.A=NAPOT and addr in range successful",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    CSRC(CSR_PMPCFG0, 1ULL << 9); //PMP1cfg.W
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sb(0x2F000000ULL << 2, 0x1);   //hs模式下执行store指令，地址在范围内
    TEST_ASSERT("hs mode store when pmpcfg.A=NAPOT and addr in range with W=0 leads to PMP exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

}

bool pmp_test_5(){
    //从napot变为tor/off再变回napot,开始和结尾pmpaddr的值不变

    TEST_START();
    goto_priv(PRIV_M);
    //模拟linknan的PMA环境
    CSRW(CSR_PMPADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMPCFG0, 0x6F);

    CSRW(CSR_PMPCFG2, 0xF006F0F00000000);
    CSRW(CSR_PMPADDR12, 0x20000000);
    CSRW(CSR_PMPADDR13, 0x120000000);
    CSRW(CSR_PMPADDR14, 0xC000000000);
    CSRW(CSR_PMPADDR15, 0x10000000000);

    CSRW(CSR_PMPCFG0, 0x0);
    CSRW(CSR_PMPADDR1, 0x0);
    CSRW(CSR_PMPADDR0, 0x0);

    //pmpcfgi.A=3，napot，根据手册Table19要求匹配地址
    CSRS(CSR_PMPCFG0, 1ULL << 8); //PMP1cfg.R
    CSRS(CSR_PMPCFG0, 1ULL <<   9); //PMP1cfg.W
    CSRS(CSR_PMPCFG0, 1ULL << 10); //PMP1cfg.X
    CSRS(CSR_PMPCFG0, 3ULL << 11); //PMP1cfg.A=3(NAPOT)
    //配置pmpaddr1为0x80000FFF对应的NAPOT地址范围
    CSRW(CSR_PMPADDR1, 0x80000FFF >> 2);
    uintptr_t start_addr = CSRR(CSR_PMPADDR1);
    //变为TOR
    CSRC(CSR_PMPCFG0, 2ULL << 11); //PMP1cfg.A=2(TOR)
    uintptr_t mid_addr = CSRR(CSR_PMPADDR1);
    //变为OFF
    CSRC(CSR_PMPCFG0, 3ULL << 11); //PMP1cfg.A=0(OFF)
    uintptr_t end_addr = CSRR(CSR_PMPADDR1);
    //变回NAPOT
    CSRS(CSR_PMPCFG0, 3ULL << 11); //PMP1cfg.A=3(NAPOT)
    uintptr_t final_addr = CSRR(CSR_PMPADDR1);
    TEST_ASSERT("pmpcfg.A change from NAPOT to TOR to OFF to NAPOT keeps pmpaddr unchanged",
        start_addr == final_addr &&
        start_addr != mid_addr &&
        start_addr != end_addr
    );
}


bool mstatus_csr_tests_1(){

TEST_START();

//暂时无法测
  /*

    //mstatus.GVA 控制异常种类时写0或1
    TEST_SETUP_EXCEPT();
    
    CSRS(CSR_MSTATUS,MSTATUS_GVA);
    CSRR(CSR_SSTATUS);
    printf("%d\n",CSR_MTVAL);
    printf("%d\n",excpt.cause);
    printf("%d\n",excpt.triggered);

    CSRC(CSR_MSTATUS,MSTATUS_GVA);
    ecall1();
    printf("%d\n",CSR_MTVAL);
    printf("%d\n",excpt.cause);


    //mstatus.MPV 控制异常发生时的虚拟化模式
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    int x=(CSRR(CSR_MSTATUS) >> 39) & 1;
    printf("%d\n",x);
    goto_priv(PRIV_VS);
    hfence_vvma();
    goto_priv(PRIV_M);
    x=(CSRR(CSR_MSTATUS) >> 39) & 1;
    printf("%d\n",x);

    reset_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_MIE);
    printf("bugai\n");
    printf("%d\n",excpt.triggered);
    goto_priv(PRIV_M);
    x=(CSRR(CSR_MSTATUS) >> 39) & 1;
    printf("%d\n",x);
*/

    //mstatus.mie 读写测试
    TEST_SETUP_EXCEPT();

    CSRS(CSR_MSTATUS,1ULL << 3);    //mie位
    TEST_ASSERT("mstatus.mie set to 1 successful",
        (CSRR(CSR_MSTATUS) & (1ULL << 3)) != 0
    );
    CSRC(CSR_MSTATUS,1ULL << 3);    //mie位
    TEST_ASSERT("mstatus.mie clear to 0 successful",
        (CSRR(CSR_MSTATUS) & (1ULL << 3)) == 0
    );


    //mstatus.SUM 阻止S对U访问，缺页异常
    goto_priv(PRIV_HS);
    hspt_init();

    //执行load指令时，设置了mstatus.SUM=0，在S模式下访问U模式可以访问的页表项
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_SUM);
    uintptr_t addr = hs_page_base(VSURWX_GURWX);

    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HS);
    ld(addr);
    TEST_ASSERT("s mode load u mode page when mstatus.sum=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );

    TEST_END();
}

bool mstatus_csr_tests_2(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    hpt_init();
    vspt_init();

    //执行load指令时，设置了mstatus.SUM=1，在S模式下访问U模式可以访问的页表项
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_SUM);
    uintptr_t addr = hs_page_base(VSURWX_GURWX);

    TEST_SETUP_EXCEPT();

    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_VS);
    hlvd(addr);
        printf("%d\n",excpt.triggered);
    printf("%d\n",excpt.cause);
    TEST_ASSERT("s mode load u mode page when mstatus.sum=1 successful",
        excpt.triggered == false
    );


    TEST_END();
}

bool mstatus_csr_tests_3(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    hpt_init();
    vspt_init();
    //执行load指令时，设置了mstatus.MXR=0，只能从只读页面读取，如果此时pte.r=0
    goto_priv(PRIV_M);

    uintptr_t addr = hs_page_base(VSX_GRWX);

    CSRC(CSR_MSTATUS, MSTATUS_MXR);
    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    hlvd(addr);   
    printf("%d\n",excpt.triggered);
    printf("%d\n",excpt.cause);
    TEST_ASSERT("s mode load when mstatus.mxr=0 and pte.r=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );


    TEST_END();
}

bool mstatus_csr_tests_4(){

    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    hpt_init();
    vspt_init();
    //执行load指令时，设置了mstatus.MXR=1，能从可读可执行页面读取，如果此时页表项pte.r=0&&pte.x=0

    uintptr_t addr = hs_page_base(VSV_GV);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_MXR);
    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_VS);
    TEST_SETUP_EXCEPT();
    hlvd(addr);
    TEST_ASSERT("s mode load when mstatus.mxr=1 and pte.r=0 and pte.x=0 leads to LPF",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF
    );
    CSRC(CSR_SSTATUS, SSTATUS_MXR);


TEST_END();

}

bool mstatus_csr_tests_5(){

    TEST_START();
    //mstatus.TSR=1时，执行sret指令
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS,1ULL << 22);    //TSR位

    goto_priv(PRIV_HS);
    set_prev_priv(PRIV_VU);

    TEST_EXEC_SRET();

    TEST_ASSERT("hs mode sret cause to ili when mstatus.TSR=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();

}

bool smrnmi_csr_tests_1(){
    TEST_START();
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRR(CSR_MNSTATUS);
    CSRR(CSR_MNSCRATCH);
    CSRR(CSR_MNEPC);
    CSRR(CSR_MNCAUSE);
    printf("mnstatus=%llx\n",CSRR(CSR_MNSTATUS));
    printf("mnscratch=%llx\n",CSRR(CSR_MNSCRATCH));
    printf("mnepc=%llx\n",CSRR(CSR_MNEPC));
    printf("mncause=%llx\n",CSRR(CSR_MNCAUSE));

    CSRW(CSR_MNSCRATCH, 0x12345678);
    printf("mnscratch=%llx\n",CSRR(CSR_MNSCRATCH)); 
    CSRW(CSR_MNEPC, 0x12345678);
    printf("mnepc=%llx\n",CSRR(CSR_MNEPC));
    CSRW(CSR_MNCAUSE, 0x123456FF);
    printf("mncause=%llx\n",CSRR(CSR_MNCAUSE));
    CSRW(CSR_MNSTATUS, 0x12345678);
    printf("mnstatus=%llx\n",CSRR(CSR_MNSTATUS));

    TEST_END();
}


bool hedeleg_csr_tests_1(){

    TEST_START();
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRW(CSR_HEDELEG, (uint64_t)-1);
    printf("%llx\n",CSRR(CSR_HEDELEG));

    TEST_ASSERT("hedeleg right",
        CSRR(CSR_HEDELEG) == 0xb1ff
    ); 

    TEST_END();

}

bool mhpmevent_csr_tests_1(){

    TEST_START();
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRW(CSR_MHPMEVENT3, (uint64_t)-1);
    printf("%llx\n",CSRR(CSR_MHPMEVENT3));

    TEST_END();

}

bool tselect_csr_tests_1(){

    TEST_START();
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, (uint64_t)-1);
    printf("%llx\n",CSRR(CSR_TSELECT));

    TEST_ASSERT("tselect right",
        CSRR(CSR_HEDELEG) == 0xb1ff
    ); 

    TEST_END();

}

bool trigger_tests(){
    
    TEST_START();

    uintptr_t target_pc;
    
    // 测试 tselect=0
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, 0); 
    asm volatile("csrw tdata1, x0");


    printf("tdata1=%llx\n",CSRR(CSR_TDATA1));

    CSRW(CSR_TDATA1, 0x600000000000005c);

    asm volatile(
        "la %0, target_instruction_0"
        : "=r"(target_pc)
    );
    CSRW(CSR_TDATA2, target_pc);
    
    printf("tselect=%llx\n",CSRR(CSR_TSELECT));
    printf("tdata1=%llx\n",CSRR(CSR_TDATA1));
    printf("tdata2=%llx\n",CSRR(CSR_TDATA2));

    // // 测试 tselect=1
    // CSRW(CSR_TSELECT, 1);
    // asm volatile("csrw tdata1, x0");   // 清除触发器
    
    // asm volatile(
    //     "la %0, target_instruction_1"
    //     : "=r"(target_pc)
    // );
    // CSRW(CSR_TDATA2, target_pc);
    // CSRW(CSR_TDATA1, 0x600000000000005c);   

    // printf("tselect=%llx\n",CSRR(CSR_TSELECT));
    // printf("tdata1=%llx\n",CSRR(CSR_TDATA1));
    // printf("tdata2=%llx\n",CSRR(CSR_TDATA2));

    // // 测试 tselect=2
    // CSRW(CSR_TSELECT, 2);
    // asm volatile("csrw tdata1, x0");   // 清除触发器
    
    // asm volatile(
    //     "la %0, target_instruction_2"
    //     : "=r"(target_pc)
    // );
    // CSRW(CSR_TDATA2, target_pc);
    // CSRW(CSR_TDATA1, 0x600000000000005c);   

    // printf("tselect=%llx\n",CSRR(CSR_TSELECT));
    // printf("tdata1=%llx\n",CSRR(CSR_TDATA1));
    // printf("tdata2=%llx\n",CSRR(CSR_TDATA2));

    // // 测试 tselect=3
    // CSRW(CSR_TSELECT, 3);
    // asm volatile("csrw tdata1, x0");   // 清除触发器

    // asm volatile(
    //     "la %0, target_instruction_3"
    //     : "=r"(target_pc)
    // );
    // CSRW(CSR_TDATA2, target_pc);
    // CSRW(CSR_TDATA1, 0x600000000000005c);   

    // printf("tselect=%llx\n",CSRR(CSR_TSELECT));
    // printf("tdata1=%llx\n",CSRR(CSR_TDATA1));
    // printf("tdata2=%llx\n",CSRR(CSR_TDATA2));

    // 重新选择触发器0进行测试
    CSRW(CSR_TSELECT, 0);
    
    //触发tselect=0
    TEST_SETUP_EXCEPT();
    asm volatile("target_instruction_0: nop");

    TEST_ASSERT("trigger execute match when tselect=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    ); 

    // //触发tselect=1
    // CSRW(CSR_TSELECT, 1);
    // TEST_SETUP_EXCEPT();
    // asm volatile("target_instruction_1: nop");

    // TEST_ASSERT("trigger execute match when tselect=1",
    //     excpt.triggered == true &&
    //     excpt.cause == CAUSE_BKP
    // ); 

    // //触发tselect=2
    // CSRW(CSR_TSELECT, 2);
    // TEST_SETUP_EXCEPT();
    // asm volatile("target_instruction_2: nop");

    // TEST_ASSERT("trigger execute match when tselect=2",
    //     excpt.triggered == true &&
    //     excpt.cause == CAUSE_BKP
    // ); 

    // //触发tselect=3
    // CSRW(CSR_TSELECT, 3);
    // TEST_SETUP_EXCEPT();
    // asm volatile("target_instruction_3: nop");

    // TEST_ASSERT("trigger execute match when tselect=3",
    //     excpt.triggered == true &&
    //     excpt.cause == CAUSE_BKP
    // ); 
    
    excpt_info();

    TEST_END();
}






bool random_instruction_tests(){

    TEST_START();
    goto_priv(PRIV_M);
    CSRW(CSR_MCYCLE,0x0);

    excpt_info();

    for (int i = 0; i < 10; i++)
    {
        random_m_instruction();
    }
    

    TEST_END();

}

bool script_test(){     //草稿

    TEST_START();
    goto_priv(PRIV_M);
    printf("mncause=%llx\n",CSRR(CSR_MNCAUSE));
    printf("mnepc=%llx\n",CSRR(CSR_MNEPC));

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MNEPC,0x8001dfde);
    mnret();
    excpt_info();
    printf("mncause=%llx\n",CSRR(CSR_MNCAUSE));

    TEST_END();

}
