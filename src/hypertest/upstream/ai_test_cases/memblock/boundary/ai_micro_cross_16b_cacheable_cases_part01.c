#include "ai_micro_cross_16b_cacheable_common.h"

bool ai_micro_hs_load_cross_16b_cacheable_data_merge()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t warm_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(warm_vaddr);
    TEST_ASSERT("a same-page aligned HS load first warms the translation before the cross-16B TLB-hit merge probe",
        excpt.triggered == false &&
        warm == AI_MICRO_CROSS_16B_WARM_PATTERN
    );

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("an HS load that crosses a 16B boundary on a cacheable page completes without exception and merges data correctly",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_cacheable_data_split()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t warm_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(warm_vaddr);
    TEST_ASSERT("a same-page aligned HS load first warms the translation before the cross-16B TLB-hit split-store probe",
        excpt.triggered == false &&
        warm == AI_MICRO_CROSS_16B_WARM_PATTERN
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS store that crosses a 16B boundary on a cacheable page completes without exception",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("a cross-16B store updates the lower aligned doubleword with the split low-half bytes",
        read64(paddr + 8U) == 0xef66554433221100ULL
    );
    TEST_ASSERT("a cross-16B store updates the upper aligned doubleword with the split high-half bytes",
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_cacheable_repeated_misaligned_then_aligned_access_recovers_corner()
{
    TEST_START();

    const uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t warm_vaddr = hs_page_base(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t good_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ld(warm_vaddr);
    TEST_ASSERT("a same-page aligned HS load first warms the translation before the repeated cacheable cross-16B misaligned-load lifecycle probe",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_WARM_PATTERN
    );

    TEST_SETUP_EXCEPT();
    value = ld(bad_vaddr);
    TEST_ASSERT("the first HS cacheable cross-16B unaligned load still completes without exception and returns the merged boundary payload",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_SETUP_EXCEPT();
    value = ld(bad_vaddr);
    TEST_ASSERT("repeating the same HS cacheable cross-16B unaligned load still returns the same merged payload instead of leaving stale load-side state behind",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated HS cacheable cross-16B unaligned loads leave both boundary doublewords unchanged before the aligned recovery load",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(good_vaddr);
    TEST_ASSERT("after repeated HS cacheable cross-16B unaligned loads, an aligned load on the same page still recovers and reads the expected payload",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_cacheable_repeated_misaligned_then_aligned_access_recovers_corner()
{
    TEST_START();

    const uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t warm_vaddr = hs_page_base(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t good_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(warm_vaddr);
    TEST_ASSERT("a same-page aligned HS load first warms the translation before the repeated cacheable cross-16B misaligned-store lifecycle probe",
        excpt.triggered == false &&
        warm == AI_MICRO_CROSS_16B_WARM_PATTERN
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("the first HS cacheable cross-16B unaligned store still retires normally without reporting a store-misaligned trap",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN_2);
    TEST_ASSERT("repeating the same HS cacheable cross-16B unaligned store still retires normally instead of leaving stale store-side state behind",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated HS cacheable cross-16B unaligned stores leave the boundary doublewords reflecting only the newest split-store payload before the aligned recovery store",
        read64(paddr + 8U) == 0x1166554433221100ULL &&
        read64(paddr + 16U) == 0xff88776655443322ULL
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(good_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN_2);
    TEST_ASSERT("after repeated HS cacheable cross-16B unaligned stores, an aligned store on the same page still retires normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the aligned HS recovery store, the lower boundary doubleword keeps the latest split low-half bytes and the aligned upper boundary doubleword takes the new payload",
        read64(paddr + 8U) == 0x1166554433221100ULL &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_STORE_PATTERN_2
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_mab_writeback_shared_ldout_no_drop_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_mab_writeback_shared_ldout_window(false);
    TEST_ASSERT("cross-16B MAB load writeback and nearby aligned load both complete with their own payloads on shared ldout pressure",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_mab_writeback_shared_ldout_no_cross_bind_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_mab_writeback_shared_ldout_window(true);
    TEST_ASSERT("shared ldout pressure between MAB and normal load preserves both owners and adjacent boundary sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_cross16_head_waits_for_two_databuffer_ports_without_hang_corner()
{
    TEST_START();

    const uintptr_t bad_base_paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t warm_vaddr = hs_page_base(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t younger_first_vaddr = hs_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t younger_first_paddr = phys_page_base(VSRWX_GRWX) + 0x200U;
    const uintptr_t younger_second_vaddr = hs_page_base(SWITCH1) + 0x208U;
    const uintptr_t younger_second_paddr = phys_page_base(SWITCH1) + 0x208U;
    const uint64_t younger_first_initial = 0x1021324354657687ULL;
    const uint64_t younger_first_updated = 0x8877665544332211ULL;
    const uint64_t younger_second_initial = 0x89abcdef01234567ULL;
    const uint64_t younger_second_updated = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(bad_base_paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(bad_base_paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(bad_base_paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);
    write64(younger_first_paddr, younger_first_initial);
    write64(younger_second_paddr, younger_second_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(warm_vaddr);
    TEST_ASSERT("a same-page aligned HS load first warms the translation before the head cross-16B misaligned-store no-hang corner",
        excpt.triggered == false &&
        warm == AI_MICRO_CROSS_16B_WARM_PATTERN
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_first = ld(younger_first_vaddr);
    TEST_ASSERT("the first younger aligned store target is readable before the head cross-16B misaligned-store no-hang corner",
        excpt.triggered == false &&
        warm_first == younger_first_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warm_second = ld(younger_second_vaddr);
    TEST_ASSERT("the second younger aligned store target is also readable before the head cross-16B misaligned-store no-hang corner",
        excpt.triggered == false &&
        warm_second == younger_second_initial
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("the older HS head cross-16B unaligned store still retires normally before the younger aligned stores behind it",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(younger_first_vaddr, younger_first_updated);
    TEST_ASSERT("after the older head cross-16B unaligned store retires, the first younger aligned HS store still retires normally without hanging behind it",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(younger_second_vaddr, younger_second_updated);
    TEST_ASSERT("after the older head cross-16B unaligned store retires, the second younger aligned HS store on another page also retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first = ld(younger_first_vaddr);
    TEST_ASSERT("after the head cross-16B unaligned store retires, the first younger aligned HS store payload is immediately readable",
        excpt.triggered == false &&
        observed_first == younger_first_updated
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second = ld(younger_second_vaddr);
    TEST_ASSERT("after the head cross-16B unaligned store retires, the second younger aligned HS store payload is also immediately readable",
        excpt.triggered == false &&
        observed_second == younger_second_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the head cross-16B unaligned-store no-hang corner commits the older split store exactly once while both younger aligned stores also commit exactly once",
        read64(bad_base_paddr + 8U) == 0xef66554433221100ULL &&
        read64(bad_base_paddr + 16U) == 0xff0123456789abcdULL &&
        read64(younger_first_paddr) == younger_first_updated &&
        read64(younger_second_paddr) == younger_second_updated
    );

    TEST_END();
}

bool ai_micro_hs_load_not_blocked_by_unrelated_older_unaligned_store_corner()
{
    TEST_START();

    const uintptr_t bad_base_paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t same_page_load_vaddr = hs_page_base(VSRWX_GRWX) + 0x88U;
    const uintptr_t same_page_load_paddr = phys_page_base(VSRWX_GRWX) + 0x88U;
    const uintptr_t other_page_load_vaddr = hs_page_base(SWITCH1) + 0x180U;
    const uintptr_t other_page_load_paddr = phys_page_base(SWITCH1) + 0x180U;
    const uint64_t same_page_value = 0x1122334455667788ULL;
    const uint64_t other_page_value = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(bad_base_paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(bad_base_paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(bad_base_paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);
    write64(same_page_load_paddr, same_page_value);
    write64(other_page_load_paddr, other_page_value);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_same_page = ld(same_page_load_vaddr);
    TEST_ASSERT("the independent same-page load target is readable before the unrelated older unaligned-store forward-progress corner",
        excpt.triggered == false &&
        warmed_same_page == same_page_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_other_page = ld(other_page_load_vaddr);
    TEST_ASSERT("the independent different-page load target is also readable before the unrelated older unaligned-store forward-progress corner",
        excpt.triggered == false &&
        warmed_other_page == other_page_value
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("the unrelated older cross-16B unaligned store still retires normally before the younger independent loads issue",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_same_page = ld(same_page_load_vaddr);
    TEST_ASSERT("the younger independent same-page load is not falsely blocked by the unrelated older unaligned store",
        excpt.triggered == false &&
        observed_same_page == same_page_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_other_page = ld(other_page_load_vaddr);
    TEST_ASSERT("the younger independent different-page load is also not falsely blocked by the unrelated older unaligned store",
        excpt.triggered == false &&
        observed_other_page == other_page_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the unrelated older unaligned-store forward-progress corner leaves both independent load targets unchanged while the older split-store boundary bytes are updated exactly once",
        read64(bad_base_paddr + 8U) == 0xef66554433221100ULL &&
        read64(bad_base_paddr + 16U) == 0xff0123456789abcdULL &&
        read64(same_page_load_paddr) == same_page_value &&
        read64(other_page_load_paddr) == other_page_value
    );

    TEST_END();
}

bool ai_micro_hs_unaligned_store_false_datainvalid_sqidx_not_reported_corner()
{
    TEST_START();

    const uintptr_t bad_base_paddr = phys_page_base(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t independent_load_vaddr = hs_page_base(SWITCH1) + 0x220U;
    const uintptr_t independent_load_paddr = phys_page_base(SWITCH1) + 0x220U;
    const uintptr_t independent_store_vaddr = hs_page_base(SWITCH2) + 0x228U;
    const uintptr_t independent_store_paddr = phys_page_base(SWITCH2) + 0x228U;
    const uint64_t independent_load_value = 0x0fedcba987654321ULL;
    const uint64_t independent_store_initial = 0x1021324354657687ULL;
    const uint64_t independent_store_updated = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(bad_base_paddr, AI_MICRO_CROSS_16B_WARM_PATTERN);
    write64(bad_base_paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(bad_base_paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);
    write64(independent_load_paddr, independent_load_value);
    write64(independent_store_paddr, independent_store_initial);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("the first unrelated older cross-16B unaligned store still retires normally before the false-dataInvalid regression probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(bad_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN_2);
    TEST_ASSERT("repeating the same unrelated older cross-16B unaligned store still retires normally before the false-dataInvalid regression probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load = ld(independent_load_vaddr);
    TEST_ASSERT("after repeated unrelated older unaligned stores, the younger independent load still retires and returns its seeded payload instead of being redirected by a false dataInvalidSqIdx",
        excpt.triggered == false &&
        observed_load == independent_load_value
    );

    TEST_SETUP_EXCEPT();
    sd(independent_store_vaddr, independent_store_updated);
    TEST_ASSERT("after repeated unrelated older unaligned stores, the younger independent aligned store also retires normally without stale false dependency state",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_store = ld(independent_store_vaddr);
    TEST_ASSERT("after the repeated unrelated older unaligned stores, the younger independent aligned store payload is immediately readable",
        excpt.triggered == false &&
        observed_store == independent_store_updated
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated unrelated older unaligned stores do not report or act like a false dataInvalidSqIdx, so the independent aligned store location is updated and the older split-store boundary bytes reflect only the latest unaligned store",
        read64(bad_base_paddr + 8U) == 0x1166554433221100ULL &&
        read64(bad_base_paddr + 16U) == 0xff88776655443322ULL &&
        read64(independent_load_paddr) == independent_load_value &&
        read64(independent_store_paddr) == independent_store_updated
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_tlb_miss_replay_merge()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("a first HS cross-16B cacheable load on a cold translation path still completes after the TLB-miss replay path and merges data correctly",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_tlb_miss_then_hit_stable_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uint64_t first = 0;
    uint64_t second = 0;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    first = ld(vaddr);
    TEST_ASSERT("the first HS cross-16B cacheable load completes with the expected merged payload before the immediate re-access corner",
        excpt.triggered == false &&
        first == 0xeeddccbbaa998877ULL
    );

    TEST_SETUP_EXCEPT();
    second = ld(vaddr);
    TEST_ASSERT("re-accessing the same HS cross-16B alias immediately after the TLB-miss replay path remains stable and returns the same merged payload",
        excpt.triggered == false &&
        second == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_load_stable_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    const uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(boundary_phys, pattern, 16U);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same cacheable 16B boundary before the scalar split-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte boundary byte sequence",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load matches the exact 16-byte boundary payload and keeps adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector cross-16B load touches the same cacheable boundary, a scalar cross-16B load still returns the expected merged payload without sticky split state",
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    const uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(boundary_phys, pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same cacheable 16B boundary before the scalar split-load adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B load, the shared boundary bytes and adjacent sentinels remain unchanged before the scalar split-load adjacent-preservation probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte boundary byte sequence in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load matches the exact 16-byte boundary payload and keeps both scratch and shared-boundary sentinels unchanged",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector cross-16B load touches the same cacheable boundary, a scalar cross-16B load still returns the expected merged payload without sticky split state in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector/scalar cross-16B load sequence, the shared boundary bytes and both sets of sentinels still match their expected values",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );

    TEST_END();
}

bool ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vector_aligned_and_cross_16b_load_stable(false);
    TEST_END();
}

bool ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vector_aligned_and_cross_16b_load_stable(true);
    TEST_END();
}

bool ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vector_aligned_and_cross_16b_store_stable(false);
    TEST_END();
}

bool ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_vector_aligned_and_cross_16b_store_stable(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_stable(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(boundary_phys, pattern, 16U);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same cacheable 16B boundary before the mixed vector/scalar trigger-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte boundary byte sequence before the scalar trigger-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load still matches the exact boundary payload and keeps adjacent sentinels unchanged before the scalar trigger-load probe",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-16B load touches the same cacheable boundary, the first scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-load probe still leaves the boundary bytes unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-16B load, the second scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-load probe still leaves the boundary bytes unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B load, clearing only the execute-PC slot still leaves the scalar cross-16B load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B load probe still leaves the boundary bytes unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-16B load still completes normally and returns the merged payload even though a vector cross-16B load touched the shared path earlier",
        excpt.triggered == false &&
        value == 0x4e4d4c4b4a494847ULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_PAGE_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_PAGE_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT, false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
            AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT, true);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_tlb_miss_replay_split()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("a first HS cross-16B cacheable store on a cold translation path completes after the TLB-miss replay path",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cold-translation cross-16B store updates the lower aligned doubleword with the expected split bytes",
        read64(paddr + 8U) == 0xef66554433221100ULL
    );
    TEST_ASSERT("the cold-translation cross-16B store updates the upper aligned doubleword with the expected split bytes",
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_tlb_miss_then_hit_stable_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("the first HS cross-16B cacheable store completes successfully before the immediate re-store corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN_2);
    TEST_ASSERT("re-storing to the same HS cross-16B alias immediately after the TLB-miss replay path remains stable and completes successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the cold-translation cross-16B store and immediate re-store sequence, the lower aligned doubleword reflects the latest split bytes",
        read64(paddr + 8U) == 0x1166554433221100ULL
    );
    TEST_ASSERT("after the cold-translation cross-16B store and immediate re-store sequence, the upper aligned doubleword reflects the latest split bytes",
        read64(paddr + 16U) == 0xff88776655443322ULL
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_store_stable_corner()
{
    TEST_START();

    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x180U;
    const uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-16B store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte payload across the same cacheable 16B boundary without fault",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the vector cross-16B store, an overlapping scalar cross-16B store still commits normally and overwrites only its own eight-byte window",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the final boundary bytes reflect the expected vector-then-scalar overwrite order while adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("the overlapping scalar cross-16B store remains immediately visible to a following scalar split-load probe after the earlier vector store",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x180U;
    const uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector/scalar cross-16B store adjacent-preservation probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the first 16-byte payload across the same cacheable 16B boundary before the scalar split-store adjacent-preservation probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store, the shared boundary bytes match the staged payload and adjacent sentinels remain unchanged before the scalar split-store adjacent-preservation probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the vector cross-16B store, an overlapping scalar cross-16B store still commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the final boundary bytes reflect the expected vector-then-scalar overwrite order while adjacent sentinels stay unchanged in the adjacent-preservation corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the mixed vector/scalar cross-16B store sequence, a following scalar split-load still observes the latest scalar payload immediately in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    TEST_END();
}

bool ai_micro_hs_vector_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_vector_store_fake_crosspage_state_window(false);
    TEST_ASSERT("same-page vector cross16 store fake-crosspage owner state cannot rebind the next boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_vector_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_vector_store_fake_crosspage_state_window(true);
    TEST_ASSERT("same-page vector cross16 fake-crosspage sequence preserves each boundary and adjacent guard gap",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_scalar_store_fake_crosspage_state_window(false);
    TEST_ASSERT("same-page scalar cross16 store fake-crosspage owner state cannot rebind the next boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_scalar_store_fake_crosspage_state_window(true);
    TEST_ASSERT("same-page scalar cross16 fake-crosspage sequence preserves each boundary and adjacent guard gap",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_scalar_samepage_store_width_switch_window(false);
    TEST_ASSERT("same-page scalar cross16 width switch refreshes fake-crosspage owner state for the next boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_scalar_samepage_store_width_switch_window(true);
    TEST_ASSERT("same-page scalar cross16 width-switch sequence preserves width-specific byte image and guard gap",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_fault_then_samepage_width_switch_no_stale_fake_crosspage_rebind_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_store_fault_then_samepage_width_switch_window(false);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_fault_then_samepage_width_switch_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_store_fault_then_samepage_width_switch_window(true);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_then_same_16b_direct_store_old_split_state_not_rebound_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_store_then_same_16b_direct_store_old_split_state_window(false);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_then_same_16b_direct_store_old_split_state_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_store_then_same_16b_direct_store_old_split_state_window(true);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_stable_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_stable(false);
    TEST_END();
}

bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_scalar_store_after_vector_load_stable(true);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[16] = {
        0x13, 0x05, 0x10, 0x00,
        0x13, 0x05, 0x15, 0x00,
        0x13, 0x05, 0x15, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[16] = {
        0x13, 0x05, 0x10, 0x00,
        0x13, 0x05, 0x25, 0x00,
        0x13, 0x05, 0x45, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t code_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + 16U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, 16U);
    ai_micro_cross_16b_write_bytes(source_phys, patched_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-16B store rewrites the code bytes, the warmed executable HS block still returns the original value across the 16B boundary",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-16B store rewrites the executable HS block",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)code_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite a 16-byte executable HS block across a 16B boundary without fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store rewrites the executable HS block, the new instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, fetching from the vector-rewritten executable HS block returns the patched value across the same 16B boundary",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store plus fence.i fetch-recovery sequence, the rewritten instruction bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, 16U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(uint64_t);
    const uint8_t sentinel_before = 0x5aU;
    const uint8_t sentinel_after = 0xa5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-16B store rewrites the code bytes, the warmed executable HS block still returns the original value across the 16B boundary",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082007005130001ULL);
    TEST_ASSERT("a scalar cross-16B store can rewrite an executable HS block across the 16B boundary without fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store rewrites the executable HS block, the new instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, fetching from the scalar-rewritten executable HS block returns the patched value across the same 16B boundary",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store plus fence.i fetch-recovery sequence, the rewritten instruction bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t final_pattern[8] = {
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1e0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1e0U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x6aU;
    const uint8_t sentinel_after = 0xa6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B code rewrite sequence starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B code rewrite, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream without fault",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after fence.i, fetching from the mixed vector-then-scalar rewritten executable HS block returns the final scalar-overwritten value across the same 16B boundary",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite plus fence.i sequence, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(uint64_t);
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x5aU;
    const uint8_t sentinel_after = 0xa5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-16B trigger-fetch probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082007005130001ULL);
    TEST_ASSERT("a scalar cross-16B store can rewrite an executable HS block before the execute-trigger fetch-recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the execute-trigger fetch-recovery probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch to the scalar-rewritten executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch to the scalar-rewritten executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after clearing the execute-PC trigger, the rewritten executable HS block fetched through the scalar cross-16B store path completes normally and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store plus repeated execute-trigger fetch sequence, the patched bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(uint64_t);
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x5bU;
    const uint8_t sentinel_after = 0xb5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-16B execute-trigger plus no-X probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082007005130001ULL);
    TEST_ASSERT("a scalar cross-16B store can rewrite an executable HS block before the repeated execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the repeated execute-trigger plus no-X fetch probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the scalar cross-16B execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the scalar cross-16B execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten executable HS block falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later no-X fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-16B corner",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(uint64_t);
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x5cU;
    const uint8_t sentinel_after = 0xc5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-16B execute-trigger plus grant-X-recovery probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082007005130001ULL);
    TEST_ASSERT("a scalar cross-16B store can rewrite an executable HS block before the repeated execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the repeated execute-trigger plus grant-X-recovery fetch probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the scalar cross-16B execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the scalar cross-16B execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten executable HS block first falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch page fault before execute recovery still leaves the patched bytes and adjacent sentinels unchanged in the scalar cross-16B execute-trigger corner",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission, the scalar-rewritten executable HS block completes normally again and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-16B corner",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_16b_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x220U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x220U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x4bU;
    const uint8_t sentinel_after = 0xb4U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B code rewrite no-X probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed no-X code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B no-X code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before execute permission is removed",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before execute permission is removed",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite, clearing execute permission forces the next fetch to report instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch fault after the mixed vector-then-scalar cross-16B rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x230U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x230U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xc4U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B code rewrite execute-recovery probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-recovery code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-recovery code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before execute permission is toggled",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before execute permission is toggled",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite, clearing execute permission first forces instruction page fault before execute recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch fault before execute recovery still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-16B rewrite corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after re-granting execute permission, fetching from the mixed vector-then-scalar rewritten executable HS block completes normally and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after execute permission is restored in the mixed vector-then-scalar cross-16B rewrite corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x240U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x240U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t bad_vaddr = ai_micro_cross_16b_make_noncanonical_vaddr(code_vaddr);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x7aU;
    const uint8_t sentinel_after = 0xa7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B non-canonical fetch probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed non-canonical code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B non-canonical code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the non-canonical fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the non-canonical fetch probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite, a non-canonical fetch still reports instruction page fault with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the non-canonical fetch fault after the mixed vector-then-scalar cross-16B rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B non-canonical fetch fault, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-16B non-canonical fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x250U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x250U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t bad_vaddr = (uintptr_t)-1;
    const uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x7bU;
    const uint8_t sentinel_after = 0xb7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B all-ones fetch probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed all-ones code-rewrite fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B all-ones code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the all-ones fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the all-ones fetch probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite, an all-ones fetch still reports instruction page fault with the JALR-cleared target in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the all-ones fetch fault after the mixed vector-then-scalar cross-16B rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B all-ones fetch fault, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-16B all-ones fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x260U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x260U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x6cU;
    const uint8_t sentinel_after = 0xc6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B leaf-overflow fetch-AF probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed leaf-overflow code-rewrite fetch-AF probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B leaf-overflow code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the leaf-overflow fetch-AF probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the leaf-overflow fetch-AF probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite, overflowing the leaf PPN forces the next fetch to report instruction access fault on the same canonical entry",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-overflow fetch access fault after the mixed vector-then-scalar cross-16B rewrite still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x270U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x270U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x6dU;
    const uint8_t sentinel_after = 0xd6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B leaf-overflow recovery probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed leaf-overflow recovery code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B leaf-overflow recovery code rewrite",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the leaf-overflow recovery probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B code rewrite, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before the leaf-overflow recovery probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite, overflowing the leaf PPN first forces instruction access fault before the leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the leaf-overflow fetch access fault before recovery still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-16B rewrite corner",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after repairing the leaf entry, fetching from the mixed vector-then-scalar rewritten executable HS block completes normally and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after leaf repair in the mixed vector-then-scalar cross-16B rewrite corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1e0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1e0U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uintptr_t trigger_pc = code_vaddr;
    const uint8_t sentinel_before = 0x69U;
    const uint8_t sentinel_after = 0x96U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B code rewrite execute-trigger probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger code rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch to the final executable HS block still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch to the mixed vector-then-scalar rewritten executable HS block still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch to the final executable HS block still reports breakpoint when the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch to the mixed vector-then-scalar rewritten executable HS block still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar rewritten executable HS block completes normally and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite plus repeated execute-trigger fetch sequence, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t vector_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x13,
        0x05, 0x50, 0x00, 0x67,
        0x80, 0x00, 0x00, 0x2f
    };
    const uint8_t expected_final[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x01, 0x00,
        0x13, 0x05, 0x90, 0x00,
        0x82, 0x80, 0x00, 0x2f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x2d0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x2d0U;
    const uintptr_t boundary_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x6aU;
    const uint8_t sentinel_after = 0xa6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, vector_pattern, sizeof(vector_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-16B execute-trigger plus no-X probe starts, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-16B instruction payload before the mixed execute-trigger plus no-X code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared 16-byte executable HS boundary before the overlapping scalar cross-16B execute-trigger plus no-X code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-16B execute-trigger plus no-X code-rewrite step, the staged boundary bytes are visible and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_16b_bytes_match(boundary_phys, vector_pattern, sizeof(vector_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082009005130001ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-16B store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B execute-trigger plus no-X code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, code_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the first fetch still reports breakpoint before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus no-X corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-16B rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-16B execute-trigger plus no-X corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_set_hs_leaf(VSRWX_GRWX, phys_page_base(VSRWX_GRWX),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_16b_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten executable HS block falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later no-X fetch page fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, sizeof(expected_final)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
