#include "ai_micro_mmode_memblock_common.h"

bool ai_micro_m_load_pbmt_narrow_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_offsets[] = { 7U, 6U, 4U };
    static const bool first_signed[] = { true, false, true };
    static const unsigned final_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned final_offsets[] = { 6U, 4U, 7U };
    static const bool final_signed[] = { false, true, false };
    static const uintptr_t scratch_offsets[] = { 0x180U, 0x1c0U, 0x200U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    /*
     * Keep this matrix on PBMT=NC.  PBMT=IO refresh/stale-exception coverage is
     * handled by the dedicated IO cases above; mixing it into the narrow decode
     * matrix pushes Spike into unsupported PMA CSR paths and can leave RTL
     * regressions waiting on IO/device response rather than testing decode
     * refresh.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

        for (unsigned round = 0; round < 3U; ++round) {
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, scratch_offsets[round]);
            const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
                ((bad_paddr + first_offsets[round]) & (PAGE_SIZE - 1U));
            const uintptr_t final_bad_vaddr = hs_page_base(alias_page) +
                ((bad_paddr + final_offsets[round]) & (PAGE_SIZE - 1U));
            const uint64_t expected_middle =
                ai_micro_m_memblock_extract_scalar(good_values[round],
                    final_offsets[round], final_sizes[round],
                    final_signed[round]);
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();

            ai_micro_m_memblock_prepare_pbmt_translation();
            TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
                ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                    alias_page, good_paddr, good_values[round])
            );
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                first_signed[round], first_sizes[round], first_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("the first no-H PBMT same-producer same-alias narrow load fault should report a precise bad-alias load access fault at the original load shape before the local width/decode refresh probe",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == first_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed_middle =
                ai_micro_m_memblock_translated_scalar_load(final_signed[round],
                    final_sizes[round], good_vaddr + final_offsets[round]);
            TEST_ASSERT("after the first no-H PBMT same-producer bad-alias load fault, retargeting the same alias to a good page and switching width/decode must let the middle good load return the value selected by its current local decode",
                excpt.triggered == false &&
                observed_middle == expected_middle
            );

            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                final_signed[round], final_sizes[round], final_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("after the middle good no-H PBMT same-producer load has already completed with the switched width/decode, returning to the same bad alias must refresh tval to the current load shape instead of reusing the first bad episode metadata",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == final_bad_vaddr
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("the no-H PBMT same-producer load width/decode refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
                read64(good_paddr) == good_values[round]
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_offsets[] = { 7U, 2U, 4U };
    static const unsigned final_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned final_offsets[] = { 2U, 4U, 7U };
    static const uintptr_t good_offsets[] = { 0x380U, 0x3c0U, 0x400U };
    static const uint64_t initial_goods[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
        0x3837363534333231ULL,
    };
    static const uint64_t final_store_values[] = {
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
        0x00000000000000a5ULL,
    };

    /*
     * Keep this matrix on PBMT=NC and seed/read it through the same alias.
     * The case checks stale exception metadata and local store-width refresh;
     * it is not a cacheable-to-uncache convergence test, and PBMT=IO needs
     * dedicated responder-backed IO cases.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

        for (unsigned round = 0; round < 3U; ++round) {
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, good_offsets[round]);
            const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
                ((bad_paddr + first_offsets[round]) & (PAGE_SIZE - 1U));
            const uintptr_t final_bad_vaddr = hs_page_base(alias_page) +
                ((bad_paddr + final_offsets[round]) & (PAGE_SIZE - 1U));
            const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
                initial_goods[round], final_store_values[round],
                final_offsets[round], final_sizes[round]);
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();

            ai_micro_m_memblock_prepare_pbmt_translation();
            TEST_ASSERT("PBMT setup store seeds the later good-store backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
                ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                    alias_page, good_paddr, initial_goods[round])
            );
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                bad_paddr);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_store_by_size(first_bad_vaddr,
                first_sizes[round], final_store_values[round]);
            TEST_ASSERT("the first no-H PBMT same-producer same-alias store fault should report a precise bad-alias store access fault at the original store shape before the local width refresh probe",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SAF &&
                excpt.tval == first_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_store_by_size(good_vaddr +
                final_offsets[round], final_sizes[round],
                final_store_values[round]);
            TEST_ASSERT("after the first no-H PBMT same-producer bad-alias store fault, retargeting the same alias to a good page and switching width must let the middle good store commit with its current local store template",
                excpt.triggered == false
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_middle =
                ai_micro_m_memblock_translated_scalar_load(false,
                    sizeof(uint64_t), good_vaddr);
            TEST_ASSERT("the middle good no-H PBMT same-producer width-switched store reads back with exactly the current byte merge through the retargeted PBMT alias",
                excpt.triggered == false &&
                observed_middle == expected_middle
            );

            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                bad_paddr);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_store_by_size(final_bad_vaddr,
                final_sizes[round], final_store_values[round]);
            TEST_ASSERT("after the middle good no-H PBMT same-producer store has already completed with the switched width, returning to the same bad alias must refresh tval to the current store shape instead of reusing the first bad episode metadata",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SAF &&
                excpt.tval == final_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed_after =
                ai_micro_m_memblock_translated_scalar_load(false,
                    sizeof(uint64_t), good_vaddr);
            TEST_ASSERT("the no-H PBMT same-producer store width refresh probe keeps the successful middle-store byte image readable through the retargeted PBMT alias",
                excpt.triggered == false &&
                observed_after == expected_middle
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_pbmt_mixed_nc_narrow_load_io_width_switch_store_oldest_wins_diagnostic_corner()
{
    TEST_START();

    static const unsigned load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = { 7U, 6U, 4U };
    static const bool load_signed[] = { true, false, true };
    static const unsigned store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned store_offsets[] = { 2U, 4U, 7U };
    static const uint64_t store_values[] = {
        0x000000000000d2c1ULL,
        0x00000000b4a59687ULL,
        0x000000000000004eULL,
    };
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD);

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t load_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr_load + load_offsets[round]) & (PAGE_SIZE - 1U));
        const uintptr_t store_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr_store + store_offsets[round]) & (PAGE_SIZE - 1U));

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(marker_paddr, 0ULL);
        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr_load);
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr_store);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            load_signed[round], load_sizes[round], load_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        struct ai_micro_m_memblock_exception_snapshot first = {
            .triggered = excpt.triggered,
            .cause = excpt.cause,
            .tval = excpt.tval,
        };
        TEST_ASSERT("in the no-H mixed PBMT width/decode oldest-wins diagnostic pair with older NC narrow bad-alias load and younger IO width-switched bad-alias store, the first observed precise trap remains the older load access fault",
            first.triggered == true &&
            first.cause == CAUSE_LAF &&
            first.tval == load_vaddr
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            store_vaddr, store_sizes[round], store_values[round]);
        TEST_ASSERT("after the in-sequence rearm step, the younger IO width-switched bad-alias store in that no-H mixed PBMT width/decode diagnostic pair still reports the expected store access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == store_vaddr
        );

        goto_priv(PRIV_M);
        write64(marker_paddr, 1ULL);
        TEST_ASSERT("the trailing marker store in the older-load-first no-H mixed PBMT width/decode diagnostic pair executes only after both precise traps have been observed",
            read64(marker_paddr) == 1ULL
        );
    }

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t load_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr_load + load_offsets[round]) & (PAGE_SIZE - 1U));
        const uintptr_t store_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr_store + store_offsets[round]) & (PAGE_SIZE - 1U));

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(marker_paddr, 0ULL);
        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr_load);
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            store_vaddr, store_sizes[round], store_values[round]);
        struct ai_micro_m_memblock_exception_snapshot first = {
            .triggered = excpt.triggered,
            .cause = excpt.cause,
            .tval = excpt.tval,
        };
        TEST_ASSERT("in the reverse no-H mixed PBMT width/decode diagnostic pair with older IO width-switched bad-alias store and younger NC narrow bad-alias load, the first observed precise trap remains the older store access fault",
            first.triggered == true &&
            first.cause == CAUSE_SAF &&
            first.tval == store_vaddr
        );

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            load_signed[round], load_sizes[round], load_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after the in-sequence rearm step, the younger NC narrow bad-alias load in the reverse no-H mixed PBMT width/decode diagnostic pair still reports the expected load access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == load_vaddr
        );

        goto_priv(PRIV_M);
        write64(marker_paddr, 1ULL);
        TEST_ASSERT("the trailing marker store in the reverse no-H mixed PBMT width/decode diagnostic pair executes only after both precise traps have been observed",
            read64(marker_paddr) == 1ULL
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}
