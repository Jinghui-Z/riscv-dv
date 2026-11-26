#include "backend_it_0911_trap.h"

#define EX_VA 0x40000000ULL
static pte_t ex_root[512] __attribute__((aligned(4096)));
static pte_t ex_l1[512] __attribute__((aligned(4096)));
static pte_t ex_l0[512] __attribute__((aligned(4096)));
static uint8_t ex_data[8192] __attribute__((aligned(4096)));
static uint8_t ex_dump[4096] __attribute__((aligned(64)));
static uint8_t ex_mask[256] __attribute__((aligned(64)));

static void ex_prepare(void)
{
    for (unsigned i = 0; i < 512; ++i) ex_root[i] = ex_l1[i] = ex_l0[i] = 0;
    for (unsigned i = 0; i < 4; ++i) ex_root[i] = ((uint64_t)i << 28) | PTE_V | PTE_AD | PTE_RWX;
    ex_root[1] = ((uintptr_t)ex_l1 >> 2) | PTE_V;
    ex_l1[0] = ((uintptr_t)ex_l0 >> 2) | PTE_V;
    ex_l0[0] = ((uintptr_t)ex_data >> 2) | PTE_V | PTE_AD | PTE_RWX;
    /* The next two virtual pages are deliberately invalid. */
    for (unsigned i = 0; i < sizeof(ex_data); ++i) ex_data[i] = 0x30 + i % 47;
    CSRW(CSR_SATP,(8ULL << 60) | ((uintptr_t)ex_root >> 12));
    sfence_vma();
}

#define EX_VECTOR_YOUNGER(n) \
    case n: \
        asm volatile(BE_ASM_BEGIN \
            "lla t0,9f\nsd t0,%[resume]\nli t1,0x40002000-(" #n ")\n" \
            "lla %[pc],1f\n1: vle8.v v8,(%[base])\n" \
            ".rept " #n "\naddi t1,t1,1\n.endr\nld t0,0(t1)\n" \
            "9: li t0,0x20000\ncsrc mstatus,t0\n" BE_ASM_END \
            : [pc] "=&r"(pc), [resume] "=m"(be_it_traps.resume) \
            : [base] "r"(base) : "t0", "t1", "memory"); \
        break

static __attribute__((noinline)) uintptr_t ex_vector_younger(uintptr_t base, unsigned skew)
{
    uintptr_t pc = 0;
    /* Selection is outside the two-fault interval; the address stays exact. */
    switch (skew) {
        EX_VECTOR_YOUNGER(0);  EX_VECTOR_YOUNGER(1);  EX_VECTOR_YOUNGER(2);
        EX_VECTOR_YOUNGER(3);  EX_VECTOR_YOUNGER(4);  EX_VECTOR_YOUNGER(5);
        EX_VECTOR_YOUNGER(6);  EX_VECTOR_YOUNGER(7);  EX_VECTOR_YOUNGER(8);
        EX_VECTOR_YOUNGER(9);  EX_VECTOR_YOUNGER(10); EX_VECTOR_YOUNGER(11);
        EX_VECTOR_YOUNGER(12); EX_VECTOR_YOUNGER(13); EX_VECTOR_YOUNGER(14);
        EX_VECTOR_YOUNGER(15); EX_VECTOR_YOUNGER(16); EX_VECTOR_YOUNGER(17);
        EX_VECTOR_YOUNGER(18); EX_VECTOR_YOUNGER(19); EX_VECTOR_YOUNGER(20);
        EX_VECTOR_YOUNGER(21); EX_VECTOR_YOUNGER(22); EX_VECTOR_YOUNGER(23);
        EX_VECTOR_YOUNGER(24); EX_VECTOR_YOUNGER(25); EX_VECTOR_YOUNGER(26);
        EX_VECTOR_YOUNGER(27); EX_VECTOR_YOUNGER(28); EX_VECTOR_YOUNGER(29);
        EX_VECTOR_YOUNGER(30); EX_VECTOR_YOUNGER(31); EX_VECTOR_YOUNGER(32);
        EX_VECTOR_YOUNGER(33); EX_VECTOR_YOUNGER(34); EX_VECTOR_YOUNGER(35);
        EX_VECTOR_YOUNGER(36); EX_VECTOR_YOUNGER(37); EX_VECTOR_YOUNGER(38);
        EX_VECTOR_YOUNGER(39); EX_VECTOR_YOUNGER(40); EX_VECTOR_YOUNGER(41);
        EX_VECTOR_YOUNGER(42); EX_VECTOR_YOUNGER(43); EX_VECTOR_YOUNGER(44);
        EX_VECTOR_YOUNGER(45); EX_VECTOR_YOUNGER(46); EX_VECTOR_YOUNGER(47);
        EX_VECTOR_YOUNGER(48); EX_VECTOR_YOUNGER(49); EX_VECTOR_YOUNGER(50);
        EX_VECTOR_YOUNGER(51); EX_VECTOR_YOUNGER(52); EX_VECTOR_YOUNGER(53);
        EX_VECTOR_YOUNGER(54); EX_VECTOR_YOUNGER(55); EX_VECTOR_YOUNGER(56);
        EX_VECTOR_YOUNGER(57); EX_VECTOR_YOUNGER(58); EX_VECTOR_YOUNGER(59);
        EX_VECTOR_YOUNGER(60); EX_VECTOR_YOUNGER(61); EX_VECTOR_YOUNGER(62);
        EX_VECTOR_YOUNGER(63); EX_VECTOR_YOUNGER(64);
    }
    return pc;
}
#undef EX_VECTOR_YOUNGER

static bool ex_vector(unsigned row, uint64_t bytes, unsigned skew)
{
    /* A single destination keeps the FOF VL-fixup close to its faulting uop. */
    unsigned lmul = row == 714 || row == 719 || row == 721 ? 1 :
        row == 716 || row == 712 || row == 710 ? 8 : 4;
    unsigned full = bytes * lmul;
    unsigned fault = row == 714 ? 0 : row == 716 || row == 712 || row == 710 ? 7 * bytes + 1 :
        row == 719 ? bytes / 2 : 2 * bytes + 3;
    if (row == 721) fault = bytes / 2;
    unsigned start = row == 706 || row == 708 ? 1 : 0;
    unsigned vl = row == 718 ? fault : full;
    uint64_t vtype = lmul == 1 ? 0 : lmul == 4 ? 2 : 3;
    uintptr_t base = EX_VA + 4096 - fault;
    for (unsigned i = 0; i < full + 64; ++i) ex_dump[i] = 0xa5;
    if (row == 717) {
        for (unsigned i = 0; i < sizeof(ex_mask); ++i) ex_mask[i] = 0;
        for (unsigned i = 0; i < fault; ++i) ex_mask[i/8] |= 1U << (i%8);
    }
    CSRW(CSR_VSTART,0);
    asm volatile("vsetvl zero,%0,%1\nvmv.v.i v8,-1\nvmv.v.i v0,-1\n"
        :: "r"((uint64_t)full), "r"(vtype) : "memory");
    if (row == 717)
        asm volatile("vlm.v v0,(%0)" :: "r"(ex_mask) : "memory");
    asm volatile("vsetvl zero,%0,%1" :: "r"((uint64_t)vl), "r"(vtype) : "memory");
    CSRW(CSR_VSTART,start);
    be_it_trap_install();
    uintptr_t pc;
    be_it_mprv_s();
    if (row == 709) {
        pc = ex_vector_younger(base,skew);
    } else if (row == 710) {
        /* Branches each allocate their own ROB entry behind the late fault. */
        asm volatile(BE_ASM_BEGIN "lla t0,9f\nsd t0,%[resume]\n"
            "lla %[pc],1f\n1: vle8.v v8,(%[base])\n"
            ".rept 128\nbne zero,zero,9f\n.endr\n"
            "9: li t0,0x20000\ncsrc mstatus,t0\n" BE_ASM_END
            : [pc] "=&r"(pc), [resume] "=m"(be_it_traps.resume)
            : [base] "r"(base) : "t0", "memory");
    } else asm volatile(BE_ASM_BEGIN "lla t0,9f\nsd t0,%[resume]\n"
        "li t0,717\nbeq %[row],t0,1f\nli t0,719\nbeq %[row],t0,2f\n"
        "li t0,721\nbeq %[row],t0,3f\n"
        "lla %[pc],4f\n4: vle8.v v8,(%[base])\nj 9f\n"
        "1: lla %[pc],5f\n5: vle8.v v8,(%[base]),v0.t\nj 9f\n"
        "2: lla %[pc],6f\n6: vle8.v v0,(%[base])\nj 9f\n"
        "3: lla %[pc],7f\n7: vle8ff.v v8,(%[base])\n"
        "9: li t0,0x20000\ncsrc mstatus,t0\n" BE_ASM_END
        : [pc] "=&r"(pc), [resume] "=m"(be_it_traps.resume)
        : [row] "r"((uint64_t)row), [base] "r"(base) : "t0","memory");
    uint64_t final_vl = CSRR(CSR_VL);
    be_it_trap_restore();
    bool suppressed = row == 717 || row == 718 || row == 721;
    bool ok = suppressed ? be_it_traps.count == 0 :
        be_it_traps.count == 1 && be_it_traps.event[0].cause == CAUSE_LPF &&
        be_it_traps.event[0].epc == pc && be_it_traps.event[0].tval == EX_VA + 4096 &&
        be_it_traps.event[0].vstart == fault;
    ok &= final_vl == (row == 721 ? fault : vl);
    CSRW(CSR_VSTART,0);
    /* Reset VL only after observing FOF's architectural VL. */
    asm volatile("vsetvl zero,%0,%1" :: "r"((uint64_t)full), "r"(vtype) : "memory");
    if (row == 719) asm volatile("vse8.v v0,(%0)" :: "r"(ex_dump) : "memory");
    else asm volatile("vse8.v v8,(%0)" :: "r"(ex_dump) : "memory");
    for (unsigned i = 0; i < full; ++i) {
        /* FOF is allowed to modify elements beyond its shortened VL. */
        if (row == 721 && i >= fault) continue;
        unsigned expected = i >= start && i < fault ? ex_data[4096 - fault + i] : 255;
        ok &= ex_dump[i] == expected;
    }
    for (unsigned i = full; i < full + 64; ++i) ok &= ex_dump[i] == 0xa5;
    if (!ok) printf("\nrow=%u skew=%u count=%lu cause=%lu vstart=%lu fault=%u vl=%lu\n",
        row,skew,be_it_traps.count,be_it_traps.event[0].cause,be_it_traps.event[0].vstart,fault,final_vl);
    return ok;
}

bool ai_micro_backend_it_vector_fault_merge(void)
{
    TEST_START();
    be_it_enable();
    uint64_t bytes = CSRR(CSR_VLENB), satp = CSRR(CSR_SATP);
    TEST_ASSERT("VLEN fits the seed/mask/dump buffers", bytes >= 16 && bytes <= 256);
    ex_prepare();
    const unsigned rows[] = {705,706,708,709,710,711,712,714,715,716,717,718,719,720,721};
    for (unsigned i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) {
        unsigned row = rows[i];
        bool software_only = row == 717 || row == 718;
        backend_it_control.target_addr = EX_VA + 4096;
        if (!software_only && !be_it_begin(row,0)) return false;
        bool correct = true;
        unsigned attempts = row == 709 ? 65 : row == 710 ? 16 : 1;
        for (unsigned skew = 0; skew < attempts; ++skew)
            correct &= ex_vector(row,bytes,skew);
        TEST_ASSERT("precise vector fault, VL/vstart, prefix and old destination merge", correct, "row=%u", row);
        printf("BE_IT_SOFTWARE_RESULT row=%u status=%s\n", row, correct ? "PASS" : "FAIL");
        if (!software_only)
            TEST_ASSERT("IT checked uop order, ROB/flush relation and exception merge paths", be_it_finish());
    }
    CSRW(CSR_VSTART,0);
    CSRW(CSR_SATP,satp);
    sfence_vma();
    TEST_END("ai_micro_backend_it_vector_fault_merge");
}
