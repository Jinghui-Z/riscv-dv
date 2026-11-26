#include "backend_it_0911_trap.h"
#include "backend_it_vmodule.h"

static volatile uint64_t serial_mem[64] __attribute__((aligned(64)));
static uint8_t serial_vec[4096] __attribute__((aligned(64)));
static uint32_t serial_code[4096] __attribute__((aligned(4096)));
static pte_t serial_root[512] __attribute__((aligned(4096)));

bool ai_micro_backend_it_serial_order(void)
{
    TEST_START();
    be_it_enable();
    const unsigned rows[] = {723,724,725,726,727,728,731,732};
    for (unsigned i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) {
        unsigned row = rows[i];
        serial_mem[0] = 73;
        serial_mem[1] = 0;
        if (!be_it_begin(row,0)) return false;
        TEST_SETUP_EXCEPT();
        uint64_t result = 0, csr = CSRR(CSR_MSCRATCH), satp = CSRR(CSR_SATP);
        if (row == 732) {
            uint64_t status = CSRR(CSR_MSTATUS), deleg = CSRR(CSR_MEDELEG);
            CSRS(CSR_MSTATUS,MSTATUS_TVM | (1ULL << 22));
            CSRC(CSR_MEDELEG,1ULL << CAUSE_ILLEGAL_INSTRUCTION);
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            asm volatile("csrr %0,mscratch" : "=r"(result));
            TEST_ASSERT("S-mode machine CSR access is illegal", excpt.triggered && excpt.cause == CAUSE_ILLEGAL_INSTRUCTION);
            TEST_SETUP_EXCEPT();
            asm volatile("sfence.vma" ::: "memory");
            TEST_ASSERT("S-mode SFENCE obeys TVM", excpt.triggered && excpt.cause == CAUSE_ILLEGAL_INSTRUCTION);
            TEST_SETUP_EXCEPT();
            asm volatile("sret" ::: "memory");
            TEST_ASSERT("S-mode SRET obeys TSR", excpt.triggered && excpt.cause == CAUSE_ILLEGAL_INSTRUCTION);
            goto_priv(PRIV_M);
            CSRW(CSR_MSTATUS,status);
            CSRW(CSR_MEDELEG,deleg);
        } else if (row == 726) {
            for (unsigned n = 0; n < 512; ++n) serial_root[n] = 0;
            serial_root[1] = (0x80000000ULL >> 2) | PTE_V | PTE_RWX | PTE_AD;
            serial_root[2] = serial_root[1];
            CSRW(CSR_SATP, (8ULL << 60) | ((uintptr_t)serial_root >> 12));
            sfence_vma();
            goto_priv(PRIV_HS);
            TEST_SETUP_EXCEPT();
            asm volatile(BE_ASM_BEGIN
                "ld t1,0(%[p])\nsfence.vma\nld t2,0(%[p])\nadd %[out],t1,t2\n"
                BE_ASM_END : [out] "=&r"(result)
                : [p] "r"((uintptr_t)serial_mem - 0x40000000ULL)
                : "t1", "t2", "memory");
            goto_priv(PRIV_M);
            CSRW(CSR_SATP, satp);
            sfence_vma();
            TEST_ASSERT("SFENCE orders a translated old load and the following load",
                !excpt.triggered && result == 146);
        } else if (row == 731) {
            asm volatile(BE_ASM_BEGIN BE_DELAY "csrr %[out],mscratch\n" BE_ASM_END
                : [out] "=r"(result) :: "t5", "t6", "memory");
            TEST_ASSERT("read-only mscratch waits for older work and keeps its value",
                !excpt.triggered && result == csr);
        } else if (row == 728) {
            uint64_t status = CSRR(CSR_MSTATUS), epc = CSRR(CSR_MEPC);
            CSRS(CSR_MSTATUS,3ULL << 11);
            asm volatile(BE_ASM_BEGIN "lla t0,1f\ncsrw mepc,t0\n" BE_DELAY
                "mret\nsd zero,0(%0)\n1: li t0,19\nsd t0,8(%0)\n" BE_ASM_END
                :: "r"(serial_mem) : "t0","t5","t6","memory");
            CSRW(CSR_MEPC,epc);
            CSRW(CSR_MSTATUS,status);
            TEST_ASSERT("MRET kills fallthrough store and reaches the target once",
                !excpt.triggered && serial_mem[0] == 73 && serial_mem[1] == 19);
        } else if (row == 727) {
            serial_code[0] = 0x01100513;
            serial_code[1] = 0x00008067;
            asm volatile("fence.i" ::: "memory");
            TEST_ASSERT("initial code executes", ((uint64_t (*)(void))serial_code)() == 17);
            serial_code[0] = 0x04900513;
            asm volatile("fence.i" ::: "memory");
            TEST_ASSERT("FENCE.I exposes the preceding code store", ((uint64_t (*)(void))serial_code)() == 73);
        } else {
            asm volatile(BE_ASM_BEGIN BE_DELAY
                "li t0,723\nbeq %[row],t0,1f\nli t0,724\nbeq %[row],t0,1f\n"
                "li t0,725\nbeq %[row],t0,2f\nli t0,726\nbeq %[row],t0,3f\n"
                "csrr %[out],mscratch\nj 9f\n"
                "1: li t0,97\ncsrw mscratch,t0\nld t1,0(%[p])\ncsrr %[out],mscratch\nadd %[out],%[out],t1\nj 9f\n"
                "2: addi t1,zero,17\ncsrw satp,%[satp]\naddi %[out],t1,19\nj 9f\n"
                "3: ld t1,0(%[p])\nsfence.vma\nld t2,0(%[p])\nadd %[out],t1,t2\n9:\n" BE_ASM_END
                : [out] "=&r"(result)
                : [row] "r"((uint64_t)row), [p] "r"(serial_mem), [satp] "r"(satp)
                : "t0","t1","t2","t5","t6","memory");
            TEST_ASSERT("serial instruction preserves preceding and following values",
                !excpt.triggered && result == (row <= 724 ? 170 : row == 725 ? 36 : row == 726 ? 146 : csr));
        }
        CSRW(CSR_MSCRATCH,csr);
        TEST_ASSERT("IT verified waitForward/blockBack, retirement and side-effect order", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_serial_order");
}

/* Train the branch without training the store/load pair as dependent.
 * Only the last iteration aliases, so the younger serial instruction can
 * enter dispatch before the older store resolves and redirects the load. */
static void serial_memory_redirect(unsigned row, bool alias, uint64_t satp)
{
    volatile uint64_t *store = serial_mem + (alias ? 0 : 2);
#define SERIAL_MEMORY_REDIRECT(op) asm volatile(BE_ASM_BEGIN \
    "mv t1,%[store]\nli t6,3\nli t3,1\n" \
    ".rept 6\nslli t2,t1,1\nadd t2,t2,t1\ndivu t1,t2,t6\n.endr\n" \
    "sd t3,0(t1)\nld t0,0(%[p])\nbnez t0,9f\n" \
    op "li t1,1\nsd t1,8(%[p])\n9:\n" BE_ASM_END \
    :: [store] "r"(store), [p] "r"(serial_mem), [satp] "r"(satp) \
    : "t0","t1","t2","t3","t6","memory")
    if (row == 748) {
        SERIAL_MEMORY_REDIRECT("li t1,0\nli t2,3\nvsetvl zero,t1,t2\n");
    } else {
        SERIAL_MEMORY_REDIRECT("sfence.vma\n");
    }
#undef SERIAL_MEMORY_REDIRECT
}

bool ai_micro_backend_it_serial_redirect(void)
{
    TEST_START();
    be_it_enable();
    const unsigned rows[] = {729,730,747,748,864,865,872};
    uint64_t csr = CSRR(CSR_MSCRATCH), satp = CSRR(CSR_SATP);
    for (unsigned k = 0; k < sizeof(rows)/sizeof(rows[0]); ++k) {
        unsigned row = rows[k];
        backend_it_control.target_addr = (uintptr_t)serial_mem;
        if (!be_it_begin(row,0)) return false;
        for (unsigned train = 0; train < 33; ++train) {
            bool test = train == 32;
            unsigned mem = row == 730 || row == 748 || row == 865;
            serial_mem[0] = 0;
            serial_mem[1] = 0;
            CSRW(CSR_MSCRATCH,0x1357);
            asm volatile("vsetivli zero,4,e32,m1,tu,mu" ::: "memory");
            TEST_SETUP_EXCEPT();
            if (row == 748 || row == 865) {
                serial_memory_redirect(row, test, satp);
            } else asm volatile(BE_ASM_BEGIN
                "beqz %[mem],2f\nmv t1,%[p]\nli t6,3\n"
                ".rept 6\nslli t2,t1,1\nadd t2,t2,t1\ndivu t1,t2,t6\n.endr\n"
                "sd %[test],0(t1)\nld t0,0(%[p])\nj 3f\n"
                "2:\n" BE_DELAY "sltu t5,t5,zero\nor t0,t5,%[test]\n"
                "3: bnez t0,9f\nli t0,729\nbeq %[row],t0,4f\n"
                "li t0,730\nbeq %[row],t0,5f\nli t0,864\nbeq %[row],t0,6f\n"
                "li t0,865\nbeq %[row],t0,6f\nli t0,872\nbeq %[row],t0,7f\n"
                "li t1,0\nli t2,3\nvsetvl zero,t1,t2\nj 8f\n"
                "4: li t1,0x2468\ncsrw mscratch,t1\nj 8f\n"
                "5: csrw satp,%[satp]\nj 8f\n6: sfence.vma\nj 8f\n7: fence.i\n"
                "8: li t1,1\nsd t1,8(%[p])\n9:\n" BE_ASM_END
                :: [p] "r"(serial_mem),[test] "r"((uint64_t)test), [row] "r"((uint64_t)row),
                   [mem] "r"((uint64_t)mem), [satp] "r"(satp)
                : "t0","t1","t2","t5","t6","memory");
            TEST_ASSERT("killed serial instruction has no visible CSR/vector/marker effect",
                !excpt.triggered && serial_mem[1] == (test ? 0 : 1) &&
                CSRR(CSR_MSCRATCH) == (row == 729 && !test ? 0x2468 : 0x1357) &&
                ((row != 747 && row != 748) || CSRR(CSR_VL) == (test ? 4 : 0)));
        }
        TEST_ASSERT("IT verified redirect age and absence of killed CSR/fence side effects", be_it_finish());
    }
    CSRW(CSR_MSCRATCH,csr);
    TEST_END("ai_micro_backend_it_serial_redirect");
}

bool ai_micro_backend_it_vsetvl_transition(void)
{
    TEST_START();
    be_it_enable();
    uint64_t bytes = CSRR(CSR_VLENB);
    TEST_ASSERT("vector buffer is large enough", bytes >= 16 && bytes <= 256);
    CSRW(CSR_VSTART,0);
    for (unsigned row = 739; row <= 746; ++row) {
        uint64_t old_type = row == 739 ? 24 : row == 742 ? (1ULL << 63) :
            row == 741 || row == 744 || row == 746 ? 0 : 2;
        uint64_t new_type = row == 740 || row == 742 ? 6 :
            row == 743 ? (1ULL << 63) : row == 744 || row == 746 ? 2 : 0;
        uint64_t request = row == 741 ? 0 : row == 746 ? 4 * bytes : bytes;
        unsigned count = new_type == 6 ? bytes/4 : row == 741 || row == 743 ? 0 : request;
        uint64_t prefetch = 0;
        if (row == 744) {
            /* Stop lookahead before it can predict the later indirect call. */
            prefetch = CSRR(0x5c1);
            CSRW(0x5c1,prefetch & ~1ULL);
        }
        for (unsigned i = 0; i < sizeof(serial_vec); ++i) serial_vec[i] = 0xa5;
        /* Fetch vsetvl in an aligned block independent of the cold vector line. */
        if (row == 744) {
            for (unsigned i = 0; i < 16; ++i) serial_code[i] = 0x00000013;
            serial_code[0] = 0x80b57557;  /* vsetvl a0,a0,a1 */
            serial_code[16] = 0x5e04b457; /* vmv.v.i v8,9 */
            serial_code[17] = 0x02060427; /* vse8.v v8,(a2) */
            serial_code[18] = 0x00008067; /* ret */
        }
        backend_it_control.target_pc = row == 744 ? (uintptr_t)&serial_code[0] : 0;
        backend_it_control.target_addr = row == 744 ? (uintptr_t)&serial_code[16] : 0;
        backend_it_control.auxiliary = 0;
        if (!be_it_begin(row,0)) return false;
        asm volatile("vsetvl zero,%0,%1" :: "r"(bytes), "r"(old_type) : "memory");
        uint64_t observed_old_type = CSRR(CSR_VTYPE), observed_old_vl = CSRR(CSR_VL);
        be_it_trap_install();
        uint64_t vl, pc = 0;
        if (row == 744) {
            be_it_traps.resume = (uintptr_t)&serial_code[18];
            asm volatile("fence.i" ::: "memory");
            vl = ((uint64_t (*)(uint64_t,uint64_t,uint8_t *))serial_code)
                (request,new_type,serial_vec);
            CSRW(0x5c1,prefetch);
        } else asm volatile(BE_ASM_BEGIN "lla t0,9f\nsd t0,%[resume]\n"
            "lla %[pc],1f\n"
            "vsetvl %[vl],%[avl],%[type]\n1: vmv.v.i v8,9\nvse8.v v8,(%[p])\n9:\n" BE_ASM_END
            : [vl] "=&r"(vl), [pc] "=&r"(pc), [resume] "=m"(be_it_traps.resume)
            : [avl] "r"(request), [type] "r"(new_type), [p] "r"(serial_vec)
            : "t0","memory");
        uint64_t actual_type = CSRR(CSR_VTYPE), actual_vl = CSRR(CSR_VL);
        be_it_trap_restore();
        bool ok = row == 743 ? be_it_traps.count == 1 &&
            be_it_traps.event[0].cause == CAUSE_ILLEGAL_INSTRUCTION &&
            be_it_traps.event[0].epc == pc && actual_type == (1ULL << 63) && vl == 0 :
            be_it_traps.count == 0 && vl == count && actual_type == new_type;
        ok &= observed_old_type == old_type && actual_vl == count;
        if (row == 741) ok &= observed_old_vl == bytes && actual_type == observed_old_type;
        /* Cover the largest legal LMUL group and an additional guard line. */
        for (unsigned i = 0; i < 8 * bytes + 64; ++i)
            ok &= serial_vec[i] == (i < count ? 9 : 0xa5);
        TEST_ASSERT("vsetvl refetch uses only the new VL/vtype/vill and exact store extent", ok, "row=%u", row);
        printf("BE_IT_SOFTWARE_RESULT row=%u status=%s\n", row, ok ? "PASS" : "FAIL");
        TEST_ASSERT("IT verified cache hit/miss, vsetvlInflight and decode uop count", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_vsetvl_transition");
}

bool ai_micro_backend_it_custom_control(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    TEST_ASSERT("VModule sources initially clear", be_vm_clear());
    for (unsigned row = 734; row <= 737; ++row) {
        backend_it_control.target_pc = 0;
        if (row == 736 && !be_it_vmodule_ecc_prepare()) return false;
        if (!be_it_begin(row,0)) return false;
        uint64_t saved, got;
#define CONTROL(csr, mask) do { \
    saved = CSRR(csr); \
    for (unsigned k=0;k<8;++k) { \
        uint64_t value = (k & 1) ? saved | (mask) : saved & ~(mask); \
        CSRW(csr,value); got = CSRR(csr); \
        TEST_ASSERT("custom control accepts the selected enable bit", (got & (mask)) == (value & (mask))); \
        uint64_t sum; \
        asm volatile(BE_ASM_BEGIN "li t0,0\nli t1,64\n1: lui t2,0x12345\naddi t2,t2,0x678\n" \
            "add t0,t0,t2\naddi t1,t1,-1\nbnez t1,1b\nmv %0,t0\n" BE_ASM_END \
            : "=r"(sum) :: "t0","t1","t2"); \
        TEST_ASSERT("control transitions preserve the fetched instruction stream", sum == 64ULL*0x12345678); \
    } \
    CSRW(csr,saved); \
} while (0)
        if (row == 734) CONTROL(0x5c0, 0x3fULL);
        if (row == 735) {
            saved = CSRR(0x5c1);
            for (unsigned k = 0; k < 8; ++k) {
                CSRW(0x5c1, saved | 1ULL);
                asm volatile("fence.i" ::: "memory");
                uint64_t sum;
                asm volatile(BE_ASM_BEGIN
                    ".balign 64\nli t0,0\n.rept 16\naddi t0,t0,1\n.endr\n"
                    "csrw 0x5c1,%[disabled]\n"
                    ".rept 512\naddi t0,t0,1\n.endr\nmv %[out],t0\n" BE_ASM_END
                    : [out] "=r"(sum) : [disabled] "r"(saved & ~1ULL)
                    : "t0", "memory");
                TEST_ASSERT("prefetch disable preserves the cold instruction stream",
                    sum == 528 && !(CSRR(0x5c1) & 1ULL));
            }
            CSRW(0x5c1, saved);
        }
        if (row == 736) {
            CONTROL(0x9e0, 1ULL);
            TEST_ASSERT("parity enable transition permits a real IAF/RNMI43 and clean refetch",be_it_vmodule_ecc_probe());
        }
        if (row == 737) CONTROL(0x5c4, 1ULL);
#undef CONTROL
        TEST_ASSERT("IT verified control visibility and outstanding transaction behavior", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_custom_control");
}

bool ai_micro_backend_it_fencei_refill(void)
{
    TEST_START();
    be_it_enable();
    for (unsigned row = 867; row <= 871; ++row) {
        uint32_t *probe = serial_code + (row - 867) * 512;
        unsigned slot = row == 870 ? 31 : 16;
        for (unsigned n = 0; n < 64; ++n) probe[n] = 0x00000013;
        /* The unresolved taken branch permits a real wrong-path fetch of the
         * next line. Returning uses the already fetched first line. */
        probe[0] = 0x0080006f;  /* jal zero,+8 */
        probe[1] = 0x00008067;  /* ret */
        probe[2] = 0xfff00f13;  /* li t5,-1 */
        probe[3] = 0x00300f93;  /* li t6,3 */
        probe[4] = probe[5] = 0x03ff5f33; /* divu t5,t5,t6 */
        probe[6] = 0x000f3f33; /* sltu t5,t5,zero */
        probe[7] = 0xfe0f04e3; /* beq t5,zero,-24 */
        probe[slot] = 0x00150513; /* addi a0,a0,1 */
        probe[slot+1] = 0x00250513;
        probe[slot+2] = 0x00008067;
        asm volatile("fence.i" ::: "memory");
        uint64_t (*fn)(uint64_t) = (void *)&probe[slot];
        backend_it_control.target_pc = (uintptr_t)fn;
        backend_it_control.target_addr = (uintptr_t)fn;
        unsigned scenario = row == 867 ? BE_IT_SC_REFILL_FENCEI_EARLY :
            row == 868 ? BE_IT_SC_REFILL_FENCEI_OVERLAP :
            row == 869 ? BE_IT_SC_REFILL_FENCEI_LATE :
            row == 870 ? BE_IT_SC_REFILL_FENCEI_CROSSLINE : BE_IT_SC_NONE;
        if (!be_it_begin_scenario(row,scenario,0)) return false;
        TEST_SETUP_EXCEPT();
        /* Keep the return, stores and fence in the fetched call-site line so
         * an old held response cannot block fetching the releasing fence. */
        register uint64_t old_result asm("a0") = 10;
        asm volatile(BE_ASM_BEGIN ".balign 64\njalr ra,0(%[probe])\n"
            "li t0,0x01150513\nsw t0,0(%[code])\n"
            "li t0,0x01350513\nsw t0,4(%[code])\nfence.i\n" BE_ASM_END
            : "+r"(old_result) : [probe] "r"(probe), [code] "r"(fn)
            : "ra","t0","t5","t6","memory");
        TEST_ASSERT("speculative old target has no architectural side effect",
            old_result == 10 && !excpt.triggered);
        TEST_ASSERT("both lines execute only the final code after FENCE.I", fn(10) == 46 && !excpt.triggered);
        TEST_ASSERT("IT checked old source IDs, refill/fence timing and no stale SRAM installation", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_fencei_refill");
}
