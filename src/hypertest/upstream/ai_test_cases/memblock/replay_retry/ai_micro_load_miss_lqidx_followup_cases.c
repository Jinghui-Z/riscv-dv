#include <rvh_test.h>
#include <stdint.h>

bool ai_micro_hs_load_tlb_miss_replay_data_visible(void);
bool ai_micro_hs_load_tlb_miss_then_hit_stable_corner(void);
bool ai_micro_hs_load_cross_16b_tlb_miss_replay_merge(void);
bool ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner(void);
bool ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner(void);
bool ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner(void);
bool ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner(void);

static bool ai_micro_lqidx_miss_round(void)
{
    bool ok = true;

    ok = ai_micro_hs_load_tlb_miss_replay_data_visible() && ok;
    ok = ai_micro_hs_load_tlb_miss_then_hit_stable_corner() && ok;
    ok = ai_micro_hs_load_cross_16b_tlb_miss_replay_merge() && ok;

    return ok;
}

static bool ai_micro_lqidx_replay_round(void)
{
    bool ok = true;

    ok = ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner() && ok;
    ok = ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner() && ok;
    ok = ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner() && ok;
    ok = ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner() && ok;

    return ok;
}

bool ai_micro_hs_load_miss_lqidx_miss_then_ldld_replay_progress_corner()
{
    TEST_START();

    TEST_ASSERT("a tlb-miss load warmup followed by ld-ld replay keeps forward progress",
        ai_micro_hs_load_tlb_miss_replay_data_visible() &&
        ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner()
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_miss_then_dense_replay_progress_corner()
{
    TEST_START();

    TEST_ASSERT("a tlb-miss load warmup followed by dense load replay keeps forward progress",
        ai_micro_hs_load_tlb_miss_replay_data_visible() &&
        ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner()
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_miss_then_replay_queue_wrap_progress_corner()
{
    TEST_START();

    TEST_ASSERT("a miss warmup followed by replay queue wrap behavior still keeps no-stuck progress",
        ai_micro_lqidx_miss_round() &&
        ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner()
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_miss_then_raw_replay_alloc_free_progress_corner()
{
    TEST_START();

    TEST_ASSERT("a miss warmup followed by raw replay queue free/alloc transition still keeps no ghost-full and no duplicate-slot behavior",
        ai_micro_lqidx_miss_round() &&
        ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner()
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_mixed_miss_sources_then_replay_bundle_progress_corner()
{
    TEST_START();

    TEST_ASSERT("mixed miss sources followed by the replay bundle still keeps forward progress",
        ai_micro_lqidx_miss_round() &&
        ai_micro_lqidx_replay_round()
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_cross16_miss_then_dual_replay_round_progress_corner()
{
    TEST_START();

    TEST_ASSERT("a cross16b miss replay-merge warmup followed by two replay rounds still keeps forward progress",
        ai_micro_hs_load_cross_16b_tlb_miss_replay_merge() &&
        ai_micro_lqidx_replay_round() &&
        ai_micro_lqidx_replay_round()
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_triple_round_miss_replay_interleave_progress_corner()
{
    TEST_START();

    bool ok = true;

    for (unsigned round = 0; round < 3; ++round) {
        ok = ai_micro_lqidx_miss_round() && ok;
        ok = ai_micro_lqidx_replay_round() && ok;
    }

    TEST_ASSERT("three rounds of miss/replay interleave still keep progress without no-commit stall",
        ok
    );

    TEST_END();
}

bool ai_micro_hs_load_miss_lqidx_alias_miss_then_replay_no_stuck_regression_corner()
{
    TEST_START();

    TEST_ASSERT("alias-flavored miss warmup order followed by replay bundle keeps no-stuck regression behavior",
        ai_micro_hs_load_cross_16b_tlb_miss_replay_merge() &&
        ai_micro_hs_load_tlb_miss_then_hit_stable_corner() &&
        ai_micro_hs_load_tlb_miss_replay_data_visible() &&
        ai_micro_lqidx_replay_round()
    );

    TEST_END();
}
