#include <rvh_test.h>
#include <stdint.h>

#ifndef CSR_SMBLOCKCTL
#define CSR_SMBLOCKCTL 0x5c3
#endif

#define AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE    (1ULL << 0)
#define AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK (0xFULL << 1)
#define AI_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

extern bool ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner(void);
extern bool ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner(void);
extern bool ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner(void);
extern bool ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner(void);
extern bool ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner(void);
extern bool ai_micro_hs_load_uncache_replay_head_data_visible(void);
extern bool ai_micro_hs_same_rob_row_dual_uncache_load_replay_no_popcount_assert_corner(void);
extern bool ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner(void);
extern bool ai_micro_hs_robhead_mmio_and_misalign_replay_same_window_no_assert_or_drop_corner(void);
extern bool ai_micro_ap86_load_replay_wrong_path_split_response_then_clean_retry_marker_no_stale_vecwb_manual_corner(void);

static inline uint64_t ai_arch_replay_escape_read_smblockctl(void)
{
    return CSRR(CSR_SMBLOCKCTL);
}

static inline void ai_arch_replay_escape_write_smblockctl(uint64_t value)
{
    CSRW(CSR_SMBLOCKCTL, value);
}

static inline uint64_t ai_arch_replay_escape_compose(
    uint64_t original,
    bool enable,
    uint64_t threshold_bits
)
{
    uint64_t value = original & ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_TIME_MASK;
    if (enable) {
        value |= AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE;
    } else {
        value &= ~AI_ARCH_SMBLOCKCTL_REPLAY_ESCAPE_ENABLE;
    }
    return value | threshold_bits;
}

static bool ai_arch_replay_escape_read_no_except(uint64_t *value)
{
    TEST_SETUP_EXCEPT();
    *value = ai_arch_replay_escape_read_smblockctl();
    return excpt.triggered == false;
}

static bool ai_arch_replay_escape_write_and_readback(uint64_t value)
{
    TEST_SETUP_EXCEPT();
    ai_arch_replay_escape_write_smblockctl(value);
    if (excpt.triggered) {
        return false;
    }

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_arch_replay_escape_read_smblockctl();
    return excpt.triggered == false && observed == value;
}

static bool ai_arch_replay_escape_pair_round(void)
{
    bool ok = true;

    ok = ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner() && ok;
    ok = ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner() && ok;

    return ok;
}

static bool ai_arch_replay_escape_stress_round(void)
{
    bool ok = true;

    ok = ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner() && ok;
    ok = ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner() && ok;
    ok = ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner() && ok;
    ok = ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner() && ok;

    return ok;
}

static bool ai_arch_replay_escape_component_round(unsigned selector)
{
    switch (selector) {
    case 0:
        return ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner();
    case 1:
        return ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner();
    case 2:
        return ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner();
    default:
        return ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner();
    }
}

bool ai_arch_smblockctl_replay_escape_enable_small_threshold_dense_replay_progress_corner()
{
    TEST_START();

    const uint64_t small_threshold = 0x1ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before enabling replay_escape with a small threshold for the dense replay wrapper corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, small_threshold);
    TEST_ASSERT("enabling replay_escape with a small threshold for the dense replay wrapper corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("with replay_escape enabled and a small threshold, the dense replay/store-boundary regression corner still completes without getting stuck",
        ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the replay_escape-enabled dense replay wrapper corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_disable_small_threshold_ldld_replay_no_regression_corner()
{
    TEST_START();

    const uint64_t small_threshold = 0x1ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before disabling replay_escape with a small threshold for the ld-ld replay wrapper corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, false, small_threshold);
    TEST_ASSERT("disabling replay_escape with a small threshold for the ld-ld replay wrapper corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("with replay_escape disabled and a small threshold, the ld-ld replay plus nearby writeback regression corner still keeps its no-stuck result",
        ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the replay_escape-disabled ld-ld replay wrapper corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_enable_threshold_sweep_replay_wrapper_matrix_corner()
{
    TEST_START();

    const uint64_t thresholds[] = {
        0x1ULL << 1,
        0x3ULL << 1,
        0x7ULL << 1,
    };
    const char * const program_messages[] = {
        "enabling replay_escape with threshold=1 for the wrapper matrix completes without exception and readback matches the programmed value",
        "enabling replay_escape with threshold=3 for the wrapper matrix completes without exception and readback matches the programmed value",
        "enabling replay_escape with threshold=7 for the wrapper matrix completes without exception and readback matches the programmed value",
    };
    const char * const round_messages[] = {
        "with replay_escape enabled and threshold=1, the paired ld-ld replay and dense replay wrapper matrix still completes without no-progress regression",
        "with replay_escape enabled and threshold=3, the paired ld-ld replay and dense replay wrapper matrix still completes without no-progress regression",
        "with replay_escape enabled and threshold=7, the paired ld-ld replay and dense replay wrapper matrix still completes without no-progress regression",
    };
    uint64_t original = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the replay_escape enable-side threshold sweep wrapper matrix",
        ai_arch_replay_escape_read_no_except(&original)
    );

    for (size_t i = 0; i < AI_ARRAY_SIZE(thresholds); ++i) {
        uint64_t reprogrammed = ai_arch_replay_escape_compose(original, true, thresholds[i]);

        TEST_ASSERT(program_messages[i],
            ai_arch_replay_escape_write_and_readback(reprogrammed)
        );

        TEST_ASSERT(round_messages[i],
            ai_arch_replay_escape_pair_round()
        );

        goto_priv(PRIV_M);
    }

    TEST_ASSERT("restoring smblockctl after the replay_escape enable-side threshold sweep wrapper matrix completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_disable_small_threshold_replay_queue_multi_round_stress_no_regression_corner()
{
    TEST_START();

    const uint64_t small_threshold = 0x1ULL << 1;
    const char * const round_messages[] = {
        "with replay_escape disabled and a small threshold, replay-sensitive load-queue stress round 0 still keeps forward progress and no-regression results",
        "with replay_escape disabled and a small threshold, replay-sensitive load-queue stress round 1 still keeps forward progress and no-regression results",
    };
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before disabling replay_escape for the multi-round replay-queue stress corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, false, small_threshold);
    TEST_ASSERT("disabling replay_escape with a small threshold for the multi-round replay-queue stress corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    for (size_t round = 0; round < AI_ARRAY_SIZE(round_messages); ++round) {
        TEST_ASSERT(round_messages[round],
            ai_arch_replay_escape_stress_round()
        );
        goto_priv(PRIV_M);
    }

    TEST_ASSERT("restoring smblockctl after the replay_escape-disabled multi-round replay-queue stress corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_disable_threshold_sweep_replay_wrapper_matrix_no_regression_corner()
{
    TEST_START();

    const uint64_t thresholds[] = {
        0x1ULL << 1,
        0x7ULL << 1,
        0xfULL << 1,
    };
    const char * const program_messages[] = {
        "disabling replay_escape with threshold=1 for the wrapper matrix completes without exception and readback matches the programmed value",
        "disabling replay_escape with threshold=7 for the wrapper matrix completes without exception and readback matches the programmed value",
        "disabling replay_escape with threshold=15 for the wrapper matrix completes without exception and readback matches the programmed value",
    };
    const char * const round_messages[] = {
        "with replay_escape disabled and threshold=1, the paired ld-ld replay and dense replay wrapper matrix still keeps its no-regression result",
        "with replay_escape disabled and threshold=7, the paired ld-ld replay and dense replay wrapper matrix still keeps its no-regression result",
        "with replay_escape disabled and threshold=15, the paired ld-ld replay and dense replay wrapper matrix still keeps its no-regression result",
    };
    uint64_t original = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the replay_escape disable-side threshold sweep wrapper matrix",
        ai_arch_replay_escape_read_no_except(&original)
    );

    for (size_t i = 0; i < AI_ARRAY_SIZE(thresholds); ++i) {
        uint64_t reprogrammed = ai_arch_replay_escape_compose(original, false, thresholds[i]);

        TEST_ASSERT(program_messages[i],
            ai_arch_replay_escape_write_and_readback(reprogrammed)
        );

        TEST_ASSERT(round_messages[i],
            ai_arch_replay_escape_pair_round()
        );

        goto_priv(PRIV_M);
    }

    TEST_ASSERT("restoring smblockctl after the replay_escape disable-side threshold sweep wrapper matrix completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_enable_small_threshold_replay_queue_multi_round_stress_progress_corner()
{
    TEST_START();

    const uint64_t small_threshold = 0x1ULL << 1;
    const char * const round_messages[] = {
        "with replay_escape enabled and a small threshold, replay-sensitive load-queue stress round 0 still keeps forward progress and no-regression results",
        "with replay_escape enabled and a small threshold, replay-sensitive load-queue stress round 1 still keeps forward progress and no-regression results",
    };
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before enabling replay_escape for the multi-round replay-queue stress corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, small_threshold);
    TEST_ASSERT("enabling replay_escape with a small threshold for the multi-round replay-queue stress corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    for (size_t round = 0; round < AI_ARRAY_SIZE(round_messages); ++round) {
        TEST_ASSERT(round_messages[round],
            ai_arch_replay_escape_stress_round()
        );
        goto_priv(PRIV_M);
    }

    TEST_ASSERT("restoring smblockctl after the replay_escape-enabled multi-round replay-queue stress corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_toggle_between_wrapper_and_stress_rounds_no_state_leak_corner()
{
    TEST_START();

    const bool enables[] = {
        true,
        false,
    };
    const unsigned component_selectors[] = {
        0U,
        2U,
    };
    const uint64_t thresholds[] = {
        0x1ULL << 1,
        0x7ULL << 1,
    };
    const char * const program_messages[] = {
        "round0: enabling replay_escape with threshold=1 for the mode-toggle no-state-leak corner completes without exception and readback matches the programmed value",
        "round1: disabling replay_escape with threshold=7 for the mode-toggle no-state-leak corner completes without exception and readback matches the programmed value",
    };
    const char * const run_messages[] = {
        "round0: after enabling replay_escape, the ld-ld replay wrapper component still completes without state-leak regression",
        "round1: after disabling replay_escape with a higher threshold, the replay-queue pointer-wrap stress component still completes without state-leak regression",
    };
    uint64_t original = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the replay_escape mode-toggle no-state-leak corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    for (size_t i = 0; i < AI_ARRAY_SIZE(enables); ++i) {
        uint64_t reprogrammed = ai_arch_replay_escape_compose(original, enables[i], thresholds[i]);

        TEST_ASSERT(program_messages[i],
            ai_arch_replay_escape_write_and_readback(reprogrammed)
        );

        TEST_ASSERT(run_messages[i],
            ai_arch_replay_escape_component_round(component_selectors[i])
        );

        goto_priv(PRIV_M);
    }

    TEST_ASSERT("restoring smblockctl after the replay_escape mode-toggle no-state-leak corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_enable_zero_threshold_pair_round_progress_corner()
{
    TEST_START();

    const uint64_t zero_threshold = 0x0ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before enabling replay_escape with threshold=0 for the immediate-escape pair-round corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, zero_threshold);
    TEST_ASSERT("enabling replay_escape with threshold=0 for the immediate-escape pair-round corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("with replay_escape enabled and threshold=0, the paired ld-ld replay and dense replay wrapper round still completes without no-progress regression",
        ai_arch_replay_escape_pair_round()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the replay_escape threshold=0 pair-round corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_enable_zero_threshold_ldld_flushafter_store_progress_corner()
{
    TEST_START();

    const uint64_t zero_threshold = 0x0ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before enabling immediate replay_escape for the ld-ld flushAfter follower-store corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, zero_threshold);
    TEST_ASSERT("enabling replay_escape with threshold=0 for the ld-ld flushAfter follower-store corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("with replay_escape enabled and threshold=0, the ld-ld flushAfter corner still lets the following independent store survive replay and retire visibly",
        ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the immediate replay_escape ld-ld flushAfter follower-store corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_zero_threshold_flushafter_freelist_no_dup_free_corner()
{
    TEST_START();

    const uint64_t zero_threshold = 0x0ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the AP-133 immediate replay_escape flushAfter plus freelist duplicate-free smoke",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, zero_threshold);
    TEST_ASSERT("AP-133 smoke enables replay_escape with threshold=0 so escapeBlock is reachable immediately in replay-pressure windows",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-133 smoke phase0: the ld-ld flushAfter redirect window still lets a following independent store retire, covering the s2_cancelReplay side effect as an architectural progress symptom",
        ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-133 smoke keeps replay_escape threshold=0 programmed between redirect and replayQ freelist phases",
        ai_arch_replay_escape_read_smblockctl() == reprogrammed
    );

    TEST_ASSERT("AP-133 smoke phase1: consecutive replay-pressure alloc/free episodes complete without ghost-full, duplicate-slot, or duplicate-free symptoms after immediate escape is enabled",
        ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the AP-133 immediate replay_escape flushAfter plus freelist smoke completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_mab_redirect_adjacent_port_manual_corner()
{
    TEST_START();

    const uint64_t zero_threshold = 0x0ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the AP-133/AP-86 immediate replay_escape plus MAB redirect cross-pressure smoke",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, zero_threshold);
    TEST_ASSERT("AP-133/AP-86 cross smoke enables replay_escape with threshold=0 before the flushAfter cancel window",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-133/AP-86 phase0: immediate replay_escape still lets the ld-ld flushAfter follower-store window retire visibly before MAB redirect pressure",
        ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-133/AP-86 cross smoke re-applies immediate replay_escape before the AP-86 wrong-path vector MAB redirect phase",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-133/AP-86 phase1: wrong-path staged vector MAB split-load candidates do not leak stale vecWriteBack while replay_escape is held at threshold=0",
        ai_micro_ap86_load_replay_wrong_path_split_response_then_clean_retry_marker_no_stale_vecwb_manual_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-133/AP-86 cross smoke re-applies immediate replay_escape before the adjacent-port dual-misalign replay-head phase",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-133/AP-86 phase2: adjacent dual-misalign replay-head entries still use their own staged recovery path and do not look like ordinary non-head escape candidates",
        ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-133/AP-86 cross smoke keeps immediate replay_escape programmed before the duplicate-free contrast phase",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-133/AP-86 phase3: replayQ free-next-alloc pressure still completes without ghost-full, duplicate-slot, or duplicate-free symptoms after MAB redirect pressure",
        ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the AP-133/AP-86 immediate replay_escape plus MAB redirect cross-pressure smoke completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_enable_max_threshold_replay_queue_stress_progress_corner()
{
    TEST_START();

    const uint64_t max_threshold = 0xfULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before enabling replay_escape with threshold=15 for the delayed-escape replay-queue stress corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, max_threshold);
    TEST_ASSERT("enabling replay_escape with threshold=15 for the delayed-escape replay-queue stress corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("with replay_escape enabled and threshold=15, the replay-queue stress round still completes without no-progress regression",
        ai_arch_replay_escape_stress_round()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the replay_escape threshold=15 replay-queue stress corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_disable_zero_threshold_replay_queue_stress_no_regression_corner()
{
    TEST_START();

    const uint64_t zero_threshold = 0x0ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before disabling replay_escape with threshold=0 for the zero-threshold replay-queue stress corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, false, zero_threshold);
    TEST_ASSERT("disabling replay_escape with threshold=0 for the zero-threshold replay-queue stress corner completes without exception and readback matches the programmed value",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("with replay_escape disabled and threshold=0, the replay-queue stress round still completes without no-regression breakage",
        ai_arch_replay_escape_stress_round()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the replay_escape disabled threshold=0 replay-queue stress corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_toggle_zero_and_max_threshold_between_modes_no_state_leak_corner()
{
    TEST_START();

    const bool enables[] = {
        true,
        false,
    };
    const unsigned component_selectors[] = {
        0U,
        2U,
    };
    const uint64_t thresholds[] = {
        0x0ULL << 1,
        0xfULL << 1,
    };
    const char * const program_messages[] = {
        "round0: enabling replay_escape with threshold=0 for the zero/max mode-toggle no-state-leak corner completes without exception and readback matches the programmed value",
        "round1: disabling replay_escape with threshold=15 for the zero/max mode-toggle no-state-leak corner completes without exception and readback matches the programmed value",
    };
    const char * const run_messages[] = {
        "round0: after enabling replay_escape with threshold=0, the ld-ld replay wrapper component still completes without state-leak regression",
        "round1: after disabling replay_escape with threshold=15, the replay-queue pointer-wrap stress component still completes without state-leak regression",
    };
    uint64_t original = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the zero/max replay_escape mode-toggle no-state-leak corner",
        ai_arch_replay_escape_read_no_except(&original)
    );

    for (size_t i = 0; i < AI_ARRAY_SIZE(enables); ++i) {
        uint64_t reprogrammed = ai_arch_replay_escape_compose(original, enables[i], thresholds[i]);

        TEST_ASSERT(program_messages[i],
            ai_arch_replay_escape_write_and_readback(reprogrammed)
        );

        TEST_ASSERT(run_messages[i],
            ai_arch_replay_escape_component_round(component_selectors[i])
        );

        goto_priv(PRIV_M);
    }

    TEST_ASSERT("restoring smblockctl after the zero/max replay_escape mode-toggle no-state-leak corner completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}

bool ai_arch_smblockctl_replay_escape_head_service_paths_remain_serialized_corner()
{
    TEST_START();

    const uint64_t zero_threshold = 0x0ULL << 1;
    const uint64_t small_threshold = 0x1ULL << 1;
    uint64_t original = 0ULL;
    uint64_t reprogrammed = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("smblockctl is readable before the AP-80 replay_escape head-service serialization wrapper",
        ai_arch_replay_escape_read_no_except(&original)
    );

    reprogrammed = ai_arch_replay_escape_compose(original, true, zero_threshold);
    TEST_ASSERT("AP-80 wrapper enables replay_escape with threshold=0 so oldest-load escape pressure is reachable while head-service paths run",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-80 phase0: immediate replay_escape still lets the ordinary ld-ld replay wrapper complete, covering oldest-load forward-progress symptoms",
        ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-80 wrapper keeps replay_escape threshold=0 programmed before the uncache/MMIO head-service phase",
        ai_arch_replay_escape_read_smblockctl() == reprogrammed
    );

    TEST_ASSERT("AP-80 phase1: immediate replay_escape does not prevent replay-head NC/MMIO load service from fetching refreshed data",
        ai_micro_hs_load_uncache_replay_head_data_visible()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-80 wrapper re-applies immediate replay_escape after PMA/PBMT head-service environment cleanup",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-80 phase2: dual uncache replay-head entries in the same ROB row still serialize without popcount/assertion-style symptoms",
        ai_micro_hs_same_rob_row_dual_uncache_load_replay_no_popcount_assert_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("AP-80 wrapper re-applies immediate replay_escape before the dual-misalign replay-head phase",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-80 phase3: dual misalign replay-head entries in the same ROB row still use their own staged-recovery path rather than normal escape candidate filtering",
        ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner()
    );

    goto_priv(PRIV_M);
    reprogrammed = ai_arch_replay_escape_compose(original, true, small_threshold);
    TEST_ASSERT("AP-80 wrapper switches to replay_escape threshold=1 before mixing MMIO and misalign head-service entries",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-80 phase4: mixed MMIO/uncache and misalign replay-head service still completes without dropped head entry or stale-service symptoms",
        ai_micro_hs_robhead_mmio_and_misalign_replay_same_window_no_assert_or_drop_corner()
    );

    goto_priv(PRIV_M);
    reprogrammed = ai_arch_replay_escape_compose(original, false, small_threshold);
    TEST_ASSERT("AP-80 wrapper disables replay_escape with threshold=1 for the ordinary-replay no-regression contrast phase",
        ai_arch_replay_escape_write_and_readback(reprogrammed)
    );

    TEST_ASSERT("AP-80 phase5: with replay_escape disabled, ordinary dense replay still makes forward progress, separating normal replay from escape-only gating",
        ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner()
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("restoring smblockctl after the AP-80 replay_escape head-service serialization wrapper completes without exception and readback returns to the original value",
        ai_arch_replay_escape_write_and_readback(original)
    );

    TEST_END();
}
