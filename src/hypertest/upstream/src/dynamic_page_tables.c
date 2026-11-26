#include <stdint.h>
#include <stdio.h>
#include <dynamic_page_tables.h>
#include <csrs.h>

// 默认sv39，可在运行时切换
page_table_mode_t current_page_table_mode = PAGE_TABLE_MODE_SV48;

void set_page_table_mode(page_table_mode_t mode) {
    current_page_table_mode = mode;
}

page_table_mode_t get_page_table_mode(void) {
    return current_page_table_mode;
}

int get_page_table_levels(void) {
    return (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 3;
}

int get_virtual_address_bits(void) {
    return (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 48 : 39;
}

int get_physical_page_number_bits(void) {
    return (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 44 : 38;
}

void print_page_table_type() {
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        printf("sv39 (dynamic)\n");
    } else {
        printf("sv48 (dynamic)\n");
    }
}

// 检查内存配置
#if (MEM_SIZE < (0x10000000))
#   error "MEM_SIZE not enough (less than 256MB)"
#endif

// 测试页权限表
struct {
    uint64_t vs;
    uint64_t h;
} test_page_perm_table[TEST_PAGE_MAX] = {
    [VSRWX_GRWX]      =   {PTE_V | PTE_RWX,         PTE_V | PTE_RWX},     
    [VSRWX_GRW]       =   {PTE_V | PTE_RWX,         PTE_V | PTE_RW},     
    [VSRWX_GRX]       =   {PTE_V | PTE_RWX,         PTE_V | PTE_RX},     
    [VSRWX_GR]        =   {PTE_V | PTE_RWX,         PTE_V | PTE_R},
    [VSRWX_GX]        =   {PTE_V | PTE_RWX,         PTE_V | PTE_X},     
    [VSRW_GRWX]       =   {PTE_V | PTE_RW,          PTE_V | PTE_RWX},     
    [VSRW_GRW]        =   {PTE_V | PTE_RW,          PTE_V | PTE_RW},      
    [VSRW_GRX]        =   {PTE_V | PTE_RW,          PTE_V | PTE_RX},  
    [VSRW_GR]         =   {PTE_V | PTE_RW,          PTE_V | PTE_R},    
    [VSRW_GX]         =   {PTE_V | PTE_RW,          PTE_V | PTE_X},       
    [VSRX_GRWX]       =   {PTE_V | PTE_RX,          PTE_V | PTE_RWX},    
    [VSRX_GRW]        =   {PTE_V | PTE_RX,          PTE_V | PTE_RW},     
    [VSRX_GRX]        =   {PTE_V | PTE_RX,          PTE_V | PTE_RX},
    [VSRX_GR]         =   {PTE_V | PTE_RX,          PTE_V | PTE_R},    
    [VSRX_GX]         =   {PTE_V | PTE_RX,          PTE_V | PTE_X}, 
    [VSR_GRWX]        =   {PTE_V | PTE_R,           PTE_V | PTE_RWX},   
    [VSR_GRW]         =   {PTE_V | PTE_R,           PTE_V | PTE_RW},    
    [VSR_GRX]         =   {PTE_V | PTE_R,           PTE_V | PTE_RX},
    [VSR_GR]          =   {PTE_V | PTE_R,           PTE_V | PTE_R},    
    [VSR_GX]          =   {PTE_V | PTE_R,           PTE_V | PTE_X},      
    [VSX_GRWX]        =   {PTE_V | PTE_X,           PTE_V | PTE_RWX},   
    [VSX_GRW]         =   {PTE_V | PTE_X,           PTE_V | PTE_RW},    
    [VSX_GRX]         =   {PTE_V | PTE_X,           PTE_V | PTE_RX}, 
    [VSX_GR]          =   {PTE_V | PTE_X,           PTE_V | PTE_R},   
    [VSX_GX]          =   {PTE_V | PTE_X,           PTE_V | PTE_X},  
    [VSRWX_GURWX]     =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RWX},            
    [VSRWX_GURW]      =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RW},             
    [VSRWX_GURX]      =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RX}, 
    [VSRWX_GUR]       =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_R},            
    [VSRWX_GUX]       =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_X},          
    [VSRW_GURWX]      =   {PTE_V | PTE_RW,          PTE_V | PTE_U | PTE_RWX},            
    [VSRW_GURW]       =   {PTE_V | PTE_RW,          PTE_V | PTE_U | PTE_RW},         
    [VSRW_GURX]       =   {PTE_V | PTE_RW,          PTE_V | PTE_U | PTE_RX},
    [VSRW_GUR]        =   {PTE_V | PTE_RW,          PTE_V | PTE_U | PTE_R},         
    [VSRW_GUX]        =   {PTE_V | PTE_RW,          PTE_V | PTE_U | PTE_X},          
    [VSRX_GURWX]      =   {PTE_V | PTE_RX,          PTE_V | PTE_U | PTE_RWX},            
    [VSRX_GURW]       =   {PTE_V | PTE_RX,          PTE_V | PTE_U | PTE_RW},         
    [VSRX_GURX]       =   {PTE_V | PTE_RX,          PTE_V | PTE_U | PTE_RX}, 
    [VSRX_GUR]        =   {PTE_V | PTE_RX,          PTE_V | PTE_U | PTE_R},        
    [VSRX_GUX]        =   {PTE_V | PTE_RX,          PTE_V | PTE_U | PTE_X},    
    [VSR_GURWX]       =   {PTE_V | PTE_R,           PTE_V | PTE_U | PTE_RWX},        
    [VSR_GURW]        =   {PTE_V | PTE_R,           PTE_V | PTE_U | PTE_RW},         
    [VSR_GURX]        =   {PTE_V | PTE_R,           PTE_V | PTE_U | PTE_RX},
    [VSR_GUR]         =   {PTE_V | PTE_R,           PTE_V | PTE_U | PTE_R},         
    [VSR_GUX]         =   {PTE_V | PTE_R,           PTE_V | PTE_U | PTE_X},      
    [VSX_GURWX]       =   {PTE_V | PTE_X,           PTE_V | PTE_U | PTE_RWX},        
    [VSX_GURW]        =   {PTE_V | PTE_X,           PTE_V | PTE_U | PTE_RW},         
    [VSX_GURX]        =   {PTE_V | PTE_X,           PTE_V | PTE_U | PTE_RX},   
    [VSX_GUR]         =   {PTE_V | PTE_X,           PTE_V | PTE_U | PTE_R},      
    [VSX_GUX]         =   {PTE_V | PTE_X,           PTE_V | PTE_U | PTE_X},          
    [VSURWX_GRWX]     =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_RWX},            
    [VSURWX_GRW]      =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_RW},         
    [VSURWX_GRX]      =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_RX},
    [VSURWX_GR]       =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_R},   
    [VSV_GV]          =   {PTE_V , PTE_V }, 
    [VSURW_GRWX]      =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_RWX},        
    [VSURW_GRW]       =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_RW},         
    [VSURW_GRX]       =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_RX},
    [VSURW_GR]        =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_R},         
    [VSURW_GX]        =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_X},          
    [VSURX_GRWX]      =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_RWX},        
    [VSURX_GRW]       =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_RW},         
    [VSURX_GRX]       =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_RX},
    [VSURX_GR]        =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_R},         
    [VSURX_GX]        =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_X},    
    [VSUR_GRWX]       =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_RWX},        
    [VSUR_GRW]        =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_RW},         
    [VSUR_GRX]        =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_RX}, 
    [VSUR_GR]         =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_R},        
    [VSUR_GX]         =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_X},         
    [VSUX_GRWX]       =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_RWX},        
    [VSUX_GRW]        =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_RW},         
    [VSUX_GRX]        =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_RX}, 
    [VSUX_GR]         =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_R},        
    [VSUX_GX]         =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_X},          
    [VSURWX_GURWX]    =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_U | PTE_RWX},            
    [VSURWX_GURW]     =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_U | PTE_RW},             
    [VSURWX_GURX]     =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_U | PTE_RX},
    [VSURWX_GUR]      =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_U | PTE_R},              
    [VSURWX_GUX]      =   {PTE_V | PTE_U | PTE_RWX, PTE_V | PTE_U | PTE_X},          
    [VSURW_GURWX]     =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_U | PTE_RWX},            
    [VSURW_GURW]      =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_U | PTE_RW},         
    [VSURW_GURX]      =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_U | PTE_RX}, 
    [VSURW_GUR]       =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_U | PTE_R},        
    [VSURW_GUX]       =   {PTE_V | PTE_U | PTE_RW,  PTE_V | PTE_U | PTE_X},          
    [VSURX_GURWX]     =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_U | PTE_RWX},            
    [VSURX_GURW]      =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_U | PTE_RW},         
    [VSURX_GURX]      =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_U | PTE_RX},
    [VSURX_GUR]       =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_U | PTE_R},         
    [VSURX_GUX]       =   {PTE_V | PTE_U | PTE_RX,  PTE_V | PTE_U | PTE_X},   
    [VSUR_GURWX]      =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_U | PTE_RWX},
    [VSUR_GURW]       =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_U | PTE_RW},
    [VSUR_GURX]       =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_U | PTE_RX},
    [VSUR_GUR]        =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_U | PTE_R},
    [VSUR_GUX]        =   {PTE_V | PTE_U | PTE_R,   PTE_V | PTE_U | PTE_X},       
    [VSUX_GURWX]      =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_U | PTE_RWX},        
    [VSUX_GURW]       =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_U | PTE_RW},         
    [VSUX_GURX]       =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_U | PTE_RX},
    [VSUX_GUR]        =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_U | PTE_R},         
    [VSUX_GUX]        =   {PTE_V | PTE_U | PTE_X,   PTE_V | PTE_U | PTE_X},
    [VSI_GI]          =   {0,                       0}, 
    [VSRWX_GI]        =   {PTE_V | PTE_RWX,         0}, 
    [VSRW_GI]        =    {PTE_V | PTE_RW,          0}, 
    [VSI_GURWX]       =   {0,                       PTE_V | PTE_U | PTE_RWX},  
    [VSI_GUX]         =   {0,                       PTE_V | PTE_U | PTE_X},
    [VSI_GUR]         =   {0,                       PTE_V | PTE_U | PTE_R},
    [VSI_GURW]        =   {0,                       PTE_V | PTE_U | PTE_RW},
    [SCRATCHPAD]      =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RWX},           
    [SWITCH1]         =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RWX},      
    [SWITCH2]         =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RWX},      
    [VSRWXPbmt0_GURWXPbmt0]         =   {PTE_V | PTE_RWX,      PTE_V | PTE_U | PTE_RWX},      
    [VSRWXPbmt0_GURWXPbmt1]         =   {PTE_V | PTE_RWX,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt1},      
    [VSRWXPbmt0_GURWXPbmt2]         =   {PTE_V | PTE_RWX,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt2},      
    [VSRWXPbmt1_GURWXPbmt0]         =   {PTE_V | PTE_RWX | PTE_Pbmt1,      PTE_V | PTE_U | PTE_RWX},      
    [VSRWXPbmt1_GURWXPbmt1]         =   {PTE_V | PTE_RWX | PTE_Pbmt1,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt1},         
    [VSRWXPbmt1_GURWXPbmt2]         =   {PTE_V | PTE_RWX | PTE_Pbmt1,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt2},     
    [VSRWXPbmt2_GURWXPbmt0]         =   {PTE_V | PTE_RWX | PTE_Pbmt2,      PTE_V | PTE_U | PTE_RWX},      
    [VSRWXPbmt2_GURWXPbmt1]         =   {PTE_V | PTE_RWX | PTE_Pbmt2,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt1},      
    [VSRWXPbmt2_GURWXPbmt2]         =   {PTE_V | PTE_RWX | PTE_Pbmt2,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt2},    
    [VSRWXPbmt3_GURWXPbmt1]         =   {PTE_V | PTE_RWX | PTE_Pbmt3,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt1},   
    [VSRWXPbmt2_GURWXPbmt3]         =   {PTE_V | PTE_RWX | PTE_Pbmt2,      PTE_V | PTE_U | PTE_RWX | PTE_Pbmt3},   
    [X]            =   {PTE_V | PTE_RWX , PTE_V | PTE_U | PTE_RWX}, 
    [TOP]             =   {PTE_V | PTE_RWX,         PTE_V | PTE_U | PTE_RWX},     
};

// 页表数组定义 - 使用最大尺寸以支持sv48
pte_t hspt[5][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));
pte_t vspt[7][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));
pte_t hpt_root[PAGE_SIZE*4/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE*4)));
pte_t hpt[7][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

// ========== hspt_init() - 根据模式动态初始化 ==========
void hspt_init(){
    uintptr_t addr;
    
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        // SV39模式: 3级页表
        addr = 0x00000000;
        for(int i = 0; i < 4; i++){
            hspt[0][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(0);
        }

        hspt[0][4] = PTE_V | (((uintptr_t)&hspt[1][0]) >> 2);
        hspt[1][0] = PTE_V | (((uintptr_t)&hspt[2][0]) >> 2);

        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            hspt[2][i] = (addr >> 2) | PTE_AD | test_page_perm_table[i].vs;
            addr += PAGE_SIZE;
        }

        if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
            uintptr_t satp = (((uintptr_t)hspt) >> 12) | (0x8ULL << 60);
            CSRW(satp, satp);
        } else {
            ERROR("trying to set hs level satp from lower privilege");
        }
    } else {
        // SV48模式: 4级页表
        hspt[0][0] = PTE_V | (((uintptr_t)&hspt[1][0]) >> 2);

        addr = 0x00000000;
        for(int i = 0; i < 4; i++){
            hspt[1][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(1);
        }

        hspt[1][4] = PTE_V | (((uintptr_t)&hspt[2][0]) >> 2);
        hspt[1][5] = PTE_V | (((uintptr_t)&hspt[3][0]) >> 2);
        hspt[2][0] = PTE_V | (((uintptr_t)&hspt[4][0]) >> 2);

        for(int i = 0; i < 4; i++){
            hspt[3][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(2);
        }

        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            hspt[4][i] = (addr >> 2) | PTE_AD | test_page_perm_table[i].vs;
            addr += PAGE_SIZE;
        }

        if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
            uintptr_t satp = (((uintptr_t)hspt) >> 12) | (0x9ULL << 60);
            CSRW(satp, satp);
        } else {
            ERROR("trying to set hs level satp from lower privilege");
        }
    }
}

// ========== vspt_init() - 根据模式动态初始化 ==========
void vspt_init(){
    uintptr_t addr;
    
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        // SV39模式
        addr = 0x00000000;
        for(int i = 0; i < 4; i++){
            vspt[0][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(0);
        }

        vspt[0][MEM_BASE/SUPERPAGE_SIZE(0)] = PTE_V | (((uintptr_t)&vspt[1][0]) >> 2);

        addr = MEM_BASE;
        for(int i = 0; i < 512; i++) vspt[1][i] = 0;
        for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
            vspt[1][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(1);
        }

        vspt[0][4] = PTE_V | (((uintptr_t)&vspt[2][0]) >> 2);
        vspt[2][0] = PTE_V | (((uintptr_t)&vspt[3][0]) >> 2);

        addr = TEST_VPAGE_BASE;
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            vspt[3][i] = (addr >> 2) | PTE_AD | test_page_perm_table[i].vs;
            addr += PAGE_SIZE;
        }

        vspt[2][1] = PTE_V | (((uintptr_t)&vspt[4][0]) >> 2);

        addr = 4 * SUPERPAGE_SIZE(0) + SUPERPAGE_SIZE(1);
        for(int i = 0; i < 512; i++){
            vspt[4][i] = (addr >> 2) | PTE_V | PTE_AD | PTE_RWX; 
            addr += PAGE_SIZE;
        }

        vspt[0][5] = PTE_V | (((uintptr_t)&vspt[5][0]) >> 2);
        
        addr = 5 * SUPERPAGE_SIZE(0);
        for(int i = 0; i < 512; i++){
            vspt[5][i] = (addr >> 2) | PTE_V | PTE_AD | PTE_RWX;  
            addr += SUPERPAGE_SIZE(1);
        }

        uintptr_t satp = (((uintptr_t)vspt) >> 12) | (0x8ULL << 60);
        if(curr_priv == PRIV_VS){
            CSRW(satp, satp);
        } else if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
            CSRW(CSR_VSATP, satp);
        } else {
            ERROR("");
        }
    } else {
        // SV48模式
        vspt[0][0] = PTE_V | (((uintptr_t)&vspt[1][0]) >> 2);

        addr = 0x00000000;
        for(int i = 0; i < 4; i++){
            vspt[1][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(1);
        }

        vspt[1][4] = PTE_V | (((uintptr_t)&vspt[2][0]) >> 2);
        vspt[1][5] = PTE_V | (((uintptr_t)&vspt[3][0]) >> 2);
        vspt[1][6] = PTE_V | (((uintptr_t)&vspt[4][0]) >> 2);
        
        addr = MEM_BASE;
        for(int i = 0; i < 512; i++) vspt[4][i] = 0;
        for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
            vspt[4][i] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(1);
        }

        addr = 4 * SUPERPAGE_SIZE(1);
        for(int i = 0; i < 512; i++){
            vspt[3][i] = (addr >> 2) | PTE_V | PTE_AD | PTE_RWX;  
            addr += SUPERPAGE_SIZE(2);
        }

        vspt[2][0] = PTE_V | (((uintptr_t)&vspt[5][0]) >> 2);

        addr = TEST_VPAGE_BASE;
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            vspt[5][i] = (addr >> 2) | PTE_AD | test_page_perm_table[i].vs;
            addr += PAGE_SIZE;
        }

        vspt[2][1] = PTE_V | (((uintptr_t)&vspt[6][0]) >> 2);

        addr = 4 * SUPERPAGE_SIZE(1) + SUPERPAGE_SIZE(2);
        for(int i = 0; i < 512; i++){
            vspt[6][i] = (addr >> 2) | PTE_V | PTE_AD | PTE_RWX; 
            addr += PAGE_SIZE;
        }

        uintptr_t satp = (((uintptr_t)vspt) >> 12) | (0x9ULL << 60);
        if(curr_priv == PRIV_VS){
            CSRW(satp, satp);
        } else if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
            CSRW(CSR_VSATP, satp);
        } else {
            ERROR("");
        }
    }
}

// ========== hpt_init() - 根据模式动态初始化 ==========
void hpt_init(){
    uintptr_t addr;
    
    for(int i = 0; i < 2048; i++){
        hpt_root[i] = 0;
    }
    
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        // SV39模式
        addr = 0x0;
        for(int i = 0; i < 4; i++){
            hpt_root[i] = PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(0);
        }

        hpt_root[MEM_BASE/SUPERPAGE_SIZE(0)] = PTE_V | (((uintptr_t)&hpt[0][0]) >> 2);

        addr = MEM_BASE;
        for(int i = 0; i < 512; i++) hpt[0][i] = 0;
        for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
            hpt[0][i] = PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(1);
        }

        hpt_root[4] = PTE_V | (((uintptr_t)&hpt[1][0]) >> 2);
        hpt_root[2047] = PTE_V | (((uintptr_t)&hpt[1][0]) >> 2);
        hpt[1][0] = PTE_V | (((uintptr_t)&hpt[2][0]) >> 2);
        hpt[1][511] = PTE_V | (((uintptr_t)&hpt[2][0]) >> 2);

        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            hpt[2][i] = (addr >> 2) | PTE_U | PTE_AD | test_page_perm_table[i].h;
            addr += PAGE_SIZE;
        }

        hpt[1][1] = PTE_V | (((uintptr_t)&hpt[3][0]) >> 2);
        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < 512; i++){
            hpt[3][i] = (addr >> 2) | PTE_U | PTE_AD | PTE_RWX;
            addr += PAGE_SIZE;
        }

        hpt_root[5] = PTE_V | (((uintptr_t)&hpt[4][0]) >> 2);
        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < 512; i++){
            hpt[4][i] = (addr >> 2) | PTE_U | PTE_AD | PTE_RWX;
            addr += SUPERPAGE_SIZE(1);
        }

        if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
            uintptr_t hsatp = (((uintptr_t)hpt_root) >> 12) | (0x8ULL << 60);
            CSRW(CSR_HGATP, hsatp);
        } else {
            ERROR("trying to set hs level hgatp from lower privilege");
        }
    } else {
        // SV48模式
        hpt_root[MEM_BASE/SUPERPAGE_SIZE(1)] = PTE_V | (((uintptr_t)&hpt[3][0]) >> 2);
        hpt_root[0] = PTE_V | (((uintptr_t)&hpt[1][0]) >> 2);
        hpt_root[2047] = PTE_V | (((uintptr_t)&hpt[2][0]) >> 2);

        addr = 0x0;
        for(int i = 0; i < 4; i++){
            hpt[1][i] = PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(1);
        }

        hpt[1][4] = PTE_V | (((uintptr_t)&hpt[0][0]) >> 2);
        hpt[0][0] = PTE_V | (((uintptr_t)&hpt[2][0]) >> 2);
        hpt[0][1] = PTE_V | (((uintptr_t)&hpt[4][0]) >> 2);

        addr = MEM_BASE;
        for(int i = 0; i < 512; i++) hpt[3][i] = 0;
        for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(2)/2; i++){
            hpt[3][i] = PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);  
            addr += SUPERPAGE_SIZE(2);
        }

        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < 512; i++){
            hpt[4][i] = (addr >> 2) | PTE_U | PTE_AD | PTE_RWX;
            addr += SUPERPAGE_SIZE(2);
        }

        hpt[2][0] = PTE_V | (((uintptr_t)&hpt[5][0]) >> 2);
        hpt[2][1] = PTE_V | (((uintptr_t)&hpt[6][0]) >> 2);
        hpt[2][511] = PTE_V | (((uintptr_t)&hpt[5][0]) >> 2);

        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            hpt[5][i] = (addr >> 2) | PTE_U | PTE_AD | test_page_perm_table[i].h;
            addr += PAGE_SIZE;
        }

        addr = TEST_PPAGE_BASE;
        for(int i = 0; i < 512; i++){
            hpt[6][i] = (addr >> 2) | PTE_U | PTE_AD | PTE_RWX;
            addr += PAGE_SIZE;
        }

        if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
            uintptr_t hsatp = (((uintptr_t)hpt_root) >> 12) | (0x9ULL << 60);
            CSRW(CSR_HGATP, hsatp);
        } else {
            ERROR("trying to set hs level hgatp from lower privilege");
        }
    }
}


// ========== 页表切换函数 ==========
void hspt_switch(){
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    pte_t temp = hspt[leaf_level][SWITCH1];
    hspt[leaf_level][SWITCH1] = hspt[leaf_level][SWITCH2];
    hspt[leaf_level][SWITCH2] = temp;
}

void vspt_switch(){
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 5 : 3;
    pte_t temp = vspt[leaf_level][SWITCH1];
    vspt[leaf_level][SWITCH1] = vspt[leaf_level][SWITCH2];
    vspt[leaf_level][SWITCH2] = temp;
}

void hpt_switch(){
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 5 : 2;
    pte_t temp = hpt[leaf_level][SWITCH1];
    hpt[leaf_level][SWITCH1] = hpt[leaf_level][SWITCH2];
    hpt[leaf_level][SWITCH2] = temp;
}

// ========== PBMT 操作 ==========
void pbmt_hspt_to_x(int pbmt_page){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = TEST_PPAGE_BASE + 250 * PAGE_SIZE;
        hspt[4][pbmt_page] = (addr >> 2) | PTE_AD | test_page_perm_table[pbmt_page].vs;
    }
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        uintptr_t addr = TEST_PPAGE_BASE + 250 * PAGE_SIZE;
        hspt[2][pbmt_page] = (addr >> 2) | PTE_AD | test_page_perm_table[pbmt_page].vs;
    }
}

void pbmt_hspt_to_x_base_paddr(int pbmt_page , uintptr_t new_base_paddr){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = new_base_paddr + 250 * PAGE_SIZE;
        hspt[4][pbmt_page] = (addr >> 2) | PTE_AD | test_page_perm_table[pbmt_page].vs;
    }
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        uintptr_t addr = new_base_paddr + 250 * PAGE_SIZE;
        hspt[2][pbmt_page] = (addr >> 2) | PTE_AD | test_page_perm_table[pbmt_page].vs;
    }
}

// ========== 权限操作函数 ==========
void hspt_u_mode_allow(){
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        for(int i = 0; i < 4; i++){
            hspt[0][i] |= PTE_U;
        }
    } else {
        for(int i = 0; i < 4; i++){
            hspt[1][i] |= PTE_U;
        }
        for(int i = 0; i < 4; i++){
            hspt[3][i] |= PTE_U;
        }
    }
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] |= PTE_U;
    }
}

void hspt_u_mode_allow_no_leaf(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        for(int i = 0; i < 4; i++){
            hspt[1][i] |= PTE_U;
        }
        for(int i = 0; i < 4; i++){
            hspt[3][i] |= PTE_U;
        }
    }
}

void hspt_del_D(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] &= ~PTE_D;
    }
}

void hspt_del_A(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] &= ~PTE_A;
    }
}

void hspt_del_R(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] &= ~PTE_R;
    }
}

void hspt_del_W(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] &= ~PTE_W;
    }
}

void hspt_del_X(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] &= ~PTE_X;
    }
}

void hspt_add_X(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;

    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] |= PTE_X;
    }
}

void hspt_add_G(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] |= PTE_G;
    }
}

void hspt_del_V(){
    uintptr_t addr;
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] &= ~PTE_V;
    }
}

// ========== PPN 高位溢出测试 ==========
void hspt_ptw_ppn_high_bit_overflow_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV39) {
        hspt[1][0] |= (1ULL << 46);
    } else {
        hspt[2][0] |= (1ULL << 46);
    }
}

void hspt_llptw_ppn_high_bit_overflow_setup(){
    int leaf_level = (current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 2;
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[leaf_level][i] |= (1ULL << 46);  
    }
}

// ========== 物理地址修改 ==========
void hspt_leaf_change_base_paddr(int page, uintptr_t new_base_paddr){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = new_base_paddr + page * PAGE_SIZE;
        hspt[4][page] = 0;
        hspt[4][page] = (addr >> 2) | PTE_AD | test_page_perm_table[page].vs;
    }
}

// ========== 超级页非对齐测试 (仅SV48支持) ==========
void hspt_2MB_superpage_misalign_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(1) * 4 + 0x100000;
        hspt[3][0] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void hspt_2MB_superpage_align_boundary_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(1) * 4 + 0x200000;
        hspt[3][0] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void hspt_1GB_superpage_misalign_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(1) * 3 + 0x30000000;
        hspt[1][3] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void hspt_1GB_superpage_align_boundary_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(1) * 3 + 0x40000000;
        hspt[1][3] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void set_superpage_512G(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(0);
        hspt[0][1] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void hspt_512GB_superpage_misalign_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(0) + 0x4000000000;
        hspt[0][1] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void hspt_512GB_superpage_align_boundary_setup(){
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        uintptr_t addr = SUPERPAGE_SIZE(0) + 0x8000000000;
        hspt[0][1] = PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
    }
}

void hspt_add_reserved_bits(){
    //设置hspt，使得页表项的保留位被设置
    if (current_page_table_mode == PAGE_TABLE_MODE_SV48) {
        for(int i = 0; i < TEST_PAGE_MAX; i++){
            hspt[4][i] |=  (0x3FULL << 54);  
        }
    }
}
