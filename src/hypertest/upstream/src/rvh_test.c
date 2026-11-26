#include <rvh_test.h>

//enum test_state {};

extern test_func_t _test_table, _test_table_size;
test_func_t* test_table = &_test_table;
size_t test_table_size = (size_t) &_test_table_size;

unsigned test_num = 0;
unsigned curr_priv = PRIV_M;
unsigned real_priv = PRIV_M;
bool rnmi_handler_enabled = false;
static bool h_extension_available = false;

struct exception excpt;

uint64_t ecall_args[2];

uint64_t ecall(uint64_t a0, uint64_t a1)
{
    ecall_args[0] = a0;
    ecall_args[1] = a1;

    asm volatile("ecall" ::: "memory");
    return ecall_args[0];

}


//v扩展时防止被编译成向量指令干扰测试环境
#ifdef __riscv_vector
__attribute__((target("arch=rv64imac_zicsr")))
void test_setup_except_function() {
    __sync_synchronize();
    excpt.testing = true;
    excpt.triggered = false;
    excpt.fault_inst = 0;
    __sync_synchronize();
    DEBUG("setting up exception test");
}
#endif


static inline bool is_user(int priv) {
    return priv == PRIV_VU || priv == PRIV_HU;
}

static inline const char *priv_name_or_unknown(unsigned priv)
{
    if(priv < PRIV_MAX && priv_strs[priv] != NULL){
        return priv_strs[priv];
    }

    return "unknown";
}

static void unexpected_trap_error(const char *handler_name, uint64_t cause,
    uint64_t epc, uint64_t tval)
{
    ERROR("untested exception! handler=%s cause=0x%llx epc=0x%llx tval=0x%llx curr=%s real=%s",
        handler_name,
        cause,
        epc,
        tval,
        priv_name_or_unknown(curr_priv),
        priv_name_or_unknown(real_priv));
}

void set_prev_priv(int priv){
    switch(curr_priv){
        case PRIV_M: {
            uint64_t temp = CSRR(mstatus);
            temp &= ~((3ULL << 11));
            if(priv == PRIV_VS || priv == PRIV_HS) temp |= (0x1ULL << 11);    //mpp   
            CSRW(mstatus, temp);
        }
        break;
        case PRIV_HS:{
            uint64_t temp = CSRR(sstatus);
            temp &= ~(0x1ULL << 8);
            if(priv == PRIV_HS || priv == PRIV_VS) temp |= (0x1ULL << 8);   //spp
            CSRW(sstatus, temp);

            // temp = CSRR(CSR_HSTATUS);   //p101
            // temp &= ~(0x3ULL << 7);
            // if(priv == PRIV_VS) temp |= (1ULL << 8);        //spvp
            // if(priv == PRIV_VU || priv == PRIV_VS) temp |= (0x1ULL << 7);       //spv
            // CSRW(CSR_HSTATUS, temp);
        }
        break;
        case PRIV_VS: {
            uint64_t temp = CSRR(sstatus);
            CSRC(sstatus, 0x1ULL << 8);
        }
        break;
        default: ERROR("unknown current priv %d (%s)\n", priv, __func__);
    }
    
}

static inline void lower_priv(unsigned priv){

    if(priv == curr_priv) 
        return;

    if(priv >= curr_priv){
        ERROR("trying to \"lower\" to a higher priv");
    }

    if(curr_priv == PRIV_VS && priv == PRIV_HU){
        ERROR("trying to go from vs to hu");
    }

    set_prev_priv(priv);
    
    real_priv = priv;

    if(curr_priv == PRIV_M) {
        asm volatile(
            "la t0, 1f\n\t"
            "csrw mepc, t0\n\t"
            "mret\n\t"
            "1:\n\t"
            ::: "t0"
        );
    } else if(curr_priv == PRIV_VS || curr_priv == PRIV_HS){
        asm volatile(
            "la t0, 1f\n\t"
            "csrw sepc, t0\n\t"
            "sret\n\t"
            "1:\n\t"
            ::: "t0"
        );       
    } else {
        ERROR("cant lower priv from u");
    }

    curr_priv = priv;

}


void goto_priv(int target_priv){

    static bool on_going = false;

    DEBUG("goto_priv: real = %s, target = %s, curr = %s",  priv_strs[real_priv], priv_strs[target_priv], priv_strs[curr_priv]);
    if(is_user(target_priv) && is_user(curr_priv)) {
        goto_priv(PRIV_M);
    }

    if(real_priv == target_priv || target_priv >= PRIV_MAX){
        if(on_going)
            VERBOSE("...entered %s mode", priv_strs[target_priv]);
        on_going = false;
        curr_priv = target_priv;
        return;
    }

    if(!on_going)
        VERBOSE("entering %s mode...", priv_strs[target_priv]);

    on_going = true;
    if(target_priv > curr_priv){
        if(target_priv == PRIV_M && curr_priv == PRIV_HU){      //配合CSRS(medeleg, (1ULL << 8));使用，使得该函数支持从U->M
            goto_priv(PRIV_HS);
            goto_priv(PRIV_M);
        }
        else ecall(ECALL_GOTO_PRIV, target_priv);
    } else {
        VERBOSE("...entered %s mode", priv_strs[target_priv]);
        on_going = false;
        lower_priv(target_priv);
    }

}

void goto_priv_u_to_m_direct(int target_priv){

    static bool on_going = false;

    DEBUG("goto_priv: real = %s, target = %s, curr = %s",  priv_strs[real_priv], priv_strs[target_priv], priv_strs[curr_priv]);

    on_going = true;
    ecall(ECALL_GOTO_PRIV, target_priv);

}

static bool is_inst_fault(uint64_t cause){

    if(!(cause == CAUSE_IAM || cause == CAUSE_IAF || 
        cause == CAUSE_IPF || cause == CAUSE_IGPF)) {
            return false;
    }

    return true;
}

static bool is_ecall(uint64_t cause){

    if(!(cause == CAUSE_ECU || cause == CAUSE_ECS || 
        cause == CAUSE_ECV || cause == CAUSE_ECM)) {
            return false;
        }

    return true;
}

static inline uint64_t next_instruction(uint64_t epc){
    if(((*(uint16_t*)epc) & 0b11) == 0b11) return epc + 4;
    else return epc + 2;
}

static inline bool is_rnmi_cause(uint64_t cause)
{
    uint64_t cause_code = cause & CAUSE_MSK;
    return (cause & CAUSE_INT_BIT) &&
        (cause_code == 31ULL || cause_code == 43ULL);
}

static inline bool rnmi_trap_active(void)
{
    return rnmi_handler_enabled &&
        !(CSRR(CSR_MNSTATUS) & MNSTATUS_NMIE) &&
        is_rnmi_cause(CSRR(CSR_MNCAUSE));
}

#if VMODULE
#define VMODULE_REG_MAIN_INTERRUPT 0x1000ULL
#define VMODULE_ADDR_CMD_PREFIX 0xffff000100000000ULL
#define VMODULE_DATA_CMD_PREFIX 0xffff000000000000ULL

static inline void vmodule_write_reg(uint64_t offset, uint64_t data)
{
    const uint64_t addr_cmd =
        VMODULE_ADDR_CMD_PREFIX | (offset & 0xffffffffULL);
    const uint64_t data_cmd =
        VMODULE_DATA_CMD_PREFIX | (data & 0xffffffffULL);

    asm volatile(
        "mv t0, %[addr]\n\t"
        "mv t1, %[data]\n\t"
        "fence.i\n\t"
        "add x0, x0, t0\n\t"
        "fence.i\n\t"
        "add x0, x0, t1\n\t"
        "fence.i\n\t"
        :
        : [addr] "r"(addr_cmd),
          [data] "r"(data_cmd)
        : "t0", "t1", "memory");
}

static inline void vmodule_clear_main_interrupt_source(void)
{
    vmodule_write_reg(VMODULE_REG_MAIN_INTERRUPT, 0ULL);
}

static inline bool vmodule_main_interrupt_cause(uint64_t cause)
{
    const uint64_t cause_code = cause & CAUSE_MSK;

    return (cause & CAUSE_INT_BIT) &&
        (cause_code == 3ULL ||
         cause_code == 7ULL ||
         cause_code == 9ULL ||
         cause_code == 11ULL);
}

static inline bool vmodule_clear_interrupt_pending(uint64_t cause)
{
    if(vmodule_main_interrupt_cause(cause)){
        vmodule_clear_main_interrupt_source();
        return true;
    }

    return false;
}
#else
static inline void vmodule_clear_main_interrupt_source(void)
{
}

static inline bool vmodule_clear_interrupt_pending(uint64_t cause)
{
    (void)cause;
    return false;
}
#endif

static inline void clear_m_interrupt_pending(uint64_t cause)
{
    if(cause & CAUSE_INT_BIT){
        if(vmodule_clear_interrupt_pending(cause)){
            return;
        }
        CSRC(mip, 1ULL << (cause & CAUSE_MSK));
    }
}

static inline void clear_s_interrupt_pending(uint64_t cause)
{
    if(cause & CAUSE_INT_BIT){
        if(vmodule_clear_interrupt_pending(cause)){
            return;
        }
        CSRC(sip, 1ULL << (cause & CAUSE_MSK));
    }
}

void excpt_info(){
    const char* x = excpt.triggered ? "yes" : "no";
    printf("excpt trigger:%s\n",x);
    printf("excpt cause:%d\n",excpt.cause);
}


// #define return_from_exception(from_priv, to_priv, cause, epc) {\
//     uint64_t _temp_status;\
//     switch(from_priv){\
//         case PRIV_M:\
//             _temp_status = CSRR(mstatus);\
//             _temp_status &= ~((3ULL << 11) | (1ULL << 39) | (1ULL << 7));\
//             if(to_priv == PRIV_M) _temp_status |= (3ULL << 11);\
//             if(to_priv == PRIV_VS || to_priv == PRIV_HS) _temp_status |= (1ULL << 11);\
//             if(to_priv == PRIV_VU || to_priv == PRIV_VS) _temp_status |= (1ULL << 39);\
//             CSRW(mstatus, _temp_status);\
//             if(!(cause & (1ULL << 63))) CSRW(mepc, next_instruction(epc));\
//         break;\
//         case PRIV_HS:\
//             _temp_status = CSRR(CSR_HSTATUS);\
//             _temp_status &= ~(0x1ULL << 7);\
//             if(to_priv == PRIV_VU || to_priv == PRIV_VS) _temp_status |= (0x1ULL << 7);\
//             CSRW(CSR_HSTATUS, _temp_status);\
//         case PRIV_VS:\
//             _temp_status = CSRR(sstatus);\
//             _temp_status &= ~((1ULL << 8) | (1ULL << 5));\
//             if(to_priv == PRIV_VS || to_priv == PRIV_HS) _temp_status |= (0x1ULL << 8);\
//             CSRW(sstatus, _temp_status);\
//             if(!(cause & (1ULL << 63))) CSRW(sepc, next_instruction(epc));\
//         break;\
//     }\
//     return from_priv;\
// }


bool double_trap_enabled = false;  // 设为 true 来启用 double_trap 逻辑


int return_from_exception(unsigned from_priv,unsigned to_priv,uint64_t cause,uint64_t epc) {
    if(double_trap_enabled == true){
        double_trap_enabled = false;
        uint64_t _temp_status;
        switch(from_priv){
            case PRIV_M:
                _temp_status = CSRR(mstatus);
                _temp_status &= ~((3ULL << 11) | (1ULL << 39) | (1ULL << 7));
                if(to_priv == PRIV_M) _temp_status |= (3ULL << 11);
                if(to_priv == PRIV_VS || to_priv == PRIV_HS) _temp_status |= (1ULL << 11);
                if(to_priv == PRIV_VU || to_priv == PRIV_VS) _temp_status |= (1ULL << 39);
                CSRW(mstatus, _temp_status);
                printf("mcause=%llx\n",CSRR(CSR_MCAUSE));
                printf("mncause=%llx\n",CSRR(CSR_MNCAUSE));
                // printf("mtval2=%llx\n",CSRR(CSR_MTVAL2));
                if (!(CSRR(CSR_MNSTATUS)& MNSTATUS_NMIE) ) 
                { 
                    if(!(CSRR(CSR_MNCAUSE) & (1ULL << 63))) CSRW(CSR_MNEPC, next_instruction(CSRR(CSR_MNEPC))); 
                } 
                else if(!(cause & (1ULL << 63))) CSRW(mepc, next_instruction(epc));
            break;
            case PRIV_HS:
                _temp_status = CSRR(CSR_HSTATUS);
                _temp_status &= ~(0x1ULL << 7);
                if(to_priv == PRIV_VU || to_priv == PRIV_VS) _temp_status |= (0x1ULL << 7);
                CSRW(CSR_HSTATUS, _temp_status);
            case PRIV_VS:
                _temp_status = CSRR(sstatus);
                _temp_status &= ~((1ULL << 8) | (1ULL << 5));
                if(to_priv == PRIV_VS || to_priv == PRIV_HS) _temp_status |= (0x1ULL << 8);
                CSRW(sstatus, _temp_status);
                if (!(CSRR(CSR_MNSTATUS)& (1ULL << 3))) 
                { 
                    if(!(CSRR(CSR_MNCAUSE) & (1ULL << 63))) CSRW(CSR_MNEPC, next_instruction(CSRR(CSR_MNEPC)));
                } 
                else if(!(cause & (1ULL << 63))) CSRW(sepc, next_instruction(epc));
            break;
        }
        return from_priv;
    }

    else{
        uint64_t _temp_status;
        switch(from_priv){
            case PRIV_M:
                _temp_status = CSRR(mstatus);
                _temp_status &= ~((3ULL << 11) | (1ULL << 39) | (1ULL << 7));
                if(to_priv == PRIV_M) _temp_status |= (3ULL << 11);
                if(to_priv == PRIV_VS || to_priv == PRIV_HS) _temp_status |= (1ULL << 11);
                if(to_priv == PRIV_VU || to_priv == PRIV_VS) _temp_status |= (1ULL << 39);
                CSRW(mstatus, _temp_status);
                if(!(cause & (1ULL << 63))) CSRW(mepc, next_instruction(epc));
            break;
            case PRIV_HS:
            //     _temp_status = CSRR(CSR_HSTATUS);
            //     _temp_status &= ~(0x1ULL << 7);
            //     if(to_priv == PRIV_VU || to_priv == PRIV_VS) _temp_status |= (0x1ULL << 7);
            //     CSRW(CSR_HSTATUS, _temp_status);
            // case PRIV_VS:
                _temp_status = CSRR(sstatus);
                _temp_status &= ~((1ULL << 8) | (1ULL << 5));
                if(to_priv == PRIV_VS || to_priv == PRIV_HS) _temp_status |= (0x1ULL << 8);
                CSRW(sstatus, _temp_status);
                if(!(cause & (1ULL << 63))) CSRW(sepc, next_instruction(epc));
                if(to_priv == PRIV_M)
            break;
        }
        return from_priv;
    }
}





uint64_t mhandler(){

    real_priv = PRIV_M;
    if(rnmi_trap_active()){
        return HANDLER_RETURN_MNRET;
    }

    uint64_t cause = CSRR(mcause);
    uint64_t epc = CSRR(mepc);
    uint64_t tval = CSRR(mtval);
    uint64_t mstatus = CSRR(mstatus);
    // uint64_t tval2 = CSRR(CSR_MTVAL2);
    // uint64_t tinst= CSRR(CSR_MTINST);

    /* Once we trap into M-mode, the handler itself must not keep running with
     * the test case's MPRV translation active, otherwise the handler's own
     * stores to `excpt` can recurse through S/U translation again.
     */
    if(mstatus & MSTATUS_MPRV){
        CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    }

    VERBOSE("machine handler (mcause 0x%llx)", cause);
    DEBUG("mepc = 0x%lx", epc);
    DEBUG("mtval = 0x%lx", tval);
    // DEBUG("mtval2 = 0x%lx", tval2);
    // DEBUG("mtinst = 0x%lx", tinst);
    DEBUG("mpp = 0x%lx", (mstatus >> 11) & 0x3);
    DEBUG("mpv = 0x%lx", (mstatus >> 39) & 0x1);
    DEBUG("gva = 0x%lx", (mstatus >> MSTATUS_GVA_OFF) & 0x1);
    bool is_goto_priv_ecall =
        is_ecall(cause) && ecall_args[0] == ECALL_GOTO_PRIV;

    if(is_goto_priv_ecall){
        goto_priv(ecall_args[1]); 
    } else if(!excpt.testing){
        unexpected_trap_error("m", cause, epc, tval);
    }
    
    clear_m_interrupt_pending(cause);

    if(!is_goto_priv_ecall){
        excpt.triggered = true;
        excpt.priv = PRIV_M;
        excpt.cause = cause;
        excpt.epc = epc;
        excpt.tval = tval;
        // excpt.tinst = tinst;
        // excpt.tval2 = tval2;
        excpt.gva = !!((CSRR(mstatus) >> MSTATUS_GVA_OFF) & 0x1);
        excpt.xpv = !!((CSRR(mstatus) >> 39) & 0x1);
        excpt.testing = false;
    }
    if(is_inst_fault(cause)){
        if(excpt.fault_inst != 0){ 
            epc = excpt.fault_inst;
            DEBUG("isntruction fault! return: 0x%lx", excpt.fault_inst);
        } else 
            ERROR("instruction fault without return address");
    }
    excpt.fault_inst = 0;

    unsigned temp_priv = real_priv;
    real_priv = curr_priv;
    return_from_exception(temp_priv, curr_priv, cause, epc);
}
uint64_t hshandler(){

    real_priv = PRIV_HS;

    uint64_t cause = CSRR(scause);
    uint64_t epc = CSRR(sepc);
    uint64_t tval = CSRR(stval);
    uint64_t htval = 0ULL;
    uint64_t htinst = 0ULL;
    uint64_t hstatus = 0ULL;

    if(h_extension_available){
        htval = CSRR(CSR_HTVAL);
        htinst = CSRR(CSR_HTINST);
        hstatus = CSRR(CSR_HSTATUS);
    }

    VERBOSE("hypervisor handler scause (scause = 0x%llx)", cause);
    DEBUG("sepc = 0x%lx", epc);
    DEBUG("stval = 0x%lx",  tval);
    DEBUG("htval = 0x%lx",  htval);
    DEBUG("htinst = 0x%lx",  htinst);
    DEBUG("hstatus = 0x%lx",  hstatus);
    DEBUG("spv = 0x%lx", (hstatus >> 7) & 0x1);
    DEBUG("spvp = 0x%lx", (hstatus >> 8) & 0x1);
    DEBUG("gva = 0x%lx", (hstatus >> HSTATUS_GVA_OFF) & 0x1);
    bool is_goto_priv_ecall =
        is_ecall(cause) && ecall_args[0] == ECALL_GOTO_PRIV;

    if(is_goto_priv_ecall){ 
        goto_priv(ecall_args[1]); 
    } else if(is_ecall(cause)) {
        ERROR("unknown ecall"); 
    } else if(!excpt.testing){
        unexpected_trap_error("hs", cause, epc, tval);
    }

    clear_s_interrupt_pending(cause);
    //CSRC(CSR_HVIP, 1ULL << (cause &  ~(1ULL << 63)));
    // CSRC(CSR_HIP, 1ULL << (cause &  ~(1ULL << 63)));

    if(!is_goto_priv_ecall){
        excpt.triggered = true;
        excpt.priv = PRIV_HS;
        excpt.cause = cause;
        excpt.epc = epc;
        excpt.tval = tval;
        excpt.tinst = htinst;
        excpt.tval2 = htval;
        excpt.gva = !!((hstatus >> HSTATUS_GVA_OFF) & 0x1);
        excpt.xpv = !!((hstatus >> 7) & 0x1);
        excpt.testing = false;
    }
    if(is_inst_fault(cause)){
        if(excpt.fault_inst != 0){ 
            epc = excpt.fault_inst;
            DEBUG("isntruction fault! return: 0x%lx", excpt.fault_inst);
        } else 
            ERROR("instruction fault without return address");
    }
    excpt.fault_inst = 0;

    unsigned temp_priv = real_priv;
    real_priv = curr_priv;
    return_from_exception(temp_priv, curr_priv, cause, epc);
}

uint64_t vshandler(){

    real_priv = PRIV_VS;

    uint64_t cause = CSRR(scause);
    uint64_t epc = CSRR(sepc);
    uint64_t tval = CSRR(stval);

    VERBOSE("virtual supervisor handler (scause = 0x%llx)", cause);
    DEBUG("scause = 0x%llx", cause);
    DEBUG("sepc = 0x%lx", epc);
    DEBUG("stval = 0x%lx", tval);
    bool is_goto_priv_ecall =
        is_ecall(cause) && ecall_args[0] == ECALL_GOTO_PRIV;
    
    if(is_goto_priv_ecall){
        goto_priv(ecall_args[1]); 
    } else if(!excpt.testing){
        unexpected_trap_error("vs", cause, epc, tval);
    }
    
    clear_s_interrupt_pending(cause);

    if(!is_goto_priv_ecall){
        excpt.triggered = true;
        excpt.priv = PRIV_VS;
        excpt.cause = cause;
        excpt.epc = epc;
        excpt.tval = tval;
        excpt.testing = false;
    }
    if(is_inst_fault(cause)){
        if(excpt.fault_inst != 0){ 
            epc = excpt.fault_inst;
            DEBUG("isntruction fault! return: 0x%lx", excpt.fault_inst);
        }
        else ERROR("instruction fault without return address");
    }
    excpt.fault_inst = 0;

    unsigned temp_priv = real_priv;
    real_priv = curr_priv;
    return_from_exception(temp_priv, curr_priv, cause, epc);
}

uint32_t expand_compressed_instruction(uint16_t ins) {

    if(!INS_COMPRESSED(ins)) {
        ERROR("trying to expand non-compressed instruction");
    }

    if(INS_MATCH_C_LW(ins) || INS_MATCH_C_LD(ins) || INS_MATCH_C_SW(ins) || INS_MATCH_C_SD(ins)){
        bool is_load = INS_MATCH_C_LW(ins) || INS_MATCH_C_LD(ins); 
        bool is_double = INS_MATCH_C_LD(ins) || INS_MATCH_C_SD(ins); 
        uint32_t opcode_funct3 = is_double? 
            (is_load ? MATCH_LD : MATCH_SD):
            (is_load ? MATCH_LW : MATCH_SW);
        uint32_t rd_rs2 =  ((ins & INS_C_RDRS2_MASK) >> INS_C_RDRS2_OFF) + 8;
        uint32_t rs1 = ((ins & INS_C_RS1_MASK) >> INS_C_RS1_OFF) + 8;
        uint32_t imm = ((ins & INS_C_IMM1_MASK)  >> INS_C_IMM1_OFF) << 3 |
            is_double ? 
            ((ins & INS_C_IMM0_MASK)  >> INS_C_IMM0_OFF) << 6 :
            ((ins & INS_C_IMM0_MASK)  >> INS_C_IMM0_OFF) & 0x1 << 6 |
            ((ins & INS_C_IMM0_MASK)  >> INS_C_IMM0_OFF) >> 1 & 0x1 << 2;
        return opcode_funct3 | rd_rs2 << (is_load ? 7 : 20) | 
            rs1 << 15 | (imm & 0x1f) << 7 | (imm >> 5) << 25;
    } else {
        ERROR("expansion not implemented for target compressed instruction: 0x%x", ins);
    }

}

extern void hshandler_entry();
extern void mhandler_entry();
extern void vshandler_entry();
    
void reset_state(){

    goto_priv(PRIV_M);
    rnmi_handler_enabled = false;
#ifdef HYPTEST_HAS_SMRNMI
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
#endif
    h_extension_available = !!(CSRR(CSR_MISA) & MISA_H);
    CSRW(mstatus, 0ULL);
    //CSRW(mtvec, 0ULL);
    CSRW(medeleg, 0ULL);
    CSRW(mideleg, 0ULL);
    CSRW(mie, 0ULL);
    vmodule_clear_main_interrupt_source();
    CSRW(mip, 0ULL);
    //CSRW(mtime, 0ULL); 
    //CSRW(mtimecmp, 0ULL); 
    CSRW(mscratch, 0ULL);
    CSRW(mepc, 0ULL);
    CSRW(mtval, 0ULL);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE11);
    // CSRW(CSR_MTINST, 0ULL);
    // CSRW(CSR_MTVAL2, 0ULL);
    //what about pmp register?
    CSRW(sstatus, 0ULL);
    //CSRW(stvec, 0ULL);
    CSRW(sip, 0ULL);
    CSRW(sie, 0ULL);
    CSRW(sscratch, 0ULL);
    CSRW(sepc, 0ULL);
    CSRW(scause, 0ULL);
    CSRW(stval, 0ULL);
    CSRW(satp, 0ULL);
    // CSRW(CSR_HSTATUS, 0ULL);
    // CSRW(CSR_HIDELEG, 0ULL);
    // CSRW(CSR_HEDELEG, 0ULL);
    // CSRW(CSR_HVIP, 0ULL);
    // CSRW(CSR_HIP, 0ULL);
    // CSRW(CSR_HIE, 0ULL);  
    // CSRW(CSR_HTVAL, 0ULL);
    // CSRW(CSR_HTINST, 0ULL);
    // CSRW(CSR_HGATP, 0ULL);
    // CSRW(CSR_VSSTATUS, 0ULL);
    // CSRW(CSR_VSIP, 0ULL);
    // CSRW(CSR_VSIE, 0ULL);
    // //CSRW(CSR_VSTVEC, 0ULL);
    // CSRW(CSR_VSSCRATCH, 0ULL);
    // CSRW(CSR_VSEPC, 0ULL);
    // CSRW(CSR_VSCAUSE, 0ULL);
    // CSRW(CSR_VSTVAL, 0ULL);
    // CSRW(CSR_VSATP, 0ULL);  

    CSRW(mtvec, mhandler_entry);
    CSRS(medeleg, (1ULL << 8));       
    // full access to physical memory to other modes
    CSRW(pmpcfg0, 0xf);
    CSRW(pmpaddr0, (uint64_t) -1);

    CSRW(stvec, hshandler_entry);
    // CSRS(CSR_HEDELEG, (1ULL << 8));

    // CSRW(CSR_VSTVEC, vshandler_entry);

    CSRS(CSR_MSTATUS, 1ULL << 9);      //设置mstatus.vs!=0
    CSRC(CSR_MSTATUS, 1ULL << 10); 
    // CSRS(CSR_VSSTATUS, 1ULL << 9);      //设置vsstatus.vs!=0
    // CSRC(CSR_VSSTATUS, 1ULL << 10); 

    sfence();
    // hfence();
}
