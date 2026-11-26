#ifndef AI_MICRO_CROSS_16B_CACHEABLE_COMMON_H
#define AI_MICRO_CROSS_16B_CACHEABLE_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>


/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_load_cross_16b_cacheable_data_merge();
bool ai_micro_hs_store_cross_16b_cacheable_data_split();
bool ai_micro_hs_load_cross_16b_cacheable_repeated_misaligned_then_aligned_access_recovers_corner();
bool ai_micro_hs_store_cross_16b_cacheable_repeated_misaligned_then_aligned_access_recovers_corner();
bool ai_micro_hs_load_cross_16b_mab_writeback_shared_ldout_no_drop_corner();
bool ai_micro_hs_load_cross_16b_mab_writeback_shared_ldout_no_cross_bind_adjacent_preserved_corner();
bool ai_micro_hs_store_cross16_head_waits_for_two_databuffer_ports_without_hang_corner();
bool ai_micro_hs_load_not_blocked_by_unrelated_older_unaligned_store_corner();
bool ai_micro_hs_unaligned_store_false_datainvalid_sqidx_not_reported_corner();
bool ai_micro_hs_load_cross_16b_tlb_miss_replay_merge();
bool ai_micro_hs_load_cross_16b_tlb_miss_then_hit_stable_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_load_stable_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_stable_corner();
bool ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_stable_corner();
bool ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_stable_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_tlb_miss_replay_split();
bool ai_micro_hs_store_cross_16b_tlb_miss_then_hit_stable_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_store_stable_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_vector_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner();
bool ai_micro_hs_vector_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner();
bool ai_micro_hs_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner();
bool ai_micro_hs_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner();
bool ai_micro_hs_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner();
bool ai_micro_hs_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_fault_then_samepage_width_switch_no_stale_fake_crosspage_rebind_corner();
bool ai_micro_hs_store_cross_16b_fault_then_samepage_width_switch_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_then_same_16b_direct_store_old_split_state_not_rebound_corner();
bool ai_micro_hs_store_cross_16b_then_same_16b_direct_store_old_split_state_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_stable_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner();
bool ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_7b1b_data_merge_corner();
bool ai_micro_hs_store_cross_16b_7b1b_data_split_corner();
bool ai_micro_hs_load_cross_16b_4b4b_3b5b_2b6b_data_merge_corner();
bool ai_micro_hs_store_cross_16b_4b4b_3b5b_2b6b_data_split_corner();
bool ai_micro_hs_load_cross_16b_5b3b_6b2b_data_merge_corner();
bool ai_micro_hs_store_cross_16b_5b3b_6b2b_data_split_corner();
bool ai_micro_hs_store_cross_16b_offset_width_switch_no_stale_split_template_corner();
bool ai_micro_hs_store_cross_16b_offset_width_switch_no_stale_split_template_adjacent_preserved_corner();
bool ai_micro_hs_store_same_16b_offset_width_switch_no_stale_direct_shift_mask_corner();
bool ai_micro_hs_store_same_16b_offset_width_switch_no_stale_direct_shift_mask_adjacent_preserved_corner();
bool ai_micro_hs_load_same_16b_offset_width_switch_no_stale_direct_data_select_corner();
bool ai_micro_hs_load_same_16b_offset_width_switch_no_stale_direct_data_select_adjacent_preserved_corner();
bool ai_micro_hs_load_same_16b_signedness_switch_no_stale_direct_data_select_corner();
bool ai_micro_hs_load_same_16b_signedness_switch_no_stale_direct_data_select_adjacent_preserved_corner();
bool ai_micro_hs_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_corner();
bool ai_micro_hs_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_adjacent_preserved_corner();
bool ai_micro_hs_same_16b_store_then_signed_unsigned_load_no_stale_data_select_corner();
bool ai_micro_hs_same_16b_store_then_signed_unsigned_load_no_stale_data_select_adjacent_preserved_corner();
bool ai_micro_hs_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_corner();
bool ai_micro_hs_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_adjacent_preserved_corner();
bool ai_micro_hs_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_corner();
bool ai_micro_hs_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_adjacent_preserved_corner();
bool ai_micro_hs_same_16b_load_path_switch_no_stale_direct_or_aligned_select_corner();
bool ai_micro_hs_same_16b_load_path_switch_no_stale_direct_or_aligned_select_adjacent_preserved_corner();
bool ai_micro_hs_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_corner();
bool ai_micro_hs_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_offset_width_switch_no_stale_merge_template_corner();
bool ai_micro_hs_load_cross_16b_offset_width_switch_no_stale_merge_template_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_signedness_switch_no_stale_data_select_corner();
bool ai_micro_hs_load_cross_16b_signedness_switch_no_stale_data_select_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_trigger_breakpoint();
bool ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_before_page_fault_spike_corner();
bool ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_before_access_fault_spike_corner();
bool ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_only_original_matches_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_priority_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_priority_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_both_breakpoint_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_ignored_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_priority_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_priority_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_corner();
bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_spike_corner();
bool ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_before_page_fault_spike_corner();
bool ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_before_access_fault_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_only_original_matches_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_16b_trigger_breakpoint();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_ignored_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_both_breakpoint_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_priority_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_priority_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_corner();
bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_page_fault();
bool ai_micro_hs_load_cross_16b_page_fault_then_valid_cross_16b_load_recovers_corner();
bool ai_micro_hs_load_cross_16b_page_fault_then_valid_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_replayq_delayed_misalign_request_not_rebound_to_reused_entry_corner();
bool ai_micro_hs_replayq_delayed_misalign_request_not_rebound_to_reused_entry_adjacent_preserved_corner();
bool ai_micro_hs_older_replayq_misalign_not_starved_by_repeated_younger_direct_requests_corner();
bool ai_micro_hs_older_replayq_misalign_not_starved_by_repeated_younger_direct_requests_adjacent_preserved_corner();
bool ai_micro_hs_replayq_head_misalign_not_freed_when_direct_exu_misalign_wins_misalignbuffer_corner();
bool ai_micro_hs_shared_misalignbuffer_misalign_mux_no_false_schedule_or_free_of_replay_head_corner();
bool ai_micro_hs_robhead_two_misalign_replay_entries_same_row_follow_uopidx_order_corner();
bool ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner();
bool ai_micro_hs_replay_port_cold_counter_not_inherited_by_next_misalign_replay_owner_corner();
bool ai_micro_hs_replay_port_cold_counter_switch_owner_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_access_fault();
bool ai_micro_hs_load_cross_16b_access_fault_then_valid_cross_16b_load_recovers_corner();
bool ai_micro_hs_load_cross_16b_access_fault_then_valid_cross_16b_load_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery_no_stale_state_corner();
bool ai_micro_hs_load_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_fault_then_different_shape_recovery_no_stale_merge_template_corner();
bool ai_micro_hs_store_cross_16b_page_fault();
bool ai_micro_hs_store_cross_16b_fault_then_different_shape_recovery_no_stale_split_template_corner();
bool ai_micro_hs_load_cross_16b_fault_then_5b3b_6b2b_recovery_no_stale_merge_template_corner();
bool ai_micro_hs_store_cross_16b_fault_then_5b3b_6b2b_recovery_no_stale_split_template_corner();
bool ai_micro_hs_store_cross_16b_page_fault_then_valid_cross_16b_store_recovers_corner();
bool ai_micro_hs_load_cross_16b_fault_then_3b5b_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_hs_store_cross_16b_fault_then_3b5b_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_hs_store_cross_16b_page_fault_then_valid_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_16b_access_fault();
bool ai_micro_hs_store_cross_16b_access_fault_then_valid_cross_16b_store_recovers_corner();
bool ai_micro_hs_store_cross_16b_access_fault_then_valid_cross_16b_store_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_spike_corner();
bool ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_spike_corner();
bool ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_spike_corner();
#define AI_MICRO_CROSS_16B_LOW_PATTERN   0x7766554433221100ULL
#define AI_MICRO_CROSS_16B_HIGH_PATTERN  0xffeeddccbbaa9988ULL
#define AI_MICRO_CROSS_16B_STORE_PATTERN 0x0123456789abcdefULL
#define AI_MICRO_CROSS_16B_STORE_PATTERN_2 0x8877665544332211ULL
#define AI_MICRO_CROSS_16B_WARM_PATTERN  0xfaceb00c1234abcdULL
#define AI_MICRO_CROSS_16B_OFFSET 15U
#define AI_MICRO_CROSS_16B_EXEC_OFFSET 14U
#define AI_MICRO_CROSS_16B_3B1B_OFFSET 13U
#define AI_MICRO_CROSS_16B_7B1B_OFFSET 9U
#define AI_MICRO_CROSS_16B_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MICRO_CROSS_16B_MCONTROL6_LOAD 0x1ULL
#define AI_MICRO_CROSS_16B_MCONTROL6_STORE 0x2ULL
#define AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE 0x4ULL

static inline uintptr_t ai_micro_cross_16b_cacheable_addr_with_offset(enum test_page page, uintptr_t offset)
{
    return hs_page_base(page) + offset;
}

static inline uintptr_t ai_micro_cross_16b_cacheable_addr(enum test_page page)
{
    return ai_micro_cross_16b_cacheable_addr_with_offset(page, AI_MICRO_CROSS_16B_OFFSET);
}

static inline uintptr_t ai_micro_cross_16b_exec_addr(enum test_page page)
{
    return ai_micro_cross_16b_cacheable_addr_with_offset(page, AI_MICRO_CROSS_16B_EXEC_OFFSET);
}

static inline uintptr_t ai_micro_cross_16b_exec_phys(enum test_page page)
{
    return phys_page_base(page) + AI_MICRO_CROSS_16B_EXEC_OFFSET;
}

static inline uintptr_t ai_micro_cross_16b_second_half_addr(enum test_page page)
{
    return ai_micro_cross_16b_cacheable_addr(page) + 1U;
}

static inline pte_t *ai_micro_cross_16b_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_cross_16b_set_hs_leaf(enum test_page page,
    uintptr_t paddr,
    uint64_t flags)
{
    *ai_micro_cross_16b_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uint64_t ai_micro_cross_16b_expected_ld_value(uint64_t low_pattern,
    uint64_t high_pattern)
{
    return ((high_pattern & 0x00ffffffffffffffULL) << 8) |
        ((low_pattern >> 56) & 0xffULL);
}

static inline uint64_t ai_micro_cross_16b_expected_unsigned_window(
    uint64_t low_pattern,
    uint64_t high_pattern,
    unsigned start_byte,
    unsigned size)
{
    uint64_t value = 0ULL;
    uint8_t bytes[16];

    for (unsigned i = 0; i < 8U; ++i) {
        bytes[i] = (uint8_t)((low_pattern >> (8U * i)) & 0xffU);
        bytes[8U + i] = (uint8_t)((high_pattern >> (8U * i)) & 0xffU);
    }

    for (unsigned i = 0; i < size; ++i) {
        value |= ((uint64_t)bytes[start_byte + i]) << (8U * i);
    }

    return value;
}

static inline uint64_t ai_micro_cross_16b_expected_sign_extended_window(
    uint64_t low_pattern,
    uint64_t high_pattern,
    unsigned start_byte,
    unsigned size)
{
    uint64_t value = ai_micro_cross_16b_expected_unsigned_window(
        low_pattern, high_pattern, start_byte, size);

    switch (size) {
    case 1U:
        return (uint64_t)(int64_t)(int8_t)(uint8_t)value;
    case 2U:
        return (uint64_t)(int64_t)(int16_t)(uint16_t)value;
    case 4U:
        return (uint64_t)(int64_t)(int32_t)(uint32_t)value;
    default:
        return value;
    }
}

static inline uint64_t ai_micro_cross_16b_signed_lh_value(uintptr_t addr)
{
    int64_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lh %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(value) : "r"(addr) : "memory");

    return (uint64_t)value;
}

static inline uint64_t ai_micro_cross_16b_signed_lw_value(uintptr_t addr)
{
    int64_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lw %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(value) : "r"(addr) : "memory");

    return (uint64_t)value;
}

static inline uint64_t ai_micro_cross_16b_observed_value(uintptr_t addr,
    unsigned size,
    bool sign_extend)
{
    if (size == 8U) {
        return ld(addr);
    }
    if (size == 4U) {
        return sign_extend ? ai_micro_cross_16b_signed_lw_value(addr) : lwu(addr);
    }
    return sign_extend ? ai_micro_cross_16b_signed_lh_value(addr) : lhu(addr);
}

static inline uint64_t ai_micro_cross_16b_expected_value(uint64_t low_pattern,
    uint64_t high_pattern,
    unsigned start_byte,
    unsigned size,
    bool sign_extend)
{
    return sign_extend ?
        ai_micro_cross_16b_expected_sign_extended_window(low_pattern, high_pattern, start_byte, size) :
        ai_micro_cross_16b_expected_unsigned_window(low_pattern, high_pattern, start_byte, size);
}

static inline uintptr_t ai_micro_cross_16b_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
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

static inline void ai_micro_cross_16b_write_bytes(uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline void ai_micro_cross_16b_overlay_little_endian_bytes(uint8_t *dst,
    const uint8_t *seed,
    unsigned len,
    unsigned start_byte,
    uint64_t value,
    unsigned size)
{
    for (unsigned i = 0; i < len; ++i) {
        dst[i] = seed[i];
    }
    for (unsigned i = 0; i < size; ++i) {
        dst[start_byte + i] = (uint8_t)((value >> (8U * i)) & 0xffU);
    }
}

static inline bool ai_micro_cross_16b_bytes_match(uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void ai_micro_cross_16b_store_by_size(uintptr_t addr,
    unsigned size,
    uint64_t value)
{
    switch (size) {
    case 8U:
        sd(addr, value);
        break;
    case 4U:
        sw(addr, (uint32_t)value);
        break;
    case 2U:
        sh(addr, (uint16_t)value);
        break;
    default:
        sb(addr, (uint8_t)value);
        break;
    }
}

static inline void ai_micro_vle8_to_v8(const uint8_t *src, int vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vle8.v v8, (%0)\n\t"
        ".option pop\n\t"
        :
        : "r"(src), "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_vse8_from_v8(uint8_t *dst, int vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %1, %2\n\t"
        "vse8.v v8, (%0)\n\t"
        ".option pop\n\t"
        : "+r"(dst)
        : "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_cross_16b_fence_i(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".4byte 0x0000100f\n\t"
        ".option pop\n\t"
        :
        :
        : "memory");
}

static inline void ai_micro_cross_16b_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline void ai_micro_cross_16b_clear_mcontrol6_0(void)
{
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
}

static inline uint64_t ai_micro_cross_16b_mcontrol6_0_tdata1(uint64_t access_bits)
{
    return AI_MICRO_CROSS_16B_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_micro_cross_16b_arm_mcontrol6_slot(size_t slot, uintptr_t addr, uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_micro_cross_16b_mcontrol6_0_tdata1(access_bits));
}

static inline void ai_micro_cross_16b_arm_mcontrol6_0(uintptr_t addr, uint64_t access_bits)
{
    ai_micro_cross_16b_arm_mcontrol6_slot(0, addr, access_bits);
}

static inline uint64_t ai_micro_cross_16b_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_micro_cross_16b_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

enum ai_micro_cross_16b_repeated_followup_mode {
    AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL = 0,
    AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES,
    AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT,
    AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX,
};

enum ai_micro_cross_16b_trigger_fault_mode {
    AI_MICRO_CROSS_16B_TRIGGER_PAGE_FAULT = 0,
    AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT,
};

static inline void ai_micro_cross_16b_prepare_trigger_fault_path(bool use_access_fault)
{
    goto_priv(PRIV_HS);
    hspt_init();
    if (use_access_fault) {
        goto_priv(PRIV_M);
        hspt_llptw_ppn_high_bit_overflow_setup();
    } else {
        goto_priv(PRIV_M);
    }
    sfence_vma();
}

static bool ai_micro_cross_16b_scalar_store_repeated_breakpoint_followup(
    enum ai_micro_cross_16b_repeated_followup_mode mode)
{
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
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;
    uintptr_t bad_vaddr = 0;
    uintptr_t fetch_target = 0;
    uint8_t sentinel_before = 0;
    uint8_t sentinel_after = 0;
    bool test_status = true;

    switch (mode) {
    case AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL:
        sentinel_before = 0x5dU;
        sentinel_after = 0xd5U;
        bad_vaddr = ai_micro_cross_16b_make_noncanonical_vaddr(code_vaddr);
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES:
        sentinel_before = 0x5eU;
        sentinel_after = 0xe5U;
        bad_vaddr = (uintptr_t)-1;
        fetch_target = bad_vaddr & ~1ULL;
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        sentinel_before = 0x5fU;
        sentinel_after = 0xf5U;
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX:
        sentinel_before = 0x6aU;
        sentinel_after = 0xa6U;
        break;
    }

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
    TEST_ASSERT("before the scalar cross-16B repeated execute-trigger follow-up probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x8082007005130001ULL);
    TEST_ASSERT("a scalar cross-16B store can rewrite an executable HS block before the repeated execute-trigger follow-up probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the repeated execute-trigger follow-up probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
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
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the scalar cross-16B repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier scalar cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the scalar cross-16B repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    switch (mode) {
    case AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL:
        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the scalar cross-16B execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the scalar cross-16B execute-trigger plus non-canonical fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES:
        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == fetch_target
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the scalar cross-16B execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the scalar cross-16B execute-trigger plus all-ones fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        goto_priv(PRIV_M);
        *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(code_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten executable HS block falls back to instruction access fault when the leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == code_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later leaf-overflow fetch access fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX:
        goto_priv(PRIV_M);
        *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(code_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten executable HS block first falls back to instruction access fault when the leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == code_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the leaf-overflow fetch access fault before repair still leaves the patched bytes and adjacent sentinels unchanged in the scalar cross-16B execute-trigger corner",
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
        TEST_ASSERT("after repairing the leaf entry, the scalar-rewritten executable HS block completes normally again and returns the patched value",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later leaf-overflow fetch access fault, and final leaf repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_16b_vector_store_repeated_breakpoint_followup(
    enum ai_micro_cross_16b_repeated_followup_mode mode)
{
    const uint8_t warm_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x30, 0x00, 0x82, 0x80
    };
    const uint8_t patched_pattern[8] = {
        0x01, 0x00, 0x13, 0x05,
        0x70, 0x00, 0x82, 0x80
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x1f0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x1f0U;
    const uintptr_t code_vaddr = ai_micro_cross_16b_exec_addr(VSRWX_GRWX);
    const uintptr_t code_phys = ai_micro_cross_16b_exec_phys(VSRWX_GRWX);
    const uintptr_t sentinel_before_addr = code_phys - 1U;
    const uintptr_t sentinel_after_addr = code_phys + sizeof(warm_pattern);
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;
    uintptr_t bad_vaddr = 0;
    uintptr_t fetch_target = 0;
    uint8_t sentinel_before = 0;
    uint8_t sentinel_after = 0;
    bool test_status = true;

    switch (mode) {
    case AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL:
        sentinel_before = 0x3fU;
        sentinel_after = 0xf3U;
        bad_vaddr = ai_micro_cross_16b_make_noncanonical_vaddr(code_vaddr);
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES:
        sentinel_before = 0x40U;
        sentinel_after = 0xa4U;
        bad_vaddr = (uintptr_t)-1;
        fetch_target = bad_vaddr & ~1ULL;
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        sentinel_before = 0x41U;
        sentinel_after = 0xb4U;
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX:
        sentinel_before = 0x42U;
        sentinel_after = 0xc4U;
        break;
    }

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(code_phys, warm_pattern, sizeof(warm_pattern));
    ai_micro_cross_16b_write_bytes(source_phys, patched_pattern, sizeof(patched_pattern));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_cross_16b_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-16B repeated execute-trigger follow-up probe rewrites the code bytes, the warmed executable HS block still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-16B repeated execute-trigger follow-up probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)code_vaddr, sizeof(patched_pattern));
    TEST_ASSERT("a vector unit-stride store can rewrite the executable HS block across a 16B boundary before the repeated execute-trigger follow-up probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store rewrites the executable HS block, the patched bytes are visible and the adjacent sentinels stay unchanged before the repeated execute-trigger follow-up probe",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
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
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the vector cross-16B repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the earlier vector cross-16B store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the vector cross-16B repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    switch (mode) {
    case AI_MICRO_CROSS_16B_FOLLOWUP_NONCANONICAL:
        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the vector cross-16B execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the vector cross-16B execute-trigger plus non-canonical fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_ALL_ONES:
        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == fetch_target
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the vector cross-16B execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the vector cross-16B execute-trigger plus all-ones fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        goto_priv(PRIV_M);
        *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(code_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten executable HS block falls back to instruction access fault when the leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == code_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later leaf-overflow fetch access fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_16B_FOLLOWUP_LEAF_OVERFLOW_FIX:
        goto_priv(PRIV_M);
        *ai_micro_cross_16b_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_16b_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(code_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten executable HS block first falls back to instruction access fault when the leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == code_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the leaf-overflow fetch access fault before repair still leaves the patched bytes and adjacent sentinels unchanged in the vector cross-16B execute-trigger corner",
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
        TEST_ASSERT("after repairing the leaf entry, the vector-rewritten executable HS block completes normally again and returns the patched value",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later leaf-overflow fetch access fault, and final leaf repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-16B corner",
            ai_micro_cross_16b_bytes_match(code_phys, patched_pattern, sizeof(patched_pattern)) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

__attribute__((naked, noinline, aligned(4))) static uint64_t ai_micro_cross_16b_execute_load_once(uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld a0, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_cross_16b_execute_store_once(uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_cross_16b_mab_writeback_pair_capture(uintptr_t misalign_vaddr,
    uintptr_t aligned_vaddr, uint64_t *misalign_out, uint64_t *aligned_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "ld t1, 0(a1)\n"
        "sd t0, 0(a2)\n"
        "sd t1, 0(a3)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_cross_16b_mab_writeback_pair_delayed_capture(
    uintptr_t misalign_vaddr, uintptr_t aligned_vaddr,
    uint64_t *misalign_out, uint64_t *aligned_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "li t1, 0xfedcba9876543210\n"
        "li t2, 7\n"
        "divu t3, t1, t2\n"
        "addi t3, t3, 1\n"
        "divu t3, t1, t3\n"
        "andi t3, t3, 0\n"
        "add t3, t3, a1\n"
        "ld t4, 0(t3)\n"
        "sd t0, 0(a2)\n"
        "sd t4, 0(a3)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}





static bool ai_micro_cross_16b_mab_writeback_shared_ldout_window(
    bool check_adjacent)
{
    static const uint64_t misalign_warm_pattern[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL,
    };
    static const uint64_t misalign_low_pattern[] = {
        0x8877665544332211ULL,
        0x1021324354657687ULL,
        0x55aa996633cc77eeULL,
    };
    static const uint64_t misalign_high_pattern[] = {
        0xa1b2c3d4e5f60718ULL,
        0x98badcfe10325476ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t misalign_trailing_pattern[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0xcafebabedeadbeefULL,
    };
    static const uint64_t aligned_warm_pattern[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    static const uint64_t aligned_before_pattern[] = {
        0xdeadbeefcafef00dULL,
        0x0badf00dfacefeedULL,
        0x1122334455667788ULL,
    };
    static const uint64_t aligned_value_pattern[] = {
        0x66778899aabbccddULL,
        0x123456789abcdef0ULL,
        0x0ff1ce55aa773311ULL,
    };
    static const uint64_t aligned_after_pattern[] = {
        0x8877665544332211ULL,
        0xf0e1d2c3b4a59687ULL,
        0x7766554433221100ULL,
    };
    const enum test_page misalign_page = VSRWX_GRWX;
    const enum test_page aligned_page = SWITCH1;
    const uintptr_t misalign_vaddr = ai_micro_cross_16b_cacheable_addr(
        misalign_page);
    const uintptr_t misalign_warm_vaddr = hs_page_base(misalign_page);
    const uintptr_t aligned_warm_vaddr = hs_page_base(aligned_page);
    const uintptr_t aligned_vaddr = hs_page_base(aligned_page) + 0x28U;
    const uintptr_t misalign_base_paddr = phys_page_base(misalign_page);
    const uintptr_t misalign_warm_paddr = misalign_base_paddr;
    const uintptr_t misalign_low_paddr = misalign_base_paddr + 8U;
    const uintptr_t misalign_high_paddr = misalign_base_paddr + 16U;
    const uintptr_t misalign_trailing_paddr = misalign_base_paddr + 24U;
    const uintptr_t aligned_base_paddr = phys_page_base(aligned_page);
    const uintptr_t aligned_warm_paddr = aligned_base_paddr;
    const uintptr_t aligned_before_paddr = aligned_base_paddr + 0x20U;
    const uintptr_t aligned_paddr = aligned_base_paddr + 0x28U;
    const uintptr_t aligned_after_paddr = aligned_base_paddr + 0x30U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t misalign_value = 0ULL;
        uint64_t aligned_value = 0ULL;
        const uint64_t misalign_expected =
            ai_micro_cross_16b_expected_ld_value(
                misalign_low_pattern[round], misalign_high_pattern[round]);

        goto_priv(PRIV_M);
        write64(misalign_warm_paddr, misalign_warm_pattern[round]);
        write64(misalign_low_paddr, misalign_low_pattern[round]);
        write64(misalign_high_paddr, misalign_high_pattern[round]);
        write64(misalign_trailing_paddr, misalign_trailing_pattern[round]);
        write64(aligned_warm_paddr, aligned_warm_pattern[round]);
        write64(aligned_before_paddr, aligned_before_pattern[round]);
        write64(aligned_paddr, aligned_value_pattern[round]);
        write64(aligned_after_paddr, aligned_after_pattern[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        warm0 = ld(misalign_warm_vaddr);
        warm1 = ld(aligned_warm_vaddr);
        TEST_ASSERT("before the shared-ldout collision probe, both the misalign page and the adjacent aligned-load page can still be warmed independently without exception",
            excpt.triggered == false &&
            warm0 == misalign_warm_pattern[round] &&
            warm1 == aligned_warm_pattern[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_mab_writeback_pair_capture(misalign_vaddr,
            aligned_vaddr, &misalign_value, &aligned_value);
        TEST_ASSERT("a back-to-back cross-16B misalign load plus adjacent aligned load keeps both results bound to their own payloads instead of dropping the misalign-buffer writeback on the shared ldout path",
            excpt.triggered == false &&
            misalign_value == misalign_expected &&
            aligned_value == aligned_value_pattern[round]
        );

        misalign_value = 0ULL;
        aligned_value = 0ULL;
        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_mab_writeback_pair_delayed_capture(misalign_vaddr,
            aligned_vaddr, &misalign_value, &aligned_value);
        TEST_ASSERT("even when a small delay is inserted before the adjacent aligned load, the later shared-ldout window still returns the cross-16B misalign result and the aligned result without cross-binding",
            excpt.triggered == false &&
            misalign_value == misalign_expected &&
            aligned_value == aligned_value_pattern[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the shared-ldout load collision probe leaves both participating payload locations unchanged after each round",
            read64(misalign_warm_paddr) == misalign_warm_pattern[round] &&
            read64(misalign_low_paddr) == misalign_low_pattern[round] &&
            read64(misalign_high_paddr) == misalign_high_pattern[round] &&
            read64(misalign_trailing_paddr) == misalign_trailing_pattern[round] &&
            read64(aligned_warm_paddr) == aligned_warm_pattern[round] &&
            read64(aligned_paddr) == aligned_value_pattern[round] &&
            (!check_adjacent ||
                (read64(aligned_before_paddr) == aligned_before_pattern[round] &&
                 read64(aligned_after_paddr) == aligned_after_pattern[round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}










static bool ai_micro_vector_aligned_and_cross_16b_load_stable(bool check_adjacent)
{
    static const uint8_t aligned_patterns[][16] = {
        {
            0x10, 0x11, 0x12, 0x13,
            0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1a, 0x1b,
            0x1c, 0x1d, 0x1e, 0x1f
        },
        {
            0x90, 0x91, 0x92, 0x93,
            0x94, 0x95, 0x96, 0x97,
            0x98, 0x99, 0x9a, 0x9b,
            0x9c, 0x9d, 0x9e, 0x9f
        }
    };
    static const uint8_t cross_patterns[][16] = {
        {
            0x40, 0x41, 0x42, 0x43,
            0x44, 0x45, 0x46, 0x47,
            0x48, 0x49, 0x4a, 0x4b,
            0x4c, 0x4d, 0x4e, 0x4f
        },
        {
            0xc0, 0xc1, 0xc2, 0xc3,
            0xc4, 0xc5, 0xc6, 0xc7,
            0xc8, 0xc9, 0xca, 0xcb,
            0xcc, 0xcd, 0xce, 0xcf
        }
    };
    const uintptr_t aligned_src_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    const uintptr_t aligned_src_paddr = phys_page_base(VSRW_GRW) + 0x180U;
    const uintptr_t cross_src_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t cross_src_paddr = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t aligned_dst_vaddr = hs_page_base(SWITCH1) + 0x80U;
    const uintptr_t aligned_dst_paddr = phys_page_base(SWITCH1) + 0x80U;
    const uintptr_t cross_dst_vaddr = hs_page_base(SWITCH2) + 0x100U;
    const uintptr_t cross_dst_paddr = phys_page_base(SWITCH2) + 0x100U;
    const uint8_t aligned_before = 0x2dU;
    const uint8_t aligned_after = 0xd2U;
    const uint8_t cross_before = 0x3eU;
    const uint8_t cross_after = 0xe3U;
    bool test_status = true;
    bool cross_first = true;

    for (unsigned round = 0; round < 2U; ++round) {
        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(aligned_src_paddr, aligned_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(cross_src_paddr, cross_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(aligned_dst_paddr, cross_patterns[0], 16U);
        ai_micro_cross_16b_write_bytes(cross_dst_paddr, aligned_patterns[0], 16U);
        write8(aligned_dst_paddr - 1U, aligned_before + (uint8_t)round);
        write8(aligned_dst_paddr + 16U, aligned_after + (uint8_t)round);
        write8(cross_dst_paddr - 1U, cross_before + (uint8_t)round);
        write8(cross_dst_paddr + 16U, cross_after + (uint8_t)round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        if (cross_first) {
            ai_micro_vle8_to_v8((const uint8_t *)cross_src_vaddr, 16);
            vle8_to_v6((const uint8_t *)aligned_src_vaddr, 16);
        } else {
            vle8_to_v6((const uint8_t *)aligned_src_vaddr, 16);
            ai_micro_vle8_to_v8((const uint8_t *)cross_src_vaddr, 16);
        }
        TEST_ASSERT("a normal aligned vector load and a cross-16B vector load can retire in adjacent issue order without trapping before the shared-ingress probe stores them out",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        if (cross_first) {
            ai_micro_vse8_from_v8((uint8_t *)cross_dst_vaddr, 16);
            vse8_from_v6((uint8_t *)aligned_dst_vaddr, 16);
        } else {
            vse8_from_v6((uint8_t *)aligned_dst_vaddr, 16);
            ai_micro_vse8_from_v8((uint8_t *)cross_dst_vaddr, 16);
        }
        TEST_ASSERT("storing the adjacent aligned and cross-16B vector-load payloads into separate scratch windows retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the aligned vector-load payload and the cross-16B vector-load payload each reach only their own scratch destination with no cross-binding",
            ai_micro_cross_16b_bytes_match(aligned_dst_paddr, aligned_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(cross_dst_paddr, cross_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("the shared-ingress adjacent vector-load probe also preserves the guard bytes around both scratch destinations",
                read8(aligned_dst_paddr - 1U) == (uint8_t)(aligned_before + round) &&
                read8(aligned_dst_paddr + 16U) == (uint8_t)(aligned_after + round) &&
                read8(cross_dst_paddr - 1U) == (uint8_t)(cross_before + round) &&
                read8(cross_dst_paddr + 16U) == (uint8_t)(cross_after + round)
            );
        }

        cross_first = !cross_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_vector_aligned_and_cross_16b_store_stable(bool check_adjacent)
{
    static const uint8_t aligned_patterns[][16] = {
        {
            0x21, 0x22, 0x23, 0x24,
            0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x2b, 0x2c,
            0x2d, 0x2e, 0x2f, 0x30
        },
        {
            0xa1, 0xa2, 0xa3, 0xa4,
            0xa5, 0xa6, 0xa7, 0xa8,
            0xa9, 0xaa, 0xab, 0xac,
            0xad, 0xae, 0xaf, 0xb0
        }
    };
    static const uint8_t cross_patterns[][16] = {
        {
            0x51, 0x52, 0x53, 0x54,
            0x55, 0x56, 0x57, 0x58,
            0x59, 0x5a, 0x5b, 0x5c,
            0x5d, 0x5e, 0x5f, 0x60
        },
        {
            0xd1, 0xd2, 0xd3, 0xd4,
            0xd5, 0xd6, 0xd7, 0xd8,
            0xd9, 0xda, 0xdb, 0xdc,
            0xdd, 0xde, 0xdf, 0xe0
        }
    };
    const uintptr_t aligned_src_vaddr = hs_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t aligned_src_paddr = phys_page_base(VSRW_GRW) + 0x1c0U;
    const uintptr_t cross_src_vaddr = hs_page_base(SWITCH1) + 0x1e0U;
    const uintptr_t cross_src_paddr = phys_page_base(SWITCH1) + 0x1e0U;
    const uintptr_t aligned_dst_vaddr = hs_page_base(SWITCH2) + 0x140U;
    const uintptr_t aligned_dst_paddr = phys_page_base(SWITCH2) + 0x140U;
    const uintptr_t cross_dst_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t cross_dst_paddr = phys_page_base(VSRWX_GRWX) + 8U;
    const uint8_t aligned_before = 0x36U;
    const uint8_t aligned_after = 0x63U;
    const uint8_t cross_before = 0x47U;
    const uint8_t cross_after = 0x74U;
    bool test_status = true;
    bool cross_first = true;

    for (unsigned round = 0; round < 2U; ++round) {
        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(aligned_src_paddr, aligned_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(cross_src_paddr, cross_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(aligned_dst_paddr, cross_patterns[0], 16U);
        ai_micro_cross_16b_write_bytes(cross_dst_paddr, aligned_patterns[0], 16U);
        write8(aligned_dst_paddr - 1U, aligned_before + (uint8_t)round);
        write8(aligned_dst_paddr + 16U, aligned_after + (uint8_t)round);
        write8(cross_dst_paddr - 1U, cross_before + (uint8_t)round);
        write8(cross_dst_paddr + 16U, cross_after + (uint8_t)round);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)aligned_src_vaddr, 16);
        ai_micro_vle8_to_v8((const uint8_t *)cross_src_vaddr, 16);
        TEST_ASSERT("two independent aligned vector loads can stage the normal-store payload and the cross-16B-store payload before the shared vector-store-ingress probe",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        if (cross_first) {
            ai_micro_vse8_from_v8((uint8_t *)cross_dst_vaddr, 16);
            vse8_from_v6((uint8_t *)aligned_dst_vaddr, 16);
        } else {
            vse8_from_v6((uint8_t *)aligned_dst_vaddr, 16);
            ai_micro_vse8_from_v8((uint8_t *)cross_dst_vaddr, 16);
        }
        TEST_ASSERT("a normal aligned vector store and a cross-16B vector store can retire in adjacent issue order without trapping before the shared store-ingress probe drains them",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the aligned vector-store payload and the cross-16B vector-store payload each reach only their own destination with no shared-ingress cross-binding",
            ai_micro_cross_16b_bytes_match(aligned_dst_paddr, aligned_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(cross_dst_paddr, cross_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("the shared vector-store-ingress adjacent probe also preserves the guard bytes around both aligned and cross-16B destinations",
                read8(aligned_dst_paddr - 1U) == (uint8_t)(aligned_before + round) &&
                read8(aligned_dst_paddr + 16U) == (uint8_t)(aligned_after + round) &&
                read8(cross_dst_paddr - 1U) == (uint8_t)(cross_before + round) &&
                read8(cross_dst_paddr + 16U) == (uint8_t)(cross_after + round)
            );
        }

        cross_first = !cross_first;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_scalar_load_after_vector_store_stable(bool check_adjacent)
{
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint64_t expected_value = 0x2e2d2c2b2a292827ULL;
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
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar cross-16B load stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload across the same cacheable 16B boundary before the scalar split-load stable probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store, the shared boundary bytes match the staged payload before the scalar split-load stable probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-16B store, the adjacent sentinels stay unchanged before the scalar split-load adjacent-preservation probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector cross-16B store touches the shared path, a scalar cross-16B load still returns the vector-written payload without sticky split state",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-load cross-16B stable sequence, the shared boundary bytes still match the vector-written payload",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-load cross-16B adjacent-preservation sequence, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    return test_status;
}




static bool ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x180U;
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x2cU;
    const uint8_t sentinel_after = 0xc2U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar cross-16B trigger-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload across the same cacheable 16B boundary before the scalar trigger-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store, the shared boundary bytes match the staged payload before the scalar trigger-load probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-16B store, the adjacent sentinels stay unchanged before the scalar trigger-load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

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
    TEST_ASSERT("after a vector cross-16B store touches the shared boundary, the first scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-16B trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-16B store, the second scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-16B trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B store, clearing only the execute-PC slot still leaves the scalar cross-16B load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-16B load still completes normally and returns the payload written earlier by the vector cross-16B store",
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-load cross-16B trigger sequence, the shared boundary bytes still match the vector-written payload",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-load cross-16B trigger sequence, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_cross_16b_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const bool use_access_fault =
        fault_mode == AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT;
    enum test_page scalar_page = use_access_fault ? VSRWX_GRWX : VSI_GI;
    uintptr_t source_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    uintptr_t source_phys = phys_page_base(VSRW_GRW) + 0x180U;
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(scalar_page);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = use_access_fault ? 0x3cU : 0x2cU;
    const uint8_t sentinel_after = use_access_fault ? 0xd2U : 0xc2U;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_16b_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar cross-16B repeated-breakpoint fault-recovery load probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload across the same cacheable 16B boundary before the scalar repeated-breakpoint fault-recovery load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-16B store, the shared boundary bytes match the staged payload before the scalar repeated-breakpoint fault-recovery load probe",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-16B store, the adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-16B store touches the shared path, the first scalar dual-trigger split-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-16B trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-16B store, the second scalar dual-trigger split-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-16B trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B store, clearing only the execute-PC slot still leaves the scalar cross-16B load address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar cross-16B load first falls back to load access fault with the original misaligned VA before translation is repaired" :
        "after clearing both trigger slots, the scalar cross-16B load first falls back to load page fault with the original misaligned VA before the fault page is remapped"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback cross-16B load access-fault path still leaves the vector-written boundary bytes unchanged" :
        "the fallback cross-16B load page-fault path still leaves the vector-written boundary bytes unchanged"),
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback cross-16B load access-fault path still leaves the adjacent sentinels unchanged" :
            "the fallback cross-16B load page-fault path still leaves the adjacent sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    if (use_access_fault) {
        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();
    } else {
        ai_micro_cross_16b_set_hs_leaf(VSI_GI, phys_page_base(VSRWX_GRWX),
            PTE_V | PTE_AD | PTE_RW);
        sfence_vma();
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_16b_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after restoring translation, the mixed vector-store then scalar-load path completes normally again and returns the expected overlapping payload" :
        "after remapping the fault page onto the vector-written boundary, the mixed vector-store then scalar-load path completes normally again and returns the expected overlapping payload"),
        excpt.triggered == false &&
        value == 0x2e2d2c2b2a292827ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery load sequence in the vector-store then scalar-load corner, the shared boundary bytes still match the vector-written payload" :
        "after the repeated-breakpoint plus page-fault recovery load sequence in the vector-store then scalar-load corner, the shared boundary bytes still match the vector-written payload"),
        ai_micro_cross_16b_bytes_match(boundary_phys, source_pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery load sequence in the vector-store then scalar-load corner, the adjacent sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery load sequence in the vector-store then scalar-load corner, the adjacent sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}









static bool ai_micro_cross_16b_vector_store_fake_crosspage_state_window(
    bool check_adjacent)
{
    static const uint8_t source_patterns[2][2][16] = {
        {
            { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
              0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f },
            { 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
              0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f },
        },
        {
            { 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
              0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f },
            { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
              0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf },
        },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const uintptr_t source_vaddr[2] = {
        hs_page_base(SWITCH1) + 0x180U,
        hs_page_base(SWITCH2) + 0x240U,
    };
    const uintptr_t source_phys[2] = {
        phys_page_base(SWITCH1) + 0x180U,
        phys_page_base(SWITCH2) + 0x240U,
    };
    const uintptr_t boundary_vaddr[2] = {
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U),
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 40U),
    };
    const uintptr_t page_phys = phys_page_base(VSRWX_GRWX);
    const uintptr_t boundary_phys[2] = {
        page_phys + 8U,
        page_phys + 40U,
    };
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_middle_phys = page_phys + 24U;
    const uintptr_t guard_after_phys = page_phys + 56U;
    const unsigned rounds = 2U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;

        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(source_phys[0], source_patterns[0][round],
            16U);
        ai_micro_cross_16b_write_bytes(source_phys[1], source_patterns[1][round],
            16U);
        ai_micro_cross_16b_write_bytes(boundary_phys[0], init_patterns[0][round],
            16U);
        ai_micro_cross_16b_write_bytes(boundary_phys[1], init_patterns[1][round],
            16U);
        ai_micro_cross_16b_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_cross_16b_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)source_vaddr[first], 16);
        TEST_ASSERT("the first same-page vector source payload stages cleanly before the fake-crosspage-owner reuse probe",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)boundary_vaddr[first], 16);
        fence_iorw_iorw();
        TEST_ASSERT("the first same-page vector cross-16B store commits without exception before the next-boundary reuse probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first same-page vector cross-16B store, only its own boundary updates while the next boundary keeps its seeded image and the in-between gap remains untouched",
            ai_micro_cross_16b_bytes_match(boundary_phys[first],
                source_patterns[first][round], 16U) &&
            ai_micro_cross_16b_bytes_match(boundary_phys[second],
                init_patterns[second][round], 16U) &&
            ai_micro_cross_16b_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the first same-page vector cross-16B store, the outer guard regions remain unchanged in the fake-crosspage-owner reuse probe",
                ai_micro_cross_16b_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_cross_16b_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)source_vaddr[second], 16);
        TEST_ASSERT("the second same-page vector source payload also stages cleanly before boundary reuse is checked",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)boundary_vaddr[second], 16);
        fence_iorw_iorw();
        TEST_ASSERT("the second same-page vector cross-16B store also commits without exception after the earlier fake-crosspage-tagged store",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after two same-page vector cross-16B stores on different boundaries, each boundary keeps only its own payload and the middle gap remains untouched",
            ai_micro_cross_16b_bytes_match(boundary_phys[0],
                source_patterns[0][round], 16U) &&
            ai_micro_cross_16b_bytes_match(boundary_phys[1],
                source_patterns[1][round], 16U) &&
            ai_micro_cross_16b_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the same-page vector boundary-reuse sequence, the outer guard regions also remain unchanged",
                ai_micro_cross_16b_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_cross_16b_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_scalar_store_fake_crosspage_state_window(
    bool check_adjacent)
{
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0x8877665544332211ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe0, 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77,
              0x68, 0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff },
            { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
              0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        },
        {
            { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
              0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
            { 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d,
              0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xf8, 0xe7, 0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
          0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08 },
        { 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12,
          0x01, 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x47, 0x58, 0x69, 0x7a, 0x8b, 0x9c, 0xad, 0xbe },
    };
    const uintptr_t store_vaddr[2] = {
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 15U),
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 47U),
    };
    const uintptr_t page_phys = phys_page_base(VSRWX_GRWX);
    const uintptr_t boundary_phys[2] = {
        page_phys + 8U,
        page_phys + 40U,
    };
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_middle_phys = page_phys + 24U;
    const uintptr_t guard_after_phys = page_phys + 56U;
    const unsigned rounds = 2U;
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;

        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(boundary_phys[0], init_patterns[0][round],
            16U);
        ai_micro_cross_16b_write_bytes(boundary_phys[1], init_patterns[1][round],
            16U);
        ai_micro_cross_16b_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_cross_16b_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, 7U, store_values[0][round], 8U);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, 7U, store_values[1][round], 8U);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(store_vaddr[first], store_values[first][round]);
        fence_iorw_iorw();
        TEST_ASSERT("the first same-page scalar cross-16B store commits without exception before the next-boundary fake-crosspage reuse probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first same-page scalar cross-16B store, only its own boundary reflects the expected eight-byte overlay while the next boundary and the in-between gap keep their seeded images",
            ai_micro_cross_16b_bytes_match(boundary_phys[first],
                expected_boundary[first], 16U) &&
            ai_micro_cross_16b_bytes_match(boundary_phys[second],
                init_patterns[second][round], 16U) &&
            ai_micro_cross_16b_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the first same-page scalar cross-16B store, the outer guard regions remain unchanged in the fake-crosspage scalar probe",
                ai_micro_cross_16b_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_cross_16b_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(store_vaddr[second], store_values[second][round]);
        fence_iorw_iorw();
        TEST_ASSERT("the second same-page scalar cross-16B store also commits without exception after the earlier fake-crosspage-tagged scalar store",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after two same-page scalar cross-16B stores on different boundaries, each boundary keeps only its own expected eight-byte overlay and the middle gap remains untouched",
            ai_micro_cross_16b_bytes_match(boundary_phys[0],
                expected_boundary[0], 16U) &&
            ai_micro_cross_16b_bytes_match(boundary_phys[1],
                expected_boundary[1], 16U) &&
            ai_micro_cross_16b_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the same-page scalar boundary-reuse sequence, the outer guard regions also remain unchanged",
                ai_micro_cross_16b_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_cross_16b_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_scalar_samepage_store_width_switch_window(
    bool check_adjacent)
{
    static const unsigned store_sizes[2] = {
        4U,
        8U,
    };
    static const unsigned start_bytes[2] = {
        5U,
        7U,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0000000089abcdefULL, 0x000000000badf00dULL },
        { 0x13579bdf2468ace0ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xd0, 0xc1, 0xb2, 0xa3, 0x94, 0x85, 0x76, 0x67,
              0x58, 0x49, 0x3a, 0x2b, 0x1c, 0x0d, 0xfe, 0xef },
            { 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98,
              0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f, 0x10 },
        },
        {
            { 0x4f, 0x5e, 0x6d, 0x7c, 0x8b, 0x9a, 0xa9, 0xb8,
              0xc7, 0xd6, 0xe5, 0xf4, 0x03, 0x12, 0x21, 0x30 },
            { 0xa4, 0x93, 0x82, 0x71, 0x60, 0x5f, 0x4e, 0x3d,
              0x2c, 0x1b, 0x0a, 0xf9, 0xe8, 0xd7, 0xc6, 0xb5 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x13, 0x35, 0x57, 0x79, 0x9b, 0xbd, 0xdf, 0xf1 },
        { 0xf2, 0xe0, 0xce, 0xbc, 0xaa, 0x98, 0x86, 0x74 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x10, 0x22, 0x34, 0x46, 0x58, 0x6a, 0x7c, 0x8e,
          0x91, 0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f },
        { 0x8f, 0x7d, 0x6b, 0x59, 0x47, 0x35, 0x23, 0x11,
          0x0e, 0xfc, 0xea, 0xd8, 0xc6, 0xb4, 0xa2, 0x90 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x24, 0x46, 0x68, 0x8a, 0xac, 0xce, 0xe0, 0xf2 },
        { 0x4a, 0x5c, 0x6e, 0x70, 0x82, 0x94, 0xa6, 0xb8 },
    };
    const uintptr_t store_vaddr[2] = {
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX,
            AI_MICRO_CROSS_16B_3B1B_OFFSET),
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 47U),
    };
    const uintptr_t page_phys = phys_page_base(VSRWX_GRWX);
    const uintptr_t boundary_phys[2] = {
        page_phys + 8U,
        page_phys + 40U,
    };
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_middle_phys = page_phys + 24U;
    const uintptr_t guard_after_phys = page_phys + 56U;
    const unsigned rounds = 2U;
    uint8_t expected_boundary[2][16];
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < rounds; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const char *final_msg = check_adjacent ?
            "after the same-page different-width scalar cross-16B store sequence, each boundary keeps only its own width-specific split bytes while the middle gap and outer guards remain unchanged" :
            "after the same-page different-width scalar cross-16B store sequence, each boundary keeps only its own width-specific split bytes and the middle gap remains untouched";

        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(boundary_phys[0], init_patterns[0][round],
            16U);
        ai_micro_cross_16b_write_bytes(boundary_phys[1], init_patterns[1][round],
            16U);
        ai_micro_cross_16b_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_cross_16b_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_boundary[0],
            init_patterns[0][round], 16U, start_bytes[0], store_values[0][round],
            store_sizes[0]);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_boundary[1],
            init_patterns[1][round], 16U, start_bytes[1], store_values[1][round],
            store_sizes[1]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_store_by_size(store_vaddr[first], store_sizes[first],
            store_values[first][round]);
        fence_iorw_iorw();
        TEST_ASSERT("the first same-page different-width scalar cross-16B store commits without exception before the fake-crosspage width-switch follow-up",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first same-page different-width scalar cross-16B store, only its own boundary reflects the expected width-specific byte overlay while the other boundary and middle gap keep their seeded images",
            ai_micro_cross_16b_bytes_match(boundary_phys[first],
                expected_boundary[first], 16U) &&
            ai_micro_cross_16b_bytes_match(boundary_phys[second],
                init_patterns[second][round], 16U) &&
            ai_micro_cross_16b_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the first same-page different-width scalar cross-16B store, the outer guard regions remain unchanged in the fake-crosspage width-switch probe",
                ai_micro_cross_16b_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                ai_micro_cross_16b_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)
            );
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_store_by_size(store_vaddr[second], store_sizes[second],
            store_values[second][round]);
        fence_iorw_iorw();
        TEST_ASSERT("the second same-page different-width scalar cross-16B store also commits without exception after the earlier fake-crosspage-tagged boundary",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            ai_micro_cross_16b_bytes_match(boundary_phys[0],
                expected_boundary[0], 16U) &&
            ai_micro_cross_16b_bytes_match(boundary_phys[1],
                expected_boundary[1], 16U) &&
            ai_micro_cross_16b_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            (!check_adjacent ||
                (ai_micro_cross_16b_bytes_match(guard_before_phys,
                    guard_before_patterns[round], 8U) &&
                 ai_micro_cross_16b_bytes_match(guard_after_phys,
                    guard_after_patterns[round], 8U)))
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_store_fault_then_samepage_width_switch_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES = 2,
        AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES = 2,
        AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS = 8,
        AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_WRAP_STORES = 64
    };
    const bool use_access_fault[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES] = {
        false,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES] = {
        VSI_GI,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES] = {
        SWITCH1,
        VSRW_GRW,
    };
    const enum test_page scratch_pages[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES] = {
        VSRWX_GRWX,
        SWITCH1,
    };
    static const uintptr_t boundary_offsets[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES] = {
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
        47U,
    };
    static const unsigned store_sizes[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES] = {
        4U,
        8U,
    };
    static const unsigned start_bytes[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES] = {
        5U,
        7U,
    };
    static const uint64_t fault_values[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES] = {
        0x00000000a1b2c3d4ULL,
        0x89abcdef01234567ULL,
    };
    static const uint64_t recovery_values[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES][AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES] = {
        { 0x0000000055667788ULL, 0x00000000d4c3b2a1ULL },
        { 0x13579bdf2468ace0ULL, 0xf0e1d2c3b4a59687ULL },
    };
    static const uint8_t ok_init_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES][16] = {
        {
            { 0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28,
              0x19, 0x0a, 0xfb, 0xec, 0xdd, 0xce, 0xbf, 0xa0 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x12, 0x23 },
        },
        {
            { 0x4e, 0x5d, 0x6c, 0x7b, 0x8a, 0x99, 0xa8, 0xb7,
              0xc6, 0xd5, 0xe4, 0xf3, 0x02, 0x11, 0x20, 0x3f },
            { 0xb4, 0xa3, 0x92, 0x81, 0x70, 0x6f, 0x5e, 0x4d,
              0x3c, 0x2b, 0x1a, 0x09, 0xf8, 0xe7, 0xd6, 0xc5 },
        },
    };
    static const uint8_t ok_guard_before_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][8] = {
        { 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe },
        { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 },
    };
    static const uint8_t ok_guard_middle_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][16] = {
        { 0x13, 0x27, 0x3b, 0x4f, 0x53, 0x67, 0x7b, 0x8f,
          0x90, 0xa4, 0xb8, 0xcc, 0xd0, 0xe4, 0xf8, 0x0c },
        { 0x8c, 0x78, 0x64, 0x50, 0x4c, 0x38, 0x24, 0x10,
          0x0f, 0xfb, 0xe7, 0xd3, 0xcf, 0xbb, 0xa7, 0x93 },
    };
    static const uint8_t ok_guard_after_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][8] = {
        { 0x21, 0x43, 0x65, 0x87, 0xa9, 0xcb, 0xed, 0x0f },
        { 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12 },
    };
    static const uint8_t fault_boundary_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][16] = {
        { 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18,
          0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90 },
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
    };
    static const uint8_t fault_guard_before_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][8] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8 },
        { 0xb9, 0xa8, 0x97, 0x86, 0x75, 0x64, 0x53, 0x42 },
    };
    static const uint8_t fault_guard_after_patterns[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][8] = {
        { 0xc1, 0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38 },
        { 0x49, 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0 },
    };
    static const uint64_t scratch_bases[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES][AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS] = {
        {
            0x1111222233334444ULL,
            0x5555666677778888ULL,
            0x9999aaaabbbbccccULL,
            0xddddeeeeffff0001ULL,
            0x0123456789abcdefULL,
            0xfedcba9876543210ULL,
            0x13579bdf2468ace0ULL,
            0x0badf00dfacefeedULL,
        },
        {
            0x1021324354657687ULL,
            0x98badcfe10325476ULL,
            0x3141592653589793ULL,
            0x2718281828459045ULL,
            0x89abcdef01234567ULL,
            0x7766554433221100ULL,
            0x8877665544332211ULL,
            0xa5a55a5ac3c33c3cULL,
        },
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_CASES; ++round) {
        const unsigned fault_idx = use_access_fault[round] ? 1U : 0U;
        const unsigned first = 1U - fault_idx;
        const unsigned second = fault_idx;
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[round]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[round]);
        const uintptr_t scratch_page_phys = phys_page_base(scratch_pages[round]);
        const uintptr_t fault_boundary_phys = fault_page_phys +
            (fault_idx == 0U ? 8U : 40U);
        const uintptr_t fault_guard_before_phys = fault_boundary_phys - 8U;
        const uintptr_t fault_guard_after_phys = fault_boundary_phys + 16U;
        const uintptr_t fault_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(
                fault_pages[round], boundary_offsets[fault_idx]);
        const uintptr_t ok_vaddr[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES] = {
            ai_micro_cross_16b_cacheable_addr_with_offset(
                ok_pages[round], boundary_offsets[0]),
            ai_micro_cross_16b_cacheable_addr_with_offset(
                ok_pages[round], boundary_offsets[1]),
        };
        const uintptr_t ok_boundary_phys[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES] = {
            ok_page_phys + 8U,
            ok_page_phys + 40U,
        };
        const uintptr_t ok_guard_before_phys = ok_page_phys;
        const uintptr_t ok_guard_middle_phys = ok_page_phys + 24U;
        const uintptr_t ok_guard_after_phys = ok_page_phys + 56U;
        const uintptr_t scratch_vaddr = hs_page_base(scratch_pages[round]);
        uint8_t expected_boundary[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_BOUNDARIES][16];
        uint64_t expected_scratch[AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS];
        bool wrap_stores_ok = true;
        const char *fault_msg = use_access_fault[round] ?
            "the faulting scalar cross-16B store first reports store access fault before the later same-page width-switch recovery sequence is attempted" :
            "the faulting scalar cross-16B store first reports store page fault before the later same-page width-switch recovery sequence is attempted";
        const char *fault_unchanged_msg = use_access_fault[round] ?
            "the access-faulting scalar cross-16B store leaves both the fault boundary and the later same-page width-switch recovery page completely unchanged before recovery begins" :
            "the page-faulting scalar cross-16B store leaves both the fault boundary and the later same-page width-switch recovery page completely unchanged before recovery begins";
        const char *wrap_msg = use_access_fault[round] ?
            "after clearing the store access fault, the aligned scalar SQ-wrap stress stores all retire normally before the same-page width-switch recovery probe" :
            "after clearing the store page fault, the aligned scalar SQ-wrap stress stores all retire normally before the same-page width-switch recovery probe";
        const char *first_store_msg = use_access_fault[round] ?
            "after the access-fault cleanup and SQ-wrap stress window, the first same-page different-width scalar cross-16B recovery store commits without exception" :
            "after the page-fault cleanup and SQ-wrap stress window, the first same-page different-width scalar cross-16B recovery store commits without exception";
        const char *first_state_msg = check_adjacent ?
            "after the first post-fault same-page width-switch recovery store, only its own boundary changes while the other boundary, middle gap, outer guards, fault window, and SQ-wrap scratch area remain exact" :
            "after the first post-fault same-page width-switch recovery store, only its own boundary changes while the other boundary, middle gap, fault window, and SQ-wrap scratch area remain exact";
        const char *second_store_msg = use_access_fault[round] ?
            "the second same-page different-width scalar cross-16B recovery store also commits cleanly after the earlier fault episode and SQ-wrap stress window" :
            "the second same-page different-width scalar cross-16B recovery store also commits cleanly after the earlier fault episode and SQ-wrap stress window";
        const char *final_msg = check_adjacent ?
            "after the fault then same-page width-switch recovery sequence, both boundaries keep only their own byte overlays while the fault window, middle gap, outer guards, and SQ-wrap scratch area remain exact" :
            "after the fault then same-page width-switch recovery sequence, both boundaries keep only their own byte overlays while the fault window, middle gap, and SQ-wrap scratch area remain exact";

        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(fault_guard_before_phys,
            fault_guard_before_patterns[round], 8U);
        ai_micro_cross_16b_write_bytes(fault_boundary_phys,
            fault_boundary_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(fault_guard_after_phys,
            fault_guard_after_patterns[round], 8U);
        ai_micro_cross_16b_write_bytes(ok_boundary_phys[0],
            ok_init_patterns[round][0], 16U);
        ai_micro_cross_16b_write_bytes(ok_boundary_phys[1],
            ok_init_patterns[round][1], 16U);
        ai_micro_cross_16b_write_bytes(ok_guard_before_phys,
            ok_guard_before_patterns[round], 8U);
        ai_micro_cross_16b_write_bytes(ok_guard_middle_phys,
            ok_guard_middle_patterns[round], 16U);
        ai_micro_cross_16b_write_bytes(ok_guard_after_phys,
            ok_guard_after_patterns[round], 8U);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_boundary[0],
            ok_init_patterns[round][0], 16U, start_bytes[0],
            recovery_values[0][round], store_sizes[0]);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_boundary[1],
            ok_init_patterns[round][1], 16U, start_bytes[1],
            recovery_values[1][round], store_sizes[1]);
        for (unsigned slot = 0;
             slot < AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS;
             ++slot) {
            expected_scratch[slot] = scratch_bases[round][slot];
            write64(scratch_page_phys + (slot << 3), expected_scratch[slot]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[round]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_store_by_size(fault_vaddr, store_sizes[fault_idx],
            fault_values[round]);
        TEST_ASSERT(fault_msg,
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[round] ? CAUSE_SAF : CAUSE_SPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(fault_unchanged_msg,
            ai_micro_cross_16b_bytes_match(fault_guard_before_phys,
                fault_guard_before_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(fault_boundary_phys,
                fault_boundary_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(fault_guard_after_phys,
                fault_guard_after_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(ok_boundary_phys[0],
                ok_init_patterns[round][0], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_boundary_phys[1],
                ok_init_patterns[round][1], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_guard_before_phys,
                ok_guard_before_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(ok_guard_middle_phys,
                ok_guard_middle_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_guard_after_phys,
                ok_guard_after_patterns[round], 8U)
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        for (unsigned j = 0;
             j < AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_WRAP_STORES;
             ++j) {
            const unsigned slot =
                j & (AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS - 1U);
            const uint64_t value = scratch_bases[round][slot] +
                (uint64_t)(j + 1U) * 0x0101010101010101ULL;

            expected_scratch[slot] = value;
            TEST_SETUP_EXCEPT();
            sd(scratch_vaddr + (slot << 3), value);
            fence_iorw_iorw();
            wrap_stores_ok = wrap_stores_ok && (excpt.triggered == false);
        }
        TEST_ASSERT(wrap_msg, wrap_stores_ok);

        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_store_by_size(ok_vaddr[first], store_sizes[first],
            recovery_values[first][round]);
        fence_iorw_iorw();
        TEST_ASSERT(first_store_msg, excpt.triggered == false);

        goto_priv(PRIV_M);
        TEST_ASSERT(first_state_msg,
            ai_micro_cross_16b_bytes_match(fault_guard_before_phys,
                fault_guard_before_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(fault_boundary_phys,
                fault_boundary_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(fault_guard_after_phys,
                fault_guard_after_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(ok_boundary_phys[first],
                expected_boundary[first], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_boundary_phys[second],
                ok_init_patterns[round][second], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_guard_middle_phys,
                ok_guard_middle_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(scratch_page_phys,
                (const uint8_t *)expected_scratch,
                AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS * 8U) &&
            (!check_adjacent ||
                (ai_micro_cross_16b_bytes_match(ok_guard_before_phys,
                    ok_guard_before_patterns[round], 8U) &&
                 ai_micro_cross_16b_bytes_match(ok_guard_after_phys,
                    ok_guard_after_patterns[round], 8U)))
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_micro_cross_16b_store_by_size(ok_vaddr[second], store_sizes[second],
            recovery_values[second][round]);
        fence_iorw_iorw();
        TEST_ASSERT(second_store_msg, excpt.triggered == false);

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            ai_micro_cross_16b_bytes_match(fault_guard_before_phys,
                fault_guard_before_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(fault_boundary_phys,
                fault_boundary_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(fault_guard_after_phys,
                fault_guard_after_patterns[round], 8U) &&
            ai_micro_cross_16b_bytes_match(ok_boundary_phys[0],
                expected_boundary[0], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_boundary_phys[1],
                expected_boundary[1], 16U) &&
            ai_micro_cross_16b_bytes_match(ok_guard_middle_phys,
                ok_guard_middle_patterns[round], 16U) &&
            ai_micro_cross_16b_bytes_match(scratch_page_phys,
                (const uint8_t *)expected_scratch,
                AI_MICRO_FAULT_SAMEPAGE_WIDTH_SWITCH_SCRATCH_SLOTS * 8U) &&
            (!check_adjacent ||
                (ai_micro_cross_16b_bytes_match(ok_guard_before_phys,
                    ok_guard_before_patterns[round], 8U) &&
                 ai_micro_cross_16b_bytes_match(ok_guard_after_phys,
                    ok_guard_after_patterns[round], 8U)))
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_store_then_same_16b_direct_store_old_split_state_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CROSS_TO_SAME_16B_ROUNDS = 2
    };
    const enum test_page cross_pages[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS] = {
        VSRWX_GRWX,
        VSRWX_GRW,
    };
    const enum test_page direct_pages[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS] = {
        VSRW_GRW,
        VSRWX_GRWX,
    };
    static const uint64_t cross_values[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint64_t direct_values[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS] = {
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    static const uint8_t cross_seed[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS][16] = {
        { 0x50, 0x41, 0x32, 0x23, 0x14, 0x05, 0xf6, 0xe7,
          0xd8, 0xc9, 0xba, 0xab, 0x9c, 0x8d, 0x7e, 0x6f },
        { 0x6e, 0x7d, 0x8c, 0x9b, 0xaa, 0xb9, 0xc8, 0xd7,
          0xe6, 0xf5, 0x04, 0x13, 0x22, 0x31, 0x40, 0x5f },
    };
    static const uint8_t direct_seed[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS][16] = {
        { 0x1f, 0x2e, 0x3d, 0x4c, 0x5b, 0x6a, 0x79, 0x88,
          0x97, 0xa6, 0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x00 },
        { 0xa0, 0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37,
          0x28, 0x19, 0x0a, 0xfb, 0xec, 0xdd, 0xce, 0xbf },
    };
    static const uint8_t cross_guard_before[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS][8] = {
        { 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe },
        { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01 },
    };
    static const uint8_t cross_guard_after[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS][8] = {
        { 0x89, 0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab },
        { 0x24, 0x68, 0xac, 0xe0, 0x13, 0x57, 0x9b, 0xdf },
    };
    static const uint8_t direct_guard_before[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS][8] = {
        { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xf0, 0x11, 0x22 },
        { 0x2a, 0x4b, 0x6c, 0x8d, 0xae, 0xcf, 0xe1, 0x03 },
    };
    static const uint8_t direct_guard_after[AI_MICRO_CROSS_TO_SAME_16B_ROUNDS][8] = {
        { 0x33, 0x55, 0x77, 0x99, 0xbb, 0xdd, 0xf1, 0x13 },
        { 0x14, 0x36, 0x58, 0x7a, 0x9c, 0xbe, 0xd0, 0xf2 },
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_CROSS_TO_SAME_16B_ROUNDS; ++round) {
        const uintptr_t cross_page_phys = phys_page_base(cross_pages[round]);
        const uintptr_t direct_page_phys = phys_page_base(direct_pages[round]);
        const uintptr_t cross_vaddr =
            ai_micro_cross_16b_cacheable_addr(cross_pages[round]);
        const uintptr_t direct_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(direct_pages[round], 17U);
        const uintptr_t cross_boundary_phys = cross_page_phys + 8U;
        const uintptr_t cross_guard_before_phys = cross_boundary_phys - 8U;
        const uintptr_t cross_guard_after_phys = cross_boundary_phys + 16U;
        const uintptr_t direct_boundary_phys = direct_page_phys + 16U;
        const uintptr_t direct_guard_before_phys = direct_boundary_phys - 8U;
        const uintptr_t direct_guard_after_phys = direct_boundary_phys + 16U;
        const char *final_msg = check_adjacent ?
            "after the cross-16B-then-same-16B direct-store sequence, the old boundary keeps its committed image, the direct window keeps only its own eight-byte overlay, and all guards stay unchanged" :
            "after the cross-16B-then-same-16B direct-store sequence, the old boundary keeps its committed image and the direct window keeps only its own eight-byte overlay";
        uint8_t expected_cross[16];
        uint8_t expected_direct[16];

        ai_micro_cross_16b_overlay_little_endian_bytes(expected_cross,
            cross_seed[round], 16U, 7U, cross_values[round], 8U);
        ai_micro_cross_16b_overlay_little_endian_bytes(expected_direct,
            direct_seed[round], 16U, 1U, direct_values[round], 8U);

        goto_priv(PRIV_M);
        ai_micro_cross_16b_write_bytes(cross_boundary_phys, cross_seed[round], 16U);
        ai_micro_cross_16b_write_bytes(direct_boundary_phys, direct_seed[round], 16U);
        ai_micro_cross_16b_write_bytes(cross_guard_before_phys,
            cross_guard_before[round], 8U);
        ai_micro_cross_16b_write_bytes(cross_guard_after_phys,
            cross_guard_after[round], 8U);
        ai_micro_cross_16b_write_bytes(direct_guard_before_phys,
            direct_guard_before[round], 8U);
        ai_micro_cross_16b_write_bytes(direct_guard_after_phys,
            direct_guard_after[round], 8U);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sd(cross_vaddr, cross_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT("the leading genuine scalar cross-16B store commits cleanly before the later same-16B direct-store stale split-state probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the leading scalar cross-16B store, only that older boundary changes while the later same-16B direct-store window keeps its seeded image",
            ai_micro_cross_16b_bytes_match(cross_boundary_phys,
                expected_cross, 16U) &&
            ai_micro_cross_16b_bytes_match(direct_boundary_phys,
                direct_seed[round], 16U)
        );
        if (check_adjacent) {
            TEST_ASSERT("after the leading scalar cross-16B store, both pages' surrounding guards remain unchanged before the same-16B direct-store follow-up",
                ai_micro_cross_16b_bytes_match(cross_guard_before_phys,
                    cross_guard_before[round], 8U) &&
                ai_micro_cross_16b_bytes_match(cross_guard_after_phys,
                    cross_guard_after[round], 8U) &&
                ai_micro_cross_16b_bytes_match(direct_guard_before_phys,
                    direct_guard_before[round], 8U) &&
                ai_micro_cross_16b_bytes_match(direct_guard_after_phys,
                    direct_guard_after[round], 8U)
            );
        }

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(direct_vaddr, direct_values[round]);
        fence_iorw_iorw();
        TEST_ASSERT("after an older scalar cross-16B store has populated split state, a later same-16B direct-misalign sd still commits cleanly",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            ai_micro_cross_16b_bytes_match(cross_boundary_phys,
                expected_cross, 16U) &&
            ai_micro_cross_16b_bytes_match(direct_boundary_phys,
                expected_direct, 16U) &&
            (!check_adjacent ||
                (ai_micro_cross_16b_bytes_match(cross_guard_before_phys,
                    cross_guard_before[round], 8U) &&
                 ai_micro_cross_16b_bytes_match(cross_guard_after_phys,
                    cross_guard_after[round], 8U) &&
                 ai_micro_cross_16b_bytes_match(direct_guard_before_phys,
                    direct_guard_before[round], 8U) &&
                 ai_micro_cross_16b_bytes_match(direct_guard_after_phys,
                    direct_guard_after[round], 8U)))
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_scalar_store_after_vector_load_stable(bool check_adjacent)
{
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    const uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    const uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    const uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    const uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    const uintptr_t sentinel_before_addr = boundary_phys - 1U;
    const uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t value = 0;
    bool test_status = true;

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
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte payload across the same cacheable 16B boundary before the mixed vector-load then scalar cross-16B store stable probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte byte sequence before the scalar split-store stable probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load still matches the exact boundary payload before the scalar split-store stable probe",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-16B load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar split-store adjacent-preservation probe",
            ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the vector cross-16B load touches the shared path, an overlapping scalar cross-16B store still commits normally and overwrites only its own eight-byte window",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-store cross-16B stable sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-store cross-16B adjacent-preservation sequence, the shared boundary sentinels and scratch sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-load then scalar-store cross-16B stable sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    return test_status;
}




































static bool ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

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
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte payload across the same cacheable 16B boundary before the mixed vector-load then scalar trigger-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte byte sequence before the scalar trigger-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load still matches the exact boundary payload before the scalar trigger-store probe",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-16B load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar trigger-store probe",
            ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after a vector cross-16B load touches the shared path, the first scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-16B trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the earlier vector cross-16B load, the second scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-16B trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B load, clearing only the execute-PC slot still leaves the scalar cross-16B store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-16B store still commits normally even though a vector cross-16B load touched the shared path earlier",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-store cross-16B trigger sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-store cross-16B trigger sequence, the shared boundary sentinels and scratch sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-load then scalar-store cross-16B trigger sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    enum ai_micro_cross_16b_trigger_fault_mode fault_mode,
    bool check_adjacent)
{
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0xef,
        0xcd, 0xab, 0x89, 0x67,
        0x45, 0x23, 0x01, 0x4f
    };
    const bool use_access_fault =
        fault_mode == AI_MICRO_CROSS_16B_TRIGGER_ACCESS_FAULT;
    enum test_page scalar_page = use_access_fault ? VSRWX_GRWX : VSI_GI;
    uintptr_t vector_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(VSRWX_GRWX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_16b_cacheable_addr(scalar_page);
    uintptr_t boundary_phys = phys_page_base(VSRWX_GRWX) + 8U;
    uintptr_t scratch_vaddr = hs_page_base(VSRW_GRW) + 0x80U;
    uintptr_t scratch_phys = phys_page_base(VSRW_GRW) + 0x80U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    uintptr_t sentinel_before_addr = boundary_phys - 1U;
    uintptr_t sentinel_after_addr = boundary_phys + 16U;
    const uint8_t sentinel_before = use_access_fault ? 0x5cU : 0x4cU;
    const uint8_t sentinel_after = use_access_fault ? 0xe4U : 0xd4U;
    const uint8_t scratch_before = use_access_fault ? 0x7bU : 0x6bU;
    const uint8_t scratch_after = use_access_fault ? 0xc6U : 0xb6U;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

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
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte payload across the same cacheable 16B boundary before the mixed vector-load then scalar repeated-breakpoint fault-recovery store probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-16B payload into a scratch page preserves the original 16-byte byte sequence before the scalar repeated-breakpoint fault-recovery store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-16B load still matches the exact boundary payload before the scalar repeated-breakpoint fault-recovery store probe",
        ai_micro_cross_16b_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-16B load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery store probe",
            ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, scalar_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after a vector cross-16B load touches the shared path, the first scalar dual-trigger split-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-16B trigger-store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-16B trigger-store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after the earlier vector cross-16B load, the second scalar dual-trigger split-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-16B trigger-store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-16B trigger-store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-16B load, clearing only the execute-PC slot still leaves the scalar cross-16B store address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-16B store probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_prepare_trigger_fault_path(use_access_fault);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar cross-16B store first falls back to store access fault with the original misaligned VA before translation is repaired" :
        "after clearing both trigger slots, the scalar cross-16B store first falls back to store page fault with the original misaligned VA before the fault page is remapped"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == scalar_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback cross-16B store access-fault path still leaves the shared boundary bytes unchanged after the earlier vector load" :
        "the fallback cross-16B store page-fault path still leaves the shared boundary bytes unchanged after the earlier vector load"),
        ai_micro_cross_16b_bytes_match(boundary_phys, pattern, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback cross-16B store access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "the fallback cross-16B store page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    if (use_access_fault) {
        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();
    } else {
        ai_micro_cross_16b_set_hs_leaf(VSI_GI, phys_page_base(VSRWX_GRWX),
            PTE_V | PTE_AD | PTE_RW);
        sfence_vma();
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(scalar_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT((use_access_fault ?
        "after restoring translation, the mixed vector-load then scalar-store path commits normally again" :
        "after remapping the fault page onto the vector-touched boundary, the mixed vector-load then scalar-store path commits normally again"),
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, the final boundary bytes reflect the expected overwrite order" :
        "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_cross_16b_bytes_match(boundary_phys, expected_final, 16U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, a following scalar split-load still sees the latest scalar store payload immediately" :
        "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, a following scalar split-load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == AI_MICRO_CROSS_16B_STORE_PATTERN
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}







static inline void ai_micro_cross_16b_expected_words_after_store(
    uint64_t low_pattern,
    uint64_t high_pattern,
    unsigned start_byte,
    unsigned size,
    uint64_t store_value,
    uint64_t *expected_low,
    uint64_t *expected_high)
{
    uint8_t bytes[16];
    uint64_t low = 0ULL;
    uint64_t high = 0ULL;

    for (unsigned i = 0; i < 8U; ++i) {
        bytes[i] = (uint8_t)((low_pattern >> (8U * i)) & 0xffU);
        bytes[8U + i] = (uint8_t)((high_pattern >> (8U * i)) & 0xffU);
    }

    for (unsigned i = 0; i < size; ++i) {
        bytes[start_byte + i] = (uint8_t)((store_value >> (8U * i)) & 0xffU);
    }

    for (unsigned i = 0; i < 8U; ++i) {
        low |= ((uint64_t)bytes[i]) << (8U * i);
        high |= ((uint64_t)bytes[8U + i]) << (8U * i);
    }

    *expected_low = low;
    *expected_high = high;
}





static bool ai_micro_cross_16b_store_offset_width_switch_no_stale_split_template_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CROSS_16B_STORE_CASES = 3,
        AI_MICRO_CROSS_16B_STORE_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_STORE_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_STORE_CASES] = {
        AI_MICRO_CROSS_16B_7B1B_OFFSET,
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
        AI_MICRO_CROSS_16B_OFFSET,
    };
    const unsigned sizes[AI_MICRO_CROSS_16B_STORE_CASES] = {
        8U,
        4U,
        2U,
    };
    const uint64_t forward_store_values[AI_MICRO_CROSS_16B_STORE_CASES][AI_MICRO_CROSS_16B_STORE_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
    };
    const uint64_t reverse_store_values[AI_MICRO_CROSS_16B_STORE_CASES][AI_MICRO_CROSS_16B_STORE_ROUNDS] = {
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_STORE_CASES][AI_MICRO_CROSS_16B_STORE_ROUNDS] = {
        { 0x1111222233334444ULL, 0x2233445566778899ULL, 0x33445566778899aaULL },
        { 0x4444333322221111ULL, 0x5566778899aabbccULL, 0x66778899aabbccddULL },
        { 0x7777666655554444ULL, 0x8899aabbccddeeffULL, 0x99aabbccddeeff00ULL },
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_STORE_CASES][AI_MICRO_CROSS_16B_STORE_ROUNDS] = {
        { 0x7766554433221100ULL, 0x66554433221100ffULL, 0x554433221100ffeeULL },
        { 0x0123456789abcdefULL, 0x89abcdef01234567ULL, 0x13579bdf2468ace0ULL },
        { 0xfedcba9876543210ULL, 0x0f1e2d3c4b5a6978ULL, 0x2468ace13579bdf0ULL },
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_STORE_CASES][AI_MICRO_CROSS_16B_STORE_ROUNDS] = {
        { 0xffeeddccbbaa9988ULL, 0xeeddccbbaa998877ULL, 0xddccbbaa99887766ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
        { 0x1122334455667788ULL, 0x2233445566778899ULL, 0x33445566778899aaULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_STORE_CASES][AI_MICRO_CROSS_16B_STORE_ROUNDS] = {
        { 0xaaaaaaaa55555555ULL, 0xbbbbbbbb66666666ULL, 0xcccccccc77777777ULL },
        { 0xdddddddd88888888ULL, 0xeeeeeeee99999999ULL, 0xffffffffaaaaaaaaULL },
        { 0x123456789abcdef0ULL, 0x0badf00dfacefeedULL, 0xdecafbadbead1234ULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order cross-16B SH/SW/SD split-store sequence, each boundary and its adjacent sentinels still reflect only that boundary's own current split bytes" :
        "after the reverse-order cross-16B SH/SW/SD split-store sequence, each boundary still reflects only its own current split bytes";
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_STORE_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_STORE_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_STORE_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_STORE_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_STORE_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_STORE_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_STORE_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_CROSS_16B_STORE_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t expected_low[AI_MICRO_CROSS_16B_STORE_CASES];
        uint64_t expected_high[AI_MICRO_CROSS_16B_STORE_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_STORE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the cross-16B store split-template switch probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        sd(vaddr[0], forward_store_values[0][round]);
        sw(vaddr[1], (uint32_t)forward_store_values[1][round]);
        sh(vaddr[2], (uint16_t)forward_store_values[2][round]);
        TEST_ASSERT("back-to-back cross-16B stores with SD 7B+1B, SW 3B+1B, and SH 1B+1B split shapes all commit without exception in the split-template switch probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_STORE_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                (unsigned)(offsets[i] - 8U),
                sizes[i],
                forward_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT("after the forward-order cross-16B SD/SW/SH split-store sequence, each boundary keeps only its own split bytes instead of inheriting a stale previous store-template width or mask",
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_STORE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sh(vaddr[2], (uint16_t)reverse_store_values[2][round]);
        sw(vaddr[1], (uint32_t)reverse_store_values[1][round]);
        sd(vaddr[0], reverse_store_values[0][round]);
        TEST_ASSERT("reversing the cross-16B SH/SW/SD split-store order still commits cleanly, so later wider split stores do not inherit stale narrow split templates and narrow stores do not poison later wider ones",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_STORE_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                (unsigned)(offsets[i] - 8U),
                sizes[i],
                reverse_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT(reverse_msg,
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_store_offset_width_switch_no_stale_direct_shift_mask_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_STORE_CASES = 3,
        AI_MICRO_SAME_16B_STORE_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_STORE_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t offsets[AI_MICRO_SAME_16B_STORE_CASES] = {
        17U,
        21U,
        23U,
    };
    const unsigned window_starts[AI_MICRO_SAME_16B_STORE_CASES] = {
        1U,
        5U,
        7U,
    };
    const unsigned sizes[AI_MICRO_SAME_16B_STORE_CASES] = {
        8U,
        4U,
        2U,
    };
    const uint64_t forward_store_values[AI_MICRO_SAME_16B_STORE_CASES][AI_MICRO_SAME_16B_STORE_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
    };
    const uint64_t reverse_store_values[AI_MICRO_SAME_16B_STORE_CASES][AI_MICRO_SAME_16B_STORE_ROUNDS] = {
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_STORE_CASES][AI_MICRO_SAME_16B_STORE_ROUNDS] = {
        { 0x1111222233334444ULL, 0x2233445566778899ULL, 0x33445566778899aaULL },
        { 0x4444333322221111ULL, 0x5566778899aabbccULL, 0x66778899aabbccddULL },
        { 0x7777666655554444ULL, 0x8899aabbccddeeffULL, 0x99aabbccddeeff00ULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_STORE_CASES][AI_MICRO_SAME_16B_STORE_ROUNDS] = {
        { 0x7766554433221100ULL, 0x66554433221100ffULL, 0x554433221100ffeeULL },
        { 0x0123456789abcdefULL, 0x89abcdef01234567ULL, 0x13579bdf2468ace0ULL },
        { 0xfedcba9876543210ULL, 0x0f1e2d3c4b5a6978ULL, 0x2468ace13579bdf0ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_STORE_CASES][AI_MICRO_SAME_16B_STORE_ROUNDS] = {
        { 0xffeeddccbbaa9988ULL, 0xeeddccbbaa998877ULL, 0xddccbbaa99887766ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
        { 0x1122334455667788ULL, 0x2233445566778899ULL, 0x33445566778899aaULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_STORE_CASES][AI_MICRO_SAME_16B_STORE_ROUNDS] = {
        { 0xaaaaaaaa55555555ULL, 0xbbbbbbbb66666666ULL, 0xcccccccc77777777ULL },
        { 0xdddddddd88888888ULL, 0xeeeeeeee99999999ULL, 0xffffffffaaaaaaaaULL },
        { 0x123456789abcdef0ULL, 0x0badf00dfacefeedULL, 0xdecafbadbead1234ULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B SH/SW/SD direct-store sequence, each 16-byte window and its adjacent sentinels still reflect only that window's own current store bytes" :
        "after the reverse-order same-16B SH/SW/SD direct-store sequence, each 16-byte window still reflects only its own current store bytes";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_STORE_CASES];
    uintptr_t vaddr[AI_MICRO_SAME_16B_STORE_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_STORE_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_STORE_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_STORE_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_STORE_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]) + 8U;
        vaddr[i] = hs_page_base(pages[i]) + offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_STORE_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t expected_low[AI_MICRO_SAME_16B_STORE_CASES];
        uint64_t expected_high[AI_MICRO_SAME_16B_STORE_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the same-16B direct-store template-switch probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        sd(vaddr[0], forward_store_values[0][round]);
        sw(vaddr[1], (uint32_t)forward_store_values[1][round]);
        sh(vaddr[2], (uint16_t)forward_store_values[2][round]);
        TEST_ASSERT("back-to-back same-16B misaligned stores with SD byte+1, SW byte+5, and SH byte+7 direct-path shapes all commit without exception in the direct shift/mask template probe",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                window_starts[i],
                sizes[i],
                forward_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT("after the forward-order same-16B SD/SW/SH direct-store sequence, each 16-byte window keeps only its own current store bytes instead of inheriting a stale previous offset shift or byte-enable template",
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sh(vaddr[2], (uint16_t)reverse_store_values[2][round]);
        sw(vaddr[1], (uint32_t)reverse_store_values[1][round]);
        sd(vaddr[0], reverse_store_values[0][round]);
        TEST_ASSERT("reversing the same-16B SH/SW/SD direct-store order still commits cleanly, so later wider direct-path stores do not inherit stale narrow offset placement and narrow stores do not poison later wider ones",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                window_starts[i],
                sizes[i],
                reverse_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT(reverse_msg,
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_load_offset_width_switch_no_stale_direct_data_select_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_LOAD_CASES = 3,
        AI_MICRO_SAME_16B_LOAD_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_LOAD_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t offsets[AI_MICRO_SAME_16B_LOAD_CASES] = {
        17U,
        21U,
        23U,
    };
    const unsigned window_starts[AI_MICRO_SAME_16B_LOAD_CASES] = {
        1U,
        5U,
        7U,
    };
    const unsigned sizes[AI_MICRO_SAME_16B_LOAD_CASES] = {
        8U,
        4U,
        2U,
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_LOAD_CASES][AI_MICRO_SAME_16B_LOAD_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_LOAD_CASES][AI_MICRO_SAME_16B_LOAD_ROUNDS] = {
        { 0x8070605040302010ULL, 0x0123456789abcde0ULL, 0xf0e1d2c3b4a59687ULL },
        { 0x5566778899aabbccULL, 0xdeadbeefcafef00dULL, 0x3141592653589793ULL },
        { 0xffeeddccbbaa9988ULL, 0x7766554433221100ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_LOAD_CASES][AI_MICRO_SAME_16B_LOAD_ROUNDS] = {
        { 0x1122334455667799ULL, 0x8899aabbccddeeffULL, 0x1021324354657687ULL },
        { 0xddeeff0011223344ULL, 0x0badf00dfacefeedULL, 0x2718281828459045ULL },
        { 0x66778899aabbccddULL, 0xcafebabedeadbeefULL, 0x02468ace13579bdfULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_LOAD_CASES][AI_MICRO_SAME_16B_LOAD_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
    };
    const char *final_msg = check_adjacent ?
        "the same-16B offset/width direct-load probe leaves each page's 16-byte window and adjacent sentinel doublewords unchanged after every round" :
        "the same-16B offset/width direct-load probe leaves each page's participating 16-byte window unchanged after every round";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_LOAD_CASES];
    uintptr_t vaddr[AI_MICRO_SAME_16B_LOAD_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_LOAD_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_LOAD_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_LOAD_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_LOAD_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]) + 8U;
        vaddr[i] = hs_page_base(pages[i]) + offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_LOAD_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t forward0 = 0ULL;
        uint64_t forward1 = 0ULL;
        uint64_t forward2 = 0ULL;
        uint64_t reverse0 = 0ULL;
        uint64_t reverse1 = 0ULL;
        uint64_t reverse2 = 0ULL;
        uint64_t expected[AI_MICRO_SAME_16B_LOAD_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
            expected[i] = ai_micro_cross_16b_expected_unsigned_window(
                low_patterns[i][round],
                high_patterns[i][round],
                window_starts[i],
                sizes[i]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the same-16B offset/width direct-load probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        forward0 = ld(vaddr[0]);
        forward1 = lwu(vaddr[1]);
        forward2 = lhu(vaddr[2]);
        TEST_ASSERT("back-to-back same-16B misaligned loads with LD byte+1, LWU byte+5, and LHU byte+7 direct-path shapes each return only their own current offset/width result instead of inheriting a stale previous direct data-select template",
            excpt.triggered == false &&
            forward0 == expected[0] &&
            forward1 == expected[1] &&
            forward2 == expected[2]
        );

        TEST_SETUP_EXCEPT();
        reverse2 = lhu(vaddr[2]);
        reverse1 = lwu(vaddr[1]);
        reverse0 = ld(vaddr[0]);
        TEST_ASSERT("reversing the same-16B offset/width direct-load order still refreshes the local offset selector per request, so the short direct loads and the later long direct load keep only their own byte windows",
            excpt.triggered == false &&
            reverse0 == expected[0] &&
            reverse1 == expected[1] &&
            reverse2 == expected[2]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            read64(low_phys[0]) == low_patterns[0][round] &&
            read64(high_phys[0]) == high_patterns[0][round] &&
            read64(low_phys[1]) == low_patterns[1][round] &&
            read64(high_phys[1]) == high_patterns[1][round] &&
            read64(low_phys[2]) == low_patterns[2][round] &&
            read64(high_phys[2]) == high_patterns[2][round] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_signedness_switch_no_stale_direct_data_select_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_SIGN_CASES = 4,
        AI_MICRO_SAME_16B_SIGN_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_SIGN_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
        SWITCH1,
    };
    const uintptr_t offsets[AI_MICRO_SAME_16B_SIGN_CASES] = {
        23U,
        23U,
        21U,
        21U,
    };
    const unsigned window_starts[AI_MICRO_SAME_16B_SIGN_CASES] = {
        7U,
        7U,
        5U,
        5U,
    };
    const unsigned sizes[AI_MICRO_SAME_16B_SIGN_CASES] = {
        2U,
        2U,
        4U,
        4U,
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_SIGN_CASES][AI_MICRO_SAME_16B_SIGN_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
        { 0x13579bdf2468ace0ULL, 0x0badf00dfacefeedULL, 0x5566778899aabbccULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_SIGN_CASES][AI_MICRO_SAME_16B_SIGN_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
        { 0xdcba987766554433ULL, 0xc0dec07766554433ULL, 0x89abcd8877665544ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_SIGN_CASES][AI_MICRO_SAME_16B_SIGN_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
        { 0x0123456789abc0feULL, 0x11223344556677c0ULL, 0x2233445566778889ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_SIGN_CASES][AI_MICRO_SAME_16B_SIGN_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
    };
    const char *final_msg = check_adjacent ?
        "the same-16B signedness-switch direct-load probe leaves every participating 16-byte window and all adjacent sentinels unchanged after each round" :
        "the same-16B signedness-switch direct-load probe leaves every participating 16-byte window unchanged after each round";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_SIGN_CASES];
    uintptr_t vaddr[AI_MICRO_SAME_16B_SIGN_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_SIGN_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_SIGN_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_SIGN_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_SIGN_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_SIGN_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]) + 8U;
        vaddr[i] = hs_page_base(pages[i]) + offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_SIGN_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t warm3 = 0ULL;
        uint64_t signed_half = 0ULL;
        uint64_t unsigned_half = 0ULL;
        uint64_t signed_word = 0ULL;
        uint64_t unsigned_word = 0ULL;
        uint64_t reverse_unsigned_word = 0ULL;
        uint64_t reverse_signed_word = 0ULL;
        uint64_t reverse_unsigned_half = 0ULL;
        uint64_t reverse_signed_half = 0ULL;
        uint64_t expected_signed_half = 0ULL;
        uint64_t expected_unsigned_half = 0ULL;
        uint64_t expected_signed_word = 0ULL;
        uint64_t expected_unsigned_word = 0ULL;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_SIGN_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }
        expected_signed_half = ai_micro_cross_16b_expected_sign_extended_window(
            low_patterns[0][round], high_patterns[0][round], window_starts[0], sizes[0]);
        expected_unsigned_half = ai_micro_cross_16b_expected_unsigned_window(
            low_patterns[1][round], high_patterns[1][round], window_starts[1], sizes[1]);
        expected_signed_word = ai_micro_cross_16b_expected_sign_extended_window(
            low_patterns[2][round], high_patterns[2][round], window_starts[2], sizes[2]);
        expected_unsigned_word = ai_micro_cross_16b_expected_unsigned_window(
            low_patterns[3][round], high_patterns[3][round], window_starts[3], sizes[3]);

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        warm3 = ld(warm_vaddr[3]);
        TEST_ASSERT("before the same-16B signedness-switch direct-load probe, all four participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round] &&
            warm3 == leading_patterns[3][round]
        );

        TEST_SETUP_EXCEPT();
        signed_half = ai_micro_cross_16b_signed_lh_value(vaddr[0]);
        unsigned_half = lhu(vaddr[1]);
        signed_word = ai_micro_cross_16b_signed_lw_value(vaddr[2]);
        unsigned_word = lwu(vaddr[3]);
        TEST_ASSERT("back-to-back same-16B misaligned loads that alternate signed and unsigned halfword/word direct-path shapes each return only their own final sign-extended or zero-extended payload instead of inheriting a stale previous data-select mode",
            excpt.triggered == false &&
            signed_half == expected_signed_half &&
            unsigned_half == expected_unsigned_half &&
            signed_word == expected_signed_word &&
            unsigned_word == expected_unsigned_word
        );

        TEST_SETUP_EXCEPT();
        reverse_unsigned_word = lwu(vaddr[3]);
        reverse_signed_word = ai_micro_cross_16b_signed_lw_value(vaddr[2]);
        reverse_unsigned_half = lhu(vaddr[1]);
        reverse_signed_half = ai_micro_cross_16b_signed_lh_value(vaddr[0]);
        TEST_ASSERT("reversing the same-16B signedness-switch direct-load order still refreshes the final data-select mode per request, so unsigned loads do not inherit stale sign extension and signed loads do not lose it",
            excpt.triggered == false &&
            reverse_signed_half == expected_signed_half &&
            reverse_unsigned_half == expected_unsigned_half &&
            reverse_signed_word == expected_signed_word &&
            reverse_unsigned_word == expected_unsigned_word
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            read64(low_phys[0]) == low_patterns[0][round] &&
            read64(high_phys[0]) == high_patterns[0][round] &&
            read64(low_phys[1]) == low_patterns[1][round] &&
            read64(high_phys[1]) == high_patterns[1][round] &&
            read64(low_phys[2]) == low_patterns[2][round] &&
            read64(high_phys[2]) == high_patterns[2][round] &&
            read64(low_phys[3]) == low_patterns[3][round] &&
            read64(high_phys[3]) == high_patterns[3][round] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round] &&
                 read64(leading_phys[3]) == leading_patterns[3][round] &&
                 read64(trailing_phys[3]) == trailing_patterns[3][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_OVERLAP_CASES = 3,
        AI_MICRO_SAME_16B_OVERLAP_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t store_offsets[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        17U,
        21U,
        23U,
    };
    const unsigned store_window_starts[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        1U,
        5U,
        7U,
    };
    const unsigned store_sizes[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        8U,
        4U,
        2U,
    };
    const uintptr_t load_offsets[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        21U,
        17U,
        21U,
    };
    const unsigned load_window_starts[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        5U,
        1U,
        5U,
    };
    const unsigned load_sizes[AI_MICRO_SAME_16B_OVERLAP_CASES] = {
        4U,
        8U,
        4U,
    };
    const uint64_t forward_store_values[AI_MICRO_SAME_16B_OVERLAP_CASES][AI_MICRO_SAME_16B_OVERLAP_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
    };
    const uint64_t reverse_store_values[AI_MICRO_SAME_16B_OVERLAP_CASES][AI_MICRO_SAME_16B_OVERLAP_ROUNDS] = {
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_OVERLAP_CASES][AI_MICRO_SAME_16B_OVERLAP_ROUNDS] = {
        { 0x1111222233334444ULL, 0x2233445566778899ULL, 0x33445566778899aaULL },
        { 0x4444333322221111ULL, 0x5566778899aabbccULL, 0x66778899aabbccddULL },
        { 0x7777666655554444ULL, 0x8899aabbccddeeffULL, 0x99aabbccddeeff00ULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_OVERLAP_CASES][AI_MICRO_SAME_16B_OVERLAP_ROUNDS] = {
        { 0x7766554433221100ULL, 0x66554433221100ffULL, 0x554433221100ffeeULL },
        { 0x0123456789abcdefULL, 0x89abcdef01234567ULL, 0x13579bdf2468ace0ULL },
        { 0xfedcba9876543210ULL, 0x0f1e2d3c4b5a6978ULL, 0x2468ace13579bdf0ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_OVERLAP_CASES][AI_MICRO_SAME_16B_OVERLAP_ROUNDS] = {
        { 0xffeeddccbbaa9988ULL, 0xeeddccbbaa998877ULL, 0xddccbbaa99887766ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
        { 0x1122334455667788ULL, 0x2233445566778899ULL, 0x33445566778899aaULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_OVERLAP_CASES][AI_MICRO_SAME_16B_OVERLAP_ROUNDS] = {
        { 0xaaaaaaaa55555555ULL, 0xbbbbbbbb66666666ULL, 0xcccccccc77777777ULL },
        { 0xdddddddd88888888ULL, 0xeeeeeeee99999999ULL, 0xffffffffaaaaaaaaULL },
        { 0x123456789abcdef0ULL, 0x0badf00dfacefeedULL, 0xdecafbadbead1234ULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B store/load overlap sequence, each immediate overlapping load still returns only the merged bytes from its own current store and all adjacent sentinels remain intact" :
        "after the reverse-order same-16B store/load overlap sequence, each immediate overlapping load still returns only the merged bytes from its own current store";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_OVERLAP_CASES];
    uintptr_t store_vaddr[AI_MICRO_SAME_16B_OVERLAP_CASES];
    uintptr_t load_vaddr[AI_MICRO_SAME_16B_OVERLAP_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_OVERLAP_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_OVERLAP_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_OVERLAP_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_OVERLAP_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_OVERLAP_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]) + 8U;
        store_vaddr[i] = hs_page_base(pages[i]) + store_offsets[i];
        load_vaddr[i] = hs_page_base(pages[i]) + load_offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_OVERLAP_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t observed[AI_MICRO_SAME_16B_OVERLAP_CASES];
        uint64_t expected_low[AI_MICRO_SAME_16B_OVERLAP_CASES];
        uint64_t expected_high[AI_MICRO_SAME_16B_OVERLAP_CASES];
        uint64_t expected_load[AI_MICRO_SAME_16B_OVERLAP_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_OVERLAP_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the same-16B store/load overlap probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        sd(store_vaddr[0], forward_store_values[0][round]);
        observed[0] = lwu(load_vaddr[0]);
        sw(store_vaddr[1], (uint32_t)forward_store_values[1][round]);
        observed[1] = ld(load_vaddr[1]);
        sh(store_vaddr[2], (uint16_t)forward_store_values[2][round]);
        observed[2] = lwu(load_vaddr[2]);
        TEST_ASSERT("back-to-back same-16B direct-path store/load overlap pairs all complete without exception, so the immediate overlapping load path remains live under different store widths and offsets",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_OVERLAP_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                store_window_starts[i],
                store_sizes[i],
                forward_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
            expected_load[i] = ai_micro_cross_16b_expected_unsigned_window(
                expected_low[i],
                expected_high[i],
                load_window_starts[i],
                load_sizes[i]);
        }
        TEST_ASSERT("after the forward-order same-16B store/load overlap sequence, every immediate overlapping load returns only the merged bytes from its own current store plus untouched surrounding bytes",
            observed[0] == expected_load[0] &&
            observed[1] == expected_load[1] &&
            observed[2] == expected_load[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_SAME_16B_OVERLAP_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sh(store_vaddr[2], (uint16_t)reverse_store_values[2][round]);
        observed[2] = lwu(load_vaddr[2]);
        sw(store_vaddr[1], (uint32_t)reverse_store_values[1][round]);
        observed[1] = ld(load_vaddr[1]);
        sd(store_vaddr[0], reverse_store_values[0][round]);
        observed[0] = lwu(load_vaddr[0]);
        TEST_ASSERT("reversing the same-16B store/load overlap order still commits cleanly, so later wider direct-path stores and their immediate overlapping loads do not inherit stale earlier overlap state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_OVERLAP_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                store_window_starts[i],
                store_sizes[i],
                reverse_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
            expected_load[i] = ai_micro_cross_16b_expected_unsigned_window(
                expected_low[i],
                expected_high[i],
                load_window_starts[i],
                load_sizes[i]);
        }
        TEST_ASSERT(reverse_msg,
            observed[0] == expected_load[0] &&
            observed[1] == expected_load[1] &&
            observed[2] == expected_load[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_store_then_signed_unsigned_load_no_stale_data_select_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_STORE_SIGN_CASES = 4,
        AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
        SWITCH1,
    };
    const uintptr_t store_offsets[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        17U,
        17U,
        21U,
        21U,
    };
    const unsigned store_window_starts[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        1U,
        1U,
        5U,
        5U,
    };
    const unsigned store_sizes[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        8U,
        8U,
        4U,
        4U,
    };
    const uintptr_t load_offsets[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        23U,
        23U,
        21U,
        21U,
    };
    const unsigned load_window_starts[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        7U,
        7U,
        5U,
        5U,
    };
    const unsigned load_sizes[AI_MICRO_SAME_16B_STORE_SIGN_CASES] = {
        2U,
        2U,
        4U,
        4U,
    };
    const uint64_t forward_store_values[AI_MICRO_SAME_16B_STORE_SIGN_CASES][AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
    };
    const uint64_t reverse_store_values[AI_MICRO_SAME_16B_STORE_SIGN_CASES][AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS] = {
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
        { 0x3141592653589793ULL, 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL },
        { 0x55aa996633cc77eeULL, 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL },
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_STORE_SIGN_CASES][AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
        { 0x13579bdf2468ace0ULL, 0x0badf00dfacefeedULL, 0x5566778899aabbccULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_STORE_SIGN_CASES][AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
        { 0xdcba987766554433ULL, 0xc0dec07766554433ULL, 0x89abcd8877665544ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_STORE_SIGN_CASES][AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
        { 0x0123456789abc0feULL, 0x11223344556677c0ULL, 0x2233445566778889ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_STORE_SIGN_CASES][AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B store-then-signed/unsigned-load sequence, every immediate overlapping load still returns only the current store bytes with the correct final sign or zero extension and all adjacent sentinels remain intact" :
        "after the reverse-order same-16B store-then-signed/unsigned-load sequence, every immediate overlapping load still returns only the current store bytes with the correct final sign or zero extension";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    uintptr_t store_vaddr[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    uintptr_t load_vaddr[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SIGN_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]) + 8U;
        store_vaddr[i] = hs_page_base(pages[i]) + store_offsets[i];
        load_vaddr[i] = hs_page_base(pages[i]) + load_offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_STORE_SIGN_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t warm3 = 0ULL;
        uint64_t sh_signed = 0ULL;
        uint64_t uh_unsigned = 0ULL;
        uint64_t sw_signed = 0ULL;
        uint64_t uw_unsigned = 0ULL;
        uint64_t sh_signed_rev = 0ULL;
        uint64_t uh_unsigned_rev = 0ULL;
        uint64_t sw_signed_rev = 0ULL;
        uint64_t uw_unsigned_rev = 0ULL;
        uint64_t expected_low[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
        uint64_t expected_high[AI_MICRO_SAME_16B_STORE_SIGN_CASES];
        uint64_t expected_signed_half = 0ULL;
        uint64_t expected_unsigned_half = 0ULL;
        uint64_t expected_signed_word = 0ULL;
        uint64_t expected_unsigned_word = 0ULL;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SIGN_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        warm3 = ld(warm_vaddr[3]);
        TEST_ASSERT("before the same-16B store-then-signed/unsigned-load probe, all four participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round] &&
            warm3 == leading_patterns[3][round]
        );

        TEST_SETUP_EXCEPT();
        sd(store_vaddr[0], forward_store_values[0][round]);
        sh_signed = ai_micro_cross_16b_signed_lh_value(load_vaddr[0]);
        sd(store_vaddr[1], forward_store_values[1][round]);
        uh_unsigned = lhu(load_vaddr[1]);
        sw(store_vaddr[2], (uint32_t)forward_store_values[2][round]);
        sw_signed = ai_micro_cross_16b_signed_lw_value(load_vaddr[2]);
        sw(store_vaddr[3], (uint32_t)forward_store_values[3][round]);
        uw_unsigned = lwu(load_vaddr[3]);
        TEST_ASSERT("back-to-back same-16B direct-path store-then-signed/unsigned-load pairs all complete without exception, so immediate overlapping signedness probes stay live under different store widths",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SIGN_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                store_window_starts[i],
                store_sizes[i],
                forward_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        expected_signed_half = ai_micro_cross_16b_expected_sign_extended_window(
            expected_low[0], expected_high[0], load_window_starts[0], load_sizes[0]);
        expected_unsigned_half = ai_micro_cross_16b_expected_unsigned_window(
            expected_low[1], expected_high[1], load_window_starts[1], load_sizes[1]);
        expected_signed_word = ai_micro_cross_16b_expected_sign_extended_window(
            expected_low[2], expected_high[2], load_window_starts[2], load_sizes[2]);
        expected_unsigned_word = ai_micro_cross_16b_expected_unsigned_window(
            expected_low[3], expected_high[3], load_window_starts[3], load_sizes[3]);
        TEST_ASSERT("after the forward-order same-16B store-then-signed/unsigned-load sequence, every immediate overlapping load returns only the current store bytes with the correct sign-extension or zero-extension semantics",
            sh_signed == expected_signed_half &&
            uh_unsigned == expected_unsigned_half &&
            sw_signed == expected_signed_word &&
            uw_unsigned == expected_unsigned_word &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            read64(low_phys[3]) == expected_low[3] &&
            read64(high_phys[3]) == expected_high[3] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round] &&
                 read64(leading_phys[3]) == leading_patterns[3][round] &&
                 read64(trailing_phys[3]) == trailing_patterns[3][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SIGN_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sw(store_vaddr[3], (uint32_t)reverse_store_values[3][round]);
        uw_unsigned_rev = lwu(load_vaddr[3]);
        sw(store_vaddr[2], (uint32_t)reverse_store_values[2][round]);
        sw_signed_rev = ai_micro_cross_16b_signed_lw_value(load_vaddr[2]);
        sd(store_vaddr[1], reverse_store_values[1][round]);
        uh_unsigned_rev = lhu(load_vaddr[1]);
        sd(store_vaddr[0], reverse_store_values[0][round]);
        sh_signed_rev = ai_micro_cross_16b_signed_lh_value(load_vaddr[0]);
        TEST_ASSERT("reversing the same-16B store-then-signed/unsigned-load order still commits cleanly, so later pairs do not inherit stale earlier final data-select mode",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SIGN_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                store_window_starts[i],
                store_sizes[i],
                reverse_store_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        expected_signed_half = ai_micro_cross_16b_expected_sign_extended_window(
            expected_low[0], expected_high[0], load_window_starts[0], load_sizes[0]);
        expected_unsigned_half = ai_micro_cross_16b_expected_unsigned_window(
            expected_low[1], expected_high[1], load_window_starts[1], load_sizes[1]);
        expected_signed_word = ai_micro_cross_16b_expected_sign_extended_window(
            expected_low[2], expected_high[2], load_window_starts[2], load_sizes[2]);
        expected_unsigned_word = ai_micro_cross_16b_expected_unsigned_window(
            expected_low[3], expected_high[3], load_window_starts[3], load_sizes[3]);
        TEST_ASSERT(reverse_msg,
            sh_signed_rev == expected_signed_half &&
            uh_unsigned_rev == expected_unsigned_half &&
            sw_signed_rev == expected_signed_word &&
            uw_unsigned_rev == expected_unsigned_word &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            read64(low_phys[3]) == expected_low[3] &&
            read64(high_phys[3]) == expected_high[3] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round] &&
                 read64(leading_phys[3]) == leading_patterns[3][round] &&
                 read64(trailing_phys[3]) == trailing_patterns[3][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_STORE_STORE_CASES = 3,
        AI_MICRO_SAME_16B_STORE_STORE_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t first_store_offsets[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        17U,
        21U,
        23U,
    };
    const unsigned first_store_starts[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        1U,
        5U,
        7U,
    };
    const unsigned first_store_sizes[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        8U,
        4U,
        2U,
    };
    const uintptr_t second_store_offsets[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        21U,
        23U,
        17U,
    };
    const unsigned second_store_starts[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        5U,
        7U,
        1U,
    };
    const unsigned second_store_sizes[AI_MICRO_SAME_16B_STORE_STORE_CASES] = {
        4U,
        2U,
        8U,
    };
    const uint64_t forward_first_values[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
    };
    const uint64_t forward_second_values[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
        { 0x0102030405060708ULL, 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
    };
    const uint64_t reverse_first_values[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
        { 0x3141592653589793ULL, 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL },
    };
    const uint64_t reverse_second_values[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_STORE_STORE_CASES][AI_MICRO_SAME_16B_STORE_STORE_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B store/store overlap sequence, every immediate aligned load still sees only the final byte-age-merged window and all adjacent sentinels remain intact" :
        "after the reverse-order same-16B store/store overlap sequence, every immediate aligned load still sees only the final byte-age-merged window";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t first_store_vaddr[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t second_store_vaddr[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t low_load_vaddr[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t high_load_vaddr[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_STORE_STORE_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_STORE_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        uintptr_t page_vaddr = hs_page_base(pages[i]);
        warm_vaddr[i] = page_vaddr + 8U;
        first_store_vaddr[i] = page_vaddr + first_store_offsets[i];
        second_store_vaddr[i] = page_vaddr + second_store_offsets[i];
        low_load_vaddr[i] = page_vaddr + 16U;
        high_load_vaddr[i] = page_vaddr + 24U;
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_STORE_STORE_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t observed_low[AI_MICRO_SAME_16B_STORE_STORE_CASES];
        uint64_t observed_high[AI_MICRO_SAME_16B_STORE_STORE_CASES];
        uint64_t expected_low[AI_MICRO_SAME_16B_STORE_STORE_CASES];
        uint64_t expected_high[AI_MICRO_SAME_16B_STORE_STORE_CASES];
        uint64_t mid_low = 0ULL;
        uint64_t mid_high = 0ULL;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_STORE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the same-16B store/store overlap probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        sd(first_store_vaddr[0], forward_first_values[0][round]);
        sw(second_store_vaddr[0], (uint32_t)forward_second_values[0][round]);
        observed_low[0] = ld(low_load_vaddr[0]);
        observed_high[0] = ld(high_load_vaddr[0]);
        sw(first_store_vaddr[1], (uint32_t)forward_first_values[1][round]);
        sh(second_store_vaddr[1], (uint16_t)forward_second_values[1][round]);
        observed_low[1] = ld(low_load_vaddr[1]);
        observed_high[1] = ld(high_load_vaddr[1]);
        sh(first_store_vaddr[2], (uint16_t)forward_first_values[2][round]);
        sd(second_store_vaddr[2], forward_second_values[2][round]);
        observed_low[2] = ld(low_load_vaddr[2]);
        observed_high[2] = ld(high_load_vaddr[2]);
        TEST_ASSERT("back-to-back same-16B direct-path store/store overlap pairs all complete without exception, so the immediate aligned load path remains live while partially overlapping store bytes are merged",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_STORE_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                first_store_starts[i],
                first_store_sizes[i],
                forward_first_values[i][round],
                &mid_low,
                &mid_high);
            ai_micro_cross_16b_expected_words_after_store(
                mid_low,
                mid_high,
                second_store_starts[i],
                second_store_sizes[i],
                forward_second_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT("after the forward-order same-16B store/store overlap sequence, every immediate aligned load sees exactly the final byte-age merge of the two current stores and untouched surrounding bytes",
            observed_low[0] == expected_low[0] &&
            observed_high[0] == expected_high[0] &&
            observed_low[1] == expected_low[1] &&
            observed_high[1] == expected_high[1] &&
            observed_low[2] == expected_low[2] &&
            observed_high[2] == expected_high[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_STORE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sw(second_store_vaddr[0], (uint32_t)reverse_second_values[0][round]);
        sd(first_store_vaddr[0], reverse_first_values[0][round]);
        observed_low[0] = ld(low_load_vaddr[0]);
        observed_high[0] = ld(high_load_vaddr[0]);
        sh(second_store_vaddr[1], (uint16_t)reverse_second_values[1][round]);
        sw(first_store_vaddr[1], (uint32_t)reverse_first_values[1][round]);
        observed_low[1] = ld(low_load_vaddr[1]);
        observed_high[1] = ld(high_load_vaddr[1]);
        sd(second_store_vaddr[2], reverse_second_values[2][round]);
        sh(first_store_vaddr[2], (uint16_t)reverse_first_values[2][round]);
        observed_low[2] = ld(low_load_vaddr[2]);
        observed_high[2] = ld(high_load_vaddr[2]);
        TEST_ASSERT("reversing the same-16B store/store overlap order still commits cleanly, so later wider or narrower stores do not inherit stale earlier shift state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_STORE_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                second_store_starts[i],
                second_store_sizes[i],
                reverse_second_values[i][round],
                &mid_low,
                &mid_high);
            ai_micro_cross_16b_expected_words_after_store(
                mid_low,
                mid_high,
                first_store_starts[i],
                first_store_sizes[i],
                reverse_first_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT(reverse_msg,
            observed_low[0] == expected_low[0] &&
            observed_high[0] == expected_high[0] &&
            observed_low[1] == expected_low[1] &&
            observed_high[1] == expected_high[1] &&
            observed_low[2] == expected_low[2] &&
            observed_high[2] == expected_high[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_STORE_SWITCH_CASES = 3,
        AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t first_store_offsets[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        17U,
        16U,
        21U,
    };
    const unsigned first_store_starts[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        1U,
        0U,
        5U,
    };
    const unsigned first_store_sizes[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        8U,
        8U,
        4U,
    };
    const uintptr_t second_store_offsets[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        24U,
        23U,
        24U,
    };
    const unsigned second_store_starts[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        8U,
        7U,
        8U,
    };
    const unsigned second_store_sizes[AI_MICRO_SAME_16B_STORE_SWITCH_CASES] = {
        4U,
        2U,
        8U,
    };
    const uint64_t forward_first_values[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
    };
    const uint64_t forward_second_values[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
        { 0x0102030405060708ULL, 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
    };
    const uint64_t reverse_first_values[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
        { 0x3141592653589793ULL, 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL },
    };
    const uint64_t reverse_second_values[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B direct-path and aligned-path store switch sequence, every immediate aligned load still sees only the final merged window and all adjacent sentinels remain intact" :
        "after the reverse-order same-16B direct-path and aligned-path store switch sequence, every immediate aligned load still sees only the final merged window";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t first_store_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t second_store_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t low_load_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t high_load_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        uintptr_t page_vaddr = hs_page_base(pages[i]);
        warm_vaddr[i] = page_vaddr + 8U;
        first_store_vaddr[i] = page_vaddr + first_store_offsets[i];
        second_store_vaddr[i] = page_vaddr + second_store_offsets[i];
        low_load_vaddr[i] = page_vaddr + 16U;
        high_load_vaddr[i] = page_vaddr + 24U;
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_STORE_SWITCH_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t observed_low[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
        uint64_t observed_high[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
        uint64_t expected_low[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
        uint64_t expected_high[AI_MICRO_SAME_16B_STORE_SWITCH_CASES];
        uint64_t mid_low = 0ULL;
        uint64_t mid_high = 0ULL;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the same-16B direct-path and aligned-path store switch probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        sd(first_store_vaddr[0], forward_first_values[0][round]);
        sw(second_store_vaddr[0], (uint32_t)forward_second_values[0][round]);
        observed_low[0] = ld(low_load_vaddr[0]);
        observed_high[0] = ld(high_load_vaddr[0]);
        sd(first_store_vaddr[1], forward_first_values[1][round]);
        sh(second_store_vaddr[1], (uint16_t)forward_second_values[1][round]);
        observed_low[1] = ld(low_load_vaddr[1]);
        observed_high[1] = ld(high_load_vaddr[1]);
        sw(first_store_vaddr[2], (uint32_t)forward_first_values[2][round]);
        sd(second_store_vaddr[2], forward_second_values[2][round]);
        observed_low[2] = ld(low_load_vaddr[2]);
        observed_high[2] = ld(high_load_vaddr[2]);
        TEST_ASSERT("back-to-back same-16B direct-path and aligned-path store-switch pairs all complete without exception, so the immediate aligned load path remains live while the packed store format changes",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                first_store_starts[i],
                first_store_sizes[i],
                forward_first_values[i][round],
                &mid_low,
                &mid_high);
            ai_micro_cross_16b_expected_words_after_store(
                mid_low,
                mid_high,
                second_store_starts[i],
                second_store_sizes[i],
                forward_second_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT("after the forward-order same-16B direct-path and aligned-path store switch sequence, every immediate aligned load sees exactly the final byte-age merge of the two current stores only",
            observed_low[0] == expected_low[0] &&
            observed_high[0] == expected_high[0] &&
            observed_low[1] == expected_low[1] &&
            observed_high[1] == expected_high[1] &&
            observed_low[2] == expected_low[2] &&
            observed_high[2] == expected_high[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sw(second_store_vaddr[0], (uint32_t)reverse_second_values[0][round]);
        sd(first_store_vaddr[0], reverse_first_values[0][round]);
        observed_low[0] = ld(low_load_vaddr[0]);
        observed_high[0] = ld(high_load_vaddr[0]);
        sh(second_store_vaddr[1], (uint16_t)reverse_second_values[1][round]);
        sd(first_store_vaddr[1], reverse_first_values[1][round]);
        observed_low[1] = ld(low_load_vaddr[1]);
        observed_high[1] = ld(high_load_vaddr[1]);
        sd(second_store_vaddr[2], reverse_second_values[2][round]);
        sw(first_store_vaddr[2], (uint32_t)reverse_first_values[2][round]);
        observed_low[2] = ld(low_load_vaddr[2]);
        observed_high[2] = ld(high_load_vaddr[2]);
        TEST_ASSERT("reversing the same-16B direct-path and aligned-path store switch order still commits cleanly, so neither store format inherits stale packed state from the other",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                second_store_starts[i],
                second_store_sizes[i],
                reverse_second_values[i][round],
                &mid_low,
                &mid_high);
            ai_micro_cross_16b_expected_words_after_store(
                mid_low,
                mid_high,
                first_store_starts[i],
                first_store_sizes[i],
                reverse_first_values[i][round],
                &expected_low[i],
                &expected_high[i]);
        }
        TEST_ASSERT(reverse_msg,
            observed_low[0] == expected_low[0] &&
            observed_high[0] == expected_high[0] &&
            observed_low[1] == expected_low[1] &&
            observed_high[1] == expected_high[1] &&
            observed_low[2] == expected_low[2] &&
            observed_high[2] == expected_high[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_load_path_switch_no_stale_direct_or_aligned_select_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_LOAD_SWITCH_CASES = 4,
        AI_MICRO_SAME_16B_LOAD_SWITCH_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
        SWITCH1,
    };
    const uintptr_t first_load_offsets[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        21U,
        24U,
        23U,
        24U,
    };
    const unsigned first_load_starts[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        5U,
        8U,
        7U,
        8U,
    };
    const unsigned first_load_sizes[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        4U,
        8U,
        2U,
        4U,
    };
    const bool first_load_signed[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        false,
        false,
        true,
        true,
    };
    const uintptr_t second_load_offsets[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        24U,
        23U,
        24U,
        21U,
    };
    const unsigned second_load_starts[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        8U,
        7U,
        8U,
        5U,
    };
    const unsigned second_load_sizes[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        8U,
        2U,
        4U,
        4U,
    };
    const bool second_load_signed[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES] = {
        false,
        false,
        false,
        true,
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES][AI_MICRO_SAME_16B_LOAD_SWITCH_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
        { 0x13579bdf2468ace0ULL, 0x0badf00dfacefeedULL, 0x5566778899aabbccULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES][AI_MICRO_SAME_16B_LOAD_SWITCH_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
        { 0xdcba987766554433ULL, 0xc0dec07766554433ULL, 0x89abcd8877665544ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES][AI_MICRO_SAME_16B_LOAD_SWITCH_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
        { 0x0123456789abc0feULL, 0x11223344556677c0ULL, 0x2233445566778889ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES][AI_MICRO_SAME_16B_LOAD_SWITCH_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B direct-path and aligned-path load switch sequence, every load still returns only its own requested byte window and all adjacent sentinels remain intact" :
        "after the reverse-order same-16B direct-path and aligned-path load switch sequence, every load still returns only its own requested byte window";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    uintptr_t first_load_vaddr[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    uintptr_t second_load_vaddr[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_SWITCH_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]) + 8U;
        first_load_vaddr[i] = hs_page_base(pages[i]) + first_load_offsets[i];
        second_load_vaddr[i] = hs_page_base(pages[i]) + second_load_offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_LOAD_SWITCH_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t warm3 = 0ULL;
        uint64_t first_observed[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
        uint64_t second_observed[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
        uint64_t expected_first[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];
        uint64_t expected_second[AI_MICRO_SAME_16B_LOAD_SWITCH_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_SWITCH_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        warm3 = ld(warm_vaddr[3]);
        TEST_ASSERT("before the same-16B direct-path and aligned-path load switch probe, all four participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round] &&
            warm3 == leading_patterns[3][round]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_SWITCH_CASES; ++i) {
            first_observed[i] = ai_micro_cross_16b_observed_value(
                first_load_vaddr[i],
                first_load_sizes[i],
                first_load_signed[i]);
            second_observed[i] = ai_micro_cross_16b_observed_value(
                second_load_vaddr[i],
                second_load_sizes[i],
                second_load_signed[i]);
        }
        TEST_ASSERT("back-to-back same-16B direct-path and aligned-path load-switch pairs all complete without exception, so the final load-select path remains live while load shape changes",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_SWITCH_CASES; ++i) {
            expected_first[i] = ai_micro_cross_16b_expected_value(
                low_patterns[i][round], high_patterns[i][round],
                first_load_starts[i], first_load_sizes[i], first_load_signed[i]);
            expected_second[i] = ai_micro_cross_16b_expected_value(
                low_patterns[i][round], high_patterns[i][round],
                second_load_starts[i], second_load_sizes[i], second_load_signed[i]);
        }
        TEST_ASSERT("after the forward-order same-16B direct-path and aligned-path load switch sequence, every load returns exactly its own requested window with the correct final width and sign/zero extension",
            first_observed[0] == expected_first[0] &&
            second_observed[0] == expected_second[0] &&
            first_observed[1] == expected_first[1] &&
            second_observed[1] == expected_second[1] &&
            first_observed[2] == expected_first[2] &&
            second_observed[2] == expected_second[2] &&
            first_observed[3] == expected_first[3] &&
            second_observed[3] == expected_second[3] &&
            read64(low_phys[0]) == low_patterns[0][round] &&
            read64(high_phys[0]) == high_patterns[0][round] &&
            read64(low_phys[1]) == low_patterns[1][round] &&
            read64(high_phys[1]) == high_patterns[1][round] &&
            read64(low_phys[2]) == low_patterns[2][round] &&
            read64(high_phys[2]) == high_patterns[2][round] &&
            read64(low_phys[3]) == low_patterns[3][round] &&
            read64(high_phys[3]) == high_patterns[3][round] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round] &&
                 read64(leading_phys[3]) == leading_patterns[3][round] &&
                 read64(trailing_phys[3]) == trailing_patterns[3][round]))
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_LOAD_SWITCH_CASES; ++i) {
            second_observed[i] = ai_micro_cross_16b_observed_value(
                second_load_vaddr[i],
                second_load_sizes[i],
                second_load_signed[i]);
            first_observed[i] = ai_micro_cross_16b_observed_value(
                first_load_vaddr[i],
                first_load_sizes[i],
                first_load_signed[i]);
        }
        TEST_ASSERT("reversing the same-16B direct-path and aligned-path load switch order still completes cleanly, so neither load shape inherits stale final select state from the other",
            excpt.triggered == false
        );

        TEST_ASSERT(reverse_msg,
            first_observed[0] == expected_first[0] &&
            second_observed[0] == expected_second[0] &&
            first_observed[1] == expected_first[1] &&
            second_observed[1] == expected_second[1] &&
            first_observed[2] == expected_first[2] &&
            second_observed[2] == expected_second[2] &&
            first_observed[3] == expected_first[3] &&
            second_observed[3] == expected_second[3] &&
            read64(low_phys[0]) == low_patterns[0][round] &&
            read64(high_phys[0]) == high_patterns[0][round] &&
            read64(low_phys[1]) == low_patterns[1][round] &&
            read64(high_phys[1]) == high_patterns[1][round] &&
            read64(low_phys[2]) == low_patterns[2][round] &&
            read64(high_phys[2]) == high_patterns[2][round] &&
            read64(low_phys[3]) == low_patterns[3][round] &&
            read64(high_phys[3]) == high_patterns[3][round] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round] &&
                 read64(leading_phys[3]) == leading_patterns[3][round] &&
                 read64(trailing_phys[3]) == trailing_patterns[3][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES = 3,
        AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t first_store_offsets[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        17U,
        16U,
        21U,
    };
    const unsigned first_store_starts[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        1U,
        0U,
        5U,
    };
    const unsigned first_store_sizes[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        8U,
        8U,
        4U,
    };
    const uintptr_t second_store_offsets[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        24U,
        23U,
        24U,
    };
    const unsigned second_store_starts[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        8U,
        7U,
        8U,
    };
    const unsigned second_store_sizes[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        4U,
        2U,
        8U,
    };
    const uintptr_t load_offsets[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        23U,
        21U,
        21U,
    };
    const unsigned load_starts[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        7U,
        5U,
        5U,
    };
    const unsigned load_sizes[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        2U,
        4U,
        4U,
    };
    const bool load_signed[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES] = {
        false,
        false,
        true,
    };
    const uint64_t forward_first_values[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
    };
    const uint64_t forward_second_values[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL, 0x2718281828459045ULL },
        { 0x0102030405060708ULL, 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
    };
    const uint64_t reverse_first_values[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0xdeadbeefcafef00dULL, 0x02468ace13579bdfULL, 0x0102030405060708ULL },
        { 0x1122334455667788ULL, 0xa1b2c3d4e5f60718ULL, 0xcafebabedeadbeefULL },
        { 0x3141592653589793ULL, 0x76543210fedcba98ULL, 0xffeeddccbbaa9988ULL },
    };
    const uint64_t reverse_second_values[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x89abcdef01234567ULL, 0x0badf00dfacefeedULL, 0x55aa996633cc77eeULL },
        { 0xfedcba9876543210ULL, 0x1021324354657687ULL, 0x3141592653589793ULL },
        { 0x0123456789abcdefULL, 0x8877665544332211ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t leading_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
    };
    const uint64_t low_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
    };
    const uint64_t high_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES][AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
    };
    const char *reverse_msg = check_adjacent ?
        "after the reverse-order same-16B direct-path and aligned-path store-switch plus overlapping-load sequence, every immediate signed or unsigned load still returns only the final merged bytes and all adjacent sentinels remain intact" :
        "after the reverse-order same-16B direct-path and aligned-path store-switch plus overlapping-load sequence, every immediate signed or unsigned load still returns only the final merged bytes";
    uintptr_t warm_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t first_store_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t second_store_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t load_vaddr[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t leading_phys[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t low_phys[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t high_phys[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    uintptr_t trailing_phys[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        uintptr_t page_vaddr = hs_page_base(pages[i]);
        warm_vaddr[i] = page_vaddr + 8U;
        first_store_vaddr[i] = page_vaddr + first_store_offsets[i];
        second_store_vaddr[i] = page_vaddr + second_store_offsets[i];
        load_vaddr[i] = page_vaddr + load_offsets[i];
        leading_phys[i] = page_phys + 8U;
        low_phys[i] = page_phys + 16U;
        high_phys[i] = page_phys + 24U;
        trailing_phys[i] = page_phys + 32U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t observed_load[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
        uint64_t expected_load[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
        uint64_t expected_low[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
        uint64_t expected_high[AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES];
        uint64_t mid_low = 0ULL;
        uint64_t mid_high = 0ULL;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the same-16B direct-path and aligned-path store-switch plus overlapping-load probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        sd(first_store_vaddr[0], forward_first_values[0][round]);
        sw(second_store_vaddr[0], (uint32_t)forward_second_values[0][round]);
        observed_load[0] = ai_micro_cross_16b_observed_value(
            load_vaddr[0], load_sizes[0], load_signed[0]);
        sd(first_store_vaddr[1], forward_first_values[1][round]);
        sh(second_store_vaddr[1], (uint16_t)forward_second_values[1][round]);
        observed_load[1] = ai_micro_cross_16b_observed_value(
            load_vaddr[1], load_sizes[1], load_signed[1]);
        sw(first_store_vaddr[2], (uint32_t)forward_first_values[2][round]);
        sd(second_store_vaddr[2], forward_second_values[2][round]);
        observed_load[2] = ai_micro_cross_16b_observed_value(
            load_vaddr[2], load_sizes[2], load_signed[2]);
        TEST_ASSERT("back-to-back same-16B direct-path and aligned-path store-switch pairs all complete without exception, so the immediate overlapping load path remains live while the packed store format changes",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                first_store_starts[i],
                first_store_sizes[i],
                forward_first_values[i][round],
                &mid_low,
                &mid_high);
            ai_micro_cross_16b_expected_words_after_store(
                mid_low,
                mid_high,
                second_store_starts[i],
                second_store_sizes[i],
                forward_second_values[i][round],
                &expected_low[i],
                &expected_high[i]);
            expected_load[i] = ai_micro_cross_16b_expected_value(
                expected_low[i],
                expected_high[i],
                load_starts[i],
                load_sizes[i],
                load_signed[i]);
        }
        TEST_ASSERT("after the forward-order same-16B direct-path and aligned-path store-switch sequence, every immediate overlapping signed or unsigned load returns only the final merged bytes with the correct final width and sign/zero extension",
            observed_load[0] == expected_load[0] &&
            observed_load[1] == expected_load[1] &&
            observed_load[2] == expected_load[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        sw(second_store_vaddr[0], (uint32_t)reverse_second_values[0][round]);
        sd(first_store_vaddr[0], reverse_first_values[0][round]);
        observed_load[0] = ai_micro_cross_16b_observed_value(
            load_vaddr[0], load_sizes[0], load_signed[0]);
        sh(second_store_vaddr[1], (uint16_t)reverse_second_values[1][round]);
        sd(first_store_vaddr[1], reverse_first_values[1][round]);
        observed_load[1] = ai_micro_cross_16b_observed_value(
            load_vaddr[1], load_sizes[1], load_signed[1]);
        sd(second_store_vaddr[2], reverse_second_values[2][round]);
        sw(first_store_vaddr[2], (uint32_t)reverse_first_values[2][round]);
        observed_load[2] = ai_micro_cross_16b_observed_value(
            load_vaddr[2], load_sizes[2], load_signed[2]);
        TEST_ASSERT("reversing the same-16B direct-path and aligned-path store-switch order still commits cleanly, so the immediate overlapping load does not inherit stale merge or final select state from the previous store format",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_SAME_16B_STORE_SWITCH_OVERLAP_LOAD_CASES; ++i) {
            ai_micro_cross_16b_expected_words_after_store(
                low_patterns[i][round],
                high_patterns[i][round],
                second_store_starts[i],
                second_store_sizes[i],
                reverse_second_values[i][round],
                &mid_low,
                &mid_high);
            ai_micro_cross_16b_expected_words_after_store(
                mid_low,
                mid_high,
                first_store_starts[i],
                first_store_sizes[i],
                reverse_first_values[i][round],
                &expected_low[i],
                &expected_high[i]);
            expected_load[i] = ai_micro_cross_16b_expected_value(
                expected_low[i],
                expected_high[i],
                load_starts[i],
                load_sizes[i],
                load_signed[i]);
        }
        TEST_ASSERT(reverse_msg,
            observed_load[0] == expected_load[0] &&
            observed_load[1] == expected_load[1] &&
            observed_load[2] == expected_load[2] &&
            read64(low_phys[0]) == expected_low[0] &&
            read64(high_phys[0]) == expected_high[0] &&
            read64(low_phys[1]) == expected_low[1] &&
            read64(high_phys[1]) == expected_high[1] &&
            read64(low_phys[2]) == expected_low[2] &&
            read64(high_phys[2]) == expected_high[2] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_offset_width_switch_no_stale_merge_template_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CROSS_16B_TEMPLATE_CASES = 3,
        AI_MICRO_CROSS_16B_TEMPLATE_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_TEMPLATE_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_TEMPLATE_CASES] = {
        9U,
        13U,
        15U,
    };
    const unsigned sizes[AI_MICRO_CROSS_16B_TEMPLATE_CASES] = {
        8U,
        4U,
        2U,
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_TEMPLATE_CASES][AI_MICRO_CROSS_16B_TEMPLATE_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_TEMPLATE_CASES][AI_MICRO_CROSS_16B_TEMPLATE_ROUNDS] = {
        { 0x8070605040302010ULL, 0x0123456789abcde0ULL, 0xf0e1d2c3b4a59687ULL },
        { 0x5566778899aabbccULL, 0xdeadbeefcafef00dULL, 0x3141592653589793ULL },
        { 0xffeeddccbbaa9988ULL, 0x7766554433221100ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_TEMPLATE_CASES][AI_MICRO_CROSS_16B_TEMPLATE_ROUNDS] = {
        { 0x1122334455667799ULL, 0x8899aabbccddeeffULL, 0x1021324354657687ULL },
        { 0xddeeff0011223344ULL, 0x0badf00dfacefeedULL, 0x2718281828459045ULL },
        { 0x66778899aabbccddULL, 0xcafebabedeadbeefULL, 0x02468ace13579bdfULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_TEMPLATE_CASES][AI_MICRO_CROSS_16B_TEMPLATE_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
    };
    const char *final_msg = check_adjacent ?
        "the cross-16B offset/width template-switch probe leaves each page's boundary doublewords and adjacent sentinel doublewords unchanged after every round" :
        "the cross-16B offset/width template-switch probe leaves each page's participating boundary doublewords unchanged after every round";
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_TEMPLATE_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_TEMPLATE_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_TEMPLATE_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_TEMPLATE_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_TEMPLATE_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_TEMPLATE_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_TEMPLATE_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_CROSS_16B_TEMPLATE_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t forward0 = 0ULL;
        uint64_t forward1 = 0ULL;
        uint64_t forward2 = 0ULL;
        uint64_t reverse0 = 0ULL;
        uint64_t reverse1 = 0ULL;
        uint64_t reverse2 = 0ULL;
        uint64_t expected[AI_MICRO_CROSS_16B_TEMPLATE_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_TEMPLATE_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
            expected[i] = ai_micro_cross_16b_expected_unsigned_window(
                low_patterns[i][round],
                high_patterns[i][round],
                (unsigned)(offsets[i] - 8U),
                sizes[i]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        TEST_ASSERT("before the cross-16B offset/width template-switch probe, all three participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round]
        );

        TEST_SETUP_EXCEPT();
        forward0 = ld(vaddr[0]);
        forward1 = lwu(vaddr[1]);
        forward2 = lhu(vaddr[2]);
        TEST_ASSERT("back-to-back cross-16B loads with LD 7B+1B, LWU 3B+1B, and LHU 1B+1B split shapes each return only their own current offset/width merge result instead of inheriting a stale previous merge template",
            excpt.triggered == false &&
            forward0 == expected[0] &&
            forward1 == expected[1] &&
            forward2 == expected[2]
        );

        TEST_SETUP_EXCEPT();
        reverse2 = lhu(vaddr[2]);
        reverse1 = lwu(vaddr[1]);
        reverse0 = ld(vaddr[0]);
        TEST_ASSERT("reversing the same cross-16B offset/width switch order still refreshes the merge template per request, so the short split loads and the later long split load keep their own zero-extended or full-width payloads",
            excpt.triggered == false &&
            reverse0 == expected[0] &&
            reverse1 == expected[1] &&
            reverse2 == expected[2]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            read64(low_phys[0]) == low_patterns[0][round] &&
            read64(high_phys[0]) == high_patterns[0][round] &&
            read64(low_phys[1]) == low_patterns[1][round] &&
            read64(high_phys[1]) == high_patterns[1][round] &&
            read64(low_phys[2]) == low_patterns[2][round] &&
            read64(high_phys[2]) == high_patterns[2][round] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_signedness_switch_no_stale_data_select_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CROSS_16B_SIGN_CASES = 4,
        AI_MICRO_CROSS_16B_SIGN_ROUNDS = 3
    };
    const enum test_page pages[AI_MICRO_CROSS_16B_SIGN_CASES] = {
        VSRWX_GRWX,
        VSRW_GRW,
        VSRWX_GRW,
        SWITCH1,
    };
    const uintptr_t offsets[AI_MICRO_CROSS_16B_SIGN_CASES] = {
        AI_MICRO_CROSS_16B_OFFSET,
        AI_MICRO_CROSS_16B_OFFSET,
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
    };
    const uint64_t leading_patterns[AI_MICRO_CROSS_16B_SIGN_CASES][AI_MICRO_CROSS_16B_SIGN_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL, 0x2233445566778899ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL, 0x33445566778899aaULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL, 0x445566778899aabbULL },
        { 0x13579bdf2468ace0ULL, 0x0badf00dfacefeedULL, 0x5566778899aabbccULL },
    };
    const uint64_t low_patterns[AI_MICRO_CROSS_16B_SIGN_CASES][AI_MICRO_CROSS_16B_SIGN_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL, 0x81cc776644ee5599ULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL, 0xfe66778899aabbccULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL, 0x8090a01122334455ULL },
        { 0xdcba987766554433ULL, 0xc0dec07766554433ULL, 0x89abcd8877665544ULL },
    };
    const uint64_t high_patterns[AI_MICRO_CROSS_16B_SIGN_CASES][AI_MICRO_CROSS_16B_SIGN_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL, 0x55443322110000feULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL, 0x10213243546576c0ULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL, 0x9988776655443380ULL },
        { 0x0123456789abc0feULL, 0x11223344556677c0ULL, 0x2233445566778889ULL },
    };
    const uint64_t trailing_patterns[AI_MICRO_CROSS_16B_SIGN_CASES][AI_MICRO_CROSS_16B_SIGN_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL, 0xdeadbeef0badf00dULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL, 0x9988776655443322ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0xfaceb00c1234abcdULL },
    };
    const char *final_msg = check_adjacent ?
        "the cross-16B signedness-switch probe leaves every participating boundary doubleword and all adjacent sentinels unchanged after each round" :
        "the cross-16B signedness-switch probe leaves every participating boundary doubleword unchanged after each round";
    uintptr_t warm_vaddr[AI_MICRO_CROSS_16B_SIGN_CASES];
    uintptr_t vaddr[AI_MICRO_CROSS_16B_SIGN_CASES];
    uintptr_t leading_phys[AI_MICRO_CROSS_16B_SIGN_CASES];
    uintptr_t low_phys[AI_MICRO_CROSS_16B_SIGN_CASES];
    uintptr_t high_phys[AI_MICRO_CROSS_16B_SIGN_CASES];
    uintptr_t trailing_phys[AI_MICRO_CROSS_16B_SIGN_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_SIGN_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(pages[i]);
        warm_vaddr[i] = hs_page_base(pages[i]);
        vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(pages[i], offsets[i]);
        leading_phys[i] = page_phys;
        low_phys[i] = page_phys + 8U;
        high_phys[i] = page_phys + 16U;
        trailing_phys[i] = page_phys + 24U;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_MICRO_CROSS_16B_SIGN_ROUNDS; ++round) {
        uint64_t warm0 = 0ULL;
        uint64_t warm1 = 0ULL;
        uint64_t warm2 = 0ULL;
        uint64_t warm3 = 0ULL;
        uint64_t sh_signed = 0ULL;
        uint64_t uh_unsigned = 0ULL;
        uint64_t sw_signed = 0ULL;
        uint64_t uw_unsigned = 0ULL;
        uint64_t sh_signed_rev = 0ULL;
        uint64_t uh_unsigned_rev = 0ULL;
        uint64_t sw_signed_rev = 0ULL;
        uint64_t uw_unsigned_rev = 0ULL;
        const uint64_t expected_signed_half =
            ai_micro_cross_16b_expected_sign_extended_window(
                low_patterns[0][round], high_patterns[0][round], 7U, 2U);
        const uint64_t expected_unsigned_half =
            ai_micro_cross_16b_expected_unsigned_window(
                low_patterns[1][round], high_patterns[1][round], 7U, 2U);
        const uint64_t expected_signed_word =
            ai_micro_cross_16b_expected_sign_extended_window(
                low_patterns[2][round], high_patterns[2][round], 5U, 4U);
        const uint64_t expected_unsigned_word =
            ai_micro_cross_16b_expected_unsigned_window(
                low_patterns[3][round], high_patterns[3][round], 5U, 4U);

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_SIGN_CASES; ++i) {
            write64(leading_phys[i], leading_patterns[i][round]);
            write64(low_phys[i], low_patterns[i][round]);
            write64(high_phys[i], high_patterns[i][round]);
            write64(trailing_phys[i], trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm0 = ld(warm_vaddr[0]);
        warm1 = ld(warm_vaddr[1]);
        warm2 = ld(warm_vaddr[2]);
        warm3 = ld(warm_vaddr[3]);
        TEST_ASSERT("before the cross-16B signedness-switch probe, all four participating HS pages still warm independently with their own leading sentinel payloads",
            excpt.triggered == false &&
            warm0 == leading_patterns[0][round] &&
            warm1 == leading_patterns[1][round] &&
            warm2 == leading_patterns[2][round] &&
            warm3 == leading_patterns[3][round]
        );

        TEST_SETUP_EXCEPT();
        sh_signed = ai_micro_cross_16b_signed_lh_value(vaddr[0]);
        uh_unsigned = lhu(vaddr[1]);
        sw_signed = ai_micro_cross_16b_signed_lw_value(vaddr[2]);
        uw_unsigned = lwu(vaddr[3]);
        TEST_ASSERT("back-to-back cross-16B signed and unsigned split loads keep their own sign-extension or zero-extension semantics instead of inheriting a stale previous request's data_select shaping",
            excpt.triggered == false &&
            sh_signed == expected_signed_half &&
            uh_unsigned == expected_unsigned_half &&
            sw_signed == expected_signed_word &&
            uw_unsigned == expected_unsigned_word
        );

        TEST_SETUP_EXCEPT();
        uw_unsigned_rev = lwu(vaddr[3]);
        sw_signed_rev = ai_micro_cross_16b_signed_lw_value(vaddr[2]);
        uh_unsigned_rev = lhu(vaddr[1]);
        sh_signed_rev = ai_micro_cross_16b_signed_lh_value(vaddr[0]);
        TEST_ASSERT("reversing the signed-versus-unsigned cross-16B order still refreshes the final data shaping per request, so zero-extended requests do not poison later signed requests and vice versa",
            excpt.triggered == false &&
            sh_signed_rev == expected_signed_half &&
            uh_unsigned_rev == expected_unsigned_half &&
            sw_signed_rev == expected_signed_word &&
            uw_unsigned_rev == expected_unsigned_word
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            read64(low_phys[0]) == low_patterns[0][round] &&
            read64(high_phys[0]) == high_patterns[0][round] &&
            read64(low_phys[1]) == low_patterns[1][round] &&
            read64(high_phys[1]) == high_patterns[1][round] &&
            read64(low_phys[2]) == low_patterns[2][round] &&
            read64(high_phys[2]) == high_patterns[2][round] &&
            read64(low_phys[3]) == low_patterns[3][round] &&
            read64(high_phys[3]) == high_patterns[3][round] &&
            (!check_adjacent ||
                (read64(leading_phys[0]) == leading_patterns[0][round] &&
                 read64(trailing_phys[0]) == trailing_patterns[0][round] &&
                 read64(leading_phys[1]) == leading_patterns[1][round] &&
                 read64(trailing_phys[1]) == trailing_patterns[1][round] &&
                 read64(leading_phys[2]) == leading_patterns[2][round] &&
                 read64(trailing_phys[2]) == trailing_patterns[2][round] &&
                 read64(leading_phys[3]) == leading_patterns[3][round] &&
                 read64(trailing_phys[3]) == trailing_patterns[3][round]))
        );

        goto_priv(PRIV_HS);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}


































































static bool ai_micro_cross_16b_page_fault_then_other_load_no_rebind(bool check_adjacent)
{
    enum test_page fault_page = VSI_GI;
    enum test_page ok_page = VSRW_GRW;
    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_page);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(ok_page);
    uintptr_t fault_phys = phys_page_base(fault_page);
    uintptr_t ok_phys = phys_page_base(ok_page);
    uintptr_t fault_leading_phys = fault_phys;
    uintptr_t fault_low_phys = fault_phys + 8U;
    uintptr_t fault_high_phys = fault_phys + 16U;
    uintptr_t fault_trailing_phys = fault_phys + 24U;
    uintptr_t ok_leading_phys = ok_phys;
    uintptr_t ok_low_phys = ok_phys + 8U;
    uintptr_t ok_high_phys = ok_phys + 16U;
    uintptr_t ok_trailing_phys = ok_phys + 24U;
    const uint64_t fault_leading_pattern = 0x3141592653589793ULL;
    const uint64_t fault_low_pattern = 0x8070605040302010ULL;
    const uint64_t fault_high_pattern = 0x1122334455667799ULL;
    const uint64_t fault_trailing_pattern = 0x2718281828459045ULL;
    const uint64_t ok_leading_pattern = 0x0123456789abcdefULL;
    const uint64_t ok_low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t ok_high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t ok_trailing_pattern = 0xfedcba9876543210ULL;
    const char *after_fault_msg = check_adjacent ?
        "after the older cross-16B faulting load is selected first, both the fault-page boundary data and the younger valid-page boundary data plus their adjacent doublewords stay unchanged before the reuse probe" :
        "after the older cross-16B faulting load is selected first, the fault-page and valid-page participating doublewords stay unchanged before the reuse probe";
    const char *after_ok_msg = check_adjacent ?
        "after the younger valid cross-16B load completes, neither the older faulting page nor the younger valid page corrupts the other's adjacent doublewords in the no-rebind corner" :
        "after the younger valid cross-16B load completes, neither the older faulting page nor the younger valid page corrupts the other's participating doublewords in the no-rebind corner";
    const char *final_msg = check_adjacent ?
        "through the older-fault then younger-valid then repaired-older cross-16B load sequence, both pages keep their participating and adjacent doublewords intact in the replay-slot no-rebind corner" :
        "through the older-fault then younger-valid then repaired-older cross-16B load sequence, both pages keep their participating doublewords intact in the replay-slot no-rebind corner";
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    write64(fault_leading_phys, fault_leading_pattern);
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(fault_trailing_phys, fault_trailing_pattern);
    write64(ok_leading_phys, ok_leading_pattern);
    write64(ok_low_phys, ok_low_pattern);
    write64(ok_high_phys, ok_high_pattern);
    write64(ok_trailing_phys, ok_trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("the first cross-16B load on the still-invalid replay-head address reports load page fault before any younger valid misaligned load can reuse its visible state",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_fault_msg,
        (!check_adjacent ||
            (read64(fault_leading_phys) == fault_leading_pattern &&
             read64(fault_trailing_phys) == fault_trailing_pattern &&
             read64(ok_leading_phys) == ok_leading_pattern &&
             read64(ok_trailing_phys) == ok_trailing_pattern)) &&
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(ok_low_phys) == ok_low_pattern &&
        read64(ok_high_phys) == ok_high_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the older cross-16B faulting load occupies the replay path, a younger valid cross-16B load at another address still returns only its own merged payload without inheriting stale state",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_ok_msg,
        (!check_adjacent ||
            (read64(fault_leading_phys) == fault_leading_pattern &&
             read64(fault_trailing_phys) == fault_trailing_pattern &&
             read64(ok_leading_phys) == ok_leading_pattern &&
             read64(ok_trailing_phys) == ok_trailing_pattern)) &&
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(ok_low_phys) == ok_low_pattern &&
        read64(ok_high_phys) == ok_high_pattern
    );

    ai_micro_cross_16b_set_hs_leaf(fault_page, fault_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(fault_vaddr);
    TEST_ASSERT("after remapping the older replay-head fault address itself, the recovered cross-16B load returns the original fault-page payload rather than metadata or data from the younger reused request",
        excpt.triggered == false &&
        value == 0x2233445566779980ULL
    );

    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the recovered older cross-16B load completes, re-reading the younger valid address still returns its own payload and shows no rebound from the repaired older request",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(final_msg,
        (!check_adjacent ||
            (read64(fault_leading_phys) == fault_leading_pattern &&
             read64(fault_trailing_phys) == fault_trailing_pattern &&
             read64(ok_leading_phys) == ok_leading_pattern &&
             read64(ok_trailing_phys) == ok_trailing_pattern)) &&
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(ok_low_phys) == ok_low_pattern &&
        read64(ok_high_phys) == ok_high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_page_fault_then_repeated_younger_load_pressure(
    bool check_adjacent)
{
    enum test_page fault_page = VSI_GI;
    enum test_page younger_pages[3] = { VSRW_GRW, VSRWX_GRW, VSRX_GRW };
    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_page);
    uintptr_t fault_phys = phys_page_base(fault_page);
    uintptr_t fault_leading_phys = fault_phys;
    uintptr_t fault_low_phys = fault_phys + 8U;
    uintptr_t fault_high_phys = fault_phys + 16U;
    uintptr_t fault_trailing_phys = fault_phys + 24U;
    uintptr_t younger_vaddr[3];
    uintptr_t younger_phys[3];
    uintptr_t younger_leading_phys[3];
    uintptr_t younger_low_phys[3];
    uintptr_t younger_high_phys[3];
    uintptr_t younger_trailing_phys[3];
    const uint64_t fault_leading_pattern = 0x3141592653589793ULL;
    const uint64_t fault_low_pattern = 0x8070605040302010ULL;
    const uint64_t fault_high_pattern = 0x1122334455667799ULL;
    const uint64_t fault_trailing_pattern = 0x2718281828459045ULL;
    const uint64_t younger_leading_pattern[3] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t younger_low_pattern[3] = {
        0x7060504030201001ULL,
        0xa0b0c0d0e0f00011ULL,
        0x5566778899aabbccULL,
    };
    const uint64_t younger_high_pattern[3] = {
        0x1112131415161718ULL,
        0x2233445566778899ULL,
        0xddeeff0011223344ULL,
    };
    const uint64_t younger_trailing_pattern[3] = {
        0x3132333435363738ULL,
        0x4142434445464748ULL,
        0x5152535455565758ULL,
    };
    uint64_t younger_expected[3];
    const char *after_fault_msg = check_adjacent ?
        "after the older faulting cross-16B replay-head access is taken first, the older boundary data and all repeated-younger-direct-request boundary plus adjacent doublewords remain untouched before the starvation-pressure window" :
        "after the older faulting cross-16B replay-head access is taken first, the older boundary data and all repeated-younger-direct-request participating doublewords remain untouched before the starvation-pressure window";
    const char *final_msg = check_adjacent ?
        "through the older-fault then repeated-younger-direct then repaired-older cross-16B sequence, the replay-head address and all younger pressure addresses keep their participating and adjacent doublewords intact" :
        "through the older-fault then repeated-younger-direct then repaired-older cross-16B sequence, the replay-head address and all younger pressure addresses keep their participating doublewords intact";
    uint64_t value = 0;
    unsigned i = 0;
    unsigned round = 0;
    bool test_status = true;

    for (i = 0; i < 3U; ++i) {
        younger_vaddr[i] = ai_micro_cross_16b_cacheable_addr(younger_pages[i]);
        younger_phys[i] = phys_page_base(younger_pages[i]);
        younger_leading_phys[i] = younger_phys[i];
        younger_low_phys[i] = younger_phys[i] + 8U;
        younger_high_phys[i] = younger_phys[i] + 16U;
        younger_trailing_phys[i] = younger_phys[i] + 24U;
        younger_expected[i] = ai_micro_cross_16b_expected_ld_value(
            younger_low_pattern[i], younger_high_pattern[i]);
    }

    goto_priv(PRIV_M);
    write64(fault_leading_phys, fault_leading_pattern);
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(fault_trailing_phys, fault_trailing_pattern);
    for (i = 0; i < 3U; ++i) {
        write64(younger_leading_phys[i], younger_leading_pattern[i]);
        write64(younger_low_phys[i], younger_low_pattern[i]);
        write64(younger_high_phys[i], younger_high_pattern[i]);
        write64(younger_trailing_phys[i], younger_trailing_pattern[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("the first older cross-16B load on the still-invalid address reports load page fault before any repeated younger direct misaligned load pressure is applied",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_fault_msg,
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        (!check_adjacent ||
            (read64(fault_leading_phys) == fault_leading_pattern &&
             read64(fault_trailing_phys) == fault_trailing_pattern)) &&
        read64(younger_low_phys[0]) == younger_low_pattern[0] &&
        read64(younger_high_phys[0]) == younger_high_pattern[0] &&
        read64(younger_low_phys[1]) == younger_low_pattern[1] &&
        read64(younger_high_phys[1]) == younger_high_pattern[1] &&
        read64(younger_low_phys[2]) == younger_low_pattern[2] &&
        read64(younger_high_phys[2]) == younger_high_pattern[2] &&
        (!check_adjacent ||
            (read64(younger_leading_phys[0]) == younger_leading_pattern[0] &&
             read64(younger_trailing_phys[0]) == younger_trailing_pattern[0] &&
             read64(younger_leading_phys[1]) == younger_leading_pattern[1] &&
             read64(younger_trailing_phys[1]) == younger_trailing_pattern[1] &&
             read64(younger_leading_phys[2]) == younger_leading_pattern[2] &&
             read64(younger_trailing_phys[2]) == younger_trailing_pattern[2]))
    );

    goto_priv(PRIV_HS);
    for (round = 0; round < 4U; ++round) {
        for (i = 0; i < 3U; ++i) {
            TEST_SETUP_EXCEPT();
            value = ld(younger_vaddr[i]);
            TEST_ASSERT("while the older faulting cross-16B request stays unresolved, each repeated younger direct cross-16B load still returns only its own merged payload without being starved or cross-bound to another request",
                excpt.triggered == false &&
                value == younger_expected[i]
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cross_16b_set_hs_leaf(fault_page, fault_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(fault_vaddr);
    TEST_ASSERT("after the older replay-head fault address is repaired, the older cross-16B load still eventually completes with its own merged payload despite the repeated younger direct misaligned requests",
        excpt.triggered == false &&
        value == ai_micro_cross_16b_expected_ld_value(
            fault_low_pattern, fault_high_pattern)
    );

    for (i = 0; i < 3U; ++i) {
        TEST_SETUP_EXCEPT();
        value = ld(younger_vaddr[i]);
        TEST_ASSERT("after the older repaired cross-16B request finally completes, each younger pressure address still reloads its own merged payload without stale rebound from the older request",
            excpt.triggered == false &&
            value == younger_expected[i]
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT(final_msg,
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        (!check_adjacent ||
            (read64(fault_leading_phys) == fault_leading_pattern &&
             read64(fault_trailing_phys) == fault_trailing_pattern)) &&
        read64(younger_low_phys[0]) == younger_low_pattern[0] &&
        read64(younger_high_phys[0]) == younger_high_pattern[0] &&
        read64(younger_low_phys[1]) == younger_low_pattern[1] &&
        read64(younger_high_phys[1]) == younger_high_pattern[1] &&
        read64(younger_low_phys[2]) == younger_low_pattern[2] &&
        read64(younger_high_phys[2]) == younger_high_pattern[2] &&
        (!check_adjacent ||
            (read64(younger_leading_phys[0]) == younger_leading_pattern[0] &&
             read64(younger_trailing_phys[0]) == younger_trailing_pattern[0] &&
             read64(younger_leading_phys[1]) == younger_leading_pattern[1] &&
             read64(younger_trailing_phys[1]) == younger_trailing_pattern[1] &&
             read64(younger_leading_phys[2]) == younger_leading_pattern[2] &&
             read64(younger_trailing_phys[2]) == younger_trailing_pattern[2]))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}





static bool ai_micro_cross_16b_dual_fault_then_staged_recovery(bool check_adjacent)
{
    enum test_page older_page = VSI_GI;
    enum test_page younger_page = VSI_GURWX;
    uintptr_t older_vaddr = ai_micro_cross_16b_cacheable_addr(older_page);
    uintptr_t younger_vaddr = ai_micro_cross_16b_cacheable_addr(younger_page);
    uintptr_t older_phys = phys_page_base(older_page);
    uintptr_t younger_phys = phys_page_base(younger_page);
    uintptr_t older_leading_phys = older_phys;
    uintptr_t older_low_phys = older_phys + 8U;
    uintptr_t older_high_phys = older_phys + 16U;
    uintptr_t older_trailing_phys = older_phys + 24U;
    uintptr_t younger_leading_phys = younger_phys;
    uintptr_t younger_low_phys = younger_phys + 8U;
    uintptr_t younger_high_phys = younger_phys + 16U;
    uintptr_t younger_trailing_phys = younger_phys + 24U;
    const uint64_t older_leading_pattern = 0x0123456789abcdefULL;
    const uint64_t older_low_pattern = 0x1021324354657687ULL;
    const uint64_t older_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t older_trailing_pattern = 0xfedcba9876543210ULL;
    const uint64_t younger_leading_pattern = 0x1122334455667788ULL;
    const uint64_t younger_low_pattern = 0x8877665544332211ULL;
    const uint64_t younger_high_pattern = 0xa1b2c3d4e5f60718ULL;
    const uint64_t younger_trailing_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t older_expected = ai_micro_cross_16b_expected_ld_value(
        older_low_pattern, older_high_pattern);
    const uint64_t younger_expected = ai_micro_cross_16b_expected_ld_value(
        younger_low_pattern, younger_high_pattern);
    const char *after_dual_fault_msg = check_adjacent ?
        "after two consecutive faulting cross-16B load episodes are opened before any repair, both page-local participating and adjacent doublewords remain untouched in the staged-recovery misalign corner" :
        "after two consecutive faulting cross-16B load episodes are opened before any repair, both page-local participating doublewords remain untouched in the staged-recovery misalign corner";
    const char *after_older_repair_msg = check_adjacent ?
        "after repairing only the older cross-16B fault page, that load recovers with its own payload while the younger page still faults and both pages keep their participating plus adjacent doublewords intact" :
        "after repairing only the older cross-16B fault page, that load recovers with its own payload while the younger page still faults and both pages keep their participating doublewords intact";
    const char *final_msg = check_adjacent ?
        "through the dual-fault then staged-single-page repair sequence, the older and younger cross-16B load episodes stay bound to their own pages and preserve both participating and adjacent doublewords" :
        "through the dual-fault then staged-single-page repair sequence, the older and younger cross-16B load episodes stay bound to their own pages and preserve their participating doublewords";
    uint64_t value = 0ULL;
    bool test_status = true;

    goto_priv(PRIV_M);
    write64(older_leading_phys, older_leading_pattern);
    write64(older_low_phys, older_low_pattern);
    write64(older_high_phys, older_high_pattern);
    write64(older_trailing_phys, older_trailing_pattern);
    write64(younger_leading_phys, younger_leading_pattern);
    write64(younger_low_phys, younger_low_pattern);
    write64(younger_high_phys, younger_high_pattern);
    write64(younger_trailing_phys, younger_trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(older_vaddr);
    TEST_ASSERT("the first older cross-16B load on an invalid leaf faults before any later younger misaligned fault episode is repaired in the staged-recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == older_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(younger_vaddr);
    TEST_ASSERT("the immediately following younger cross-16B load on its own invalid leaf also faults and records its own virtual address before any repair happens",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == younger_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_dual_fault_msg,
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(younger_low_phys) == younger_low_pattern &&
        read64(younger_high_phys) == younger_high_pattern &&
        (!check_adjacent ||
            (read64(older_leading_phys) == older_leading_pattern &&
             read64(older_trailing_phys) == older_trailing_pattern &&
             read64(younger_leading_phys) == younger_leading_pattern &&
             read64(younger_trailing_phys) == younger_trailing_pattern))
    );

    ai_micro_cross_16b_set_hs_leaf(older_page, older_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(older_vaddr);
    TEST_ASSERT("once only the older invalid-leaf page is repaired, the older cross-16B load recovers with its own merged payload instead of inheriting the younger fault episode",
        excpt.triggered == false &&
        value == older_expected
    );

    TEST_SETUP_EXCEPT();
    ld(younger_vaddr);
    TEST_ASSERT("while the younger page remains invalid, the younger cross-16B load still faults with its own virtual address after the older episode has already recovered",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == younger_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_older_repair_msg,
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(younger_low_phys) == younger_low_pattern &&
        read64(younger_high_phys) == younger_high_pattern &&
        (!check_adjacent ||
            (read64(older_leading_phys) == older_leading_pattern &&
             read64(older_trailing_phys) == older_trailing_pattern &&
             read64(younger_leading_phys) == younger_leading_pattern &&
             read64(younger_trailing_phys) == younger_trailing_pattern))
    );

    ai_micro_cross_16b_set_hs_leaf(younger_page, younger_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(younger_vaddr);
    TEST_ASSERT("after repairing the younger invalid-leaf page too, the younger cross-16B load recovers with its own merged payload instead of any stale older-owner state",
        excpt.triggered == false &&
        value == younger_expected
    );

    TEST_SETUP_EXCEPT();
    value = ld(older_vaddr);
    TEST_ASSERT("re-reading the already repaired older page after the younger page recovers still returns only the older merged payload with no cross-binding back from the younger episode",
        excpt.triggered == false &&
        value == older_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(final_msg,
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(younger_low_phys) == younger_low_pattern &&
        read64(younger_high_phys) == younger_high_pattern &&
        (!check_adjacent ||
            (read64(older_leading_phys) == older_leading_pattern &&
             read64(older_trailing_phys) == older_trailing_pattern &&
             read64(younger_leading_phys) == younger_leading_pattern &&
             read64(younger_trailing_phys) == younger_trailing_pattern))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}



static bool ai_micro_cross_16b_sequential_replay_owners_under_direct_pressure(
    bool check_adjacent)
{
    enum test_page older_page = VSI_GI;
    enum test_page younger_page = VSI_GURWX;
    enum test_page pressure_pages[3] = { VSRW_GRW, VSRWX_GRW, VSRX_GRW };
    uintptr_t older_vaddr = ai_micro_cross_16b_cacheable_addr(older_page);
    uintptr_t younger_vaddr = ai_micro_cross_16b_cacheable_addr(younger_page);
    uintptr_t older_phys = phys_page_base(older_page);
    uintptr_t younger_phys = phys_page_base(younger_page);
    uintptr_t older_leading_phys = older_phys;
    uintptr_t older_low_phys = older_phys + 8U;
    uintptr_t older_high_phys = older_phys + 16U;
    uintptr_t older_trailing_phys = older_phys + 24U;
    uintptr_t younger_leading_phys = younger_phys;
    uintptr_t younger_low_phys = younger_phys + 8U;
    uintptr_t younger_high_phys = younger_phys + 16U;
    uintptr_t younger_trailing_phys = younger_phys + 24U;
    uintptr_t pressure_vaddr[3];
    uintptr_t pressure_phys[3];
    uintptr_t pressure_leading_phys[3];
    uintptr_t pressure_low_phys[3];
    uintptr_t pressure_high_phys[3];
    uintptr_t pressure_trailing_phys[3];
    const uint64_t older_leading_pattern = 0x0123456789abcdefULL;
    const uint64_t older_low_pattern = 0x1021324354657687ULL;
    const uint64_t older_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t older_trailing_pattern = 0xfedcba9876543210ULL;
    const uint64_t younger_leading_pattern = 0x1122334455667788ULL;
    const uint64_t younger_low_pattern = 0x8877665544332211ULL;
    const uint64_t younger_high_pattern = 0xa1b2c3d4e5f60718ULL;
    const uint64_t younger_trailing_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t pressure_leading_pattern[3] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t pressure_low_pattern[3] = {
        0x7060504030201001ULL,
        0xa0b0c0d0e0f00011ULL,
        0x5566778899aabbccULL,
    };
    const uint64_t pressure_high_pattern[3] = {
        0x1112131415161718ULL,
        0x2233445566778899ULL,
        0xddeeff0011223344ULL,
    };
    const uint64_t pressure_trailing_pattern[3] = {
        0x3132333435363738ULL,
        0x4142434445464748ULL,
        0x5152535455565758ULL,
    };
    const uint64_t older_expected = ai_micro_cross_16b_expected_ld_value(
        older_low_pattern, older_high_pattern);
    const uint64_t younger_expected = ai_micro_cross_16b_expected_ld_value(
        younger_low_pattern, younger_high_pattern);
    uint64_t pressure_expected[3];
    const char *after_fault_msg = check_adjacent ?
        "after opening two faulting replay-owner cross-16B loads before any repair, both owners and all direct-pressure addresses keep their participating and adjacent doublewords unchanged" :
        "after opening two faulting replay-owner cross-16B loads before any repair, both owners and all direct-pressure addresses keep their participating doublewords unchanged";
    const char *after_older_recovery_msg = check_adjacent ?
        "after the older replay owner recovers under direct misalign pressure while the younger owner still faults, all owner-local and pressure-address adjacent doublewords remain intact" :
        "after the older replay owner recovers under direct misalign pressure while the younger owner still faults, all owner-local and pressure-address participating doublewords remain intact";
    const char *final_msg = check_adjacent ?
        "through the sequential replay-owner handoff under continued direct misalign pressure, both repaired owners and all pressure addresses keep their participating and adjacent doublewords intact" :
        "through the sequential replay-owner handoff under continued direct misalign pressure, both repaired owners and all pressure addresses keep their participating doublewords intact";
    uint64_t value = 0ULL;
    unsigned i = 0U;
    unsigned round = 0U;
    bool test_status = true;

    for (i = 0U; i < 3U; ++i) {
        pressure_vaddr[i] = ai_micro_cross_16b_cacheable_addr(pressure_pages[i]);
        pressure_phys[i] = phys_page_base(pressure_pages[i]);
        pressure_leading_phys[i] = pressure_phys[i];
        pressure_low_phys[i] = pressure_phys[i] + 8U;
        pressure_high_phys[i] = pressure_phys[i] + 16U;
        pressure_trailing_phys[i] = pressure_phys[i] + 24U;
        pressure_expected[i] = ai_micro_cross_16b_expected_ld_value(
            pressure_low_pattern[i], pressure_high_pattern[i]);
    }

    goto_priv(PRIV_M);
    write64(older_leading_phys, older_leading_pattern);
    write64(older_low_phys, older_low_pattern);
    write64(older_high_phys, older_high_pattern);
    write64(older_trailing_phys, older_trailing_pattern);
    write64(younger_leading_phys, younger_leading_pattern);
    write64(younger_low_phys, younger_low_pattern);
    write64(younger_high_phys, younger_high_pattern);
    write64(younger_trailing_phys, younger_trailing_pattern);
    for (i = 0U; i < 3U; ++i) {
        write64(pressure_leading_phys[i], pressure_leading_pattern[i]);
        write64(pressure_low_phys[i], pressure_low_pattern[i]);
        write64(pressure_high_phys[i], pressure_high_pattern[i]);
        write64(pressure_trailing_phys[i], pressure_trailing_pattern[i]);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(older_vaddr);
    TEST_ASSERT("the first replay-owner cross-16B load faults on its invalid leaf before any later owner or direct-pressure request is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == older_vaddr
    );

    TEST_SETUP_EXCEPT();
    ld(younger_vaddr);
    TEST_ASSERT("the second replay-owner cross-16B load on a different invalid leaf also faults and records its own address before any repair occurs",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == younger_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_fault_msg,
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(younger_low_phys) == younger_low_pattern &&
        read64(younger_high_phys) == younger_high_pattern &&
        read64(pressure_low_phys[0]) == pressure_low_pattern[0] &&
        read64(pressure_high_phys[0]) == pressure_high_pattern[0] &&
        read64(pressure_low_phys[1]) == pressure_low_pattern[1] &&
        read64(pressure_high_phys[1]) == pressure_high_pattern[1] &&
        read64(pressure_low_phys[2]) == pressure_low_pattern[2] &&
        read64(pressure_high_phys[2]) == pressure_high_pattern[2] &&
        (!check_adjacent ||
            (read64(older_leading_phys) == older_leading_pattern &&
             read64(older_trailing_phys) == older_trailing_pattern &&
             read64(younger_leading_phys) == younger_leading_pattern &&
             read64(younger_trailing_phys) == younger_trailing_pattern &&
             read64(pressure_leading_phys[0]) == pressure_leading_pattern[0] &&
             read64(pressure_trailing_phys[0]) == pressure_trailing_pattern[0] &&
             read64(pressure_leading_phys[1]) == pressure_leading_pattern[1] &&
             read64(pressure_trailing_phys[1]) == pressure_trailing_pattern[1] &&
             read64(pressure_leading_phys[2]) == pressure_leading_pattern[2] &&
             read64(pressure_trailing_phys[2]) == pressure_trailing_pattern[2]))
    );

    ai_micro_cross_16b_set_hs_leaf(older_page, older_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    for (round = 0U; round < 4U; ++round) {
        for (i = 0U; i < 3U; ++i) {
            TEST_SETUP_EXCEPT();
            value = ld(pressure_vaddr[i]);
            TEST_ASSERT("while the second replay owner is still fault-pending, each direct-pressure cross-16B load keeps returning only its own payload during the first-owner recovery window",
                excpt.triggered == false &&
                value == pressure_expected[i]
            );
        }
    }

    TEST_SETUP_EXCEPT();
    value = ld(older_vaddr);
    TEST_ASSERT("after repairing only the first replay owner, it still recovers with its own merged payload despite the surrounding direct cross-16B pressure",
        excpt.triggered == false &&
        value == older_expected
    );

    TEST_SETUP_EXCEPT();
    ld(younger_vaddr);
    TEST_ASSERT("after the first replay owner has already recovered, the second owner still faults on its own address until its page is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == younger_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(after_older_recovery_msg,
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(younger_low_phys) == younger_low_pattern &&
        read64(younger_high_phys) == younger_high_pattern &&
        read64(pressure_low_phys[0]) == pressure_low_pattern[0] &&
        read64(pressure_high_phys[0]) == pressure_high_pattern[0] &&
        read64(pressure_low_phys[1]) == pressure_low_pattern[1] &&
        read64(pressure_high_phys[1]) == pressure_high_pattern[1] &&
        read64(pressure_low_phys[2]) == pressure_low_pattern[2] &&
        read64(pressure_high_phys[2]) == pressure_high_pattern[2] &&
        (!check_adjacent ||
            (read64(older_leading_phys) == older_leading_pattern &&
             read64(older_trailing_phys) == older_trailing_pattern &&
             read64(younger_leading_phys) == younger_leading_pattern &&
             read64(younger_trailing_phys) == younger_trailing_pattern &&
             read64(pressure_leading_phys[0]) == pressure_leading_pattern[0] &&
             read64(pressure_trailing_phys[0]) == pressure_trailing_pattern[0] &&
             read64(pressure_leading_phys[1]) == pressure_leading_pattern[1] &&
             read64(pressure_trailing_phys[1]) == pressure_trailing_pattern[1] &&
             read64(pressure_leading_phys[2]) == pressure_leading_pattern[2] &&
             read64(pressure_trailing_phys[2]) == pressure_trailing_pattern[2]))
    );

    ai_micro_cross_16b_set_hs_leaf(younger_page, younger_phys, PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    for (round = 0U; round < 4U; ++round) {
        for (i = 0U; i < 3U; ++i) {
            TEST_SETUP_EXCEPT();
            value = ld(pressure_vaddr[i]);
            TEST_ASSERT("while the next replay owner is waiting to recover, the direct-pressure cross-16B loads still keep their own payload identity with no false starvation side effect",
                excpt.triggered == false &&
                value == pressure_expected[i]
            );
        }
    }

    TEST_SETUP_EXCEPT();
    value = ld(younger_vaddr);
    TEST_ASSERT("after repairing the second replay owner without any quiet gap after the first owner, the second cross-16B load also recovers with its own merged payload",
        excpt.triggered == false &&
        value == younger_expected
    );

    TEST_SETUP_EXCEPT();
    value = ld(older_vaddr);
    TEST_ASSERT("after the second replay owner recovers too, rereading the first repaired owner still returns only the first owner's payload with no cross-owner rebound",
        excpt.triggered == false &&
        value == older_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT(final_msg,
        read64(older_low_phys) == older_low_pattern &&
        read64(older_high_phys) == older_high_pattern &&
        read64(younger_low_phys) == younger_low_pattern &&
        read64(younger_high_phys) == younger_high_pattern &&
        read64(pressure_low_phys[0]) == pressure_low_pattern[0] &&
        read64(pressure_high_phys[0]) == pressure_high_pattern[0] &&
        read64(pressure_low_phys[1]) == pressure_low_pattern[1] &&
        read64(pressure_high_phys[1]) == pressure_high_pattern[1] &&
        read64(pressure_low_phys[2]) == pressure_low_pattern[2] &&
        read64(pressure_high_phys[2]) == pressure_high_pattern[2] &&
        (!check_adjacent ||
            (read64(older_leading_phys) == older_leading_pattern &&
             read64(older_trailing_phys) == older_trailing_pattern &&
             read64(younger_leading_phys) == younger_leading_pattern &&
             read64(younger_trailing_phys) == younger_trailing_pattern &&
             read64(pressure_leading_phys[0]) == pressure_leading_pattern[0] &&
             read64(pressure_trailing_phys[0]) == pressure_trailing_pattern[0] &&
             read64(pressure_leading_phys[1]) == pressure_leading_pattern[1] &&
             read64(pressure_trailing_phys[1]) == pressure_trailing_pattern[1] &&
             read64(pressure_leading_phys[2]) == pressure_leading_pattern[2] &&
             read64(pressure_trailing_phys[2]) == pressure_trailing_pattern[2]))
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}






static bool ai_micro_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS = 2,
        AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES = 4
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        false,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        VSI_GI,
        VSRWX_GRWX,
    };
    const enum test_page recover_pages[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES] = {
        VSRWX_GRW,
        VSRW_GRW,
        VSRX_GRW,
        SWITCH2,
    };
    const uintptr_t recover_offsets[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES] = {
        AI_MICRO_CROSS_16B_OFFSET,
        AI_MICRO_CROSS_16B_OFFSET,
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
    };
    const unsigned recover_sizes[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES] = {
        2U,
        2U,
        4U,
        4U,
    };
    const bool recover_sign_extend[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES] = {
        true,
        false,
        true,
        false,
    };
    const enum test_page warm_page = SWITCH1;
    const uintptr_t warm_offset = AI_MICRO_CROSS_16B_7B1B_OFFSET;
    const uint64_t warm_leading_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0x1122334455667788ULL,
        0x3141592653589793ULL,
    };
    const uint64_t warm_low_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0x8070605040302010ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t warm_high_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0x0123456789abcdefULL,
        0x1021324354657687ULL,
    };
    const uint64_t warm_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0x8877665544332211ULL,
        0x2718281828459045ULL,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0x5566778899aabbccULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0xdeadbeefcafef00dULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0xddeeff0011223344ULL,
        0x76543210fedcbafeULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        0x99aabbccddeeff00ULL,
        0xcafed00d12345678ULL,
    };
    const uint64_t recover_leading_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES][AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        { 0x0123456789abcdefULL, 0x1111222233334444ULL },
        { 0x89abcdef01234567ULL, 0x5555666677778888ULL },
        { 0xfedcba9876543210ULL, 0x9999aaaabbbbccccULL },
        { 0x13579bdf2468ace0ULL, 0x0badf00dfacefeedULL },
    };
    const uint64_t recover_low_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES][AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        { 0x34aa996633cc77eeULL, 0x23bb887755dd66aaULL },
        { 0xbc11223344556677ULL, 0xd05566778899aabbULL },
        { 0xabcdef6655443322ULL, 0x2345671122334455ULL },
        { 0xdcba987766554433ULL, 0xc0dec07766554433ULL },
    };
    const uint64_t recover_high_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES][AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        { 0x77665544332211f2ULL, 0x6655443322111081ULL },
        { 0x89abcdef0123459aULL, 0x76543210fedcbafeULL },
        { 0x7766554433221189ULL, 0x88776655443322f1ULL },
        { 0x0123456789abc0feULL, 0x11223344556677c0ULL },
    };
    const uint64_t recover_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES][AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS] = {
        { 0x76543210fedcba98ULL, 0x0f1e2d3c4b5a6978ULL },
        { 0x55aa996633cc77eeULL, 0xcafed00d12345678ULL },
        { 0x2468ace13579bdf0ULL, 0x0102030405060708ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL },
    };
    const char *after_fault_msg = check_adjacent ?
        "after the successful priming cross-16B load and the later faulting cross-16B probe, the priming page, the fault page, and all later signedness-switch recovery boundaries plus their adjacent sentinels stay unchanged before recovery" :
        "after the successful priming cross-16B load and the later faulting cross-16B probe, the priming page, the fault page, and all later signedness-switch recovery boundaries stay unchanged before recovery";
    const char *final_msg = check_adjacent ?
        "through the priming-success then fault then signedness-switch recovery sequence, the priming page, the fault page, and all recovery boundaries plus their adjacent sentinels stay unchanged" :
        "through the priming-success then fault then signedness-switch recovery sequence, the priming page, the fault page, and all recovery boundaries stay unchanged";
    const uintptr_t warm_vaddr =
        ai_micro_cross_16b_cacheable_addr_with_offset(warm_page, warm_offset);
    const uintptr_t warm_page_phys = phys_page_base(warm_page);
    const uintptr_t warm_leading_phys = warm_page_phys;
    const uintptr_t warm_low_phys = warm_page_phys + 8U;
    const uintptr_t warm_high_phys = warm_page_phys + 16U;
    const uintptr_t warm_trailing_phys = warm_page_phys + 24U;
    uintptr_t recover_vaddr[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES];
    uintptr_t recover_leading_phys[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES];
    uintptr_t recover_low_phys[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES];
    uintptr_t recover_high_phys[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES];
    uintptr_t recover_trailing_phys[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES; ++i) {
        uintptr_t page_phys = phys_page_base(recover_pages[i]);
        recover_vaddr[i] = ai_micro_cross_16b_cacheable_addr_with_offset(
            recover_pages[i], recover_offsets[i]);
        recover_leading_phys[i] = page_phys;
        recover_low_phys[i] = page_phys + 8U;
        recover_high_phys[i] = page_phys + 16U;
        recover_trailing_phys[i] = page_phys + 24U;
    }

    for (unsigned round = 0; round < AI_MICRO_CROSS_16B_FAULT_SIGN_ROUNDS; ++round) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[round]);
        const uintptr_t fault_vaddr =
            ai_micro_cross_16b_cacheable_addr(fault_pages[round]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uint64_t expected_warm = ai_micro_cross_16b_expected_unsigned_window(
            warm_low_patterns[round],
            warm_high_patterns[round],
            (unsigned)(warm_offset - 8U),
            8U);
        uint64_t expected_recover[AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES];
        uint64_t warm_observed = 0ULL;
        uint64_t forward_signed_half = 0ULL;
        uint64_t forward_unsigned_half = 0ULL;
        uint64_t forward_signed_word = 0ULL;
        uint64_t forward_unsigned_word = 0ULL;
        uint64_t reverse_signed_half = 0ULL;
        uint64_t reverse_unsigned_half = 0ULL;
        uint64_t reverse_signed_word = 0ULL;
        uint64_t reverse_unsigned_word = 0ULL;

        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES; ++i) {
            expected_recover[i] = ai_micro_cross_16b_expected_value(
                recover_low_patterns[i][round],
                recover_high_patterns[i][round],
                (unsigned)(recover_offsets[i] - 8U),
                recover_sizes[i],
                recover_sign_extend[i]);
        }

        goto_priv(PRIV_M);
        write64(warm_leading_phys, warm_leading_patterns[round]);
        write64(warm_low_phys, warm_low_patterns[round]);
        write64(warm_high_phys, warm_high_patterns[round]);
        write64(warm_trailing_phys, warm_trailing_patterns[round]);
        write64(fault_leading_phys, fault_leading_patterns[round]);
        write64(fault_low_phys, fault_low_patterns[round]);
        write64(fault_high_phys, fault_high_patterns[round]);
        write64(fault_trailing_phys, fault_trailing_patterns[round]);
        for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_SIGN_RECOVER_CASES; ++i) {
            write64(recover_leading_phys[i], recover_leading_patterns[i][round]);
            write64(recover_low_phys[i], recover_low_patterns[i][round]);
            write64(recover_high_phys[i], recover_high_patterns[i][round]);
            write64(recover_trailing_phys[i], recover_trailing_patterns[i][round]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        warm_observed = ld(warm_vaddr);
        TEST_ASSERT("before the fault-cleanup cross-16B signedness-switch recovery probe, the priming cross-16B load completes with its own merged payload",
            excpt.triggered == false &&
            warm_observed == expected_warm
        );

        if (use_access_fault[round]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
            sfence_vma();
            goto_priv(PRIV_HS);
        }

        TEST_SETUP_EXCEPT();
        ld(fault_vaddr);
        TEST_ASSERT("after the successful priming cross-16B load, the next faulting cross-16B load still reports the expected precise fault with the original bad virtual address before recovery",
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[round] ? CAUSE_LAF : CAUSE_LPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(after_fault_msg,
            read64(warm_low_phys) == warm_low_patterns[round] &&
            read64(warm_high_phys) == warm_high_patterns[round] &&
            read64(fault_low_phys) == fault_low_patterns[round] &&
            read64(fault_high_phys) == fault_high_patterns[round] &&
            read64(recover_low_phys[0]) == recover_low_patterns[0][round] &&
            read64(recover_high_phys[0]) == recover_high_patterns[0][round] &&
            read64(recover_low_phys[1]) == recover_low_patterns[1][round] &&
            read64(recover_high_phys[1]) == recover_high_patterns[1][round] &&
            read64(recover_low_phys[2]) == recover_low_patterns[2][round] &&
            read64(recover_high_phys[2]) == recover_high_patterns[2][round] &&
            read64(recover_low_phys[3]) == recover_low_patterns[3][round] &&
            read64(recover_high_phys[3]) == recover_high_patterns[3][round] &&
            (!check_adjacent ||
                (read64(warm_leading_phys) == warm_leading_patterns[round] &&
                 read64(warm_trailing_phys) == warm_trailing_patterns[round] &&
                 read64(fault_leading_phys) == fault_leading_patterns[round] &&
                 read64(fault_trailing_phys) == fault_trailing_patterns[round] &&
                 read64(recover_leading_phys[0]) == recover_leading_patterns[0][round] &&
                 read64(recover_trailing_phys[0]) == recover_trailing_patterns[0][round] &&
                 read64(recover_leading_phys[1]) == recover_leading_patterns[1][round] &&
                 read64(recover_trailing_phys[1]) == recover_trailing_patterns[1][round] &&
                 read64(recover_leading_phys[2]) == recover_leading_patterns[2][round] &&
                 read64(recover_trailing_phys[2]) == recover_trailing_patterns[2][round] &&
                 read64(recover_leading_phys[3]) == recover_leading_patterns[3][round] &&
                 read64(recover_trailing_phys[3]) == recover_trailing_patterns[3][round]))
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        forward_signed_half = ai_micro_cross_16b_observed_value(
            recover_vaddr[0], recover_sizes[0], recover_sign_extend[0]);
        forward_unsigned_half = ai_micro_cross_16b_observed_value(
            recover_vaddr[1], recover_sizes[1], recover_sign_extend[1]);
        forward_signed_word = ai_micro_cross_16b_observed_value(
            recover_vaddr[2], recover_sizes[2], recover_sign_extend[2]);
        forward_unsigned_word = ai_micro_cross_16b_observed_value(
            recover_vaddr[3], recover_sizes[3], recover_sign_extend[3]);
        TEST_ASSERT("after the faulting cross-16B episode is cleared, back-to-back signed and unsigned recovery loads refresh their own merge and data-select state instead of inheriting stale pre-fault state",
            excpt.triggered == false &&
            forward_signed_half == expected_recover[0] &&
            forward_unsigned_half == expected_recover[1] &&
            forward_signed_word == expected_recover[2] &&
            forward_unsigned_word == expected_recover[3]
        );

        TEST_SETUP_EXCEPT();
        reverse_unsigned_word = ai_micro_cross_16b_observed_value(
            recover_vaddr[3], recover_sizes[3], recover_sign_extend[3]);
        reverse_signed_word = ai_micro_cross_16b_observed_value(
            recover_vaddr[2], recover_sizes[2], recover_sign_extend[2]);
        reverse_unsigned_half = ai_micro_cross_16b_observed_value(
            recover_vaddr[1], recover_sizes[1], recover_sign_extend[1]);
        reverse_signed_half = ai_micro_cross_16b_observed_value(
            recover_vaddr[0], recover_sizes[0], recover_sign_extend[0]);
        TEST_ASSERT("reversing the post-fault signed-versus-unsigned cross-16B recovery order still keeps each request's own payload and sign behavior with no stale carryover from the priming success or the intervening fault",
            excpt.triggered == false &&
            reverse_signed_half == expected_recover[0] &&
            reverse_unsigned_half == expected_recover[1] &&
            reverse_signed_word == expected_recover[2] &&
            reverse_unsigned_word == expected_recover[3]
        );

        TEST_SETUP_EXCEPT();
        warm_observed = ld(warm_vaddr);
        TEST_ASSERT("after the post-fault signedness-switch recovery sequence, rereading the original priming cross-16B page still returns only its own earlier payload",
            excpt.triggered == false &&
            warm_observed == expected_warm
        );

        goto_priv(PRIV_M);
        TEST_ASSERT(final_msg,
            read64(warm_low_phys) == warm_low_patterns[round] &&
            read64(warm_high_phys) == warm_high_patterns[round] &&
            read64(fault_low_phys) == fault_low_patterns[round] &&
            read64(fault_high_phys) == fault_high_patterns[round] &&
            read64(recover_low_phys[0]) == recover_low_patterns[0][round] &&
            read64(recover_high_phys[0]) == recover_high_patterns[0][round] &&
            read64(recover_low_phys[1]) == recover_low_patterns[1][round] &&
            read64(recover_high_phys[1]) == recover_high_patterns[1][round] &&
            read64(recover_low_phys[2]) == recover_low_patterns[2][round] &&
            read64(recover_high_phys[2]) == recover_high_patterns[2][round] &&
            read64(recover_low_phys[3]) == recover_low_patterns[3][round] &&
            read64(recover_high_phys[3]) == recover_high_patterns[3][round] &&
            (!check_adjacent ||
                (read64(warm_leading_phys) == warm_leading_patterns[round] &&
                 read64(warm_trailing_phys) == warm_trailing_patterns[round] &&
                 read64(fault_leading_phys) == fault_leading_patterns[round] &&
                 read64(fault_trailing_phys) == fault_trailing_patterns[round] &&
                 read64(recover_leading_phys[0]) == recover_leading_patterns[0][round] &&
                 read64(recover_trailing_phys[0]) == recover_trailing_patterns[0][round] &&
                 read64(recover_leading_phys[1]) == recover_leading_patterns[1][round] &&
                 read64(recover_trailing_phys[1]) == recover_trailing_patterns[1][round] &&
                 read64(recover_leading_phys[2]) == recover_leading_patterns[2][round] &&
                 read64(recover_trailing_phys[2]) == recover_trailing_patterns[2][round] &&
                 read64(recover_leading_phys[3]) == recover_leading_patterns[3][round] &&
                 read64(recover_trailing_phys[3]) == recover_trailing_patterns[3][round]))
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

#endif /* AI_MICRO_CROSS_16B_CACHEABLE_COMMON_H */
