#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>
#include <stdbool.h>

#define AI_ARP_VEC_BYTES 16U
#define AI_ARP_ROUNDS 3U

#define AI_ARP_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

static inline void ai_arp_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint8_t ai_arp_vec_byte(uint8_t role, unsigned round,
    unsigned offset)
{
    return (uint8_t)(0x4bU ^ (uint8_t)(role * 0x25U) ^
        (uint8_t)(round * 0x31U) ^ (uint8_t)(offset * 0x13U) ^
        (uint8_t)((offset & 3U) * 0x47U));
}

static inline void ai_arp_make_vec(uint8_t *bytes, uint8_t role,
    unsigned round)
{
    for (unsigned i = 0; i < AI_ARP_VEC_BYTES; ++i) {
        bytes[i] = ai_arp_vec_byte(role, round, i);
    }
}

static inline void ai_arp_write_bytes(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_arp_bytes_match(uintptr_t paddr, const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_arp_atomic_load_vector_window(uintptr_t amo_a_vaddr,
    uintptr_t amo_b_vaddr, uintptr_t lrsc_vaddr, uintptr_t load_vaddr,
    uintptr_t vector_src_vaddr, uintptr_t vector_dst_vaddr,
    uintptr_t marker_vaddr, uint64_t amo_a_addend, uint64_t amo_b_addend,
    uint64_t sc_payload, uint64_t marker_value, uint64_t *amo_a_old_out,
    uint64_t *amo_b_old_out, uint64_t *lr_old_out,
    uint64_t *sc_status_out, uint64_t *load_out)
{
    uint64_t amo_a_old = 0ULL;
    uint64_t amo_b_old = 0ULL;
    uint64_t lr_old = 0ULL;
    uint64_t sc_status = 0ULL;
    uint64_t load_value = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "amoadd.d %[amo_a_old], %[amo_a_addend], 0(%[amo_a_addr])\n\t"
        "ld %[load_value], 0(%[load_addr])\n\t"
        "amoadd.d %[amo_b_old], %[amo_b_addend], 0(%[amo_b_addr])\n\t"
        "lr.d %[lr_old], 0(%[lrsc_addr])\n\t"
        "sc.d %[sc_status], %[sc_payload], 0(%[lrsc_addr])\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[vector_src_addr])\n\t"
        "vse8.v v8, (%[vector_dst_addr])\n\t"
        "sd %[amo_a_old], 0(%[marker_addr])\n\t"
        "sd %[load_value], 8(%[marker_addr])\n\t"
        "sd %[amo_b_old], 16(%[marker_addr])\n\t"
        "sd %[lr_old], 24(%[marker_addr])\n\t"
        "sd %[sc_status], 32(%[marker_addr])\n\t"
        "sd %[marker_value], 40(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [amo_a_old] "=&r"(amo_a_old),
          [amo_b_old] "=&r"(amo_b_old),
          [lr_old] "=&r"(lr_old),
          [sc_status] "=&r"(sc_status),
          [load_value] "=&r"(load_value)
        : [amo_a_addr] "r"(amo_a_vaddr),
          [amo_b_addr] "r"(amo_b_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [load_addr] "r"(load_vaddr),
          [vector_src_addr] "r"(vector_src_vaddr),
          [vector_dst_addr] "r"(vector_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [amo_a_addend] "r"(amo_a_addend),
          [amo_b_addend] "r"(amo_b_addend),
          [sc_payload] "r"(sc_payload),
          [marker_value] "r"(marker_value),
          [vl] "r"((int)AI_ARP_VEC_BYTES)
        : "t0", "memory");

    *amo_a_old_out = amo_a_old;
    *amo_b_old_out = amo_b_old;
    *lr_old_out = lr_old;
    *sc_status_out = sc_status;
    *load_out = load_value;
}

bool ai_micro_hs_atomic_load0_vector_lrsc_pressure_no_early_release_or_lost_update_corner()
{
    TEST_START();

    const enum test_page amo_a_page = VSRWX_GRWX;
    const enum test_page amo_b_page = VSRW_GRWX;
    const enum test_page lrsc_page = SWITCH1;
    const enum test_page load_page = SWITCH2;
    const enum test_page vector_src_page = VSRWX_GURWX;
    const enum test_page scratch_page = SCRATCHPAD;
    const uintptr_t amo_a_vaddr = hs_page_base(amo_a_page) + 0xc00U;
    const uintptr_t amo_b_vaddr = hs_page_base(amo_b_page) + 0xc00U;
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0xc00U;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0xc00U;
    const uintptr_t vector_src_vaddr =
        hs_page_base(vector_src_page) + 0xc00U;
    const uintptr_t vector_dst_vaddr = hs_page_base(scratch_page) + 0xc00U;
    const uintptr_t c_store_vaddr = hs_page_base(scratch_page) + 0xc40U;
    const uintptr_t marker_vaddr = hs_page_base(scratch_page) + 0xc80U;
    const uintptr_t amo_a_paddr = phys_page_base(amo_a_page) + 0xc00U;
    const uintptr_t amo_b_paddr = phys_page_base(amo_b_page) + 0xc00U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0xc00U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0xc00U;
    const uintptr_t vector_src_paddr =
        phys_page_base(vector_src_page) + 0xc00U;
    const uintptr_t vector_dst_paddr = phys_page_base(scratch_page) + 0xc00U;
    const uintptr_t c_store_paddr = phys_page_base(scratch_page) + 0xc40U;
    const uintptr_t marker_paddr = phys_page_base(scratch_page) + 0xc80U;
    const uintptr_t marker_guard_paddr = marker_paddr + 6U * sizeof(uint64_t);
    const uint64_t amo_a_seeds[AI_ARP_ROUNDS] = {
        0x1021324354657687ULL,
        0x8877665544332211ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t amo_a_addends[AI_ARP_ROUNDS] = {
        0x0100010001000100ULL,
        0x0001000200030004ULL,
        0x1111111111111111ULL,
    };
    const uint64_t amo_b_seeds[AI_ARP_ROUNDS] = {
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t amo_b_addends[AI_ARP_ROUNDS] = {
        0x0004000300020001ULL,
        0x0102030405060708ULL,
        0x0000ffff00000001ULL,
    };
    const uint64_t lrsc_seeds[AI_ARP_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t sc_payloads[AI_ARP_ROUNDS] = {
        0xa5a55a5af0f00f0fULL,
        0x13579bdf2468ace0ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t load_seeds[AI_ARP_ROUNDS] = {
        0xc001d00dcafebeefULL,
        0xfeedface01234567ULL,
        0xdeadc0def00dbabeULL,
    };
    const uint64_t c_seeds[AI_ARP_ROUNDS] = {
        0x0102030405060708ULL,
        0x8899aabbccddeeffULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t c_updates[AI_ARP_ROUNDS] = {
        0x9081726354453627ULL,
        0x1029384756abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t marker_guards[AI_ARP_ROUNDS] = {
        0xabcdef0123456789ULL,
        0x76543210fedcba98ULL,
        0xfaceb00c13579bdfULL,
    };
    uint64_t original_menvcfg = 0ULL;
    uint64_t original_mstatus = 0ULL;

    goto_priv(PRIV_M);
    original_menvcfg = CSRR(CSR_MENVCFG);
    original_mstatus = CSRR(CSR_MSTATUS);
    CSRW(CSR_MENVCFG, (original_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    ai_arp_enable_vector_state();

    for (unsigned round = 0; round < AI_ARP_ROUNDS; ++round) {
        uint8_t vector_src_bytes[AI_ARP_VEC_BYTES];
        uint8_t vector_dst_fill[AI_ARP_VEC_BYTES];
        const uint64_t amo_a_expected =
            amo_a_seeds[round] + amo_a_addends[round];
        const uint64_t amo_b_expected =
            amo_b_seeds[round] + amo_b_addends[round];
        const uint64_t marker_value =
            0x9e00000000000000ULL | (uint64_t)round;
        uint64_t amo_a_old = 0ULL;
        uint64_t amo_b_old = 0ULL;
        uint64_t lr_old = 0ULL;
        uint64_t sc_status = 0ULL;
        uint64_t load_value = 0ULL;

        ai_arp_make_vec(vector_src_bytes, 0x11U, round);
        ai_arp_make_vec(vector_dst_fill, 0x42U, round);

        goto_priv(PRIV_M);
        write64(amo_a_paddr, amo_a_seeds[round]);
        write64(amo_a_paddr + sizeof(uint64_t), marker_guards[round]);
        write64(amo_b_paddr, amo_b_seeds[round]);
        write64(amo_b_paddr + sizeof(uint64_t), marker_guards[round] ^ 1ULL);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(lrsc_paddr + sizeof(uint64_t), marker_guards[round] ^ 2ULL);
        write64(load_paddr, load_seeds[round]);
        write64(load_paddr + sizeof(uint64_t), marker_guards[round] ^ 3ULL);
        ai_arp_write_bytes(vector_src_paddr, vector_src_bytes,
            AI_ARP_VEC_BYTES);
        ai_arp_write_bytes(vector_dst_paddr, vector_dst_fill,
            AI_ARP_VEC_BYTES);
        write64(c_store_paddr, c_seeds[round]);
        write64(c_store_paddr + sizeof(uint64_t),
            marker_guards[round] ^ 4ULL);
        for (unsigned i = 0; i < 6U; ++i) {
            write64(marker_paddr + i * sizeof(uint64_t), 0ULL);
        }
        write64(marker_guard_paddr, marker_guards[round] ^ 5ULL);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        bool warm_ok =
            ld(amo_a_vaddr) == amo_a_seeds[round] &&
            ld(amo_b_vaddr) == amo_b_seeds[round] &&
            ld(lrsc_vaddr) == lrsc_seeds[round] &&
            ld(load_vaddr) == load_seeds[round] &&
            ld(c_store_vaddr) == c_seeds[round];
        AI_ARP_ASSERT_NO_EXCEPT_AND("atomic release pressure setup warms AMO, LR/SC, scalar-load, compressed-store, and vector pages before the cold atomic window",
            warm_ok
        );

        TEST_SETUP_EXCEPT();
        c_sd(c_store_vaddr, c_updates[round]);
        prefetch_r(load_vaddr);
        prefetch_w(amo_b_vaddr);
        cbo_inval(amo_a_vaddr);
        cbo_inval(amo_b_vaddr);
        cbo_inval(lrsc_vaddr);
        AI_ARP_ASSERT_NO_EXCEPT_AND("compressed store, prefetch, and CBO invalidation pressure retire before the AMO/LRSC/load0/vector window",
            ld(c_store_vaddr) == c_updates[round]
        );

        TEST_SETUP_EXCEPT();
        ai_arp_atomic_load_vector_window(amo_a_vaddr, amo_b_vaddr,
            lrsc_vaddr, load_vaddr, vector_src_vaddr, vector_dst_vaddr,
            marker_vaddr, amo_a_addends[round], amo_b_addends[round],
            sc_payloads[round], marker_value, &amo_a_old, &amo_b_old,
            &lr_old, &sc_status, &load_value);
        AI_ARP_ASSERT_NO_EXCEPT_AND("AMO/LRSC completion pressure keeps atomic writeback, load0, vector writeback, and marker results bound to their own producers",
            amo_a_old == amo_a_seeds[round] &&
            amo_b_old == amo_b_seeds[round] &&
            lr_old == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            load_value == load_seeds[round] &&
            ld(amo_a_vaddr) == amo_a_expected &&
            ld(amo_b_vaddr) == amo_b_expected &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(c_store_vaddr) == c_updates[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("MemBlock atomic release-on-valid smoke keeps AMO A/B, LR/SC, load0, compressed store, vector dump, marker, and guards exactly once under shared writeback pressure",
            read64(amo_a_paddr) == amo_a_expected &&
            read64(amo_a_paddr + sizeof(uint64_t)) ==
                marker_guards[round] &&
            read64(amo_b_paddr) == amo_b_expected &&
            read64(amo_b_paddr + sizeof(uint64_t)) ==
                (marker_guards[round] ^ 1ULL) &&
            read64(lrsc_paddr) == sc_payloads[round] &&
            read64(lrsc_paddr + sizeof(uint64_t)) ==
                (marker_guards[round] ^ 2ULL) &&
            read64(load_paddr) == load_seeds[round] &&
            read64(load_paddr + sizeof(uint64_t)) ==
                (marker_guards[round] ^ 3ULL) &&
            read64(c_store_paddr) == c_updates[round] &&
            read64(c_store_paddr + sizeof(uint64_t)) ==
                (marker_guards[round] ^ 4ULL) &&
            ai_arp_bytes_match(vector_src_paddr, vector_src_bytes,
                AI_ARP_VEC_BYTES) &&
            ai_arp_bytes_match(vector_dst_paddr, vector_src_bytes,
                AI_ARP_VEC_BYTES) &&
            read64(marker_paddr) == amo_a_seeds[round] &&
            read64(marker_paddr + 8U) == load_seeds[round] &&
            read64(marker_paddr + 16U) == amo_b_seeds[round] &&
            read64(marker_paddr + 24U) == lrsc_seeds[round] &&
            read64(marker_paddr + 32U) == 0ULL &&
            read64(marker_paddr + 40U) == marker_value &&
            read64(marker_guard_paddr) == (marker_guards[round] ^ 5ULL)
        );
    }

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, original_menvcfg);
    CSRW(CSR_MSTATUS, original_mstatus);
    hspt_init();
    sfence_vma();

    TEST_END();
}
