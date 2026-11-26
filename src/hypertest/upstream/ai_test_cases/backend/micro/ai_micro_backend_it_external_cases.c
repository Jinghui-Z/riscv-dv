#include "backend_it_vmodule.h"

/* VModule owns MEI/debug request injection. The monitor observes real queue
 * states and the debug agent owns trigger setup/resume. Software never forges
 * intrBitSet, a safety signal, or a ROB index. Reset still needs its TB agent.
 */
bool be_it_pressure(unsigned kind,unsigned rounds);
bool be_it_redirect_probe(unsigned row);
extern bool ai_micro_hs_pbmt_nc_mixed_cjr_cebreak_call_ret_fetch_manual_corner(void);
static volatile uint64_t external_data[256][8] __attribute__((aligned(4096)));
static pte_t external_root[512] __attribute__((aligned(4096)));

bool ai_micro_backend_it_interrupt_commit(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    uint64_t sscratch = CSRR(CSR_SSCRATCH);
    for (unsigned row = 691; row <= 699; ++row) {
        be_it_trap_install();
        bool source_ok = be_vm_clear();
        if (row >= 692) {
            /* Commit-agent ABI: scenario 0x4100 + (row - 690), iteration
             * is the number of independent attempts inside this window.
             * target_pc/end delimit the actual stimulus, not its helpers.
             * The agent delays a real response for target_addr; it never
             * changes a ROB bit. Keep these selectors in sync with the
             * shared scenario enum when integrating the independent agent.
             */
            unsigned scenario = 0x4100U + row - 690;
            unsigned attempts = row == 692 || row >= 698 ? 64 : 1;
            bool serial = row == 696 || row == 697;
            bool no_irq = row == 693 || row == 697;
            bool full_enq = row == 698;
            uintptr_t load_pc, fence_pc, end_pc;
            if (full_enq) {
                asm volatile(BE_ASM_BEGIN
                    "lla %0,.Lbe_commit_enq_load\nlla %1,.Lbe_commit_enq_end\n"
                    BE_ASM_END : "=r"(load_pc), "=r"(end_pc));
                fence_pc = 0;
            } else {
                asm volatile(BE_ASM_BEGIN
                    "lla %0,.Lbe_commit_load\nlla %1,.Lbe_commit_fence\n"
                    "lla %2,.Lbe_commit_end\n" BE_ASM_END
                    : "=r"(load_pc), "=r"(fence_pc), "=r"(end_pc));
            }
            backend_it_control.target_pc = serial ? fence_pc : load_pc;
            backend_it_control.target_addr = (uintptr_t)external_data;
            backend_it_control.auxiliary = end_pc;
            /* Seed before arming the gate: a cold seed store may itself
             * allocate the line and must not become the held transaction.
             */
            external_data[0][0] = 73;
            if (full_enq) external_data[2][0] = 1;
            asm volatile("cbo.flush 0(%0)\nfence rw,rw"
                :: "r"(external_data) : "memory");
            if (!be_it_prepare_scenario(row, scenario, attempts) ||
                !(backend_it_control.capabilities & BE_IT_CAP_EVENT)) {
                be_it_trap_restore();
                return false;
            }
            be_it_activate();
            bool architectural_ok = source_ok;
            unsigned irq_count = 0;
            for (unsigned attempt = 0; attempt < attempts; ++attempt) {
                CSRC(CSR_MSTATUS, MSTATUS_MIE);
                be_it_traps.count = 0;
                be_it_traps.event[0].cause = UINT64_MAX;
                be_it_traps.event[0].epc = 0;
                external_data[1][0] = 0;
                external_data[1][1] = 0x123456789abcdef0ULL;
                /* Force a real demand transaction on every attempt. The
                 * store/fence variant waits for the
                 * same cold line through the real store-buffer path.
                 */
                asm volatile("cbo.flush 0(%0)\nfence rw,rw"
                    :: "r"(external_data) : "memory");
                if (full_enq) architectural_ok &= external_data[2][0] == 1;
                CSRW(CSR_MIE, 1ULL << 11);
                CSRS(CSR_MSTATUS, MSTATUS_MIE);
                uint64_t got, sum;
                if (full_enq) {
                    /* Adjacent ADDIs compress into one ROB entry on this
                     * core. Alternating real hot loads and ALUs gives each
                     * instruction its own entry and keeps enqueue active.
                     */
                    asm volatile(BE_ASM_BEGIN
                        "li t2,0\nli t3,0\nli t4,0\nli t5,0\n"
                        "mv t0,%[pad]\n1: beqz t0,2f\naddi t0,t0,-1\nj 1b\n2:\n"
                        ".Lbe_commit_enq_load: ld t0,0(%[p])\n"
                        ".rept 64\n"
                        "ld t1,128(%[p])\nadd t2,t2,t1\n"
                        "ld t1,128(%[p])\nadd t3,t3,t1\n"
                        "ld t1,128(%[p])\nadd t4,t4,t1\n"
                        "ld t1,128(%[p])\nadd t5,t5,t1\n.endr\n"
                        "sd t0,64(%[p])\nmv %[got],t0\n"
                        "add t2,t2,t3\nadd t4,t4,t5\nadd %[sum],t2,t4\n"
                        ".Lbe_commit_enq_end:\n" BE_ASM_END
                        : [got] "=&r"(got), [sum] "=&r"(sum)
                        : [p] "r"(external_data), [pad] "r"((uint64_t)(attempt & 7))
                        : "t0","t1","t2","t3","t4","t5","memory");
                } else asm volatile(BE_ASM_BEGIN
                    "li t1,0\nli t2,0\nli t3,0\nli t4,0\nli t5,0\nli t6,0\n"
                    /* Vary ROB bank alignment without changing the target
                     * PC. All six ALU chains below are independent of ld.
                     */
                    "mv t0,%[pad]\n1: beqz t0,2f\naddi t0,t0,-1\nj 1b\n2:\n"
                    "beqz %[serial],.Lbe_commit_load\n"
                    "li t0,83\nsd t0,0(%[p])\n"
                    ".Lbe_commit_fence: fence rw,rw\n"
                    "j .Lbe_commit_alu\n"
                    ".Lbe_commit_load: ld t0,0(%[p])\n"
                    ".Lbe_commit_alu:\n.rept 32\n"
                    "addi t1,t1,1\naddi t2,t2,1\naddi t3,t3,1\n"
                    "addi t4,t4,1\naddi t5,t5,1\naddi t6,t6,1\n.endr\n"
                    "sd t0,64(%[p])\nmv %[got],t0\n"
                    "add t1,t1,t2\nadd t3,t3,t4\nadd t5,t5,t6\n"
                    "add t1,t1,t3\nadd %[sum],t1,t5\n"
                    ".Lbe_commit_end:\n" BE_ASM_END
                    : [got] "=&r"(got), [sum] "=&r"(sum)
                    : [p] "r"(external_data), [serial] "r"((uint64_t)serial),
                      [pad] "r"((uint64_t)(attempt & 7))
                    : "t0","t1","t2","t3","t4","t5","t6","memory");
                bool trap_ok = no_irq || be_vm_wait_traps(1);
                /* MIE stays enabled until the agent has cleared the real
                 * source and released the load (693) or fence (697).
                 * A missed suppression window remains an architectural failure.
                 */
                if (no_irq)
                    for (unsigned spin = 0; spin < 64; ++spin)
                        asm volatile("nop" ::: "memory");
                CSRC(CSR_MSTATUS, MSTATUS_MIE);
                bool pending_clear = be_vm_pending(false);
                uint64_t count = be_it_traps.count;
                bool outcome = no_irq ? count == 0 : count == 1 &&
                    be_it_traps.event[0].cause == ((1ULL << 63) | 11);
                bool data_ok = got == (serial ? 83 : 73) && sum == (full_enq ? 256 : 192) &&
                    external_data[1][0] == got &&
                    external_data[1][1] == 0x123456789abcdef0ULL &&
                    external_data[0][0] == (serial ? 83 : 73) &&
                    (!full_enq || external_data[2][0] == 1);
                architectural_ok &= trap_ok && pending_clear && outcome && data_ok;
                irq_count += count;
                if (!(trap_ok && pending_clear && outcome && data_ok)) {
                    printf("\nBACKEND_IT_COMMIT_ORACLE row=%u attempt=%u traps=%lu cause=%lx epc=%lx data=%u pending_clear=%u\n",
                        row, attempt, count, be_it_traps.event[0].cause,
                        be_it_traps.event[0].epc, data_ok, pending_clear);
                    break;
                }
            }
            architectural_ok &= irq_count == (no_irq ? 0 : attempts);
            /* Finish before software cleanup so cleanup cannot satisfy a
             * source-clear witness that the event agent failed to produce.
             */
            bool window_ok = be_it_finish();
            source_ok = be_vm_clear();
            be_it_trap_restore();
            TEST_ASSERT("commit attempts preserve data and have the exact interrupt count",
                architectural_ok && source_ok);
            TEST_ASSERT("commit agent proved the real window and matched event identity", window_ok);
            continue;
        }
        unsigned delay = 512;
        bool cleared = row == 693;
        external_data[0][0] = 73;
        external_data[1][0] = 0;
        backend_it_control.target_pc = 0;
        backend_it_control.target_addr = (uintptr_t)external_data;
        backend_it_control.auxiliary = delay;
        if (!be_it_begin(row,0)) { be_it_trap_restore(); return false; }
        CSRW(CSR_MIE,1ULL << 11);
        CSRS(CSR_MSTATUS,MSTATUS_MIE);
        be_vm_mei(delay, cleared);
        uint64_t got;
        asm volatile(BE_ASM_BEGIN
            "li t0,695\nbeq %[row],t0,1f\nli t0,696\nbeq %[row],t0,2f\n"
            "li t0,697\nbeq %[row],t0,2f\nj 3f\n"
            "1: ld t1,0(%[p])\nsd t1,64(%[p])\nj 3f\n"
            "2:\n" BE_DELAY "csrw sscratch,t5\nfence rw,rw\nvsetivli zero,4,e32,m1,tu,mu\n"
            "3: li t0,0\n.rept 256\naddi t0,t0,1\naddi t1,t0,7\nadd t2,t0,t1\n.endr\n"
            "mv %[out],t0\n" BE_ASM_END
            : [out] "=r"(got) : [row] "r"((uint64_t)row), [p] "r"(external_data)
            : "t0","t1","t2","t5","t6","memory");
        if (!cleared) source_ok &= be_vm_wait_traps(1);
        else {
            /* Keep interrupts enabled through the complete bounded pulse
             * window. The monitor must still prove safe-point suppression. */
            for (unsigned spin=0; spin<4096; ++spin) asm volatile("nop" ::: "memory");
        }
        CSRC(CSR_MSTATUS,MSTATUS_MIE);
        source_ok &= be_vm_pending(false);
        source_ok &= be_vm_clear();
        be_it_trap_restore();
        TEST_ASSERT("sampled/cleared interrupt has the specified unique architectural outcome",
            source_ok && got == 256 && (cleared ? be_it_traps.count == 0 :
                be_it_traps.count == 1 && be_it_traps.event[0].cause == ((1ULL<<63)|11)));
        TEST_ASSERT("IT checked sampling cycle, interrupt_safe, commit width and ROB wrap", be_it_finish());
    }
    CSRW(CSR_SSCRATCH,sscratch);
    TEST_END("ai_micro_backend_it_interrupt_commit");
}

/* The monitor descriptors refer to unique assembler labels in this loop. */
__attribute__((optimize("no-peel-loops", "no-unswitch-loops", "no-split-loops")))
bool ai_micro_backend_it_wfi_wakeup(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    for (unsigned row = 750; row <= 764; ++row) {
        bool early_irq = row == 753 || row == 758 || row == 760 || row == 763;
        bool cold_stores = row == 752 || row == 753 || row == 754 || row == 759;
        unsigned store_end = 10;
        uint64_t prefetch_control = CSRR(0x5c1);
        if (row == 760 || row == 761) {
            /* Unissued prefetch MSHRs stay valid throughout WFI although
             * they are safe. Disable new prefetch and flush old requests
             * so this row observes frontend buffers without any MSHR.
             */
            CSRW(0x5c1, prefetch_control & ~1ULL);
            asm volatile("fence.i" ::: "memory");
        }
        be_it_trap_install();
        bool source_ok = be_vm_clear();
        external_data[0][0] = 73;
        external_data[1][0] = 0;
        if (row == 763) external_data[2][0] = 73;
        if (cold_stores) {
            for (unsigned i = 2; i < store_end; ++i) {
                external_data[i][0] = 0;
                asm volatile("cbo.flush 0(%0)" :: "r"(&external_data[i][0]) : "memory");
            }
            asm volatile("fence rw,rw" ::: "memory");
        }
        uintptr_t wfi_pc;
        if (row == 753) {
            asm volatile("lla %0,be_it_external_l2_wfi" : "=r"(wfi_pc));
        } else if (row == 763) {
            asm volatile("lla %0,be_it_external_store_wfi" : "=r"(wfi_pc));
        } else if (row == 760) {
            asm volatile("lla %0,be_it_external_demand_wfi" : "=r"(wfi_pc));
        } else if (row == 762) {
            asm volatile("lla %0,be_it_external_old_load_wfi" : "=r"(wfi_pc));
            asm volatile("cbo.flush 0(%0)\nfence rw,rw"
                :: "r"(external_data) : "memory");
        } else {
            asm volatile("lla %0,be_it_external_wfi" : "=r"(wfi_pc));
        }
        backend_it_control.target_pc = wfi_pc;
        backend_it_control.target_addr = row == 760 ? (wfi_pc & ~(uintptr_t)63) + 128 :
            (uintptr_t)&external_data[cold_stores ? 2 : 0][0];
        backend_it_control.auxiliary = early_irq ? 0 : 8192;
        unsigned scenario = row == 756 ? BE_IT_SC_DEBUG_WFI_HALTREQ :
            row == 757 ? BE_IT_SC_DEBUG_WFI_TRIGGER : BE_IT_SC_NONE;
        if (!be_it_begin_scenario(row,scenario,0)) {
            be_it_trap_restore();
            if (row == 760 || row == 761) CSRW(0x5c1, prefetch_control);
            return false;
        }
        be_vm_watchdog(2000000);
        bool debug = row == 756 || row == 757;
        unsigned episodes = row == 753 ? 64 : row == 759 ? 2 : 1;
        bool repeated_traps_ok = true;
        for (unsigned episode = 0; episode < episodes; ++episode) {
        if (row == 753) {
            /* Keep each attempt's trap oracle within the four-record log.
             * The preceding attempt has returned with MIE off and its
             * physical source cleared before this count is reused.
             */
            be_it_traps.count = 0;
            be_it_traps.event[0].cause = UINT64_MAX;
            if (episode != 0) {
                for (unsigned i = 2; i < store_end; ++i) {
                    external_data[i][0] = 0;
                    asm volatile("cbo.flush 0(%0)" :: "r"(&external_data[i][0]) : "memory");
                }
                asm volatile("fence rw,rw" ::: "memory");
            }
        }
        unsigned expected_traps = row == 753 ? 1 : episode + 1;
        be_it_traps.defer_irq_clear = !debug;
        CSRW(CSR_MIE,1ULL << 11);
        if (!debug) {
            be_vm_mei(early_irq ? 0 : 8192, false);
            if (early_irq) source_ok &= be_vm_pending(true);
        }
        if (!debug && !early_irq) CSRS(CSR_MSTATUS,MSTATUS_MIE);
        /* The debug agent configures action=1 before row 757 is acknowledged.
         * A forced debug request must not stand in for an execution trigger.
         * An early IRQ keeps its physical level and local enable until after
         * WFI, but disables global MIE on return to prevent repeated traps. */
        if (row == 753) {
            /* Eight distinct lines exceed the SBuffer eviction threshold.
             * Age their real transactions with MIE masked, then observe
             * WFI at successive completion phases. The monitor still
             * requires busy -> idle -> IRQ within the original bounds.
             */
            asm volatile(BE_ASM_BEGIN
                "ld t0,0(%[p])\n"
                "sd t0,128(%[p])\nsd t0,192(%[p])\nsd t0,256(%[p])\n"
                "sd t0,320(%[p])\nsd t0,384(%[p])\nsd t0,448(%[p])\n"
                "sd t0,512(%[p])\nsd t0,576(%[p])\n"
                "mv t1,%[phase]\nli t2,0\nbeqz t1,2f\n"
                "1: addi t2,t2,1\naddi t2,t2,1\naddi t2,t2,1\naddi t2,t2,1\n"
                "addi t1,t1,-1\nbnez t1,1b\n2:\n"
                "csrs mstatus,%[mie]\n"
                ".global be_it_external_l2_wfi\nbe_it_external_l2_wfi: wfi\n"
                "addi t0,t0,1\n"
                "ld t1,64(%[p])\naddi t1,t1,1\nsd t1,64(%[p])\n"
                BE_ASM_END :: [p] "r"(external_data), [mie] "r"((uint64_t)MSTATUS_MIE),
                    [phase] "r"((uint64_t)episode)
                : "t0", "t1", "t2", "memory");
        } else if (row == 763) {
            /* All heads after enabling MEI are real stores until WFI.
             * A CSR after the stores drains the LQ/SQ before the WFI
             * request, leaving only SBuffer/MissQueue work to observe.
             */
            asm volatile(BE_ASM_BEGIN
                "ld t0,0(%[p])\ncsrs mstatus,%[mie]\n"
                ".rept 32\nsd t0,128(%[p])\n.endr\n"
                ".global be_it_external_store_wfi\nbe_it_external_store_wfi: wfi\n"
                "addi t0,t0,1\n"
                "ld t1,64(%[p])\naddi t1,t1,1\nsd t1,64(%[p])\n"
                BE_ASM_END :: [p] "r"(external_data), [mie] "r"((uint64_t)MSTATUS_MIE)
                : "t0", "t1", "memory");
        } else if (row == 760) {
            /* The first post-WFI line must use a demand fetch, not a
             * prefetch-filled line. CSRS/WFI end the preceding 64B line.
             */
            asm volatile(BE_ASM_BEGIN
                "ld t0,0(%[p])\nsd t0,128(%[p])\n"
                /* Keep an alignment relocation despite BE_ASM_BEGIN's
                 * norelax: earlier C calls can shrink during linking. */
                ".option push\n.option relax\n.balign 64\n.option pop\n"
                ".rept 14\nnop\n.endr\n"
                "csrs mstatus,%[mie]\n"
                ".global be_it_external_demand_wfi\n"
                "be_it_external_demand_wfi: wfi\n"
                "addi t0,t0,1\n"
                "ld t1,64(%[p])\naddi t1,t1,1\nsd t1,64(%[p])\n"
                ".rept 64\naddi t0,t0,1\n.endr\n" BE_ASM_END
                :: [p] "r"(external_data), [mie] "r"((uint64_t)MSTATUS_MIE)
                : "t0", "t1", "memory");
        } else if (row == 762) {
            /* Keep the demand load and waiting WFI in one fetch line.
             * The other rows' store stream lets this load finish before
             * WFI reaches dispatch and cannot witness an older load.
             */
            asm volatile(BE_ASM_BEGIN
                ".option push\n.option relax\n.balign 64\n.option pop\n"
                "ld t0,0(%[p])\n"
                ".global be_it_external_old_load_wfi\n"
                "be_it_external_old_load_wfi: wfi\n"
                "addi t1,t0,0\nsd t1,128(%[p])\n"
                "ld t1,64(%[p])\naddi t1,t1,1\nsd t1,64(%[p])\n"
                ".rept 64\naddi t0,t0,1\n.endr\n" BE_ASM_END
                :: [p] "r"(external_data) : "t0", "t1", "memory");
        } else asm volatile(BE_ASM_BEGIN
            "ld t0,0(%[p])\n"
            "beqz %[cold],3f\n"
            "sd t0,128(%[p])\nsd t0,192(%[p])\nsd t0,256(%[p])\n"
            "sd t0,320(%[p])\nsd t0,384(%[p])\nsd t0,448(%[p])\n"
            "sd t0,512(%[p])\nsd t0,576(%[p])\nj 4f\n"
            "3:\n.rept 32\nsd t0,128(%[p])\n.endr\n4:\n"
            /* Pending-before-WFI rows keep MEI globally masked until this
             * point. Injection after wfi_req loses the overlap because
             * wfiEvent reaches the ROB before intrBitSetReg does.
             */
            ".option push\n.option relax\n.balign 64\n.option pop\n"
            "beqz %[late_mie],1f\ncsrs mstatus,%[late_mie]\n1:\n"
            ".global be_it_external_wfi\nbe_it_external_wfi: wfi\n"
            /* Nanhu defers interrupt/debug acceptance at a load/store head. */
            "addi t0,t0,1\n"
            "ld t1,64(%[p])\naddi t1,t1,1\nsd t1,64(%[p])\n"
            ".rept 64\naddi t0,t0,1\n.endr\n" BE_ASM_END
            :: [p] "r"(external_data), [cold] "r"((uint64_t)(cold_stores && episode == 0)),
               [late_mie] "r"(early_irq ? (uint64_t)MSTATUS_MIE : 0)
            : "t0","t1","memory");
        if (!debug) {
            if (be_it_traps.count + 1 == expected_traps) CSRS(CSR_MSTATUS,MSTATUS_MIE);
            source_ok &= be_vm_wait_traps(expected_traps);
        }
        CSRC(CSR_MSTATUS,MSTATUS_MIE);
        source_ok &= be_vm_pending(!debug);
        source_ok &= be_vm_clear();
        if (row == 753) {
            repeated_traps_ok &= be_it_traps.count == 1 &&
                be_it_traps.event[0].cause == ((1ULL<<63)|11) &&
                external_data[1][0] == episode + 1;
            for (unsigned i = 2; i < store_end; ++i)
                repeated_traps_ok &= external_data[i][0] == 73;
        }
        }
        be_vm_watchdog(UINT32_MAX);
        be_it_trap_restore();
        bool stores_ok = external_data[2][0] == 73;
        if (cold_stores)
            for (unsigned i = 2; i < store_end; ++i) stores_ok &= external_data[i][0] == 73;
        if (row == 760 || row == 761) CSRW(0x5c1, prefetch_control);
        bool traps_ok = debug ? be_it_traps.count == 0 : row == 753 ?
            repeated_traps_ok : be_it_traps.count == episodes;
        if (!debug && row != 753)
            for (unsigned i = 0; i < episodes; ++i)
                traps_ok &= be_it_traps.event[i].cause == ((1ULL<<63)|11);
        TEST_ASSERT("Each WFI wake/resume executes its following marker exactly once",
            source_ok && external_data[1][0] == episodes && stores_ok && traps_ok,
            "row=%u marker=%lu traps=%lu episodes=%u", row, external_data[1][0],
            be_it_traps.count, episodes);
        TEST_ASSERT("IT checked real safety inputs, halt, debug dpc and source cleanup", be_it_finish());
    }
    TEST_END("ai_micro_backend_it_wfi_wakeup");
}

extern void be_it_priority_illegal(void), be_it_priority_trigger(void);
asm(".pushsection .text.be_it_priority,\"ax\",@progbits\n" BE_ASM_BEGIN
    ".balign 64\n.global be_it_priority_illegal\nbe_it_priority_illegal: csrr t0,mscratch\nret\n"
    ".balign 64\n.global be_it_priority_trigger\nbe_it_priority_trigger: addi t0,t0,1\nret\n"
    BE_ASM_END ".popsection\n");

bool ai_micro_backend_it_frontend_event_priority(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    uint64_t satp=CSRR(CSR_SATP), status=CSRR(CSR_MSTATUS), epc=CSRR(CSR_MEPC);
    uint64_t select=CSRR(CSR_TSELECT);
    CSRW(CSR_TSELECT,0);
    uint64_t data1=CSRR(CSR_TDATA1),data2=CSRR(CSR_TDATA2);
    for (unsigned i=0;i<512;++i) external_root[i]=0;
    external_root[2]=(0x80000000ULL>>2)|PTE_V|PTE_RWX|PTE_AD;
    external_root[3]=(0xc0000000ULL>>2)|PTE_V|PTE_RWX|PTE_AD;
    for (unsigned row=833;row<=836;++row) {
        uintptr_t pc = row==833 ? 0x40000000 : row==834 ? 0x0100000000000000ULL :
            row==835 ? (uintptr_t)be_it_priority_illegal : (uintptr_t)be_it_priority_trigger;
        CSRW(CSR_SATP,row==833 ? (8ULL<<60)|((uintptr_t)external_root>>12) : 0);
        sfence_vma();
        CSRW(CSR_TDATA1,0);
        if (row==836) {
            CSRW(CSR_TDATA2,pc);
            CSRW(CSR_TDATA1,0x6000000000000014ULL); /* S execute, action=breakpoint. */
        }
        be_it_trap_install();
        bool source_ok = be_vm_clear();
        be_it_traps.return_m=1;
        backend_it_control.target_pc=pc;
        if (!be_it_begin(row,0)) { be_it_trap_restore(); return false; }
        /* Observe real MEIP before the S-mode event. Global MIE is still
         * clear here, and row 834 deliberately keeps local MEIE disabled. */
        be_vm_mei(0, false);
        source_ok &= be_vm_pending(true);
        CSRW(CSR_MIE,row==834 ? 0 : 1ULL<<11);
        CSRC(CSR_MSTATUS,3ULL<<11);
        CSRS(CSR_MSTATUS,1ULL<<11);
        asm volatile(BE_ASM_BEGIN "lla t0,9f\nsd t0,%0\ncsrw mepc,%1\nmret\n9:\n" BE_ASM_END
            : "=m"(be_it_traps.resume) : "r"(pc) : "t0","memory");
        CSRC(CSR_MSTATUS,MSTATUS_MIE);
        CSRW(CSR_TDATA1,0);
        bool first_irq = row==833 || row==836;
        unsigned index=first_irq ? 1 : 0;
        uint64_t cause=row==833 ? CAUSE_IPF : row==834 ? CAUSE_IAF :
            row==835 ? CAUSE_ILLEGAL_INSTRUCTION : CAUSE_BKP;
        bool ok=source_ok && be_it_traps.count>=index+1 && be_it_traps.event[index].cause==cause &&
            be_it_traps.event[index].epc==pc &&
            (!first_irq || be_it_traps.event[0].cause==((1ULL<<63)|11));
        if (row==833 || row==834) ok &= be_it_traps.event[index].tval==pc;
        if (row==834 || row==835) {
            CSRW(CSR_MIE,1ULL<<11);
            CSRS(CSR_MSTATUS,MSTATUS_MIE);
            ok &= be_vm_wait_traps(2);
            ok &= be_it_traps.count==2 && be_it_traps.event[1].cause==((1ULL<<63)|11);
        } else ok &= be_it_traps.count==2;
        CSRC(CSR_MSTATUS,MSTATUS_MIE);
        ok &= be_vm_pending(false);
        ok &= be_vm_clear();
        be_it_trap_restore();
        CSRW(CSR_SATP,satp);
        sfence_vma();
        TEST_ASSERT("event order, cause, target PC and subsequent pending interrupt stay distinct",ok,"row=%u",row);
        TEST_ASSERT("IT checked pending timing and ROB interrupt_safe at the target",be_it_finish());
    }
    CSRW(CSR_TDATA2,data2);
    CSRW(CSR_TDATA1,data1);
    CSRW(CSR_TSELECT,select);
    CSRW(CSR_MEPC,epc);
    CSRW(CSR_MSTATUS,status);
    TEST_END("ai_micro_backend_it_frontend_event_priority");
}

/* .data is not cleared by asm/boot.S. Warm reset must preserve DRAM and must
 * NOT reload the ELF. The TB writes observed_row only after reset assertions
 * and reset-vector/initial-CSR checks have passed, then resumes this ELF.
 */
volatile struct {
    uint64_t magic,row,armed,observed_row,errors;
} backend_it_reset_cookie __attribute__((aligned(64))) = {0x4245495452535431ULL,874,0,0,0};

bool ai_micro_backend_it_reset_inflight(void)
{
    TEST_START();
    be_it_enable();
    TEST_ASSERT("warm-reset cookie has its expected format",backend_it_reset_cookie.magic==0x4245495452535431ULL);
    if (backend_it_reset_cookie.armed) {
        TEST_ASSERT("TB observed coordinated reset, reset vector, CSR and old-transaction isolation",
            backend_it_reset_cookie.observed_row==backend_it_reset_cookie.row && backend_it_reset_cookie.errors==0);
        backend_it_reset_cookie.armed=0;
        backend_it_reset_cookie.row++;
    }
    unsigned row=backend_it_reset_cookie.row;
    if (row<=882) {
        backend_it_control.target_addr=(uintptr_t)external_data;
        backend_it_control.auxiliary=(uintptr_t)&backend_it_reset_cookie;
        unsigned scenario = BE_IT_SC_RESET_FTQ + (row - 874);
        if (!be_it_begin_scenario(row,scenario,0)) return false;
        backend_it_reset_cookie.armed=1;
        asm volatile("cbo.flush 0(%0)\nfence rw,rw" :: "r"(&backend_it_reset_cookie) : "memory");
        for (unsigned round=0;round<256;++round) {
            if (row==876) {
                /* Existing PBMT=NC executable DRAM path. TB resets while its
                 * InstrUncache transaction is pending, before helper cleanup. */
                ai_micro_hs_pbmt_nc_mixed_cjr_cebreak_call_ret_fetch_manual_corner();
            } else {
                if (row==874 || row==875 || row==877 || row==882)
                    asm volatile("fence.i" ::: "memory");
                TEST_ASSERT("pre-reset stimulus remains valid until reset",be_it_pressure(
                    row==878 ? 1 : row==880 ? 5 : 6,1));
                if (row==881) TEST_ASSERT("reset window includes a real trained branch reversal",be_it_redirect_probe(674));
            }
        }
        TEST_ASSERT("a real reset must interrupt the advertised active window",false);
    }
    TEST_END("ai_micro_backend_it_reset_inflight");
}
