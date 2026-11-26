#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

static inline uint64_t ai_arch_hint_amoswap_w_raw64(uintptr_t addr, uint32_t value)
{
    uint64_t old_value;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(old_value) : "r"(addr), "r"(operand) : "memory");

    return old_value;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_hint_amoadd_d_probe_rd(uintptr_t addr, uint64_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(value)
        : "memory");

    return rd;
}

static inline __attribute__((always_inline)) uint64_t
ai_arch_hint_amoswap_w_probe_rd(uintptr_t addr, uint32_t value, uint64_t sentinel)
{
    uint64_t rd = sentinel;
    uint64_t operand = value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoswap.w %0, %2, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(rd)
        : "r"(addr), "r"(operand)
        : "memory");

    return rd;
}

static inline uintptr_t ai_arch_prefetch_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
{
#ifdef sv48
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit47 = (canonical_vaddr >> 47) & 1ULL;

    if (bit47) {
        bad_vaddr &= ~(1ULL << 48);
    } else {
        bad_vaddr |= (1ULL << 48);
    }
#else
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit38 = (canonical_vaddr >> 38) & 1ULL;

    if (bit38) {
        bad_vaddr &= ~(1ULL << 39);
    } else {
        bad_vaddr |= (1ULL << 39);
    }
#endif

    return bad_vaddr;
}

static inline pte_t *ai_arch_prefetch_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_prefetch_set_hs_leaf(enum test_page page,
    uintptr_t paddr, uint64_t flags)
{
    *ai_arch_prefetch_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uint64_t ai_arch_prefetch_ld_rd_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "+r"(value)
        : "r"(addr)
        : "memory");
    return value;
}

static inline void ai_arch_prefetch_seed_excpt_metadata(uint64_t cause,
    uint64_t tval, uint64_t epc)
{
    excpt.cause = cause;
    excpt.tval = tval;
    excpt.epc = epc;
}

static inline bool ai_arch_prefetch_excpt_metadata_match(uint64_t cause,
    uint64_t tval, uint64_t epc)
{
    return !excpt.triggered &&
        excpt.cause == cause &&
        excpt.tval == tval &&
        excpt.epc == epc;
}

static inline void ai_arch_prefetch_seed_s_trap_csrs(uint64_t cause,
    uint64_t tval, uint64_t epc)
{
    CSRW(CSR_SCAUSE, cause);
    CSRW(CSR_STVAL, tval);
    CSRW(CSR_SEPC, epc);
}

static inline bool ai_arch_prefetch_s_trap_csrs_match(uint64_t cause,
    uint64_t tval, uint64_t epc)
{
    return CSRR(CSR_SCAUSE) == cause &&
        CSRR(CSR_STVAL) == tval &&
        CSRR(CSR_SEPC) == epc;
}

static inline uint64_t ai_arch_prefetch_i_gpr_sentinel(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t observed;

    asm volatile(
        "mv t0, %1\n\t"
        "mv t1, %2\n\t"
        ".insn i 0x13, 0x6, x0, t0, 0\n\t"
        "mv %0, t1\n\t"
        : "=r"(observed)
        : "r"(addr), "r"(sentinel)
        : "t0", "t1", "memory");
    return observed;
}

static inline uint64_t ai_arch_prefetch_r_gpr_sentinel(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t observed;

    asm volatile(
        "mv t0, %1\n\t"
        "mv t1, %2\n\t"
        ".insn i 0x13, 0x6, x0, t0, 1\n\t"
        "mv %0, t1\n\t"
        : "=r"(observed)
        : "r"(addr), "r"(sentinel)
        : "t0", "t1", "memory");
    return observed;
}

static inline uint64_t ai_arch_prefetch_w_gpr_sentinel(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t observed;

    asm volatile(
        "mv t0, %1\n\t"
        "mv t1, %2\n\t"
        ".insn i 0x13, 0x6, x0, t0, 3\n\t"
        "mv %0, t1\n\t"
        : "=r"(observed)
        : "r"(addr), "r"(sentinel)
        : "t0", "t1", "memory");
    return observed;
}

bool ai_arch_prefetch_i_no_x_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;
    const uint64_t scause_seed = 0x1496ULL;
    const uint64_t stval_seed = 0x14960000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_i_no_x_after_probe;
    const uint64_t gpr_sentinel = 0x4950524546475052ULL;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t gpr_after = ai_arch_prefetch_i_gpr_sentinel(vaddr,
        gpr_sentinel);
prefetch_i_no_x_after_probe:
    TEST_ASSERT("prefetch.i to a page without execute permission is silently dropped without changing ordinary GPR state, S trap CSRs, or local exception metadata",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed,
            sepc_seed) &&
        gpr_after == gpr_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent no-X prefetch.i leaves target and adjacent instruction words unchanged",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_r_no_r_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;
    const uint64_t scause_seed = 0x1498ULL;
    const uint64_t stval_seed = 0x14980000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_r_no_r_after_probe;
    const uint64_t gpr_sentinel = 0x5250524546475052ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t gpr_after = ai_arch_prefetch_r_gpr_sentinel(vaddr,
        gpr_sentinel);
prefetch_r_no_r_after_probe:
    TEST_ASSERT("prefetch.r to a page without read permission is silently dropped without changing ordinary GPR state, S trap CSRs, or local exception metadata",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed,
            sepc_seed) &&
        gpr_after == gpr_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent no-R prefetch.r leaves target and adjacent data words unchanged",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_w_no_w_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x1122334455667788ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;
    const uint64_t scause_seed = 0x1500ULL;
    const uint64_t stval_seed = 0x15000000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_w_no_w_after_probe;
    const uint64_t gpr_sentinel = 0x5750524546475052ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    uint64_t gpr_after = ai_arch_prefetch_w_gpr_sentinel(vaddr,
        gpr_sentinel);
prefetch_w_no_w_after_probe:
    TEST_ASSERT("prefetch.w to a page without write permission is silently dropped without changing ordinary GPR state, S trap CSRs, or local exception metadata",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed,
            sepc_seed) &&
        gpr_after == gpr_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent no-W prefetch.w leaves target and adjacent data words unchanged before a real store",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    ai_arch_prefetch_set_hs_leaf(VSRX_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after the silent no-W prefetch.w, a repaired real store writes the target only when it actually executes",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered store updates only the target word and keeps the adjacent guard intact",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_silent_then_fetch_still_faults()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    const uint32_t instruction = 0x00008067U;
    const uint64_t scause_seed = 0x1510ULL;
    const uint64_t stval_seed = 0x15100000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_i_fetch_fault_after_probe;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
prefetch_i_fetch_fault_after_probe:
    TEST_ASSERT("prefetch.i to a page without execute permission stays silent and leaves stale S trap metadata untouched before a real fetch probe",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("a real instruction fetch to the same no-X page independently reports instruction page fault after the silent prefetch.i",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.i and later faulting fetch leave the backing instruction word unchanged",
        read32(paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_silent_then_fetch_still_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a page without execute permission stays silent before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same no-X page also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same no-X page still reports instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.i probes and the later faulting fetch leave the backing instruction word unchanged",
        read32(paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_silent_then_fetch_still_faults_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a page without execute permission stays silent before the adjacent-preservation fetch-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same no-X page also stays silent before the adjacent-preservation fetch-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same no-X page still reports instruction page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.i probes and the later faulting fetch leave both the target and adjacent backing instruction words unchanged",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_silent_then_grant_x_executes_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRW_GURWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("prefetch.i to a no-X page stays silent before execute permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_arch_prefetch_set_hs_leaf(VSRW_GURWX, phys_page_base(VSRW_GURWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after a silent prefetch.i, restoring execute permission and flushing translation state lets the real fetch retire successfully",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_silent_then_grant_x_executes_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a no-X page stays silent before execute permission is restored",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same no-X page also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.i probes leave the backing instruction word unchanged before execute permission is restored",
        read32(paddr) == instruction
    );
    ai_arch_prefetch_set_hs_leaf(VSRW_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, restoring execute permission and flushing translation state lets the real fetch retire successfully",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_silent_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a no-X page stays silent before execute permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same no-X page also stays silent before execute permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.i probes leave both the target and adjacent backing instruction words unchanged before execute permission is restored",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );
    ai_arch_prefetch_set_hs_leaf(VSRW_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, restoring execute permission and flushing translation state lets the real fetch retire successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered prefetch.i execute path, both the target and adjacent backing instruction words remain untouched",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_noncanonical_silent_then_fetch_page_faults()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(exec_vaddr);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("prefetch.i to a non-canonical virtual address is silently dropped before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("a real instruction fetch to the same non-canonical virtual address still reports instruction page fault with the raw VA in tval after the silent prefetch.i",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    TEST_END();
}

bool ai_arch_prefetch_i_noncanonical_repeated_silent_then_fetch_page_faults_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to a non-canonical virtual address stays silent before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same non-canonical virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same non-canonical virtual address still reports instruction page fault with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.i probes and the later faulting fetch leave the backing instruction word unchanged",
        read32(exec_paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_prefetch_i_noncanonical_repeated_silent_then_fetch_page_faults_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to a non-canonical virtual address stays silent before the adjacent-preservation fetch-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same non-canonical virtual address also stays silent before the adjacent-preservation fetch-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same non-canonical virtual address still reports instruction page fault with the raw VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.i probes and the later faulting fetch leave both the target and adjacent backing instruction words unchanged",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_noncanonical_silent_then_valid_fetch_recovers_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(exec_vaddr);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("prefetch.i to a non-canonical virtual address stays silent before a later canonical fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after a silent non-canonical prefetch.i, a subsequent canonical executable HS address still runs successfully",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_noncanonical_repeated_silent_then_valid_fetch_recovers_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to a non-canonical virtual address stays silent before a later canonical fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same non-canonical virtual address also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.i probes leave the backing instruction word unchanged",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated silent non-canonical prefetch.i probes, a subsequent canonical executable HS address still runs successfully",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_noncanonical_repeated_silent_then_valid_fetch_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(exec_vaddr);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to a non-canonical virtual address stays silent before a later canonical fetch probe in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same non-canonical virtual address also stays silent in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.i probes leave both the target and adjacent backing instruction words unchanged",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated silent non-canonical prefetch.i probes, a subsequent canonical executable HS address still runs successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered non-canonical prefetch.i execute path, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_all_ones_silent_then_fetch_page_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = vaddr & ~1ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("prefetch.i to an all-ones virtual address is silently dropped before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("a real instruction fetch after the silent all-ones prefetch.i still reports instruction page fault, with tval reflecting the JALR-cleared fetch target",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    TEST_END();
}

bool ai_arch_prefetch_i_all_ones_repeated_silent_then_fetch_page_faults_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to an all-ones virtual address stays silent before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same all-ones virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same all-ones virtual address still reports instruction page fault with the JALR-cleared fetch target in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.i probes and the later faulting fetch leave the backing instruction word unchanged",
        read32(exec_paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_prefetch_i_all_ones_repeated_silent_then_fetch_page_faults_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to an all-ones virtual address stays silent before the adjacent-preservation fetch-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same all-ones virtual address also stays silent before the adjacent-preservation fetch-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same all-ones virtual address still reports instruction page fault with the JALR-cleared fetch target in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.i probes and the later faulting fetch leave both the target and adjacent backing instruction words unchanged",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_all_ones_silent_then_valid_fetch_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("prefetch.i to an all-ones virtual address stays silent before a later valid fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after a silent all-ones prefetch.i, a subsequent canonical executable HS address still runs successfully",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_all_ones_repeated_silent_then_valid_fetch_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to an all-ones virtual address stays silent before a later canonical fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same all-ones virtual address also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.i probes leave the backing instruction word unchanged",
        read32(exec_paddr) == instruction
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated silent all-ones prefetch.i probes, a subsequent canonical executable HS address still runs successfully",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_all_ones_repeated_silent_then_valid_fetch_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the first prefetch.i to an all-ones virtual address stays silent before a later canonical fetch probe in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(bad_vaddr);
    TEST_ASSERT("the second prefetch.i to the same all-ones virtual address also stays silent in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.i probes leave both the target and adjacent backing instruction words unchanged",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated silent all-ones prefetch.i probes, a subsequent canonical executable HS address still runs successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered all-ones prefetch.i execute path, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_leaf_overflow_silent_then_fetch_access_faults()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("prefetch.i to a leaf whose PPN overflows with access-fault semantics is silently dropped before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("a real instruction fetch to the same leaf-overflow virtual address still reports instruction access fault after the silent prefetch.i",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fetch_access_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a leaf-overflow virtual address stays silent before a real fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same leaf-overflow virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same leaf-overflow virtual address still reports instruction access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.i probes and the later access-faulting fetch leave the backing instruction word unchanged",
        read32(paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fetch_access_faults_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a leaf-overflow virtual address stays silent before the adjacent-preservation fetch-AF probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same leaf-overflow virtual address also stays silent before the adjacent-preservation fetch-AF probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated silent prefetch.i probes, a real instruction fetch to the same leaf-overflow virtual address still reports instruction access fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.i probes and the later access-faulting fetch leave both the target and adjacent backing instruction words unchanged",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_leaf_overflow_silent_then_fix_leaf_executes_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write32(phys_page_base(VSRWX_GRWX), 0x00008067U);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("prefetch.i to a leaf-overflow virtual address stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after a silent leaf-overflow prefetch.i, repairing the leaf entry restores normal execution on the same HS address",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fix_leaf_executes_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a leaf-overflow virtual address stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same leaf-overflow virtual address also stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.i probes leave the backing instruction word unchanged before repair",
        read32(paddr) == instruction
    );
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after repeated silent leaf-overflow prefetch.i probes, repairing the leaf entry restores normal execution on the same HS address",
        excpt.triggered == false
    );

    TEST_END();
}

bool ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the first prefetch.i to a leaf-overflow virtual address stays silent before the leaf encoding is repaired in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_i(vaddr);
    TEST_ASSERT("the second prefetch.i to the same leaf-overflow virtual address also stays silent before the leaf encoding is repaired in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.i probes leave both the target and adjacent backing instruction words unchanged before repair",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after repeated silent leaf-overflow prefetch.i probes, repairing the leaf entry restores normal execution on the same HS address in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered leaf-overflow prefetch.i execute path, both the target and adjacent backing instruction words remain untouched",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_r_silent_then_load_still_faults()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t rd_sentinel = 0x5152535455565758ULL;
    const uint64_t scause_seed = 0x1517ULL;
    const uint64_t stval_seed = 0x15170000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_r_load_fault_after_probe;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
prefetch_r_load_fault_after_probe:
    TEST_ASSERT("prefetch.r to a page without read permission stays silent and leaves stale S trap metadata untouched before a real load probe",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ai_arch_prefetch_ld_rd_probe(vaddr, rd_sentinel);
    TEST_ASSERT("a real load to the same no-R page independently reports load page fault after the silent prefetch.r and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        loaded == rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.r and later faulting load leave backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_repeated_silent_then_load_still_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t rd_sentinel = 0x5152535455565759ULL;
    const uint64_t scause_seed = 0x1554ULL;
    const uint64_t stval_seed = 0x15540000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_r_repeated_load_fault_after_first;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
prefetch_r_repeated_load_fault_after_first:
    TEST_ASSERT("the first prefetch.r to a page without read permission stays silent and preserves stale S trap metadata before a real load probe",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the second prefetch.r to the same no-R page also stays silent and still preserves stale S trap metadata",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ai_arch_prefetch_ld_rd_probe(vaddr, rd_sentinel);
    TEST_ASSERT("after repeated silent prefetch.r probes, a real load to the same no-R page still reports load page fault and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr &&
        loaded == rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.r probes and the later faulting load leave backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_silent_then_grant_r_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    const uint64_t seed = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSX_GURWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("prefetch.r to a no-R page stays silent before read permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_arch_prefetch_set_hs_leaf(VSX_GURWX, phys_page_base(VSX_GURWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("after a silent prefetch.r, restoring read permission and flushing translation state lets the real load return the seeded payload",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_repeated_silent_then_grant_r_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the first prefetch.r to a no-R page stays silent before read permission is restored",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the second prefetch.r to the same no-R page also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.r probes leave the backing data unchanged before read permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_prefetch_set_hs_leaf(VSX_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("after repeated silent prefetch.r probes, restoring read permission and flushing translation state lets the real load return the seeded payload",
        excpt.triggered == false &&
        value == seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered load path, both target data and adjacent guard remain unchanged",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_r_noncanonical_silent_then_load_page_faults_corner()
{
    TEST_START();

    uintptr_t base_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(base_vaddr);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("prefetch.r to a non-canonical virtual address is silently dropped before a real load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("a real load to the same non-canonical virtual address still reports load page fault with the raw VA in tval after the silent prefetch.r",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.r and later non-canonical load fault leave backing memory unchanged",
        read64(phys_page_base(VSRWX_GRWX)) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_noncanonical_repeated_silent_then_load_page_faults_corner()
{
    TEST_START();

    uintptr_t base_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t base_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(base_vaddr);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(base_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the first prefetch.r to a non-canonical virtual address stays silent before a real load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the second prefetch.r to the same non-canonical virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("after repeated silent prefetch.r probes, a real load to the same non-canonical virtual address still reports load page fault with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.r probes and the later faulting load leave backing memory unchanged",
        read64(base_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_noncanonical_silent_then_valid_load_recovers_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(valid_vaddr);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("prefetch.r to a non-canonical virtual address stays silent before a later canonical load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after a silent non-canonical prefetch.r, a subsequent canonical HS load still returns the expected payload",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_noncanonical_repeated_silent_then_valid_load_recovers_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(valid_vaddr);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the first prefetch.r to a non-canonical virtual address stays silent before a later canonical load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the second prefetch.r to the same non-canonical virtual address also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.r probes leave the backing data unchanged before the recovery load",
        read64(valid_paddr) == seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after repeated silent non-canonical prefetch.r probes, a subsequent canonical HS load still returns the expected payload",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_all_ones_silent_then_load_page_faults()
{
    TEST_START();

    uintptr_t vaddr = (uintptr_t)-1;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("prefetch.r to an all-ones virtual address is silently dropped before a real load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a real load to the same all-ones virtual address still reports load page fault with the original VA in tval after the silent prefetch.r",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_arch_prefetch_r_all_ones_repeated_silent_then_load_page_faults_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the first prefetch.r to an all-ones virtual address stays silent before a real load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the second prefetch.r to the same all-ones virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("after repeated silent prefetch.r probes, a real load to the same all-ones virtual address still reports load page fault with the original VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.r probes and the later faulting load leave backing memory unchanged",
        read64(valid_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_all_ones_silent_then_valid_load_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("prefetch.r to an all-ones virtual address stays silent before a later valid load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after a silent all-ones prefetch.r, a subsequent canonical HS load still returns the expected payload",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_all_ones_repeated_silent_then_valid_load_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the first prefetch.r to an all-ones virtual address stays silent before a later canonical load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("the second prefetch.r to the same all-ones virtual address also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.r probes leave the backing data unchanged before the recovery load",
        read64(valid_paddr) == seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after repeated silent all-ones prefetch.r probes, a subsequent canonical HS load still returns the expected payload",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_leaf_overflow_silent_then_load_access_faults()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uint64_t seed = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("prefetch.r to a leaf whose PPN overflows with access-fault semantics is silently dropped before a real load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a real load to the same leaf-overflow virtual address still reports load access fault after the silent prefetch.r",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.r and later access-faulting load leave backing memory unchanged",
        read64(phys_page_base(VSRWX_GRWX)) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_repeated_leaf_overflow_silent_then_load_access_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the first prefetch.r to a leaf-overflow virtual address stays silent before a real load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the second prefetch.r to the same leaf-overflow virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after repeated silent prefetch.r probes, a real load to the same leaf-overflow virtual address still reports load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.r probes and the later access-faulting load leave backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_leaf_overflow_silent_then_fix_leaf_loads_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("prefetch.r to a leaf-overflow virtual address stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("after a silent leaf-overflow prefetch.r, repairing the leaf entry restores a successful load on the same HS address",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_repeated_leaf_overflow_silent_then_fix_leaf_loads_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x0102030405060708ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the first prefetch.r to a leaf-overflow virtual address stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("the second prefetch.r to the same leaf-overflow virtual address also stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.r probes leave the backing data unchanged before repair",
        read64(paddr) == seed
    );
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("after repeated silent leaf-overflow prefetch.r probes, repairing the leaf entry restores a successful load on the same HS address",
        excpt.triggered == false &&
        value == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_silent_then_store_still_faults()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x1122334455667788ULL;
    const uint64_t denied_store = 0xdeadbeefcafef00dULL;
    const uint64_t scause_seed = 0x1519ULL;
    const uint64_t stval_seed = 0x15190000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_w_store_fault_after_probe;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
prefetch_w_store_fault_after_probe:
    TEST_ASSERT("prefetch.w to a page without write permission stays silent and leaves stale S trap metadata untouched before a real store probe",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_store);
    TEST_ASSERT("a real store to the same no-W page independently reports store page fault after the silent prefetch.w",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.w and later faulting store leave target and adjacent backing memory unchanged",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_w_repeated_silent_then_store_still_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x1122334455667788ULL;
    const uint64_t denied_store = 0xdeadbeefcafef00dULL;
    const uint64_t scause_seed = 0x1556ULL;
    const uint64_t stval_seed = 0x15560000ULL;
    const uint64_t sepc_seed = (uint64_t)&&prefetch_w_repeated_store_fault_after_first;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed, sepc_seed);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed, sepc_seed);

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
prefetch_w_repeated_store_fault_after_first:
    TEST_ASSERT("the first prefetch.w to a page without write permission stays silent and preserves stale S trap metadata before a real store probe",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the second prefetch.w to the same no-W page also stays silent and still preserves stale S trap metadata",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            sepc_seed) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed, sepc_seed)
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_store);
    TEST_ASSERT("after repeated silent prefetch.w probes, a real store to the same no-W page still reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.w probes and the later faulting store leave target and adjacent backing memory unchanged",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_w_silent_then_grant_w_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t store_vaddr = vaddr + 3U;
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x1122334455667788ULL;
    const uint16_t stored_half = 0xcafeU;
    const uint64_t expected = (seed & ~(0xffffULL << 24)) |
        ((uint64_t)stored_half << 24);

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("prefetch.w to a no-W page stays silent before write permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("single silent prefetch.w leaves the target and adjacent guard unchanged before write permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_prefetch_set_hs_leaf(VSRX_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sh(store_vaddr, stored_half);
    TEST_ASSERT("after a silent prefetch.w, restoring write permission and flushing translation state lets the real halfword store retire successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("after the recovered halfword store path, the same translated address reads back the byte-enabled merge",
        excpt.triggered == false &&
        value == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered halfword store updates only its byte lanes after write permission is restored and keeps the adjacent guard intact",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_w_repeated_silent_then_grant_w_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t store_vaddr = vaddr + 5U;
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x1122334455667788ULL;
    const uint16_t stored_half = 0xf00dU;
    const uint64_t expected = (seed & ~(0xffffULL << 40)) |
        ((uint64_t)stored_half << 40);

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the first prefetch.w to a no-W page stays silent before write permission is restored",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the second prefetch.w to the same no-W page also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent prefetch.w probes leave the backing data unchanged before write permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_prefetch_set_hs_leaf(VSRX_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sh(store_vaddr, stored_half);
    TEST_ASSERT("after repeated silent prefetch.w probes, restoring write permission and flushing translation state lets the real halfword store retire successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(vaddr);
    TEST_ASSERT("after the recovered halfword store path, the same translated address reads back the byte-enabled merge",
        excpt.triggered == false &&
        value == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered halfword store updates backing memory only in its byte lanes after write permission is restored, even if prefetch.w was issued repeatedly while the page was no-W",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_w_noncanonical_silent_then_store_page_faults_corner()
{
    TEST_START();

    uintptr_t base_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(base_vaddr);
    const uint64_t seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("prefetch.w to a non-canonical virtual address is silently dropped before a real store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("a real store to the same non-canonical virtual address still reports store page fault with the raw VA in tval after the silent prefetch.w",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.w and later non-canonical store fault leave backing memory unchanged",
        read64(phys_page_base(VSRWX_GRWX)) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_noncanonical_repeated_silent_then_store_page_faults_corner()
{
    TEST_START();

    uintptr_t base_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t base_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(base_vaddr);
    const uint64_t seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(base_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the first prefetch.w to a non-canonical virtual address stays silent before a real store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the second prefetch.w to the same non-canonical virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after repeated silent prefetch.w probes, a real store to the same non-canonical virtual address still reports store page fault with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.w probes and the later faulting store leave backing memory unchanged",
        read64(base_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_noncanonical_silent_then_valid_store_recovers_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(valid_vaddr);
    const uint64_t seed = 0x8899aabbccddeeffULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("prefetch.w to a non-canonical virtual address stays silent before a later canonical store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after a silent non-canonical prefetch.w, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after the recovered canonical store path, the same HS address reads back the newly stored payload",
        excpt.triggered == false &&
        value == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered non-canonical prefetch.w corner, backing memory matches the committed store payload",
        read64(phys_page_base(VSRWX_GRWX)) == stored
    );

    TEST_END();
}

bool ai_arch_prefetch_w_noncanonical_repeated_silent_then_valid_store_recovers_corner()
{
    TEST_START();

    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_prefetch_make_noncanonical_vaddr(valid_vaddr);
    const uint64_t seed = 0x8899aabbccddeeffULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the first prefetch.w to a non-canonical virtual address stays silent before a later canonical store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the second prefetch.w to the same non-canonical virtual address also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent non-canonical prefetch.w probes leave the backing data unchanged before the recovery store",
        read64(valid_paddr) == seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after repeated silent non-canonical prefetch.w probes, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after the recovered canonical store path, the same HS address reads back the newly stored payload",
        excpt.triggered == false &&
        value == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated non-canonical prefetch.w recovery corner, backing memory matches the committed store payload",
        read64(valid_paddr) == stored
    );

    TEST_END();
}

bool ai_arch_prefetch_w_all_ones_silent_then_store_page_faults()
{
    TEST_START();

    uintptr_t vaddr = (uintptr_t)-1;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("prefetch.w to an all-ones virtual address is silently dropped before a real store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("a real store to the same all-ones virtual address still reports store page fault with the original VA in tval after the silent prefetch.w",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_arch_prefetch_w_all_ones_repeated_silent_then_store_page_faults_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x1111222233334444ULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the first prefetch.w to an all-ones virtual address stays silent before a real store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the second prefetch.w to the same all-ones virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after repeated silent prefetch.w probes, a real store to the same all-ones virtual address still reports store page fault with the original VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.w probes and the later faulting store leave backing memory unchanged",
        read64(valid_paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_all_ones_silent_then_valid_store_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x1111222233334444ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("prefetch.w to an all-ones virtual address stays silent before a later valid store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after a silent all-ones prefetch.w, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after the recovered store path, the same canonical HS address reads back the newly stored payload",
        excpt.triggered == false &&
        value == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered all-ones prefetch.w corner, backing memory matches the committed store payload",
        read64(phys_page_base(VSRWX_GRWX)) == stored
    );

    TEST_END();
}

bool ai_arch_prefetch_w_all_ones_repeated_silent_then_valid_store_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t valid_paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x1111222233334444ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(valid_paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the first prefetch.w to an all-ones virtual address stays silent before a later canonical store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(bad_vaddr);
    TEST_ASSERT("the second prefetch.w to the same all-ones virtual address also stays silent",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent all-ones prefetch.w probes leave the backing data unchanged before the recovery store",
        read64(valid_paddr) == seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(valid_vaddr, stored);
    TEST_ASSERT("after repeated silent all-ones prefetch.w probes, a subsequent canonical HS store still retires successfully",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(valid_vaddr);
    TEST_ASSERT("after the recovered canonical store path, the same HS address reads back the newly stored payload",
        excpt.triggered == false &&
        value == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated all-ones prefetch.w recovery corner, backing memory matches the committed store payload",
        read64(valid_paddr) == stored
    );

    TEST_END();
}

bool ai_arch_prefetch_w_leaf_overflow_silent_then_store_access_faults()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uint64_t seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("prefetch.w to a leaf whose PPN overflows with access-fault semantics is silently dropped before a real store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("a real store to the same leaf-overflow virtual address still reports store access fault after the silent prefetch.w",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the silent prefetch.w and later access-faulting store leave backing memory unchanged",
        read64(phys_page_base(VSRWX_GRWX)) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_repeated_leaf_overflow_silent_then_store_access_faults_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8899aabbccddeeffULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the first prefetch.w to a leaf-overflow virtual address stays silent before a real store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the second prefetch.w to the same leaf-overflow virtual address also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after repeated silent prefetch.w probes, a real store to the same leaf-overflow virtual address still reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.w probes and the later access-faulting store leave backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_leaf_overflow_silent_then_fix_leaf_stores_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8899aabbccddeeffULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(phys_page_base(VSRWX_GRWX), seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("prefetch.w to a leaf-overflow virtual address stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after a silent leaf-overflow prefetch.w, repairing the leaf entry restores a successful store on the same HS address",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repaired leaf-overflow store corner, backing memory matches the newly stored payload",
        read64(phys_page_base(VSRWX_GRWX)) == stored
    );

    TEST_END();
}

bool ai_arch_prefetch_w_repeated_leaf_overflow_silent_then_fix_leaf_stores_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uint64_t seed = 0x8899aabbccddeeffULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_prefetch_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the first prefetch.w to a leaf-overflow virtual address stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(vaddr);
    TEST_ASSERT("the second prefetch.w to the same leaf-overflow virtual address also stays silent before the leaf encoding is repaired",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated silent leaf-overflow prefetch.w probes leave the backing data unchanged before repair",
        read64(paddr) == seed
    );
    ai_arch_prefetch_set_hs_leaf(VSRWX_GRWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after repeated silent leaf-overflow prefetch.w probes, repairing the leaf entry restores a successful store on the same HS address",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repaired repeated leaf-overflow store corner, backing memory matches the newly stored payload",
        read64(paddr) == stored
    );

    TEST_END();
}

bool ai_arch_prefetch_pte_ad_no_update_then_real_access_rejudges_corner()
{
    TEST_START();

    const enum test_page exec_page = VSRWX_GRWX;
    const enum test_page load_page = VSR_GR;
    const enum test_page store_page = VSRW_GRW;
    const uintptr_t exec_vaddr = hs_page_base(exec_page) + 0x100U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x140U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x180U;
    const uintptr_t exec_paddr = phys_page_base(exec_page) + 0x100U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x140U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x180U;
    const uintptr_t store_adjacent_paddr = store_paddr + sizeof(uint64_t);
    const uint64_t load_value = 0x7072656661646c64ULL;
    const uint64_t store_initial = 0x7072656661647330ULL;
    const uint64_t store_payload = 0x7072656661647331ULL;
    const uint64_t store_denied = 0x7072656661647332ULL;
    const uint64_t store_adjacent = 0x7072656661646164ULL;
    const uint64_t load_sentinel = 0xadad000000001588ULL;
    const uint64_t scause_seed = 0x1588ULL;
    const uint64_t stval_seed = 0x15880000ULL;
    const uint64_t exec_a0_leaf_flags = PTE_V | PTE_X;
    const uint64_t load_a0_leaf_flags = PTE_V | PTE_D | PTE_R;
    const uint64_t store_a0d0_leaf_flags = PTE_V | PTE_RW;
    const uint64_t exec_a1_leaf_flags = PTE_V | PTE_A | PTE_X;
    const uint64_t load_a1_leaf_flags = PTE_V | PTE_AD | PTE_R;
    const uint64_t store_d0_leaf_flags = PTE_V | PTE_A | PTE_RW;
    const uint64_t store_ad_leaf_flags = PTE_V | PTE_AD | PTE_RW;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write32(exec_paddr, 0x00008067U);
    write64(load_paddr, load_value);
    write64(store_paddr, store_initial);
    write64(store_adjacent_paddr, store_adjacent);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_arch_prefetch_set_hs_leaf(exec_page, phys_page_base(exec_page),
        exec_a0_leaf_flags);
    ai_arch_prefetch_set_hs_leaf(load_page, phys_page_base(load_page),
        load_a0_leaf_flags);
    ai_arch_prefetch_set_hs_leaf(store_page, phys_page_base(store_page),
        store_a0d0_leaf_flags);
    const uint64_t exec_a0_leaf =
        *ai_arch_prefetch_hs_leaf_entry(exec_page);
    const uint64_t load_a0_leaf =
        *ai_arch_prefetch_hs_leaf_entry(load_page);
    const uint64_t store_a0d0_leaf =
        *ai_arch_prefetch_hs_leaf_entry(store_page);
    sfence_vma();

    TEST_ASSERT("the prefetch A/D probe starts with executable and readable leaves whose PTE.A is clear plus a writable leaf whose PTE.A/PTE.D are both clear",
        (exec_a0_leaf & (PTE_V | PTE_X)) == (PTE_V | PTE_X) &&
        (exec_a0_leaf & PTE_A) == 0 &&
        (load_a0_leaf & (PTE_V | PTE_D | PTE_R)) ==
            (PTE_V | PTE_D | PTE_R) &&
        (load_a0_leaf & PTE_A) == 0 &&
        (store_a0d0_leaf & (PTE_V | PTE_RW)) == (PTE_V | PTE_RW) &&
        (store_a0d0_leaf & PTE_AD) == 0
    );

    goto_priv(PRIV_HS);
    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed,
        (uint64_t)&&prefetch_pte_ad_after_i);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed,
        (uint64_t)&&prefetch_pte_ad_after_i);
    TEST_SETUP_EXCEPT();
    prefetch_i(exec_vaddr);
prefetch_pte_ad_after_i:
    TEST_ASSERT("prefetch.i to an executable leaf with PTE.A=0 retires silently and leaves trap metadata untouched",
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            (uint64_t)&&prefetch_pte_ad_after_i) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed,
            (uint64_t)&&prefetch_pte_ad_after_i)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 1ULL, stval_seed + 1ULL,
        (uint64_t)&&prefetch_pte_ad_after_r);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 1ULL,
        stval_seed + 1ULL, (uint64_t)&&prefetch_pte_ad_after_r);
    TEST_SETUP_EXCEPT();
    prefetch_r(load_vaddr);
prefetch_pte_ad_after_r:
    TEST_ASSERT("prefetch.r to a readable leaf with PTE.A=0 retires silently and leaves trap metadata untouched",
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 1ULL,
            stval_seed + 1ULL, (uint64_t)&&prefetch_pte_ad_after_r) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 1ULL,
            stval_seed + 1ULL, (uint64_t)&&prefetch_pte_ad_after_r)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 2ULL, stval_seed + 2ULL,
        (uint64_t)&&prefetch_pte_ad_after_w);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 2ULL,
        stval_seed + 2ULL, (uint64_t)&&prefetch_pte_ad_after_w);
    TEST_SETUP_EXCEPT();
    prefetch_w(store_vaddr);
prefetch_pte_ad_after_w:
    TEST_ASSERT("prefetch.w to a writable leaf with PTE.A/PTE.D clear retires silently and leaves trap metadata untouched",
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 2ULL,
            stval_seed + 2ULL, (uint64_t)&&prefetch_pte_ad_after_w) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 2ULL,
            stval_seed + 2ULL, (uint64_t)&&prefetch_pte_ad_after_w)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch.i/r/w probes do not set PTE.A or PTE.D and do not change target or adjacent backing memory",
        *ai_arch_prefetch_hs_leaf_entry(exec_page) == exec_a0_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(load_page) == load_a0_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(store_page) == store_a0d0_leaf &&
        read32(exec_paddr) == 0x00008067U &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_initial &&
        read64(store_adjacent_paddr) == store_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(exec_vaddr);
    TEST_ASSERT("after silent prefetch.i, a real fetch to the same PTE.A=0 executable leaf independently reports instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == exec_vaddr
    );

    TEST_SETUP_EXCEPT();
    uint64_t load_a0_rd =
        ai_arch_prefetch_ld_rd_probe(load_vaddr, load_sentinel);
    TEST_ASSERT("after silent prefetch.r, a real load to the same PTE.A=0 readable leaf independently reports load page fault without writing rd",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr &&
        load_a0_rd == load_sentinel
    );

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_denied);
    TEST_ASSERT("after silent prefetch.w, a real store to the same PTE.A/PTE.D clear writable leaf independently reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("real accesses that fault on PTE.A/PTE.D leave the PTE words and backing memory unchanged",
        *ai_arch_prefetch_hs_leaf_entry(exec_page) == exec_a0_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(load_page) == load_a0_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(store_page) == store_a0d0_leaf &&
        read32(exec_paddr) == 0x00008067U &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_initial &&
        read64(store_adjacent_paddr) == store_adjacent
    );

    ai_arch_prefetch_set_hs_leaf(exec_page, phys_page_base(exec_page),
        exec_a1_leaf_flags);
    ai_arch_prefetch_set_hs_leaf(load_page, phys_page_base(load_page),
        load_a1_leaf_flags);
    ai_arch_prefetch_set_hs_leaf(store_page, phys_page_base(store_page),
        store_d0_leaf_flags);
    const uint64_t exec_a1_leaf =
        *ai_arch_prefetch_hs_leaf_entry(exec_page);
    const uint64_t load_a1_leaf =
        *ai_arch_prefetch_hs_leaf_entry(load_page);
    const uint64_t store_d0_leaf =
        *ai_arch_prefetch_hs_leaf_entry(store_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after setting PTE.A explicitly and executing SFENCE.VMA, the previously prefetched instruction leaf executes normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t load_a1_value = ld(load_vaddr);
    TEST_ASSERT("after setting PTE.A explicitly and executing SFENCE.VMA, the previously prefetched readable leaf loads normally",
        excpt.triggered == false &&
        load_a1_value == load_value
    );

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_denied);
    TEST_ASSERT("after setting only PTE.A on the store leaf, the real store is still rejudged as a PTE.D=0 store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the A-only repair phase leaves the store target unchanged and records only the explicit PTE updates",
        *ai_arch_prefetch_hs_leaf_entry(exec_page) == exec_a1_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(load_page) == load_a1_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(store_page) == store_d0_leaf &&
        read64(store_paddr) == store_initial &&
        read64(store_adjacent_paddr) == store_adjacent
    );

    ai_arch_prefetch_set_hs_leaf(store_page, phys_page_base(store_page),
        store_ad_leaf_flags);
    const uint64_t store_ad_leaf =
        *ai_arch_prefetch_hs_leaf_entry(store_page);
    fence_iorw_iorw();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_payload);
    TEST_ASSERT("after explicitly setting PTE.D and fencing, the previously prefetched store leaf commits the real store",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the final repaired store updates only the target word while preserving instruction, load, adjacent data, and final PTE images",
        *ai_arch_prefetch_hs_leaf_entry(exec_page) == exec_a1_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(load_page) == load_a1_leaf &&
        *ai_arch_prefetch_hs_leaf_entry(store_page) == store_ad_leaf &&
        read32(exec_paddr) == 0x00008067U &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_payload &&
        read64(store_adjacent_paddr) == store_adjacent
    );

    TEST_END();
}

bool ai_arch_amo_misaligned_fault()
{
    TEST_START();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    uintptr_t vaddr = hs_page_base(VSRWX_GURWX) + 1;

    TEST_SETUP_EXCEPT();
    amoadd_d(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("an AMO to a misaligned address reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}

bool ai_arch_amo_misaligned_fault_tval_raw_no_side_effect_corner()
{
    TEST_START();

    const uint64_t lower_seed = 0x1122334455667788ULL;
    const uint64_t upper_seed = 0x99aabbccddeeff00ULL;
    const uint64_t rd_sentinel = 0xface000000000addULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, lower_seed);
    write64(paddr + sizeof(uint64_t), upper_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t fault_rd = ai_arch_hint_amoadd_d_probe_rd(misaligned_vaddr,
        0xdeadbeefcafef00dULL, rd_sentinel);
    TEST_ASSERT("a misaligned amoadd.d reports store access fault with the raw misaligned virtual address in tval and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        fault_rd == rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the misaligned amoadd.d corner leaves both the target doubleword and its adjacent doubleword unchanged",
        read64(paddr) == lower_seed &&
        read64(paddr + sizeof(uint64_t)) == upper_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_misaligned_fault_tval_raw_no_side_effect_corner()
{
    TEST_START();

    const uint64_t seed = 0x0123456789abcdefULL;
    const uint64_t rd_sentinel = 0xface00000000a5a5ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t fault_rd = ai_arch_hint_amoswap_w_probe_rd(misaligned_vaddr,
        0x55aa33ccU, rd_sentinel);
    TEST_ASSERT("a misaligned amoswap.w reports store access fault with the raw misaligned virtual address in tval and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        fault_rd == rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the misaligned amoswap.w corner leaves the surrounding doubleword unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_misaligned_fault_then_upper_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x99aabbcc11223344ULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x7fedcba911223344ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;
    uintptr_t aligned_upper_vaddr = base_vaddr + 4U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0x12345678U);
    TEST_ASSERT("a misaligned amoswap.w targeting the upper 32-bit lane reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery AMO runs, the misaligned upper-word amoswap.w has not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_hint_amoswap_w_raw64(aligned_upper_vaddr, recovered_word);
    TEST_ASSERT("after the misaligned upper-word AMO fault, a subsequent aligned amoswap.w to the same upper lane succeeds and returns the sign-extended old upper word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned upper-word recovery AMO updates only the target upper 32-bit lane while preserving the lower lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_misaligned_fault_then_lower_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1122334499aabbccULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x112233447fedcba9ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_lower_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_lower_vaddr, 0x12345678U);
    TEST_ASSERT("a misaligned amoswap.w targeting the lower 32-bit lane reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery AMO runs, the misaligned lower-word amoswap.w has not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_hint_amoswap_w_raw64(base_vaddr, recovered_word);
    TEST_ASSERT("after the misaligned lower-word AMO fault, a subsequent aligned amoswap.w to the same lower lane succeeds and returns the sign-extended old lower word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned lower-word recovery AMO updates only the target lower 32-bit lane while preserving the upper lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoadd_w_misaligned_fault_then_lower_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x5566778880000010ULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint32_t recovered_delta = 0x7ffffff0U;
    const uint64_t expected_target_after_recovery = 0x5566778800000000ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_vaddr, recovered_delta);
    TEST_ASSERT("a misaligned amoadd.w targeting the lower 32-bit lane reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery amoadd.w runs, the misaligned lower-word fault has not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(base_vaddr, recovered_delta);
    TEST_ASSERT("after the misaligned lower-word AMO fault, a subsequent aligned amoadd.w on the same lower lane succeeds and returns the old lower word",
        excpt.triggered == false &&
        old_value == 0x80000010U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned lower-word recovery amoadd.w wraps only the target lower lane while preserving the upper lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoadd_w_misaligned_fault_then_upper_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x8000001055667788ULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint32_t recovered_delta = 0x7ffffff0U;
    const uint64_t expected_target_after_recovery = 0x0000000055667788ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;
    uintptr_t aligned_upper_vaddr = base_vaddr + 4U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_upper_vaddr, recovered_delta);
    TEST_ASSERT("a misaligned amoadd.w targeting the upper 32-bit lane reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery amoadd.w runs, the misaligned upper-word fault has not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(aligned_upper_vaddr, recovered_delta);
    TEST_ASSERT("after the misaligned upper-word AMO fault, a subsequent aligned amoadd.w on the same upper lane succeeds and returns the old upper word",
        excpt.triggered == false &&
        old_value == 0x80000010U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned upper-word recovery amoadd.w wraps only the target upper lane while preserving the lower lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amo_misaligned_fault_then_aligned_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1020304050607080ULL;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t recovered_value = 0xa1b2c3d4e5f60718ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(misaligned_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first misaligned AMO in the recovery corner reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery AMO runs, the misaligned fault has not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoswap_d(base_vaddr, recovered_value);
    TEST_ASSERT("after the misaligned AMO fault, a subsequent aligned amoswap.d on the same backing word still succeeds normally",
        excpt.triggered == false &&
        old_value == initial_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned recovery AMO overwrites only the target doubleword while the adjacent doubleword stays unchanged",
        read64(paddr) == recovered_value &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amo_misaligned_repeated_faults_then_aligned_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1020304050607080ULL;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t recovered_value = 0xa1b2c3d4e5f60718ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_d(misaligned_vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("the first misaligned AMO in the repeated recovery corner reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_d(misaligned_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("the second misaligned AMO in the repeated recovery corner still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery AMO runs, repeated misaligned faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoswap_d(base_vaddr, recovered_value);
    TEST_ASSERT("after repeated misaligned AMO faults, a subsequent aligned amoswap.d on the same backing word still succeeds normally",
        excpt.triggered == false &&
        old_value == initial_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned recovery AMO after repeated misaligned faults overwrites only the target doubleword while the adjacent doubleword stays unchanged",
        read64(paddr) == recovered_value &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amo_misaligned_repeated_faults_tval_raw_no_side_effect_corner()
{
    TEST_START();

    const uint64_t lower_seed = 0x1122334455667788ULL;
    const uint64_t upper_seed = 0x99aabbccddeeff00ULL;
    const uint64_t first_rd_sentinel = 0xface00000000b001ULL;
    const uint64_t second_rd_sentinel = 0xface00000000b002ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, lower_seed);
    write64(paddr + sizeof(uint64_t), upper_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t first_fault_rd = ai_arch_hint_amoadd_d_probe_rd(
        misaligned_vaddr, 0xdeadbeefcafef00dULL, first_rd_sentinel);
    TEST_ASSERT("the first repeated misaligned amoadd.d reports store access fault with the raw misaligned virtual address in tval and suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        first_fault_rd == first_rd_sentinel
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_fault_rd = ai_arch_hint_amoadd_d_probe_rd(
        misaligned_vaddr, 0x0123456789abcdefULL, second_rd_sentinel);
    TEST_ASSERT("the second repeated misaligned amoadd.d still reports store access fault with the same raw misaligned virtual address in tval and suppresses rd writeback independently",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr &&
        second_fault_rd == second_rd_sentinel
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated misaligned amoadd.d faults leave both the target doubleword and its adjacent doubleword unchanged",
        read64(paddr) == lower_seed &&
        read64(paddr + sizeof(uint64_t)) == upper_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_repeated_misaligned_faults_tval_raw_no_side_effect_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x99aabbcc11223344ULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0x12345678U);
    TEST_ASSERT("the first repeated cross-doubleword misaligned amoswap.w reports store access fault with the raw misaligned virtual address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0xa5a55a5aU);
    TEST_ASSERT("the second repeated cross-doubleword misaligned amoswap.w still reports store access fault with the same raw misaligned virtual address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated cross-doubleword misaligned amoswap.w faults leave both the target doubleword and its adjacent doubleword unchanged",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_repeated_misaligned_fault_then_upper_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x99aabbcc11223344ULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x7fedcba911223344ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;
    uintptr_t aligned_upper_vaddr = base_vaddr + 4U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0x12345678U);
    TEST_ASSERT("the first repeated upper-word misaligned amoswap.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0xa5a55a5aU);
    TEST_ASSERT("the second repeated upper-word misaligned amoswap.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery AMO runs, repeated misaligned upper-word amoswap.w faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_hint_amoswap_w_raw64(aligned_upper_vaddr, recovered_word);
    TEST_ASSERT("after repeated misaligned upper-word AMO faults, a subsequent aligned amoswap.w to the same upper lane succeeds and returns the sign-extended old upper word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned upper-word recovery AMO after repeated faults updates only the target upper 32-bit lane while preserving the lower lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_repeated_misaligned_fault_then_lower_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1122334499aabbccULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x112233447fedcba9ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_lower_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_lower_vaddr, 0x12345678U);
    TEST_ASSERT("the first repeated lower-word misaligned amoswap.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_lower_vaddr, 0xa5a55a5aU);
    TEST_ASSERT("the second repeated lower-word misaligned amoswap.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery AMO runs, repeated misaligned lower-word amoswap.w faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_hint_amoswap_w_raw64(base_vaddr, recovered_word);
    TEST_ASSERT("after repeated misaligned lower-word AMO faults, a subsequent aligned amoswap.w to the same lower lane succeeds and returns the sign-extended old lower word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned lower-word recovery AMO after repeated faults updates only the target lower 32-bit lane while preserving the upper lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoadd_w_repeated_misaligned_fault_then_lower_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x5566778880000010ULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint32_t recovered_delta = 0x7ffffff0U;
    const uint64_t expected_target_after_recovery = 0x5566778800000000ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_vaddr, 0x11111111U);
    TEST_ASSERT("the first repeated lower-word misaligned amoadd.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_vaddr, 0x22222222U);
    TEST_ASSERT("the second repeated lower-word misaligned amoadd.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery amoadd.w runs, repeated misaligned lower-word faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(base_vaddr, recovered_delta);
    TEST_ASSERT("after repeated misaligned lower-word AMO faults, a subsequent aligned amoadd.w on the same lower lane succeeds and returns the old lower word",
        excpt.triggered == false &&
        old_value == 0x80000010U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned lower-word recovery amoadd.w after repeated faults wraps only the target lower lane while preserving the upper lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoadd_w_repeated_misaligned_fault_then_upper_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x8000001055667788ULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint32_t recovered_delta = 0x7ffffff0U;
    const uint64_t expected_target_after_recovery = 0x0000000055667788ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;
    uintptr_t aligned_upper_vaddr = base_vaddr + 4U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_upper_vaddr, 0x11111111U);
    TEST_ASSERT("the first repeated upper-word misaligned amoadd.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_upper_vaddr, 0x22222222U);
    TEST_ASSERT("the second repeated upper-word misaligned amoadd.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the recovery amoadd.w runs, repeated misaligned upper-word faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(aligned_upper_vaddr, recovered_delta);
    TEST_ASSERT("after repeated misaligned upper-word AMO faults, a subsequent aligned amoadd.w on the same upper lane succeeds and returns the old upper word",
        excpt.triggered == false &&
        old_value == 0x80000010U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned upper-word recovery amoadd.w after repeated faults wraps only the target upper lane while preserving the lower lane and adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_repeated_lower_lane_misaligned_fault_then_upper_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x99aabbcc11223344ULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x7fedcba911223344ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_lower_vaddr = base_vaddr + 1U;
    uintptr_t aligned_upper_vaddr = base_vaddr + 4U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_lower_vaddr, 0x12345678U);
    TEST_ASSERT("the first repeated lower-lane misaligned amoswap.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_lower_vaddr, 0xa5a55a5aU);
    TEST_ASSERT("the second repeated lower-lane misaligned amoswap.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the cross-lane recovery AMO runs, repeated lower-lane misaligned amoswap.w faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_hint_amoswap_w_raw64(aligned_upper_vaddr, recovered_word);
    TEST_ASSERT("after repeated lower-lane misaligned amoswap.w faults, a subsequent aligned upper-word amoswap.w still succeeds and returns the sign-extended old upper word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-lane recovery amoswap.w updates only the upper 32-bit lane while preserving the lower lane and adjacent doubleword after repeated lower-lane faults",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoswap_w_repeated_upper_lane_misaligned_fault_then_lower_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1122334499aabbccULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x112233447fedcba9ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0x12345678U);
    TEST_ASSERT("the first repeated upper-lane misaligned amoswap.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_upper_vaddr, 0xa5a55a5aU);
    TEST_ASSERT("the second repeated upper-lane misaligned amoswap.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the cross-lane recovery AMO runs, repeated upper-lane misaligned amoswap.w faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_hint_amoswap_w_raw64(base_vaddr, recovered_word);
    TEST_ASSERT("after repeated upper-lane misaligned amoswap.w faults, a subsequent aligned lower-word amoswap.w still succeeds and returns the sign-extended old lower word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-lane recovery amoswap.w updates only the lower 32-bit lane while preserving the upper lane and adjacent doubleword after repeated upper-lane faults",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoadd_w_repeated_lower_lane_misaligned_fault_then_upper_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x8000001055667788ULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint32_t recovered_delta = 0x7ffffff0U;
    const uint64_t expected_target_after_recovery = 0x0000000055667788ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_lower_vaddr = base_vaddr + 1U;
    uintptr_t aligned_upper_vaddr = base_vaddr + 4U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_lower_vaddr, 0x11111111U);
    TEST_ASSERT("the first repeated lower-lane misaligned amoadd.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_lower_vaddr, 0x22222222U);
    TEST_ASSERT("the second repeated lower-lane misaligned amoadd.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_lower_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the cross-lane recovery amoadd.w runs, repeated lower-lane misaligned faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(aligned_upper_vaddr, recovered_delta);
    TEST_ASSERT("after repeated lower-lane misaligned amoadd.w faults, a subsequent aligned upper-word amoadd.w still succeeds and returns the old upper word",
        excpt.triggered == false &&
        old_value == 0x80000010U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-lane recovery amoadd.w wraps only the upper 32-bit lane while preserving the lower lane and adjacent doubleword after repeated lower-lane faults",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amoadd_w_repeated_upper_lane_misaligned_fault_then_lower_word_recovers_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x5566778880000010ULL;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint32_t recovered_delta = 0x7ffffff0U;
    const uint64_t expected_target_after_recovery = 0x5566778800000000ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_upper_vaddr = base_vaddr + 5U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_upper_vaddr, 0x11111111U);
    TEST_ASSERT("the first repeated upper-lane misaligned amoadd.w reports store access fault with the raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    TEST_SETUP_EXCEPT();
    amoadd_w(misaligned_upper_vaddr, 0x22222222U);
    TEST_ASSERT("the second repeated upper-lane misaligned amoadd.w still reports store access fault with the same raw misaligned virtual address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_upper_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the cross-lane recovery amoadd.w runs, repeated upper-lane misaligned faults have not modified either the target or adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoadd_w(base_vaddr, recovered_delta);
    TEST_ASSERT("after repeated upper-lane misaligned amoadd.w faults, a subsequent aligned lower-word amoadd.w still succeeds and returns the old lower word",
        excpt.triggered == false &&
        old_value == 0x80000010U
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-lane recovery amoadd.w wraps only the lower 32-bit lane while preserving the upper lane and adjacent doubleword after repeated upper-lane faults",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_prefetch_i_r_w_repeated_legal_no_arch_writeback_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX) + 0x2c0U;
    uintptr_t paddr = phys_page_base(VSRWX_GRWX) + 0x2c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x707265666c676c30ULL;
    const uint64_t adjacent = 0x707265666c676164ULL;
    const uint64_t stored = 0x707265666c676c31ULL;
    const uint64_t scause_seed = 0x1596ULL;
    const uint64_t stval_seed = 0x15960000ULL;
    const uint64_t gpr_i = 0x494c4547414c5052ULL;
    const uint64_t gpr_r = 0x524c4547414c5052ULL;
    const uint64_t gpr_w = 0x574c4547414c5052ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed, stval_seed,
        (uint64_t)&&prefetch_legal_after_i_first);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed, stval_seed,
        (uint64_t)&&prefetch_legal_after_i_first);
    TEST_SETUP_EXCEPT();
    uint64_t i_first = ai_arch_prefetch_i_gpr_sentinel(vaddr, gpr_i);
prefetch_legal_after_i_first:
    TEST_ASSERT("the first legal prefetch.i stays silent and preserves GPR, S trap CSR, and local exception metadata",
        i_first == gpr_i &&
        ai_arch_prefetch_excpt_metadata_match(scause_seed, stval_seed,
            (uint64_t)&&prefetch_legal_after_i_first) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed, stval_seed,
            (uint64_t)&&prefetch_legal_after_i_first)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 1ULL, stval_seed + 1ULL,
        (uint64_t)&&prefetch_legal_after_i_second);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 1ULL,
        stval_seed + 1ULL, (uint64_t)&&prefetch_legal_after_i_second);
    TEST_SETUP_EXCEPT();
    uint64_t i_second = ai_arch_prefetch_i_gpr_sentinel(vaddr, gpr_i);
prefetch_legal_after_i_second:
    TEST_ASSERT("the second legal prefetch.i is idempotent and still produces no architectural writeback",
        i_second == gpr_i &&
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 1ULL,
            stval_seed + 1ULL, (uint64_t)&&prefetch_legal_after_i_second) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 1ULL,
            stval_seed + 1ULL, (uint64_t)&&prefetch_legal_after_i_second)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 2ULL, stval_seed + 2ULL,
        (uint64_t)&&prefetch_legal_after_r_first);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 2ULL,
        stval_seed + 2ULL, (uint64_t)&&prefetch_legal_after_r_first);
    TEST_SETUP_EXCEPT();
    uint64_t r_first = ai_arch_prefetch_r_gpr_sentinel(vaddr, gpr_r);
prefetch_legal_after_r_first:
    TEST_ASSERT("the first legal prefetch.r stays silent and preserves GPR, S trap CSR, and local exception metadata",
        r_first == gpr_r &&
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 2ULL,
            stval_seed + 2ULL, (uint64_t)&&prefetch_legal_after_r_first) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 2ULL,
            stval_seed + 2ULL, (uint64_t)&&prefetch_legal_after_r_first)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 3ULL, stval_seed + 3ULL,
        (uint64_t)&&prefetch_legal_after_r_second);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 3ULL,
        stval_seed + 3ULL, (uint64_t)&&prefetch_legal_after_r_second);
    TEST_SETUP_EXCEPT();
    uint64_t r_second = ai_arch_prefetch_r_gpr_sentinel(vaddr, gpr_r);
prefetch_legal_after_r_second:
    TEST_ASSERT("the second legal prefetch.r is idempotent and still produces no architectural writeback",
        r_second == gpr_r &&
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 3ULL,
            stval_seed + 3ULL, (uint64_t)&&prefetch_legal_after_r_second) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 3ULL,
            stval_seed + 3ULL, (uint64_t)&&prefetch_legal_after_r_second)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 4ULL, stval_seed + 4ULL,
        (uint64_t)&&prefetch_legal_after_w_first);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 4ULL,
        stval_seed + 4ULL, (uint64_t)&&prefetch_legal_after_w_first);
    TEST_SETUP_EXCEPT();
    uint64_t w_first = ai_arch_prefetch_w_gpr_sentinel(vaddr, gpr_w);
prefetch_legal_after_w_first:
    TEST_ASSERT("the first legal prefetch.w stays silent and preserves GPR, S trap CSR, and local exception metadata",
        w_first == gpr_w &&
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 4ULL,
            stval_seed + 4ULL, (uint64_t)&&prefetch_legal_after_w_first) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 4ULL,
            stval_seed + 4ULL, (uint64_t)&&prefetch_legal_after_w_first)
    );

    ai_arch_prefetch_seed_s_trap_csrs(scause_seed + 5ULL, stval_seed + 5ULL,
        (uint64_t)&&prefetch_legal_after_w_second);
    ai_arch_prefetch_seed_excpt_metadata(scause_seed + 5ULL,
        stval_seed + 5ULL, (uint64_t)&&prefetch_legal_after_w_second);
    TEST_SETUP_EXCEPT();
    uint64_t w_second = ai_arch_prefetch_w_gpr_sentinel(vaddr, gpr_w);
prefetch_legal_after_w_second:
    TEST_ASSERT("the second legal prefetch.w is idempotent and still produces no architectural writeback",
        w_second == gpr_w &&
        ai_arch_prefetch_excpt_metadata_match(scause_seed + 5ULL,
            stval_seed + 5ULL, (uint64_t)&&prefetch_legal_after_w_second) &&
        ai_arch_prefetch_s_trap_csrs_match(scause_seed + 5ULL,
            stval_seed + 5ULL, (uint64_t)&&prefetch_legal_after_w_second)
    );

    TEST_SETUP_EXCEPT();
    uint64_t loaded = ld(vaddr);
    TEST_ASSERT("after repeated legal prefetch.i/r/w hints, a real load still returns the original target payload",
        excpt.triggered == false &&
        loaded == seed
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after repeated legal prefetch.i/r/w hints, a real store remains the first operation that changes target memory",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t after_store = ld(vaddr);
    TEST_ASSERT("after the real store following repeated legal prefetch hints, the target reads back the store payload",
        excpt.triggered == false &&
        after_store == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated legal prefetch.i/r/w hints preserve target and adjacent backing memory until the explicit store commits",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}
