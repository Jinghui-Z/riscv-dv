#include "backend_it_0911_trap.h"

#define AGE_VA 0x40000000ULL
static pte_t age_root[512] __attribute__((aligned(4096)));
static pte_t age_l1[512] __attribute__((aligned(4096)));
static pte_t age_l0[512] __attribute__((aligned(4096)));
static uint64_t age_data[1024] __attribute__((aligned(4096)));

static void age_prepare(unsigned row)
{
    for (unsigned i = 0; i < 512; ++i)
        age_root[i] = age_l1[i] = age_l0[i] = 0;
    for (unsigned i = 0; i < 4; ++i)
        age_root[i] = ((uint64_t)i << 28) | PTE_V | PTE_AD | PTE_RWX;
    age_root[1] = ((uintptr_t)age_l1 >> 2) | PTE_V;
    age_l1[0] = ((uintptr_t)age_l0 >> 2) | PTE_V;
    /* MXR permits prewarming both pages. Clearing MXR retains their TLB
     * entries: R-only rejects stores, X-only rejects scalar/vector loads. */
    age_l0[1] = ((uintptr_t)age_data >> 2) | PTE_V | PTE_AD |
        (row == 702 ? PTE_X : PTE_R);
    age_l0[2] = ((uintptr_t)(age_data + 512) >> 2) | PTE_V | PTE_AD |
        (row == 702 ? PTE_R : PTE_X);
    CSRW(CSR_SATP, (8ULL << 60) | ((uintptr_t)age_root >> 12));
    sfence_vma();
}

static void age_warm(void)
{
    CSRS(CSR_MSTATUS, MSTATUS_MXR);
    be_it_mprv_s();
    asm volatile("ld t0,0(%0)\nld t0,0(%1)\nfence r,r\n"
        :: "r"(AGE_VA + 4096), "r"(AGE_VA + 8192) : "t0", "memory");
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | MSTATUS_MXR);
}

/* Select 0..32 dependent additions, restoring the original target address.
 * Each addition is real address work, not an elapsed-time claim. */
#define AGE_SKEW(reg, label) \
    "sub " reg "," reg ",%[skew]\nslli t4,%[skew],2\n" \
    "lla t3," label "f\nsub t3,t3,t4\njr t3\n" \
    ".rept 32\naddi " reg "," reg ",1\n.endr\n" label ":\n"

/* Keep all selection branches outside the two-fault instruction interval. */
#define AGE_STRAIGHT(older, younger, old_delay, young_delay) \
    asm volatile(BE_ASM_BEGIN \
        "lla t0,9f\nsd t0,%[resume]\n" \
        "li t1,0x40001000-(" old_delay ")\n" \
        "li t2,0x40002000-(" young_delay ")\n" \
        BE_DELAY \
        ".rept " old_delay "\naddi t1,t1,1\n.endr\n" \
        "lla %[pc],4f\n4: " older "\n" \
        ".rept " young_delay "\naddi t2,t2,1\n.endr\n" \
        younger "\n9: li t0,0x20000\ncsrc mstatus,t0\n" BE_ASM_END \
        : [pc] "=&r"(pc), [resume] "=m"(be_it_traps.resume) \
        : : "t0", "t1", "t2", "t3", "t5", "t6", "memory")

#define AGE_STRAIGHT_CASE(n) \
    case n: \
        if (row == 702) { \
            AGE_STRAIGHT("vle8.v v8,(t1)", "sd zero,0(t2)", "0", #n); \
        } else if (skew & 1) { \
            AGE_STRAIGHT("sd zero,0(t1)", "ld t3,0(t2)", "0", #n); \
        } else { \
            AGE_STRAIGHT("sd zero,0(t1)", "ld t3,0(t2)", #n, "0"); \
        } \
        break

static __attribute__((noinline)) uintptr_t age_straight_pair(unsigned row, unsigned skew)
{
    uintptr_t pc = 0;
    unsigned delay = row == 702 ? skew : skew / 2;
    switch (delay) {
        AGE_STRAIGHT_CASE(0);  AGE_STRAIGHT_CASE(1);  AGE_STRAIGHT_CASE(2);
        AGE_STRAIGHT_CASE(3);  AGE_STRAIGHT_CASE(4);  AGE_STRAIGHT_CASE(5);
        AGE_STRAIGHT_CASE(6);  AGE_STRAIGHT_CASE(7);  AGE_STRAIGHT_CASE(8);
        AGE_STRAIGHT_CASE(9);  AGE_STRAIGHT_CASE(10); AGE_STRAIGHT_CASE(11);
        AGE_STRAIGHT_CASE(12); AGE_STRAIGHT_CASE(13); AGE_STRAIGHT_CASE(14);
        AGE_STRAIGHT_CASE(15); AGE_STRAIGHT_CASE(16); AGE_STRAIGHT_CASE(17);
        AGE_STRAIGHT_CASE(18); AGE_STRAIGHT_CASE(19); AGE_STRAIGHT_CASE(20);
        AGE_STRAIGHT_CASE(21); AGE_STRAIGHT_CASE(22); AGE_STRAIGHT_CASE(23);
        AGE_STRAIGHT_CASE(24); AGE_STRAIGHT_CASE(25); AGE_STRAIGHT_CASE(26);
        AGE_STRAIGHT_CASE(27); AGE_STRAIGHT_CASE(28); AGE_STRAIGHT_CASE(29);
        AGE_STRAIGHT_CASE(30); AGE_STRAIGHT_CASE(31); AGE_STRAIGHT_CASE(32);
    }
    return pc;
}

bool ai_micro_backend_it_exception_age_matrix(void)
{
    TEST_START();
    be_it_enable();
    uint64_t satp = CSRR(CSR_SATP), status = CSRR(CSR_MSTATUS);
    age_data[0] = 0x123456789abcdef0ULL;
    age_data[512] = 0x0fedcba987654321ULL;
    for (unsigned row = 701; row <= 704; ++row) {
        age_prepare(row);
        backend_it_control.target_addr = AGE_VA + 4096;
        backend_it_control.auxiliary = AGE_VA + 8192;
        if (!be_it_begin(row, 0)) return false;
        unsigned attempts = row == 703 ? 1 : 33;
        for (unsigned skew = 0; skew < attempts; ++skew) {
            TEST_SETUP_EXCEPT();
            age_warm();
            TEST_ASSERT("MXR prewarming of both permission-fault pages is legal", !excpt.triggered);
            be_it_trap_install();
            uintptr_t old_pc;
            asm volatile("vsetivli zero,4,e8,m1,tu,mu\nvmv.v.i v8,7\n"
                ::: "memory");
            be_it_mprv_s();
            if (row == 702 || row == 704) {
                old_pc = age_straight_pair(row, skew);
            } else {
            asm volatile(BE_ASM_BEGIN
                "lla t0,9f\nsd t0,%[resume]\nli t1,0x40001000\nli t2,0x40002000\n"
                /* Independent older DIVs keep either fault away from ROB head. */
                BE_DELAY
                "li t3,702\nbeq %[row],t3,3f\nli t3,703\nbeq %[row],t3,6f\n"
                "li t3,704\nbeq %[row],t3,7f\n"
                AGE_SKEW("t1", "1")
                "j 7f\n"
                "6: li t6,3\n.rept 6\nslli t4,t1,1\nadd t4,t4,t1\ndivu t1,t4,t6\n.endr\n"
                "7: lla %[pc],4f\n4: sd zero,0(t1)\n"
                "li t3,704\nbne %[row],t3,8f\n"
                AGE_SKEW("t2", "2")
                "8: ld t3,0(t2)\nj 9f\n"
                "3: lla %[pc],5f\n5: vle8.v v8,(t1)\n"
                AGE_SKEW("t2", "0")
                "sd zero,0(t2)\n"
                "9: li t0,0x20000\ncsrc mstatus,t0\n" BE_ASM_END
                : [pc] "=&r"(old_pc), [resume] "=m"(be_it_traps.resume)
                : [row] "r"((uint64_t)row), [skew] "r"((uint64_t)skew)
                : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory");
            }
            be_it_trap_restore();
            TEST_ASSERT("only the older permission fault traps with its own metadata",
                be_it_traps.count == 1 &&
                be_it_traps.event[0].cause == (row == 702 ? CAUSE_LPF : CAUSE_SPF) &&
                be_it_traps.event[0].epc == old_pc &&
                be_it_traps.event[0].tval == AGE_VA + 4096 &&
                (row != 702 || be_it_traps.event[0].vstart == 0),
                "row=%u skew=%u", row, skew);
            TEST_ASSERT("faulting and younger stores leave both physical pages unchanged",
                age_data[0] == 0x123456789abcdef0ULL &&
                age_data[512] == 0x0fedcba987654321ULL);
        }
        TEST_ASSERT("IT observed the required exception arrival and age relation", be_it_finish());
    }
    CSRW(CSR_SATP, satp);
    sfence_vma();
    CSRW(CSR_MSTATUS, status);
    TEST_END("ai_micro_backend_it_exception_age_matrix");
}
