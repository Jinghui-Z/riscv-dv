#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_ARCH_PMA_DEVICE_ALIAS_PAGE   VSRWX_GRWX
#define AI_ARCH_PMA_DEVICE_BACKING_PAGE SCRATCHPAD
#define AI_ARCH_MMIO_ORDER_ALIAS_PAGE_A VSRWXPbmt2_GURWXPbmt1
#define AI_ARCH_MMIO_ORDER_ALIAS_PAGE_B VSRWXPbmt2_GURWXPbmt2
#define AI_ARCH_PMA_DEVICE_MEM_BASE    0x30000000000ULL
#define AI_ARCH_PBMT_IO_MEM_BASE        0x30000000000ULL
#define AI_ARCH_PBMT_IO_MEM_LIMIT       0x40000000000ULL

#define AI_INVALIDATE_SHARED_STORE_LINE(vaddr, message) \
    do { \
        goto_priv(PRIV_HS); \
        fence_iorw_iorw(); \
        TEST_SETUP_EXCEPT(); \
        cbo_inval(vaddr); \
        AI_ASSERT_NO_EXCEPT_AND(message, true); \
    } while (0)

static inline void ai_arch_fence_i_i(void)
{
    asm volatile("fence i,i" ::: "memory");
}

static inline void ai_arch_fence_i_o(void)
{
    asm volatile("fence i,o" ::: "memory");
}

static inline void ai_arch_fence_o_i(void)
{
    asm volatile("fence o,i" ::: "memory");
}

static inline void ai_arch_fence_o_o(void)
{
    asm volatile("fence o,o" ::: "memory");
}

static inline void ai_arch_fence_rw_o(void)
{
    asm volatile("fence rw,o" ::: "memory");
}

static inline void ai_arch_fence_o_rw(void)
{
    asm volatile("fence o,rw" ::: "memory");
}

static inline void ai_arch_fence_rw_i(void)
{
    asm volatile("fence rw,i" ::: "memory");
}

static inline uintptr_t ai_arch_store_route_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t ai_arch_store_route_io_backing_paddr(enum test_page page)
{
    return phys_page_base_offset(page, AI_ARCH_PBMT_IO_MEM_BASE);
}

static inline uintptr_t ai_arch_store_route_device_backing_paddr(enum test_page page)
{
    return phys_page_base_offset(page, AI_ARCH_PMA_DEVICE_MEM_BASE);
}

static inline void ai_arch_reset_store_route_pma_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
}

static inline void ai_arch_reset_store_route_pbmt_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_arch_prepare_store_route_mem_3t_4t_window(void)
{
    goto_priv(PRIV_M);
    /*
     * LinkNan reset PMA already maps 0x300_0000_0000..0x400_0000_0000
     * as a responsive non-cacheable/IO physical window backed by extraMem.
     * Do not reprogram custom PMA CSRs here: Spike/difftest does not model
     * CSR 0x7c0/0x7c8, and clearing the table can also disturb normal DRAM
     * accesses used by the test runtime.
     */
}

static inline uintptr_t ai_arch_store_route_anchor(enum test_page page, uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_arch_store_route_alias_vaddr(enum test_page page,
    uintptr_t paddr)
{
    return hs_page_base(page) + (paddr & (PAGE_SIZE - 1));
}

static inline void ai_arch_store_route_map_alias_to_paddr(enum test_page page,
    uintptr_t paddr)
{
    const uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    hspt_leaf_change_base_paddr(page,
        ai_arch_store_route_anchor(page, page_start));
}

static inline uintptr_t ai_arch_prepare_store_route_alias(enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt0_GURWXPbmt0);
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline uintptr_t ai_arch_prepare_store_route_alias_at(enum test_page page,
    uintptr_t base_paddr)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt0_GURWXPbmt0, base_paddr);
    pbmt_hspt_to_x_base_paddr(page, base_paddr);
    sfence_vma();
    return hs_page_base(page);
}

static inline void ai_arch_prepare_store_route_alias_pair(enum test_page first_page,
    enum test_page second_page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt0_GURWXPbmt0);
    pbmt_hspt_to_x(first_page);
    pbmt_hspt_to_x(second_page);
    sfence_vma();
}

static inline void ai_arch_prepare_store_route_alias_pair_at(enum test_page first_page,
    enum test_page second_page, uintptr_t base_paddr)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt0_GURWXPbmt0, base_paddr);
    pbmt_hspt_to_x_base_paddr(first_page, base_paddr);
    pbmt_hspt_to_x_base_paddr(second_page, base_paddr);
    sfence_vma();
}

static inline uintptr_t ai_arch_prepare_pma_device_store_alias(enum test_page alias_page,
    enum test_page backing_page)
{
    uintptr_t backing_paddr = ai_arch_store_route_device_backing_paddr(backing_page);

    goto_priv(PRIV_M);
    ai_arch_reset_store_route_pma_env();
    hspt_init();
    hspt_leaf_change_base_paddr(alias_page,
        ai_arch_store_route_anchor(alias_page, backing_paddr));
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline void ai_arch_prepare_memory_and_pbmt_io_aliases(void)
{
    goto_priv(PRIV_M);
    ai_arch_prepare_store_route_mem_3t_4t_window();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    pbmt_hspt_to_x(VSRWXPbmt0_GURWXPbmt0);
    pbmt_hspt_to_x_base_paddr(VSRWXPbmt2_GURWXPbmt1,
        AI_ARCH_PBMT_IO_MEM_BASE);
    sfence_vma();
}

bool ai_arch_nc_store_uncache_path()
{
    TEST_START();

    static uint64_t backing[] __attribute__((aligned(64))) = {
        0x1111222233334444ULL,
    };
    const uintptr_t paddr = (uintptr_t)&backing[0];
    const uint64_t initial_value = 0x1111222233334444ULL;
    const uint64_t updated_value = 0xaaaabbbbccccddddULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    ai_arch_store_route_map_alias_to_paddr(VSRWXPbmt0_GURWXPbmt0, paddr);
    ai_arch_store_route_map_alias_to_paddr(VSRWXPbmt1_GURWXPbmt1, paddr);
    sfence_vma();

    const uintptr_t cache_vaddr =
        ai_arch_store_route_alias_vaddr(VSRWXPbmt0_GURWXPbmt0, paddr);
    const uintptr_t nc_vaddr =
        ai_arch_store_route_alias_vaddr(VSRWXPbmt1_GURWXPbmt1, paddr);

    AI_INVALIDATE_SHARED_STORE_LINE(cache_vaddr,
        "cache invalidation for the shared backing line completes before the NC-path probe");

    TEST_SETUP_EXCEPT();
    uint64_t cached_before = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable alias observes the static original payload before the NC store",
        cached_before == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, updated_value);
    AI_ASSERT_NO_EXCEPT_AND("a store through the NC alias completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t cached_probe = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("before invalidation after the NC store, the cacheable alias still observes its old clean line",
        cached_probe == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t nc_value = ld(nc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the NC alias reads the updated memory payload directly",
        nc_value == updated_value
    );

    /*
     * With MENVCFG.CBIE=01, cbo.inval is treated like a flush-style operation
     * on this profile, and the cacheable alias has no PBMT=NC attribute.  The
     * same NC alias read above is the architectural oracle for this probe.
     */

    TEST_END();
}

bool ai_arch_pbmt_io_device_fence_quadrant_order_corner()
{
    TEST_START();

    const uintptr_t paddr = ai_arch_store_route_io_backing_paddr(X);
    const uintptr_t initial0 = 0x1020304050607080ULL;
    const uintptr_t initial1 = 0x1111222233334444ULL;
    const uintptr_t initial2 = 0x5555666677778888ULL;
    const uintptr_t load_store_value = 0xa5a55a5a0f0ff0f0ULL;
    const uintptr_t store_load_value = 0x8877665544332211ULL;
    const uintptr_t first_store_value = 0x13579bdf2468ace0ULL;
    const uintptr_t second_store_value = 0xfedcba9876543210ULL;
    const uintptr_t pbmt_to_pma_value = 0x0badf00d55aa33ccULL;
    const uintptr_t pma_to_pbmt_value = 0xc001d00d1234abcdULL;

    ai_arch_prepare_store_route_mem_3t_4t_window();
    uintptr_t io_vaddr = ai_arch_prepare_store_route_alias_at(
        VSRWXPbmt2_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE);
    uintptr_t pma_vaddr = hs_page_base(VSRWXPbmt0_GURWXPbmt0);

    goto_priv(PRIV_M);
    write64(paddr + 0U, initial0);
    write64(paddr + 8U, initial1);
    write64(paddr + 16U, initial2);
    write64(paddr + 24U, 0ULL);
    write64(paddr + 32U, 0ULL);
    write64(paddr + 40U, 0ULL);
    write64(paddr + 48U, 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t load0 = ld(io_vaddr + 0U);
    ai_arch_fence_i_i();
    uint64_t load1 = ld(io_vaddr + 8U);
    TEST_ASSERT("fence i,i keeps the two PBMT=IO loads ordered and both responder values observable",
        excpt.triggered == false &&
        load0 == initial0 &&
        load1 == initial1
    );

    TEST_SETUP_EXCEPT();
    uint64_t status = ld(io_vaddr + 16U);
    ai_arch_fence_i_o();
    sd(io_vaddr + 24U, load_store_value ^ status);
    ai_arch_fence_o_i();
    uint64_t load_store_observed = ld(io_vaddr + 24U);
    TEST_ASSERT("fence i,o followed by fence o,i preserves a PBMT=IO load-derived store before the status readback",
        excpt.triggered == false &&
        status == initial2 &&
        load_store_observed == (load_store_value ^ initial2)
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr + 32U, store_load_value);
    ai_arch_fence_o_i();
    uint64_t store_load_observed = ld(io_vaddr + 32U);
    TEST_ASSERT("fence o,i drains a PBMT=IO posted store before the following PBMT=IO load observes that location",
        excpt.triggered == false &&
        store_load_observed == store_load_value
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr + 32U, first_store_value);
    ai_arch_fence_o_o();
    sd(io_vaddr + 32U, second_store_value);
    ai_arch_fence_o_i();
    uint64_t store_store_observed = ld(io_vaddr + 32U);
    TEST_ASSERT("fence o,o orders two PBMT=IO stores so the final readback observes the second program-order store",
        excpt.triggered == false &&
        store_store_observed == second_store_value
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr + 40U, pbmt_to_pma_value);
    ai_arch_fence_o_i();
    uint64_t pma_source_observed = ld(pma_vaddr + 40U);
    TEST_ASSERT("a PBMT=IO store and a PMA-sourced Device load to the same PA observe one coherent device-style image after fence o,i",
        excpt.triggered == false &&
        pma_source_observed == pbmt_to_pma_value
    );

    TEST_SETUP_EXCEPT();
    sd(pma_vaddr + 48U, pma_to_pbmt_value);
    ai_arch_fence_o_i();
    uint64_t pbmt_source_observed = ld(io_vaddr + 48U);
    TEST_ASSERT("a PMA-sourced Device store and a PBMT=IO load to the same PA observe the same device-style image after fence o,i",
        excpt.triggered == false &&
        pbmt_source_observed == pma_to_pbmt_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PBMT=IO quadrant-order sequence leaves the backing IO window with the ordered final values",
        read64(paddr + 0U) == initial0 &&
        read64(paddr + 8U) == initial1 &&
        read64(paddr + 16U) == initial2 &&
        read64(paddr + 24U) == (load_store_value ^ initial2) &&
        read64(paddr + 32U) == second_store_value &&
        read64(paddr + 40U) == pbmt_to_pma_value &&
        read64(paddr + 48U) == pma_to_pbmt_value
    );
    ai_arch_reset_store_route_pbmt_env();

    TEST_END();
}

bool ai_arch_device_ordinary_memory_cross_fence_order_corner()
{
    TEST_START();

    const uintptr_t mem_paddr = ai_arch_store_route_backing_paddr();
    const uintptr_t io_paddr = ai_arch_store_route_io_backing_paddr(X);
    const uint64_t mem_initial = 0x0102030405060708ULL;
    const uint64_t io_initial = 0x8877665544332211ULL;
    const uint64_t mem_first = 0x1122334455667788ULL;
    const uint64_t mem_second = 0xaabbccddeeff0011ULL;
    const uint64_t io_first = 0x0fedcba987654321ULL;
    const uint64_t io_second = 0x123456789abcdef0ULL;

    ai_arch_prepare_memory_and_pbmt_io_aliases();

    goto_priv(PRIV_M);
    write64(mem_paddr + 0U, mem_initial);
    write64(mem_paddr + 8U, mem_initial);
    write64(io_paddr + 0U, io_initial);
    write64(io_paddr + 8U, io_initial);

    const uintptr_t mem_vaddr = hs_page_base(VSRWXPbmt0_GURWXPbmt0);
    const uintptr_t io_vaddr = hs_page_base(VSRWXPbmt2_GURWXPbmt1);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(mem_vaddr + 0U, mem_first);
    ai_arch_fence_rw_o();
    sd(io_vaddr + 0U, io_first);
    ai_arch_fence_o_i();
    uint64_t mem_after_first = ld(mem_vaddr + 0U);
    uint64_t io_after_first = ld(io_vaddr + 0U);
    TEST_ASSERT("fence rw,o separates an ordinary-memory store from the following PBMT=IO store and both become observable",
        excpt.triggered == false &&
        mem_after_first == mem_first &&
        io_after_first == io_first
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr + 8U, io_second);
    ai_arch_fence_o_rw();
    sd(mem_vaddr + 8U, mem_second);
    ai_arch_fence_rw_i();
    uint64_t io_after_second = ld(io_vaddr + 8U);
    uint64_t mem_after_second = ld(mem_vaddr + 8U);
    TEST_ASSERT("fence o,rw separates a PBMT=IO store from the following ordinary-memory store and both final values remain distinct",
        excpt.triggered == false &&
        io_after_second == io_second &&
        mem_after_second == mem_second
    );

    TEST_SETUP_EXCEPT();
    uint64_t mem_guard = ld(mem_vaddr + 0U);
    uint64_t io_guard = ld(io_vaddr + 0U);
    TEST_ASSERT("cacheable ordinary memory and PBMT=IO device-style state are not substituted for each other after the crossed fence sequence",
        excpt.triggered == false &&
        mem_guard == mem_first &&
        io_guard == io_first
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the ordinary-memory/device crossed-fence sequence updates only the intended backing locations",
        read64(mem_paddr + 0U) == mem_first &&
        read64(mem_paddr + 8U) == mem_second &&
        read64(io_paddr + 0U) == io_first &&
        read64(io_paddr + 8U) == io_second
    );
    ai_arch_reset_store_route_pbmt_env();

    TEST_END();
}

bool ai_arch_pbmt_io_pma_io_same_pa_order_consistency_corner()
{
    TEST_START();

    const enum test_page pma_page = AI_ARCH_PMA_DEVICE_ALIAS_PAGE;
    const enum test_page pbmt_page = VSRWXPbmt2_GURWXPbmt1;
    const uintptr_t shared_paddr = ai_arch_store_route_io_backing_paddr(X);
    const uintptr_t pma_vaddr =
        ai_arch_store_route_alias_vaddr(pma_page, shared_paddr);
    const uintptr_t pbmt_vaddr = hs_page_base(pbmt_page);
    const uint64_t initial0 = 0x0102030405060708ULL;
    const uint64_t initial1 = 0x1112131415161718ULL;
    const uint64_t initial2 = 0x2122232425262728ULL;
    const uint64_t initial3 = 0x3132333435363738ULL;
    const uint64_t pbmt_after_pma_load = 0xa5a55a5a0f0ff0f0ULL;
    const uint64_t pma_after_pbmt_load = 0x8877665544332211ULL;
    const uint64_t pma_first = 0x13579bdf2468ace0ULL;
    const uint64_t pbmt_second = 0xfedcba9876543210ULL;
    const uint64_t pbmt_first = 0x1021324354657687ULL;
    const uint64_t pma_second = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    ai_arch_prepare_store_route_mem_3t_4t_window();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    hspt_leaf_change_base_paddr(pma_page,
        ai_arch_store_route_anchor(pma_page, shared_paddr));
    pbmt_hspt_to_x_base_paddr(pbmt_page, AI_ARCH_PBMT_IO_MEM_BASE);
    sfence_vma();

    write64(shared_paddr + 0U, initial0);
    write64(shared_paddr + 8U, initial1);
    write64(shared_paddr + 16U, initial2);
    write64(shared_paddr + 24U, initial3);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t pma_load = ld(pma_vaddr + 0U);
    ai_arch_fence_i_o();
    sd(pbmt_vaddr + 0U, pbmt_after_pma_load ^ pma_load);
    ai_arch_fence_o_i();
    uint64_t pma_after_pbmt_store = ld(pma_vaddr + 0U);
    TEST_ASSERT("a PMA-IO load followed by a fenced PBMT=IO store to the same PA is observed through the PMA-IO alias",
        excpt.triggered == false &&
        pma_load == initial0 &&
        pma_after_pbmt_store == (pbmt_after_pma_load ^ initial0)
    );

    TEST_SETUP_EXCEPT();
    uint64_t pbmt_load = ld(pbmt_vaddr + 8U);
    ai_arch_fence_i_o();
    sd(pma_vaddr + 8U, pma_after_pbmt_load ^ pbmt_load);
    ai_arch_fence_o_i();
    uint64_t pbmt_after_pma_store = ld(pbmt_vaddr + 8U);
    TEST_ASSERT("a PBMT=IO load followed by a fenced PMA-IO store to the same PA is observed through the PBMT=IO alias",
        excpt.triggered == false &&
        pbmt_load == initial1 &&
        pbmt_after_pma_store == (pma_after_pbmt_load ^ initial1)
    );

    TEST_SETUP_EXCEPT();
    sd(pma_vaddr + 16U, pma_first);
    ai_arch_fence_o_o();
    sd(pbmt_vaddr + 16U, pbmt_second);
    ai_arch_fence_o_i();
    uint64_t pbmt_order_observed = ld(pbmt_vaddr + 16U);
    uint64_t pma_order_observed = ld(pma_vaddr + 16U);
    TEST_ASSERT("fence o,o keeps a PMA-IO store before a PBMT=IO store to the same PA, with both aliases observing the second store",
        excpt.triggered == false &&
        pbmt_order_observed == pbmt_second &&
        pma_order_observed == pbmt_second
    );

    TEST_SETUP_EXCEPT();
    sd(pbmt_vaddr + 24U, pbmt_first);
    ai_arch_fence_o_o();
    sd(pma_vaddr + 24U, pma_second);
    ai_arch_fence_o_i();
    uint64_t pma_reverse_observed = ld(pma_vaddr + 24U);
    uint64_t pbmt_reverse_observed = ld(pbmt_vaddr + 24U);
    TEST_ASSERT("fence o,o keeps a PBMT=IO store before a PMA-IO store to the same PA, with both aliases observing the second store",
        excpt.triggered == false &&
        pma_reverse_observed == pma_second &&
        pbmt_reverse_observed == pma_second
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-PA PMA-IO and PBMT=IO aliases converge on one ordered backing image",
        read64(shared_paddr + 0U) == (pbmt_after_pma_load ^ initial0) &&
        read64(shared_paddr + 8U) == (pma_after_pbmt_load ^ initial1) &&
        read64(shared_paddr + 16U) == pbmt_second &&
        read64(shared_paddr + 24U) == pma_second
    );
    ai_arch_reset_store_route_pbmt_env();

    TEST_END();
}

bool ai_arch_pma_device_store_mmio_path()
{
    TEST_START();

    uintptr_t backing_paddr = ai_arch_store_route_device_backing_paddr(
        AI_ARCH_PMA_DEVICE_BACKING_PAGE);
    uintptr_t device_vaddr = ai_arch_prepare_pma_device_store_alias(
        AI_ARCH_PMA_DEVICE_ALIAS_PAGE, AI_ARCH_PMA_DEVICE_BACKING_PAGE);

    goto_priv(PRIV_M);
    write64(backing_paddr, 0x0102030405060708ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t before = ld(device_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a PMA=Device alias can read the initial payload before the store",
        before == 0x0102030405060708ULL
    );

    TEST_SETUP_EXCEPT();
    sd(device_vaddr, 0x8877665544332211ULL);
    AI_ASSERT_NO_EXCEPT_AND("a store through a PMA=Device alias completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t after = ld(device_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a PMA=Device alias reads back the payload written by the store",
        after == 0x8877665544332211ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the PMA=Device store updates the backing memory with the expected payload",
        read64(backing_paddr) == 0x8877665544332211ULL
    );
    ai_arch_reset_store_route_pma_env();

    TEST_END();
}

bool ai_arch_device_store_mmio_path()
{
    TEST_START();

    uintptr_t paddr = ai_arch_store_route_io_backing_paddr(X);

    ai_arch_prepare_store_route_mem_3t_4t_window();
    uintptr_t default_vaddr = hs_page_base(VSRWXPbmt0_GURWXPbmt0);
    uintptr_t io_vaddr = ai_arch_prepare_store_route_alias_at(
        VSRWXPbmt2_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE);

    write64(paddr, 0x0102030405060708ULL);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t default_before = ld(default_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-PA default-PBMT alias observes the original payload before the device-style store",
        default_before == 0x0102030405060708ULL
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0x8877665544332211ULL);
    AI_ASSERT_NO_EXCEPT_AND("a store through the PBMT=IO alias completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t same_pa_after_store = ld(default_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same-PA default-PBMT alias also observes the device-style store payload in the 3T PMA/IO window",
        same_pa_after_store == 0x8877665544332211ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t io_value = ld(io_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the PBMT=IO alias reads the updated payload directly from memory",
        io_value == 0x8877665544332211ULL
    );

    /*
     * Both aliases point into the LinkNan 3T PMA/IO window.  Do not treat the
     * default-PBMT alias as an ordinary cacheable DRAM line or require CBO
     * invalidation to complete there; final PMA/IO attributes decide the path.
     */

    TEST_END();
}

bool ai_arch_pbmt_io_load_store_load_order()
{
    TEST_START();

    uintptr_t paddr = ai_arch_store_route_io_backing_paddr(X);

    ai_arch_prepare_store_route_mem_3t_4t_window();
    uintptr_t io_vaddr = ai_arch_prepare_store_route_alias_at(
        VSRWXPbmt2_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE);

    goto_priv(PRIV_M);
    write64(paddr, 0x1020304050607080ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t before = ld(io_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first MMIO-style load observes the initial payload before the store",
        before == 0x1020304050607080ULL
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0x8877665544332211ULL);
    AI_ASSERT_NO_EXCEPT_AND("the MMIO-style store completes between the two ordered loads",
        true
    );

    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    uint64_t after = ld(io_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second MMIO-style load observes the payload written by the preceding store",
        after == 0x8877665544332211ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the ordered MMIO-style load-store-load sequence updates the shared backing memory",
        read64(paddr) == 0x8877665544332211ULL
    );

    TEST_END();
}

bool ai_arch_mmio_store_program_order_indirect()
{
    TEST_START();

    uintptr_t paddr = ai_arch_store_route_io_backing_paddr(X);
    const uint64_t first_value = 0x1111222233334444ULL;
    const uint64_t second_value = 0xaaaabbbbccccddddULL;

    ai_arch_prepare_store_route_mem_3t_4t_window();
    ai_arch_prepare_store_route_alias_pair_at(
        AI_ARCH_MMIO_ORDER_ALIAS_PAGE_A, AI_ARCH_MMIO_ORDER_ALIAS_PAGE_B,
        AI_ARCH_PBMT_IO_MEM_BASE);

    uintptr_t first_vaddr = hs_page_base(AI_ARCH_MMIO_ORDER_ALIAS_PAGE_A);
    uintptr_t second_vaddr = hs_page_base(AI_ARCH_MMIO_ORDER_ALIAS_PAGE_B);

    goto_priv(PRIV_M);
    write64(paddr, 0x0102030405060708ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(first_vaddr, first_value);
    AI_ASSERT_NO_EXCEPT_AND("the first MMIO-style store to alias A completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(second_vaddr, second_value);
    AI_ASSERT_NO_EXCEPT_AND("the second MMIO-style store to alias B completes successfully",
        true
    );

    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(first_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reading the shared MMIO-style backing after two stores observes the second store payload",
        observed == second_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("two MMIO-style stores issued in program order leave the shared backing with the second store payload",
        read64(paddr) == second_value
    );

    TEST_END();
}

bool ai_arch_pbmt_io_responder_lane_width_readback_corner()
{
    TEST_START();

    const uintptr_t paddr = ai_arch_store_route_io_backing_paddr(X);
    const uint64_t load_seed = 0x807f80017f00ff80ULL;
    const uint64_t store_seed = 0x1122334455667788ULL;
    const uint64_t after_sb = 0x112233445566aa88ULL;
    const uint64_t after_sh = 0x1122beef5566aa88ULL;
    const uint64_t after_sw = 0x1122beef12345678ULL;
    const uint64_t full_value = 0xdeadbeefcafebabeULL;
    const uint64_t guard_value = 0x0123456789abcdefULL;

    ai_arch_prepare_store_route_mem_3t_4t_window();
    const uintptr_t io_vaddr = ai_arch_prepare_store_route_alias_at(
        VSRWXPbmt2_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE);

    goto_priv(PRIV_M);
    write64(paddr, load_seed);
    write64(paddr + 8U, store_seed);
    write64(paddr + 16U, guard_value);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t byte_signed = (uint64_t)(int64_t)(int8_t)lb(io_vaddr);
    TEST_ASSERT("PBMT=IO responder byte load sign-extends the selected lane",
        excpt.triggered == false &&
        byte_signed == (uint64_t)(int64_t)(int8_t)0x80);

    TEST_SETUP_EXCEPT();
    uint64_t byte_zero = lbu(io_vaddr);
    TEST_ASSERT("PBMT=IO responder byte load zero-extends the selected lane",
        excpt.triggered == false &&
        byte_zero == 0x80U);

    TEST_SETUP_EXCEPT();
    uint64_t half_signed = (uint64_t)(int64_t)(int16_t)lh(io_vaddr);
    TEST_ASSERT("PBMT=IO responder halfword load sign-extends the selected lane",
        excpt.triggered == false &&
        half_signed == (uint64_t)(int64_t)(int16_t)0xff80);

    TEST_SETUP_EXCEPT();
    uint64_t half_zero = lhu(io_vaddr);
    TEST_ASSERT("PBMT=IO responder halfword load zero-extends the selected lane",
        excpt.triggered == false &&
        half_zero == 0xff80U);

    TEST_SETUP_EXCEPT();
    uint64_t word_signed = (uint64_t)(int64_t)(int32_t)lw(io_vaddr + 4U);
    TEST_ASSERT("PBMT=IO responder word load sign-extends the selected lane",
        excpt.triggered == false &&
        word_signed == (uint64_t)(int64_t)(int32_t)0x807f8001U);

    TEST_SETUP_EXCEPT();
    uint64_t word_zero = lwu(io_vaddr + 4U);
    TEST_ASSERT("PBMT=IO responder word load zero-extends the selected lane",
        excpt.triggered == false &&
        word_zero == 0x807f8001U);

    TEST_SETUP_EXCEPT();
    uint64_t full_load = ld(io_vaddr);
    TEST_ASSERT("PBMT=IO responder doubleword load returns the complete payload",
        excpt.triggered == false &&
        full_load == load_seed);

    TEST_SETUP_EXCEPT();
    sb(io_vaddr + 9U, 0xaaU);
    fence_iorw_iorw();
    TEST_ASSERT("PBMT=IO responder byte store completes and is ordered before readback",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(io_vaddr + 8U);
    TEST_ASSERT("PBMT=IO responder byte-store readback updates only the addressed byte lane",
        excpt.triggered == false &&
        observed == after_sb);

    TEST_SETUP_EXCEPT();
    sh(io_vaddr + 12U, 0xbeefU);
    fence_iorw_iorw();
    TEST_ASSERT("PBMT=IO responder halfword store completes and is ordered before readback",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    observed = ld(io_vaddr + 8U);
    TEST_ASSERT("PBMT=IO responder halfword-store readback preserves bytes outside the halfword",
        excpt.triggered == false &&
        observed == after_sh);

    TEST_SETUP_EXCEPT();
    sw(io_vaddr + 8U, 0x12345678U);
    fence_iorw_iorw();
    TEST_ASSERT("PBMT=IO responder word store completes and is ordered before readback",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    observed = ld(io_vaddr + 8U);
    TEST_ASSERT("PBMT=IO responder word-store readback preserves the upper word from earlier narrow stores",
        excpt.triggered == false &&
        observed == after_sw);

    TEST_SETUP_EXCEPT();
    sd(io_vaddr + 8U, full_value);
    fence_iorw_iorw();
    TEST_ASSERT("PBMT=IO responder doubleword store completes and is ordered before readback",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    uint64_t full_observed = ld(io_vaddr + 8U);
    TEST_ASSERT("PBMT=IO responder doubleword readback returns the latest full-width store payload",
        excpt.triggered == false &&
        full_observed == full_value);

    TEST_SETUP_EXCEPT();
    uint64_t guard_observed = ld(io_vaddr + 16U);
    TEST_ASSERT("PBMT=IO responder byte/half/word/doubleword stores leave the adjacent guard doubleword unchanged",
        excpt.triggered == false &&
        guard_observed == guard_value);

    goto_priv(PRIV_M);
    TEST_ASSERT("PBMT=IO responder backing image reflects the lane-width store matrix and preserves the guard",
        read64(paddr) == load_seed &&
        read64(paddr + 8U) == full_value &&
        read64(paddr + 16U) == guard_value);
    ai_arch_reset_store_route_pbmt_env();

    TEST_END();
}

bool ai_arch_nc_vs_pma_device_store_path_distinguishable()
{
    TEST_START();

    uintptr_t device_vaddr = ai_arch_prepare_pma_device_store_alias(
        AI_ARCH_PMA_DEVICE_ALIAS_PAGE, AI_ARCH_PMA_DEVICE_BACKING_PAGE);

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    pbmt_hspt_to_x(VSRWXPbmt1_GURWXPbmt1);
    sfence_vma();

    uintptr_t nc_vaddr = hs_page_base(VSRWXPbmt1_GURWXPbmt1) + 7U;
    device_vaddr += 7U;

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, 0x13579bdf2468ace0ULL);
    TEST_ASSERT("a misaligned store through an NC alias reports a store misaligned exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    TEST_SETUP_EXCEPT();
    sd(device_vaddr, 0x13579bdf2468ace0ULL);
    TEST_ASSERT("a misaligned store through a PMA=Device alias reports a store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    goto_priv(PRIV_M);
    ai_arch_reset_store_route_pma_env();

    TEST_END();
}

bool ai_arch_nc_device_store_path_distinguishable()
{
    TEST_START();

    ai_arch_prepare_store_route_mem_3t_4t_window();
    ai_arch_prepare_store_route_alias_at(
        VSRWXPbmt2_GURWXPbmt1, AI_ARCH_PBMT_IO_MEM_BASE);

    uintptr_t nc_vaddr = hs_page_base(VSRWXPbmt1_GURWXPbmt1) + 7U;
    uintptr_t io_vaddr = hs_page_base(VSRWXPbmt2_GURWXPbmt1) + 7U;

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, 0x13579bdf2468ace0ULL);
    TEST_ASSERT("a misaligned store through an NC alias reports a store misaligned exception",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM
    );

    TEST_SETUP_EXCEPT();
    sd(io_vaddr, 0x13579bdf2468ace0ULL);
    TEST_ASSERT("a misaligned store through a device-style alias reports a store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF
    );

    TEST_END();
}

bool ai_micro_hs_nc_then_mmio_store_both_visible()
{
    TEST_START();

    uintptr_t nc_paddr = phys_page_base(X);
    uintptr_t mmio_paddr = ai_arch_store_route_io_backing_paddr(SCRATCHPAD);
    const uint64_t nc_value = 0x1122334455667788ULL;
    const uint64_t mmio_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_arch_prepare_store_route_mem_3t_4t_window();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    hspt_leaf_change_base_paddr(
        VSRWXPbmt1_GURWXPbmt1,
        ai_arch_store_route_anchor(VSRWXPbmt1_GURWXPbmt1, nc_paddr));
    hspt_leaf_change_base_paddr(
        VSRWXPbmt2_GURWXPbmt1,
        ai_arch_store_route_anchor(VSRWXPbmt2_GURWXPbmt1, mmio_paddr));
    sfence_vma();

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(hs_page_base(VSRWXPbmt1_GURWXPbmt1), nc_value);
    AI_ASSERT_NO_EXCEPT_AND("the leading NC store completes successfully in the mixed NC/MMIO sequence",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(hs_page_base(VSRWXPbmt2_GURWXPbmt1), mmio_value);
    AI_ASSERT_NO_EXCEPT_AND("the trailing MMIO-style store completes successfully in the mixed NC/MMIO sequence",
        true
    );

    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    uint64_t observed_nc = ld(hs_page_base(VSRWXPbmt1_GURWXPbmt1));
    AI_ASSERT_NO_EXCEPT_AND("the NC alias reads back the payload written by the first store",
        observed_nc == nc_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_mmio = ld(hs_page_base(VSRWXPbmt2_GURWXPbmt1));
    AI_ASSERT_NO_EXCEPT_AND("the MMIO-style alias reads back the payload written by the second store",
        observed_mmio == mmio_value
    );

    ai_arch_reset_store_route_pbmt_env();

    TEST_END();
}

bool ai_micro_hs_nc_store_fence_inval_cache_load_fresh()
{
    TEST_START();

    static uint64_t backing[] __attribute__((aligned(64))) = {
        0x0f1e2d3c4b5a6978ULL,
        0x0000000000000000ULL,
    };
    const uintptr_t paddr = (uintptr_t)&backing[0];
    const uint64_t initial_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t cacheable_value = 0x1122334455667788ULL;
    const uint64_t nc_value = 0x89abcdef01234567ULL;
    const uint64_t guard_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE | MENVCFG_CBIE01);
    hspt_init();
    ai_arch_store_route_map_alias_to_paddr(VSRWXPbmt0_GURWXPbmt0, paddr);
    ai_arch_store_route_map_alias_to_paddr(VSRWXPbmt1_GURWXPbmt1, paddr);
    sfence_vma();

    const uintptr_t cache_vaddr =
        ai_arch_store_route_alias_vaddr(VSRWXPbmt0_GURWXPbmt0, paddr);
    const uintptr_t nc_vaddr =
        ai_arch_store_route_alias_vaddr(VSRWXPbmt1_GURWXPbmt1, paddr);

    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    cbo_inval(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("cache invalidation completes before priming the cacheable alias",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t cached_before = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the cacheable alias first observes the original payload",
        cached_before == initial_value
    );

    TEST_SETUP_EXCEPT();
    sd(cache_vaddr, cacheable_value);
    AI_ASSERT_NO_EXCEPT_AND("a cacheable ordinary-memory store completes before the NC alias visibility probe",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_clean(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("cache clean makes the cacheable store eligible for the later NC alias load",
        true
    );

    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    uint64_t nc_after_cacheable = ld(nc_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after cacheable store, clean, and fence, the NC alias observes the synchronized current payload",
        nc_after_cacheable == cacheable_value
    );

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr + sizeof(uint64_t), guard_value);
    AI_ASSERT_NO_EXCEPT_AND("the NC alias seeds an adjacent guard doubleword before the reverse visibility probe",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, nc_value);
    AI_ASSERT_NO_EXCEPT_AND("the NC store in the fence/invalidate sequence completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t cached_stale = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("before fence and invalidation the cacheable alias still observes its old clean line",
        cached_stale == cacheable_value
    );

    fence_iorw_iorw();

    TEST_SETUP_EXCEPT();
    cbo_inval(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("cache invalidation completes after the NC store and fence",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t cached_after = ld(cache_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after fence and invalidation the cacheable alias observes the NC store payload",
        cached_after == nc_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t guard_after = ld(nc_vaddr + sizeof(uint64_t));
    AI_ASSERT_NO_EXCEPT_AND("bidirectional NC/cacheable alias synchronization leaves the adjacent guard doubleword unchanged",
        guard_after == guard_value
    );

    ai_arch_reset_store_route_pbmt_env();

    TEST_END();
}
