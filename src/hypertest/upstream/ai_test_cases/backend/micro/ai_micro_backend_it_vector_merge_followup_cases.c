#include "backend_it_0911_trap.h"

#define MERGE_VA 0x50000000ULL
static pte_t merge_root[512] __attribute__((aligned(4096)));
static pte_t merge_l1[512] __attribute__((aligned(4096)));
static pte_t merge_l0[512] __attribute__((aligned(4096)));
static uint8_t merge_data[4096] __attribute__((aligned(4096)));
static uint8_t merge_dump[576] __attribute__((aligned(64)));
static uint8_t merge_mask[64] __attribute__((aligned(64)));

static void merge_prepare(void)
{
    for (unsigned i = 0; i < 512; i++) merge_root[i] = merge_l1[i] = merge_l0[i] = 0;
    for (unsigned i = 0; i < 4; i++)
        merge_root[i] = ((uint64_t)i << 28) | PTE_V | PTE_AD | PTE_RWX;
    merge_root[1] = ((uintptr_t)merge_l1 >> 2) | PTE_V;
    merge_l1[128] = ((uintptr_t)merge_l0 >> 2) | PTE_V;
    merge_l0[0] = ((uintptr_t)merge_data >> 2) | PTE_V | PTE_AD | PTE_RWX;
    for (unsigned i = 0; i < sizeof(merge_data); i++) merge_data[i] = 0x30 + i % 47;
    CSRW(CSR_SATP, (8ULL << 60) | ((uintptr_t)merge_root >> 12));
    sfence_vma();
}

static bool merge_variant(unsigned variant, unsigned bytes)
{
    unsigned full = 2 * bytes, fault = 5;
    unsigned vl = variant == 1 ? full - 3 : full;
    uintptr_t base = MERGE_VA + 4096 - fault, pc;
    for (unsigned i = 0; i < sizeof(merge_dump); i++) merge_dump[i] = 0xa5;
    for (unsigned i = 0; i < sizeof(merge_mask); i++) merge_mask[i] = 0xaa;
    CSRW(CSR_VSTART, 0);
    asm volatile("vsetvli zero,%0,e8,m2,tu,mu\nvmv.v.i v8,-1\nvmv.v.i v0,-1"
                 :: "r"((uint64_t)full) : "memory");
    if (variant == 0)
        asm volatile("vlm.v v0,(%0)" :: "r"(merge_mask) : "memory");
    asm volatile("vsetvli zero,%0,e8,m2,tu,mu" :: "r"((uint64_t)vl) : "memory");
    be_it_trap_install();
    be_it_mprv_s();
    asm volatile(BE_ASM_BEGIN
        "lla t0,9f\nsd t0,%[resume]\n"
        "beqz %[variant],1f\nli t0,2\nbeq %[variant],t0,2f\n"
        "lla %[pc],3f\n3: vle8.v v8,(%[base])\nj 9f\n"
        "1: lla %[pc],4f\n4: vle8.v v8,(%[base]),v0.t\nj 9f\n"
        "2: lla %[pc],5f\n5: vle8.v v0,(%[base])\n"
        "9: li t0,0x20000\ncsrc mstatus,t0\n" BE_ASM_END
        : [pc] "=&r"(pc), [resume] "=m"(be_it_traps.resume)
        : [base] "r"(base), [variant] "r"((uint64_t)variant)
        : "t0", "memory");
    uint64_t actual_vl = CSRR(CSR_VL);
    be_it_trap_restore();
    bool ok = be_it_traps.count == 1 &&
        be_it_traps.event[0].cause == CAUSE_LPF && be_it_traps.event[0].epc == pc &&
        be_it_traps.event[0].tval == MERGE_VA + 4096 &&
        be_it_traps.event[0].vstart == fault && actual_vl == vl;
    CSRW(CSR_VSTART, 0);
    asm volatile("vsetvli zero,%0,e8,m2,tu,mu" :: "r"((uint64_t)full) : "memory");
    if (variant == 2) asm volatile("vse8.v v0,(%0)" :: "r"(merge_dump) : "memory");
    else asm volatile("vse8.v v8,(%0)" :: "r"(merge_dump) : "memory");
    for (unsigned i = 0; i < full; i++) {
        bool active = variant != 0 || (merge_mask[i / 8] & (1U << (i % 8)));
        unsigned expected = active && i < fault ? merge_data[4096 - fault + i] : 255;
        ok &= merge_dump[i] == expected;
    }
    for (unsigned i = full; i < sizeof(merge_dump); i++) ok &= merge_dump[i] == 0xa5;
    printf("BE_IT_MERGE variant=%u count=%lu cause=%lu epc=%lx vstart=%lu vl=%lu status=%s\n",
           variant, be_it_traps.count, be_it_traps.event[0].cause,
           be_it_traps.event[0].epc, be_it_traps.event[0].vstart, actual_vl, ok ? "PASS" : "FAIL");
    return ok;
}

bool ai_micro_backend_it_vector_merge_corners(void)
{
    TEST_START();
    be_it_enable();
    uint64_t bytes = CSRR(CSR_VLENB), satp = CSRR(CSR_SATP);
    TEST_ASSERT("VLEN fits vector merge buffers", bytes >= 16 && bytes <= 256);
    merge_prepare();
    if (!be_it_begin(713, 0)) return false;
    for (unsigned variant = 0; variant < 3; variant++)
        TEST_ASSERT("active-element LPF preserves mask, tail and old destination data",
                    merge_variant(variant, bytes), "variant=%u", variant);
    TEST_ASSERT("IT checked the same exception transaction through destination merge", be_it_finish());
    CSRW(CSR_VSTART, 0);
    CSRW(CSR_SATP, satp);
    sfence_vma();
    TEST_END("ai_micro_backend_it_vector_merge_corners");
}
