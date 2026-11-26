#include "backend_it_0911.h"

static pte_t trans_tables[2][12][512] __attribute__((aligned(4096)));
static unsigned trans_used[2];
static uint32_t trans_code[4][1024] __attribute__((aligned(4096)));

static pte_t trans_leaf(uintptr_t pa)
{
    return (pa >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static void trans_init(unsigned bank, unsigned levels)
{
    for (unsigned t = 0; t < 12; ++t)
        for (unsigned i = 0; i < 512; ++i) trans_tables[bank][t][i] = 0;
    trans_used[bank] = levels == 4 ? 2 : 1;
    pte_t *l2 = trans_tables[bank][levels == 4 ? 1 : 0];
    if (levels == 4) trans_tables[bank][0][0] = ((uintptr_t)l2 >> 2) | PTE_V;
    for (unsigned i = 0; i < 4; ++i) l2[i] = trans_leaf((uintptr_t)i << 30);
}

static pte_t *trans_map(unsigned bank, unsigned levels, uintptr_t va, uintptr_t pa)
{
    pte_t *table = trans_tables[bank][0];
    for (unsigned level = levels-1; level; --level) {
        unsigned index = (va >> (12+9*level)) & 511;
        pte_t old = table[index];
        if (!(old & PTE_V) || (old & PTE_RWX)) {
            if (trans_used[bank] >= 12) return NULL;
            pte_t *next = trans_tables[bank][trans_used[bank]++];
            if (old & PTE_RWX) {
                uintptr_t base = (old >> 10) << 12;
                for (unsigned i = 0; i < 512; ++i)
                    next[i] = trans_leaf(base + ((uintptr_t)i << (12+9*(level-1))));
            }
            table[index] = ((uintptr_t)next >> 2) | PTE_V;
        }
        table = (pte_t *)((table[index] >> 10) << 12);
    }
    pte_t *leaf = &table[(va >> 12) & 511];
    *leaf = trans_leaf(pa);
    return leaf;
}

static uint64_t trans_satp(unsigned bank,unsigned levels)
{
    return ((uint64_t)(levels == 4 ? 9 : 8) << 60) | (1ULL << 44) |
        ((uintptr_t)trans_tables[bank][0] >> 12);
}

static uint64_t trans_call(uintptr_t pc, uint64_t next, uintptr_t target)
{
    uint64_t out;
    asm volatile(BE_ASM_BEGIN "lla t0,1f\nsd t0,%[fault]\nmv a0,%[next]\nmv a1,%[target]\n"
        "1: jalr ra,0(%[pc])\nmv %[out],a0\n" BE_ASM_END
        : [out] "=r"(out),[fault] "=m"(excpt.fault_inst)
        : [pc] "r"(pc),[next] "r"(next),[target] "r"(target)
        : "t0","t5","t6","a0","a1","ra","memory");
    excpt.fault_inst = 0;
    return out;
}

static uint64_t trans_sfence_call(uintptr_t pc, uintptr_t va, pte_t *leaf, pte_t next)
{
    uint64_t out;
    asm volatile(BE_ASM_BEGIN "lla t0,1f\nsd t0,%[fault]\n"
        "li a0,1\nmv a1,%[va]\nmv a2,%[next]\nmv a3,%[leaf]\n"
        "1: jalr ra,0(%[pc])\nmv %[out],a0\n" BE_ASM_END
        : [out] "=r"(out), [fault] "=m"(excpt.fault_inst)
        : [pc] "r"(pc), [va] "r"(va), [leaf] "r"(leaf), [next] "r"(next)
        : "t0","t5","t6","a0","a1","a2","a3","ra","memory");
    excpt.fault_inst = 0;
    return out;
}

bool ai_micro_backend_it_satp_transition(void)
{
    TEST_START();
    be_it_enable();
    uint64_t original = CSRR(CSR_SATP);
    CSRC(CSR_MSTATUS,MSTATUS_TVM);
    for (unsigned row = 852; row <= 858; ++row) {
        unsigned old_levels = row <= 853 ? 4 : 3;
        unsigned new_levels = row == 854 || row == 857 ? 4 : 3;
        trans_init(0,old_levels);
        trans_init(1,new_levels);
        for (unsigned i = 0; i < 4; ++i) {
            trans_code[i][0] = 0x18051073; /* csrw satp,a0 */
            trans_code[i][1] = i & 1 ? 0x04900513 : 0x01100513;
            trans_code[i][2] = 0x00008067;
        }
        uintptr_t entry = (uintptr_t)trans_code[0];
        uintptr_t target = 0x40010000;
        uintptr_t satp_pc = 0;
        pte_t *target_leaf = NULL;
        bool expect_fault = row <= 853;
        bool maps = true;
        if (expect_fault) {
            entry = 0xffff800000000000ULL;
            target_leaf = trans_map(0,4,entry,(uintptr_t)trans_code[0]);
            maps &= target_leaf != NULL;
            if (row == 853) {
                // Invalidate after this block has been fetched. The older
                // divisions let a new real old-mode PTW precede SATP flush.
                for (unsigned i = 0; i < 1024; ++i) trans_code[0][i] = 0x00000013;
                trans_code[0][0] = 0x16058073; /* sinval.vma a1,x0 */
                trans_code[0][1] = 0x0005e013; /* prefetch.i 0(a1) */
                trans_code[0][2] = 0xfff00f13; /* li t5,-1 */
                trans_code[0][3] = 0x00300f93; /* li t6,3 */
                for (unsigned i = 4; i < 8; ++i) trans_code[0][i] = 0x03ff5f33;
                trans_code[0][8] = 0x18051073;
                trans_code[0][9] = 0x01100513;
                trans_code[0][10] = 0x00008067;
                satp_pc = entry + 32;
                target = satp_pc + 4;
            }
        } else if (row == 854) {
            entry = 0x3ffffffffcULL;
            trans_code[0][1023] = 0x18051073;
            trans_code[1][0] = 0x04900513;
            trans_code[1][1] = 0x00008067;
            maps &= trans_map(0,3,entry & ~4095ULL,(uintptr_t)trans_code[0]) != NULL;
            maps &= trans_map(1,4,entry+4,(uintptr_t)trans_code[1]) != NULL;
        } else if (row == 855) {
            entry = (uintptr_t)trans_code[1];
            maps &= trans_map(0,3,entry,(uintptr_t)trans_code[0]) != NULL;
        } else if (row == 856 || row == 857) {
            maps &= trans_map(1,new_levels,entry,(uintptr_t)trans_code[1]) != NULL;
        } else {
            trans_code[0][1] = 0x12000073; /* sfence.vma x0,x0 */
            trans_code[0][2] = 0x00058067; /* jr a1 */
            trans_code[2][0] = 0x01100513;
            trans_code[2][1] = 0x00008067;
            trans_code[3][0] = 0x04900513;
            trans_code[3][1] = 0x00008067;
            maps &= trans_map(0,3,target,(uintptr_t)trans_code[2]) != NULL;
            maps &= trans_map(1,3,target,(uintptr_t)trans_code[3]) != NULL;
        }
        TEST_ASSERT("SATP transition page-table construction fits", maps);
        uint64_t old_satp = row == 856 || row == 857 ? 0 : trans_satp(0,old_levels);
        uint64_t next_satp = row == 855 ? 0 : trans_satp(1,new_levels);
        // A fresh ASID prevents old-root negative translations from surviving
        // across the boundary; writing SATP does not invalidate those entries.
        if (row == 854) next_satp = (next_satp & ~(0xffffULL << 44)) | (2ULL << 44);
        asm volatile("fence.i" ::: "memory");
        CSRW(CSR_SATP,old_satp);
        sfence_vma();
        if (!satp_pc) satp_pc = entry;
        backend_it_control.target_pc = satp_pc;
        backend_it_control.target_addr = row == 853 ? (uintptr_t)target_leaf : satp_pc+4;
        backend_it_control.auxiliary = row == 858 ? target : satp_pc+4;
        bool monitored = row == 852 || row == 853 || row == 858;
        if (monitored && !be_it_begin(row,0)) return false;
        goto_priv(PRIV_HS);
        if (row==852) {
            TEST_SETUP_EXCEPT();
            TEST_ASSERT("warm the sequential PC under the old Sv48 translation",
                trans_call(entry+4,0,0)==17 && !excpt.triggered);
        } else if (row==858) {
            TEST_SETUP_EXCEPT();
            TEST_ASSERT("warm the target using the old root before changing PPN",
                trans_call(target,0,0)==17 && !excpt.triggered);
        }
        TEST_SETUP_EXCEPT();
        uint64_t out = trans_call(entry,next_satp,target);
        bool fault = excpt.triggered;
        uint64_t cause = excpt.cause, epc = excpt.epc, tval = excpt.tval;
        goto_priv(PRIV_M);
        CSRW(CSR_SATP,original);
        sfence_vma();
        bool correct = expect_fault ? fault && cause == CAUSE_IPF &&
            epc == satp_pc+4 && tval == satp_pc+4 : !fault && out == 73;
        if (!correct)
            printf("\nBE_IT_SATP_ARCH_FAIL row=%u fault=%u cause=%lu epc=%lx tval=%lx expected_pc=%lx out=%lu\n",
                row,fault,cause,epc,tval,satp_pc+4,out);
        TEST_ASSERT("SATP flush resumes at the sequential PC using the new mode/root",
            correct, "row=%u out=%lu cause=%lu epc=%lx",row,out,cause,epc);
        if (monitored)
            TEST_ASSERT("IT checked old hit/PTW state, satpFlush and first-instruction tagging", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_satp_transition");
}

bool ai_micro_backend_it_sfence_scope(void)
{
    TEST_START();
    be_it_enable();
    uint64_t original = CSRR(CSR_SATP);
    uintptr_t va = 0x50000000, control = va + 4096;
    trans_code[0][0] = 0x01100513;
    trans_code[0][1] = 0x00008067;
    trans_code[1][0] = 0x04900513;
    trans_code[1][1] = 0x00008067;
    asm volatile("fence.i" ::: "memory");
    for (unsigned row = 860; row <= 863; ++row) {
        trans_init(0,3);
        pte_t *leaf = trans_map(0,3,va,(uintptr_t)trans_code[0]);
        TEST_ASSERT("map target and unaffected control VA", leaf && trans_map(0,3,control,(uintptr_t)trans_code[0]));
        CSRW(CSR_SATP,trans_satp(0,3));
        sfence_vma();
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        TEST_ASSERT("old translation warmup", trans_call(va,0,0) == 17 && !excpt.triggered);
        goto_priv(PRIV_M);
        if (row==862) {
            CSRW(CSR_SATP,(trans_satp(0,3)&~(0xffffULL<<44))|(2ULL<<44));
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            TEST_ASSERT("ASID2 control translation warmup",trans_call(control,0,0)==17 && !excpt.triggered);
            goto_priv(PRIV_M);
            CSRW(CSR_SATP,trans_satp(0,3));
        }
        /* Put SFENCE immediately before a cold executable page. Fetch can
         * walk that page while the serial instruction is still in flight. */
        TEST_ASSERT("map the page preceding the SFENCE target",
            trans_map(0,3,va-4096,(uintptr_t)trans_code[2]) != NULL);
        for (unsigned n=0; n<1024; ++n) trans_code[2][n]=0x00000013;
        trans_code[2][1008] = 0xfff00f13; /* li t5,-1 */
        trans_code[2][1009] = 0x00300f93; /* li t6,3 */
        trans_code[2][1010] = 0x03ff5f33; /* divu t5,t5,t6 */
        trans_code[2][1011] = 0x03ff5f33;
        trans_code[2][1012] = 0x000f3f33; /* sltu t5,t5,zero */
        trans_code[2][1013] = 0x00df0f33; /* add t5,t5,a3 */
        trans_code[2][1014] = 0x00cf3023; /* sd a2,0(t5) */
        /* Keep SFENCE in the first 32-byte fetch block. Putting it at
         * page-4 makes its own cross-line fetch wait for the target PTW. */
        trans_code[2][1015] = 0x12000073 |
            ((row==861 || row==863) ? (11U<<15) : 0) |
            ((row==862 || row==863) ? (10U<<20) : 0);
        asm volatile("fence.i" ::: "memory");
        sfence_vma();
        backend_it_control.target_pc = va;
        backend_it_control.target_addr = (uintptr_t)leaf;
        if (!be_it_begin(row,0)) return false;
        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        /* The dependency-delayed store publishes the new PTE after the
         * speculative old walk has started, before SFENCE drains stores. */
        uint64_t got = trans_sfence_call(va-64,va,leaf,
            trans_leaf((uintptr_t)trans_code[1]));
        uint64_t other = trans_call(control,0,0);
        TEST_ASSERT("matching translation uses new PTE and control VA keeps correct contents",
            !excpt.triggered && got == 73 && other == 17);
        goto_priv(PRIV_M);
        if (row==862) {
            CSRW(CSR_SATP,(trans_satp(0,3)&~(0xffffULL<<44))|(2ULL<<44));
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            TEST_ASSERT("ASID2 control result is not contaminated by ASID1 invalidation",
                trans_call(control,0,0)==17 && !excpt.triggered);
            goto_priv(PRIV_M);
        }
        CSRW(CSR_SATP,original);
        sfence_vma();
        TEST_ASSERT("IT checked matched in-flight response and actual invalidation scope", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_sfence_scope");
}

bool ai_micro_backend_it_backend_fault_marker(void)
{
    TEST_START();
    be_it_enable();
    uint64_t original = CSRR(CSR_SATP);
    trans_init(0,3);
    trans_code[0][0] = 0x04900513;
    trans_code[0][1] = 0x00008067;
    for (unsigned n=0; n<32; ++n) trans_code[3][n] = 0x00000013;
    trans_code[3][0] = 0x0080006f;
    trans_code[3][1] = 0x00008067;
    trans_code[3][2] = 0xfff00f13;
    trans_code[3][3] = 0x00300f93;
    trans_code[3][4] = trans_code[3][5] = 0x03ff5f33;
    trans_code[3][6] = 0x000f3f33;
    trans_code[3][7] = 0xfe0f04e3;
    asm volatile("fence.i" ::: "memory");
    for (unsigned row = 767; row <= 773; ++row) {
        if (row == 769) {
            /* Same-epoch target delivery and output isolation have a
             * dedicated scan; this simple marker loop is not its oracle. */
            printf("\nBE_IT_SEPARATE_CASE row=769 case=ai_micro_backend_it_ftq_fault_pointer_isolation\n");
            continue;
        }
        bool access = row == 768;
        uintptr_t bad = access ? 0x0100000000000000ULL : 0x4000000000ULL;
        if (row==772) {
            /* The jump is noncanonical in Sv39. Its truncated VPN still
             * walks these PTEs, with a separate invalid second-page leaf. */
            bad=0x4000000ffeULL;
            TEST_ASSERT("cross-page fetch has one mapped and one invalid page",
                trans_map(0,3,bad&~4095ULL,(uintptr_t)trans_code[1]) != NULL);
            pte_t *second=trans_map(0,3,(bad+2)&~4095ULL,(uintptr_t)trans_code[2]);
            TEST_ASSERT("second-page leaf exists",second != NULL);
            *second=0;
            write16((uintptr_t)trans_code[1]+4094,0x0513);
            write16((uintptr_t)trans_code[2],0x0015);
            asm volatile("fence.i" ::: "memory");
        }
        CSRW(CSR_SATP,access ? 0 : trans_satp(0,3));
        sfence_vma();
        backend_it_control.target_pc = bad;
        backend_it_control.target_addr = row == 770 ? (uintptr_t)&trans_code[3][16] : 0;
        backend_it_control.auxiliary = row==772 ? bad+2 : 0;
        if (!be_it_begin(row,0)) return false;
        if (!access) goto_priv(PRIV_HS);
        if (row == 770) {
            // The taken branch returns from the first line while its cold
            // fall-through line remains a genuine old fetch transaction.
            asm volatile(BE_ASM_BEGIN "jalr ra,0(%0)\n" BE_ASM_END
                :: "r"(trans_code[3]) : "ra","t5","t6","memory");
        }
        TEST_SETUP_EXCEPT();
        trans_call(bad,0,0);
        bool correct = excpt.triggered && excpt.cause == (access ? CAUSE_IAF : CAUSE_IPF) &&
            excpt.epc == bad && (excpt.tval == bad || (row==772 && excpt.tval==bad+2));
        goto_priv(PRIV_M);
        CSRW(CSR_SATP,original);
        sfence_vma();
        TEST_ASSERT("faulting jump reports the target's own cause/PC/address", correct);
        TEST_SETUP_EXCEPT();
        TEST_ASSERT("a subsequent clean target does not inherit the fault tag",
            trans_call((uintptr_t)trans_code[0],0,0) == 73 && !excpt.triggered);
        TEST_ASSERT("IT checked backend origin, FTQ binding, pipeline stalls and tag clear", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_backend_fault_marker");
}
