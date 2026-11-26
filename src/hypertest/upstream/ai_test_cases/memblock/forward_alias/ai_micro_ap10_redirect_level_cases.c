#include "ai_micro_overlap_common.h"

extern bool ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_same_uop_escape_corner(void);
extern bool ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_min_escape_corner(void);
extern bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_amo_no_stale_zero_or_lost_update_corner(void);
extern bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_normal_vector_no_stale_zero_vector_payload_or_marker_drop_corner(void);
extern bool ai_micro_hs_amo_then_vsegment_store_no_stale_atomic_owner_or_segment_payload_corner(void);
extern bool ai_micro_hs_vsegment_store_then_amo_no_flush_owner_or_stale_old_value_corner(void);

bool ai_micro_ap10_vpmatch_ldld_raw_mixed_level_marker_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-10 phase0: vp_match_fail-like alias mismatch and ld-ld flushAfter approximation must preserve the between-load marker and converge all aliases to the delayed-store payload",
        ai_micro_hs_load_vpmatchfail_and_ldld_same_cycle_rollback_priority_correct_corner()
    );

    TEST_ASSERT("AP-10 phase1: repeating the combined alias/ld-ld window with sibling load order swapped must not leave stale replay state or wrong flush scope",
        ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner()
    );

    TEST_ASSERT("AP-10 phase2: LoadQueueRAW level=flush rollback and LoadUnit-style replay pressure on two targets must not cross-flush or bind marker data to the wrong target",
        ai_micro_hs_loadqueue_raw_dual_store_violation_two_targets_no_cross_flush_corner()
    );

    TEST_END();
}

bool ai_micro_ap10_vector_same_rob_flow_and_scalar_raw_marker_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-10 phase0: vector indexed-load same-uop element0 replay plus element1 precise fault keeps only the committed prefix and does not leak tail vector state",
        ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_same_uop_escape_corner()
    );

    TEST_ASSERT("AP-10 phase1: the minimal vector same-uop replay/fault shape also preserves the repaired element0 payload without depending on stale DCache data",
        ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_min_escape_corner()
    );

    TEST_ASSERT("AP-10 phase2: scalar same-ROB-row RAW oldest-uop approximation records the delayed-store payload in both markers, so the rollback target is not the later sibling load",
        ai_micro_hs_loadqueue_raw_same_rob_two_loads_oldest_uop_selected_corner()
    );

    TEST_ASSERT("AP-10 phase3: swapping same-PA sibling load order across repeated RAW rollback windows still refreshes both markers to the latest payload",
        ai_micro_hs_stld_violation_same_rob_diff_uopidx_redirect_target_correct_corner()
    );

    TEST_END();
}

bool ai_micro_ap10_wrap_pressure_mixed_level_replay_boundary_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-10 phase0: dense store pressure approximates pointer wrap while a delayed-store replay and independent direct-store load keep their own payloads",
        ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner()
    );

    TEST_ASSERT("AP-10 phase1: ld-ld flushAfter preserves the immediately following independent marker store, catching over-flush of the current boundary",
        ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner()
    );

    TEST_ASSERT("AP-10 phase2: repeated mixed alias/ld-ld rollback after the wrap-pressure phase still selects the correct visible boundary and latest marker payload",
        ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner()
    );

    TEST_END();
}

bool ai_micro_ap10_mixed_redirect_then_cbo_zero_amo_side_effect_manual_smoke_corner()
{
    TEST_START();

    TEST_ASSERT("AP-10 phase0: same-paddr alias refill followed by cbo.zero and normal vector traffic must zero only the target block while preserving vector payload and marker progress",
        ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_normal_vector_no_stale_zero_vector_payload_or_marker_drop_corner()
    );

    TEST_ASSERT("AP-10 phase1: cbo.zero followed by AMO must not lose either the zero side effect or the atomic update after refill/replay ownership changes",
        ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_amo_no_stale_zero_or_lost_update_corner()
    );

    TEST_ASSERT("AP-10 phase2: AMO before vsegment store keeps atomic old-value/update and post-vector marker state independent",
        ai_micro_hs_amo_then_vsegment_store_no_stale_atomic_owner_or_segment_payload_corner()
    );

    TEST_ASSERT("AP-10 phase3: vsegment store before AMO keeps vector store payload and atomic side effect independent across the same MemBlock ownership boundary",
        ai_micro_hs_vsegment_store_then_amo_no_flush_owner_or_stale_old_value_corner()
    );

    TEST_END();
}
