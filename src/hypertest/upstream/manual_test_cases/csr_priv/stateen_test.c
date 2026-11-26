#include <rvh_test.h>


bool stateen_C_test_1(){

    TEST_START();
    
    goto_priv(PRIV_M);
    //当mstateen.C=0，u、s mode访问自定义寄存器
    CSRC(CSR_MSTATEEN0 , MSTATEEN_C);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(0X803);


    TEST_ASSERT("HU mode accesss Custom register cause to illegal except when mstateen.c=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(0X5C0);


    TEST_ASSERT("hs mode accesss Custom register cause to illegal except when mstateen.c=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 


    TEST_END();

}



bool stateen_C_test_2(){

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_C);
    CSRC(CSR_SSTATEEN0 , MSTATEEN_C);

    //当mstateen.C=1，sstateem.C=0，U访问自定义寄存器
    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(0X801);    //目前未实现u模式相关自定义寄存器


    TEST_ASSERT("HU mode accesss Custom register cause to ILI when mstateen.c=1 hstateen.c=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 


}

bool stateen_C_test_3(){

    //当mstateen.C=0，m mode访问自定义寄存器

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATEEN0 , MSTATEEN_C);
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    CSRR(0X5C1);

    TEST_ASSERT("m mode accesss Custom register success  when mstateen.c=0",
        excpt.triggered == false
    ); 

    TEST_END();

}

bool stateen_C_test_4(){

    //当mstateen.C=1，sstateen.C=0，u访问自定义寄存器

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_C);
    CSRC(CSR_SSTATEEN0 , MSTATEEN_C);


    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(0X804);    //目前未实现u模式相关自定义寄存器


    TEST_ASSERT("hu mode accesss Custom register cause to ILI when mstateen.c=1 sstateen.c=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 


}



bool stateen_C_test_5(){

    TEST_START();
    
    goto_priv(PRIV_M);
    // 当前U/HU模式自定义寄存器尚未实现；即使stateen.C放开，访问仍按未实现CSR处理。
    CSRS(CSR_MSTATEEN0 , MSTATEEN_C);
    CSRS(CSR_SSTATEEN0 , MSTATEEN_C);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(0X8FE);    //目前未实现u模式相关自定义寄存器

    TEST_ASSERT("hu mode accesss unimplemented Custom register causes ILI even when mstateen.c=1 sstateen.c=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();

}


bool stateen_C_test_6(){

    TEST_START();
    
    goto_priv(PRIV_M);
    //当mstateen.C=1 sstateen.c=0 时HS模式可以访问自定义寄存器
    CSRS(CSR_MSTATEEN0 , MSTATEEN_C);
    CSRS(CSR_SSTATEEN0 , MSTATEEN_C);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(0X5c3);

    TEST_ASSERT("hs mode accesss Custom register successful when mstateen.c=1 sstateen.c=0",
        excpt.triggered == false
    ); 
}

// ----------


bool stateen_ENVCFG_test_1(){

    TEST_START();
    
    goto_priv(PRIV_M);
    //当mstateen.ENVCFG=0，s、u mode 访问Senvcfg寄存器
    CSRC(CSR_MSTATEEN0 , MSTATEEN_ENVCFG);

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SENVCFG);


    TEST_ASSERT("HU mode accesss senvcfg register cause to illegal except when mstateen.envcfg=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_SENVCFG);


    TEST_ASSERT("HS mode accesss senvcfg register cause to illegal except when mstateen.envcfg=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

}


bool stateen_ENVCFG_test_2(){

    //当mstateen.ENVCFG=0，m mode 访问Senvcfg寄存器

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATEEN0 , MSTATEEN_ENVCFG);
    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_SENVCFG);


    TEST_ASSERT("m mode accesss senvcfg register success  when mstateen.envcfg=0",
        excpt.triggered == false
    ); 



    TEST_END();

}



bool stateen_ENVCFG_test_3(){

    //当mstateen.ENVCFG=1,s mode 访问Senvcfg寄存器

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_ENVCFG);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_SENVCFG);


    TEST_ASSERT("hs mode accesss senvcfg register successful when mstateen.envcfg=0",
        excpt.triggered == false
    ); 

    //当mstateen.ENVCFG=1，m mode 可访问Senvcfg寄存器
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_ENVCFG);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_SENVCFG);


    TEST_ASSERT("m mode accesss senvcfg register successful when mstateen.envcfg=0",
        excpt.triggered == false
    ); 

    //当mstateen.ENVCFG=1 sstateen.ENVCFG=0，u mode访问Senvcfg寄存器

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_ENVCFG);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    CSRR(CSR_SENVCFG);


    TEST_ASSERT("hu mode accesss senvcfg register cause to ILI when mstateen.envcfg=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();

}

  

// -------------


bool stateen_SE0_test_1(){

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATEEN0 , MSTATEEN_SE0);

    //当mstateen.SE0=0，s mode访问Sstateen0寄存器
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);


    TEST_ASSERT("HS mode accesss sstateen0 register cause to illegal except when mstateen.SE0=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    //当mstateen.SE0=0，u mode访问Sstateen0寄存器
    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);


    TEST_ASSERT("HU mode accesss sstateen0 register cause to illegal except when mstateen.SE0=0",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 

    TEST_END();

}


bool stateen_SE0_test_2(){

    //当mstateen.SE0=0，m mode访问Sstateen0寄存器

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATEEN0 , MSTATEEN_SE0);
    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);


    TEST_ASSERT("m mode accesss sstateen0 register success  when mstateen.SE0=0",
        excpt.triggered == false
    ); 



    TEST_END();

}


bool stateen_SE0_test_3(){

    //当mstateen.SE0=1，s mode访问Sstateen0寄存器

    TEST_START();
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_SE0);

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);


    TEST_ASSERT("hs mode accesss sstateen0 register successful when mstateen.SE0=1",
        excpt.triggered == false
    ); 

    //当mstateen.SE0=1，m mode访问Sstateen0寄存器
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_SE0);

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);


    TEST_ASSERT("m mode accesss sstateen0 register successful when mstateen.SE0=1",
        excpt.triggered == false
    ); 


    //当mstateen.SE0=1 sstateen.SE0=0，u mode访问Sstateen0寄存器，引发ILI
    
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0 , MSTATEEN_SE0);

    goto_priv(PRIV_HU);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);


    TEST_ASSERT("hu mode accesss sstateen0 register successful when mstateen.SE0=1",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI
    ); 


    TEST_END();

}



bool stateen_SE0_test_4(){

    TEST_START();
    
    goto_priv(PRIV_M);
    //当mstateen.SE0=1，访问Sstateen0寄存器
    CSRS(CSR_MSTATEEN0 , MSTATEEN_SE0);
    CSRS(CSR_SSTATEEN0 , MSTATEEN_SE0);




    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    CSRR(CSR_SSTATEEN0);

    excpt_info();

    TEST_ASSERT("m mode accesss sstateen0 register successful when mstateen.SE0=1 hstateen.SE0=1",
        excpt.triggered == false
    ); 




    TEST_END();

}


bool stateen_test_1(){

    TEST_START();
    
    goto_priv(PRIV_M);
    //mstateen CSR中未实现的位为read-only zeros
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 0);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 1);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 2);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 56);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 57);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 58);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRS(CSR_MSTATEEN0,1ULL << 59);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));
    CSRW(CSR_MSTATEEN0,(uint64_t)-1);
    printf("mstateen0=%lx\n",CSRR(CSR_MSTATEEN0));


    TEST_END();

}


