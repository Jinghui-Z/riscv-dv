#ifndef DYNAMIC_PAGE_TABLES_H
#define DYNAMIC_PAGE_TABLES_H

#include <rvh_test.h>

// 动态页表模式枚举
typedef enum {
    PAGE_TABLE_MODE_SV39 = 0,
    PAGE_TABLE_MODE_SV48 = 1
} page_table_mode_t;

// 全局页表模式变量（可在运行时切换）
extern page_table_mode_t current_page_table_mode;

// 页表模式控制函数
void set_page_table_mode(page_table_mode_t mode);
page_table_mode_t get_page_table_mode(void);

// 动态获取页表参数的宏和函数
int get_page_table_levels(void);
int get_virtual_address_bits(void);
int get_physical_page_number_bits(void);

// 基本定义
#define PAGE_SIZE 0x1000ULL
#define PT_SIZE (PAGE_SIZE)
#define PAGE_ADDR_MSK (~(PAGE_SIZE - 1))
#define PAGE_SHIFT (12)

// 动态计算宏（运行时根据模式计算）
#define SUPERPAGE_SIZE(N) \
    ((current_page_table_mode == PAGE_TABLE_MODE_SV48) ? \
     ((PAGE_SIZE) << (((3-(N)))*9)) : \
     ((PAGE_SIZE) << (((2-(N)))*9)))

#define PT_LVLS \
    ((current_page_table_mode == PAGE_TABLE_MODE_SV48) ? 4 : 3)

#define PTE_INDEX_SHIFT(LEVEL) \
    ((9 * (PT_LVLS - 1 - (LEVEL))) + 12)

#define PTE_ADDR_MSK BIT_MASK(12, 44)

#define PTE_INDEX(LEVEL, ADDR) (((ADDR) >> PTE_INDEX_SHIFT(LEVEL)) & (0x1FF))
#define PTE_FLAGS_MSK BIT_MASK(0, 8)

// PTE标志位定义
#define PTE_VALID (1ULL << 0)
#define PTE_READ (1ULL << 1)
#define PTE_WRITE (1ULL << 2)
#define PTE_EXECUTE (1ULL << 3)
#define PTE_USER (1ULL << 4)
#define PTE_GLOBAL (1ULL << 5)
#define PTE_ACCESS (1ULL << 6)
#define PTE_DIRTY (1ULL << 7)

#define PTE_V PTE_VALID
#define PTE_A PTE_ACCESS
#define PTE_AD (PTE_ACCESS | PTE_DIRTY)
#define PTE_D PTE_DIRTY
#define PTE_U PTE_USER
#define PTE_G PTE_GLOBAL
#define PTE_R (PTE_READ)
#define PTE_RW (PTE_READ | PTE_WRITE)
#define PTE_X (PTE_EXECUTE)
#define PTE_W (PTE_WRITE)
#define PTE_RX (PTE_READ | PTE_EXECUTE)
#define PTE_RWX (PTE_READ | PTE_WRITE | PTE_EXECUTE)

#define PTE_Pbmt1 (1ULL << 61)
#define PTE_Pbmt2 (1ULL << 62)
#define PTE_Pbmt3 (3ULL << 61)

#define PTE_RSW_OFF 8
#define PTE_RSW_LEN 2
#define PTE_RSW_MSK BIT_MASK(PTE_RSW_OFF, PTE_RSW_LEN)

#define PTE_TABLE (0)
#define PTE_PAGE (PTE_RWX)
#define PTE_SUPERPAGE (PTE_PAGE)

// 测试页基址定义
#ifndef TEST_PPAGE_BASE
    #define TEST_PPAGE_BASE (MEM_BASE+(MEM_SIZE/2))
#endif
#define TEST_VPAGE_BASE (0x100000000)

// 测试页枚举
enum test_page { 
    VSRWX_GURWX,
    VSRWX_GURW,
    VSRWX_GURX,
    VSRWX_GUR,
    VSRWX_GUX,
    VSRW_GURWX,
    VSRW_GURW,
    VSRW_GURX,
    VSRW_GUR,
    VSRW_GUX,
    VSRX_GURWX,
    VSRX_GURW,
    VSRX_GURX,
    VSRX_GUR,
    VSRX_GUX,
    VSR_GURWX,
    VSR_GURW,
    VSR_GURX,
    VSR_GUR,
    VSR_GUX,
    VSX_GURWX,
    VSX_GURW,
    VSX_GURX,
    VSX_GUR,
    VSX_GUX,
    VSURWX_GRWX,
    VSURWX_GRW,
    VSURWX_GRX,
    VSURWX_GR,
    VSV_GV,
    VSURW_GRWX,
    VSURW_GRW,
    VSURW_GRX,
    VSURW_GR,
    VSURW_GX,
    VSURX_GRWX,
    VSURX_GRW,
    VSURX_GRX,
    VSURX_GR,
    VSURX_GX,
    VSUR_GRWX,
    VSUR_GRW,
    VSUR_GRX,
    VSUR_GR,
    VSUR_GX,
    VSUX_GRWX,
    VSUX_GRW,
    VSUX_GRX,
    VSUX_GR,
    VSUX_GX,
    VSURWX_GURWX,
    VSURWX_GURW,
    VSURWX_GURX,
    VSURWX_GUR,
    VSURWX_GUX,
    VSURW_GURWX,
    VSURW_GURW,
    VSURW_GURX,
    VSURW_GUR,
    VSURW_GUX,
    VSURX_GURWX,
    VSURX_GURW,
    VSURX_GURX,
    VSURX_GUR,
    VSURX_GUX,
    VSUR_GURWX,
    VSUR_GURW,
    VSUR_GURX,
    VSUR_GUR,
    VSUR_GUX,
    VSUX_GURWX,
    VSUX_GURW,
    VSUX_GURX,
    VSUX_GUR,
    VSUX_GUX,
    VSRWX_GRWX,
    VSRWX_GRW,
    VSRWX_GRX,
    VSRWX_GR,
    VSRWX_GX,
    VSRW_GRWX,
    VSRW_GRW,
    VSRW_GRX,
    VSRW_GR,
    VSRW_GX,
    VSRX_GRWX,
    VSRX_GRW,
    VSRX_GRX,
    VSRX_GR,
    VSRX_GX,
    VSR_GRWX,
    VSR_GRW,
    VSR_GRX,
    VSR_GR,
    VSR_GX,
    VSX_GRWX,
    VSX_GRW,
    VSX_GRX,
    VSX_GR,
    VSX_GX,
    VSI_GI,
    VSRWX_GI,
    VSRW_GI,
    VSI_GURWX,
    VSI_GUX,
    VSI_GUR,
    VSI_GURW,
    SCRATCHPAD,
    SWITCH1,
    SWITCH2,
    VSRWXPbmt0_GURWXPbmt0,
    VSRWXPbmt0_GURWXPbmt1,
    VSRWXPbmt0_GURWXPbmt2,
    VSRWXPbmt1_GURWXPbmt0,
    VSRWXPbmt1_GURWXPbmt1,
    VSRWXPbmt1_GURWXPbmt2,
    VSRWXPbmt2_GURWXPbmt0,
    VSRWXPbmt2_GURWXPbmt1,
    VSRWXPbmt2_GURWXPbmt2,
    VSRWXPbmt3_GURWXPbmt1,
    VSRWXPbmt2_GURWXPbmt3,
    X = 250,
    TOP = 511,
    TEST_PAGE_MAX
};

typedef uint64_t pte_t;

// 页表数组声明
extern pte_t hspt[][512];
extern pte_t vspt[][512];
extern pte_t hpt_root[];
extern pte_t hpt[][512];

// 辅助函数
static inline uintptr_t vs_page_base(enum test_page tp){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)(TEST_VPAGE_BASE+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test page address");
    }
}

static inline uintptr_t hs_page_base(enum test_page tp){
    return vs_page_base(tp);
}

static inline uintptr_t vs_page_base_limit(enum test_page tp){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)((0x20000000000-0x200000)+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test page address");
    }
}

static inline uintptr_t phys_page_base(enum test_page tp){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)(TEST_PPAGE_BASE+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test page address");
    }
}

static inline uintptr_t phys_page_base_offset(enum test_page tp, uintptr_t base){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)(base+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test page address");
    }
}

// 页表初始化和操作函数
void hspt_init();
void vspt_init();
void hpt_init();
void hspt_switch();
void vspt_switch();
void hpt_switch();
void print_page_table_type();
void pbmt_hspt_to_x(int);
void hspt_u_mode_allow();
void hspt_u_mode_allow_no_leaf();
void hspt_del_D();
void hspt_del_A();
void hspt_del_V();
void hspt_del_R();
void hspt_del_W();
void hspt_del_X();
void hspt_add_G();
void hspt_ptw_ppn_high_bit_overflow_setup();
void hspt_llptw_ppn_high_bit_overflow_setup();
void hspt_leaf_change_base_paddr(int, uintptr_t);
void hspt_2MB_superpage_misalign_setup();
void hspt_2MB_superpage_align_boundary_setup();
void hspt_1GB_superpage_misalign_setup();
void hspt_1GB_superpage_align_boundary_setup();
void hspt_512GB_superpage_misalign_setup();
void hspt_512GB_superpage_align_boundary_setup();
void set_superpage_512G();
void hspt_add_reserved_bits();
void pbmt_hspt_to_x_base_paddr(int, uintptr_t);

#endif // DYNAMIC_PAGE_TABLES_H
