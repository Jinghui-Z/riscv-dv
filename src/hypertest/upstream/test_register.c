#include <rvh_test.h>
#include <page_tables.h>

// ai_test_cases/memblock/exception_trap/ai_micro_nested_exception_handler_cases.c
TEST_REGISTER(ai_micro_m_nested_exception_inside_mtvec_handler_recovers_corner);
TEST_REGISTER(ai_micro_m_nested_exception_no_mepc_save_reenters_handler_corner);
TEST_REGISTER(ai_micro_priv_non_address_exception_tval_matrix_corner);
TEST_REGISTER(ai_micro_m_handler_updates_mtvec_next_trap_uses_new_entry_corner);
TEST_REGISTER(ai_micro_m_handler_updates_delegation_and_stvec_next_hs_trap_corner);
// ai_test_cases/memblock/exception_trap/ai_micro_interrupt_overlay_memblock_cases.c
// Basic ordinary-interrupt overlay smoke; precise replay/SQ/vector same-cycle hit still needs scoped stress or RTL evidence.
// TEST_REGISTER(ai_micro_m_software_interrupt_between_store_and_amo_memblock_marker_progress_corner);

// Backend IT 0911 workbook rows 573+: IT monitor required, D-MANUAL-NONGATE.
// Architecture smoke builds do not satisfy the microarchitecture checkpoints.
// TEST_REGISTER(ai_micro_backend_it_rvc_producer_matrix);
// TEST_REGISTER(ai_micro_backend_it_rvc_consumer_matrix);
// TEST_REGISTER(ai_micro_backend_it_vector_raw_waw);
// TEST_REGISTER(ai_micro_backend_it_rename_cross_domain);
// TEST_REGISTER(ai_micro_backend_it_integer_source_matrix);
// TEST_REGISTER(ai_micro_backend_it_fma_source_matrix);
// TEST_REGISTER(ai_micro_backend_it_freelist_pressure);
// TEST_REGISTER(ai_micro_backend_it_dispatch_pressure);
// TEST_REGISTER(ai_micro_backend_it_queue_boundary);
// TEST_REGISTER(ai_micro_backend_it_cancel_reissue);
// TEST_REGISTER(ai_micro_backend_it_redirect_age_matrix);
// TEST_REGISTER(ai_micro_backend_it_rob_commit_pressure);
// TEST_REGISTER(ai_micro_backend_it_backpressure_release);
// TEST_REGISTER(ai_micro_backend_it_exception_age_matrix);
// TEST_REGISTER(ai_micro_backend_it_vector_fault_merge);
// TEST_REGISTER(ai_micro_backend_it_vector_merge_corners);
// TEST_REGISTER(ai_micro_backend_it_serial_order);
// TEST_REGISTER(ai_micro_backend_it_serial_redirect);
// TEST_REGISTER(ai_micro_backend_it_vsetvl_transition);
// TEST_REGISTER(ai_micro_backend_it_custom_control);
// TEST_REGISTER(ai_micro_backend_it_satp_transition);
// TEST_REGISTER(ai_micro_backend_it_sfence_scope);
// TEST_REGISTER(ai_micro_backend_it_fencei_refill);
// TEST_REGISTER(ai_micro_backend_it_backend_fault_marker);
// TEST_REGISTER(ai_micro_backend_it_ftq_fault_pointer_isolation);
// TEST_REGISTER(ai_micro_backend_it_interrupt_commit);
// TEST_REGISTER(ai_micro_backend_it_wfi_wakeup);
// TEST_REGISTER(ai_micro_backend_it_wfi_single_step); // manual, D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_backend_it_wfi_l2_tail_scan); // manual, D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_backend_it_frontend_event_priority);
// TEST_REGISTER(ai_micro_backend_it_reset_inflight);
// TEST_REGISTER(ai_micro_backend_it_reset_full_pipe);
// Backend IT 0922 external-agent additions; AP-IT special runner only.
// TEST_REGISTER(ai_micro_backend_it_cmo_completion_baseline);
// TEST_REGISTER(ai_micro_backend_it_boot_reset_contract);
// TEST_REGISTER(ai_micro_backend_it_warm_reset_frontend_contract);
// TEST_REGISTER(ai_micro_backend_it_boundary_refill_cross);
// TEST_REGISTER(ai_micro_backend_it_boundary_irq_cross);
// TEST_REGISTER(ai_micro_backend_it_boundary_debug_cross);
// TEST_REGISTER(ai_micro_backend_it_boundary_rnmi_cross);
// TEST_REGISTER(ai_micro_backend_it_boundary_pointer_wrap_scan);
// TEST_REGISTER(ai_micro_backend_it_boundary_vector_load_pf);
// TEST_REGISTER(ai_micro_backend_it_boundary_vector_store_pf_retry);
// TEST_REGISTER(ai_micro_backend_it_boundary_vector_older_fault);

// Backend 0922 directed rows 775-805; no external agent required.
// Manual: D-MANUAL-NONGATE; internal flush sources require RTL observation.
// TEST_REGISTER(ai_micro_backend_flush_sv39_sie_next_ipf);
// TEST_REGISTER(ai_micro_backend_flush_sv48_sie_next_ipf);
// TEST_REGISTER(ai_micro_backend_flush_sie_sip_write_forms);
// TEST_REGISTER(ai_micro_backend_flush_sie_same_value);
// TEST_REGISTER(ai_micro_backend_flush_sstatus_sie);
// TEST_REGISTER(ai_micro_backend_flush_sstatus_fs_vs);
// TEST_REGISTER(ai_micro_backend_flush_frm_reserved_alias);
// TEST_REGISTER(ai_micro_backend_flush_vstart_change);
// TEST_REGISTER(ai_micro_backend_flush_sfence_selectors);
// TEST_REGISTER(ai_micro_backend_flush_vsetvl_config);
// TEST_REGISTER(ai_micro_backend_flush_sv39_sv48_same_target);
// TEST_REGISTER(ai_micro_backend_flush_negative_canonical);
// TEST_REGISTER(ai_micro_backend_flush_xlen_wrap_zero);
// TEST_REGISTER(ai_micro_backend_flush_successor_rvc_boundary);
// TEST_REGISTER(ai_micro_backend_flush_successor_split32_ipf);
// TEST_REGISTER(ai_micro_backend_flush_self_split32_no_write);
// TEST_REGISTER(ai_micro_backend_flush_illegal_csr_priority);
// TEST_REGISTER(ai_micro_backend_flush_sret_target_priority);
// TEST_REGISTER(ai_micro_backend_flush_csr_read_only);
// TEST_REGISTER(ai_micro_backend_flush_canonical_alias_warm);
// TEST_REGISTER(ai_micro_backend_flush_adjacent_csr_flushes);
// TEST_REGISTER(ai_micro_backend_flush_satp_then_sie);
// TEST_REGISTER(ai_micro_backend_flush_delegated_handler_tag_clear);
// TEST_REGISTER(ai_micro_backend_flush_next_page_no_exec);
// TEST_REGISTER(ai_micro_backend_flush_next_page_pmp_deny);

// VModule injection acceptance: VMODULE=1 special runner, D-MANUAL-NONGATE.
// TEST_REGISTER(ai_micro_backend_vmodule_mei_smoke);
// TEST_REGISTER(ai_micro_backend_vmodule_wfi_smoke);
// TEST_REGISTER(ai_micro_backend_vmodule_ecc_smoke);
// Backend IT monitor/JTAG/reset acceptance; requires the dedicated IT simv.
// TEST_REGISTER(ai_micro_backend_it_debug_agent_smoke);
// TEST_REGISTER(ai_micro_backend_it_reset_agent_smoke);

// ai_test_cases/backend/micro/ai_micro_backend_core_cases.c
// Manual/nongate: nested ROB redirect snapshot selection requires branch-mispredict / memory-violation RTL evidence; C case is black-box symptom smoke only.
// TEST_REGISTER(ai_micro_backend_nested_redirect_same_snapshot_marker_integrity_corner);
// TEST_REGISTER(ai_micro_backend_nested_redirect_different_snapshot_marker_integrity_corner);
// TEST_REGISTER(ai_micro_backend_nested_redirect_outer_snapshot_inner_no_snapshot_corner);
// TEST_REGISTER(ai_micro_backend_nested_redirect_no_snapshot_double_redirect_corner);
TEST_REGISTER(ai_micro_backend_issuequeue_long_alu_dependency_chain_corner);
TEST_REGISTER(ai_micro_backend_issuequeue_long_dependency_chain_with_branches_corner);
TEST_REGISTER(ai_micro_backend_issuequeue_interleaved_multi_source_dependency_network_corner);
TEST_REGISTER(ai_micro_backend_vector_integer_operand_readiness_mixed_source_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_old_lmul_illegal_killed_by_new_vtype_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_vill_to_fractional_lmul_no_stale_illegal_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_dense_vector_uops_no_old_vtype_exception_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_user_sequence_zero_avl_fractional_lmul_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_initial_vill_keepvl_zero_gap_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_decode_old_large_new_small_split_count_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_decode_old_small_new_large_split_count_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_decode_old_lmul_group_side_effect_corner);
TEST_REGISTER(ai_micro_vsetvl_flush_commit_delayed_old_lmul_illegal_refetch_corner);
// ai_test_cases/backend/micro/ai_micro_backend_fcov_gap_cases.c
TEST_REGISTER(ai_micro_backend_fcov_decode_fusion_vset_corner);
// Manual/LinkNan fcov terminal: executes a lane3 XSTrap terminator and does not return to the test loop.
// TEST_REGISTER(ai_micro_backend_fcov_decode_lane3_xstrap_terminal_corner);
TEST_REGISTER(ai_micro_backend_fcov_iq_mou_dispatch_pressure_corner);
TEST_REGISTER(ai_micro_backend_fcov_readarb_redirect_pressure_corner);
TEST_REGISTER(ai_micro_backend_fcov_readarb_cancelcounter_corner);
TEST_REGISTER(ai_micro_backend_fcov_readarb_aligned_branch_counter_corner);
TEST_REGISTER(ai_micro_backend_fcov_readarb_delayed_branch_counter_corner);
TEST_REGISTER(ai_micro_backend_fcov_readarb_bju3_div_counter_corner);
TEST_REGISTER(ai_micro_backend_fcov_iq_load_cancel_regcache_wakeup_corner);
TEST_REGISTER(ai_micro_backend_fcov_iq_pair_vector_mem_mix_corner);
// ai_test_cases/backend/micro/ai_micro_backend_condition_0803_cases.c
TEST_REGISTER(ai_micro_backend_i2f_iq_full_long_div_dependency_corner);
// Depends on a 16-cycle Backend-to-Mem cache/backpressure timeout; official Spike has no such timing model.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_backend_ldu_final_issue_timeout_backpressure_corner);
// Analysis-only reachability probe: Dispatch rewrites every VF-IQ scalar FP source to SrcType.vp, so it cannot close the target bin.
// TEST_REGISTER(ai_micro_backend_vf_iq_vexu1_fp_wakeup_corner);
// Analysis-only reachability probe: the 13 missing VecMem one-hot bins exclude the sole legal I2V producer port 2.
// TEST_REGISTER(ai_micro_backend_vecmem_i2v_wakeup_port2_corner);
// Analysis-only reachability probe: current LDU dependency/cancel timing never overlaps source cancel with a success response.
// TEST_REGISTER(ai_micro_backend_ldu_success_cancel_same_cycle_pressure_corner);
// RTL coverage probe: waveform checked VFRegFile p0 write as normal-software unreachable; keep out of default gate.
// TEST_REGISTER(ai_micro_backend_vfregfile_preg0_write_pressure_corner);
// Manual/nongate: SRNCTL fusion_enable is a LinkNan custom CSR path that official Spike does not model.
// TEST_REGISTER(ai_micro_backend_fcov_fusion_disable_custom_csr_corner);
// Manual/nongate: backend custom CSR behavior is a Nanhu project-specific path that official Spike does not model as a semantic gate.
// TEST_REGISTER(ai_arch_backend_custom_csr_privilege_programming_matrix_corner);

// ai_test_cases/backend/spec/ai_arch_scalar_instruction_cases.c
TEST_REGISTER(ai_arch_scalar_integer_alu_immediate_register_matrix_corner);
TEST_REGISTER(ai_arch_scalar_branch_jal_jalr_condition_and_link_corner);
TEST_REGISTER(ai_arch_scalar_mul_div_edge_matrix_corner);
TEST_REGISTER(ai_arch_scalar_fence_ecall_ebreak_csr_zifencei_corner);
TEST_REGISTER(ai_arch_scalar_fp_f_d_compute_convert_compare_matrix_corner);
TEST_REGISTER(ai_arch_scalar_b_extension_matrix_corner);
TEST_REGISTER(ai_arch_scalar_k_extension_opcode_smoke_matrix_corner);
TEST_REGISTER(ai_arch_scalar_zicond_zimop_zcmop_zcb_matrix_corner);
TEST_REGISTER(ai_arch_scalar_illegal_field_encoding_matrix_corner);
TEST_REGISTER(ai_arch_rvc_control_integer_expansion_visible_matrix_corner);
// ai_test_cases/backend/spec/ai_arch_scalar_crypto_extension_cases.c
TEST_REGISTER(ai_arch_scalar_crypto_bitmanip_permute_reference_matrix_corner);
TEST_REGISTER(ai_arch_scalar_crypto_hash_aes_schedule_reference_matrix_corner);
// ai_test_cases/backend/spec/ai_arch_vector_zvbb_cases.c
TEST_REGISTER(ai_arch_vector_zvbb_full_opcode_sew_reference_matrix_corner);
// ai_test_cases/backend/spec/ai_arch_scalar_fp_boundary_operand_cases.c
TEST_REGISTER(ai_arch_scalar_integer_boundary_operand_matrix_corner);
TEST_REGISTER(ai_arch_scalar_mul_div_boundary_operand_matrix_corner);
TEST_REGISTER(ai_arch_scalar_bitmanip_boundary_operand_matrix_corner);
TEST_REGISTER(ai_arch_fp_f_boundary_special_operand_matrix_corner);
TEST_REGISTER(ai_arch_fp_d_boundary_special_operand_matrix_corner);

// ai_test_cases/interrupt/ai_vmodule_basic_interrupt_cases.c
// Manual VModule-only basic interrupt smoke; requires a confirmed VModule-capable AP-IT/special-run with VMODULE=1.
// Current ordinary LinkNan/Spike runners do not run the VModule injection path.
// TEST_REGISTER(ai_vmodule_m_mei_basic_trap_corner);
// TEST_REGISTER(ai_vmodule_m_msi_basic_trap_corner);
// TEST_REGISTER(ai_vmodule_m_mti_basic_trap_corner);
// TEST_REGISTER(ai_vmodule_m_mei_delayed_trap_progress_corner);
// TEST_REGISTER(ai_vmodule_m_mei_during_repeated_mip_stip_writes_corner);

// ai_test_cases/interrupt/ai_interrupt_memory_cross_cases.c
TEST_REGISTER(ai_interrupt_memory_m_stip_disabled_load_store_then_enable_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pre_first_load_store_precise_return_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_fence_order_marker_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_amo_lrsc_contract_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_lrsc_interrupt_window_contract_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_load_access_fault_pending_then_enable_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_access_fault_no_side_effect_then_enable_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_mprv_u_permission_restore_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_prefetch_hint_no_side_effect_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_fault_younger_store_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_store_fault_remap_new_pa_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_cbozero_line_marker_smoke_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_cbozero_noncanonical_fault_marker_isolation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_cbozero_partial_store_line_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_cbozero_amo_handler_amo_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_misaligned_store_byte_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_dual_misaligned_store_template_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_scalar_cross_page_split_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_misaligned_store_fault_handler_line_image_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_misaligned_load_fault_handler_line_merge_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_fence_handler_log_marker_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_amo_aqrl_ordering_marker_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_amo_aqrl_handler_log_order_marker_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_handler_log_target_isolation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_fault_interrupt_trap_csr_isolation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pte_ad_fault_retry_generation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pmp_reconfig_deny_allow_retry_corner);
TEST_REGISTER(ai_interrupt_memory_hs_ssip_sum_mxr_permission_restore_corner);
TEST_REGISTER(ai_interrupt_memory_m_ssip_stip_fault_retry_priority_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_load_use_dependency_result_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_partial_store_forward_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_forward_raw_two_generation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_partial_overlap_negative_forward_smoke_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_burst_post_handler_progress_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_handler_spill_stack_isolation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_interrupt_storm_memory_progress_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_sc_failure_storm_status_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_fp_int_store_dirty_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_fp_load_fault_repair_handler_int_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_vs_dirty_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_load_queue_pressure_younger_load_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_rvc_load_store_pc2_mask_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_rvc_fp_load_store_pc2_dirty_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_compressed_store_fault_pc2_retry_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_amo_w_d_width_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_lrsc_w_d_reservation_width_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_lrsc_amo_handler_same_target_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_lrsc_handler_amo_log_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_load_amo_wb_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_misaligned_atomic_fault_pending_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pmp_lr_fault_handler_amo_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_fault_class_rotation_retry_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_active_mask_store_load_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_store_inactive_final_two_epoch_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_scalar_mixed_lsq_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_maskedoff_invalid_no_fault_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_tail_policy_store_load_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_indexed_unordered_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_strided_stride_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_segment_field_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_ssip_vector_segment_handler_scalar_probe_two_epoch_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_cross64_split_merge_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_whole_register_group_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_zero_active_no_side_effect_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vfof_nonzero_vl_then_fault_isolation_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vfof_elem0_fault_pending_retry_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vfof_shortened_tail_policy_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_segment_fof_fixvl_retry_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vector_store_fault_repair_vstart_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vector_store_two_late_faults_suffix_isolation_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vector_segment_store_fault_retry_vstart_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vsegment_vsmerge_feedback_source_split_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_vector_segment_load_fault_handler_spill_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_ordered_indexed_duplicate_vstore_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_ordered_indexed_vload_fault_repair_vstart_corner);
// Manual/RTL-only symptom: VirtualLoadQueue redirect recovery pointer/order needs LinkNan RTL/FSDB; this only supplies a black-box new-generation stimulus.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_vlq_fault_recovery_new_load_generation_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_sfence_remap_new_pa_visible_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_same_pa_alias_store_load_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_same_pa_alias_lrsc_handler_load_corner);
// Manual/nongate: PBMT=NC alias side-effect oracle is profile-specific and not an official Spike gate.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_same_alias_side_effect_corner);
// Manual/nongate: PBMT=NC/uncache request owner is a LinkNan/RTL model boundary, not an official Spike gate.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_old_new_request_owner_corner);
// Manual/nongate: direct-vs-replay UncacheBuffer arbitration is an RTL owner point; this is a PBMT=NC replay/direct progress smoke only.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_replay_direct_progress_smoke_corner);
// Manual/nongate: LoadReplayQ-to-Uncache needFree/no-response closure is an RTL/responder owner point; this is a PBMT=NC replay progress smoke only.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_replay_needfree_progress_smoke_corner);
// Manual/nongate: PBMT=NC store snapshot/cacheable marker ordering is a LinkNan/RTL model boundary, not an official Spike gate.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_store_snapshot_cache_marker_corner);
// Manual/nongate: PBMT=NC/cacheable same-PA alias oracle is profile-specific and not an official Spike gate.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_cache_alias_oracle_corner);
// Manual/nongate: PBMT=NC to cacheable same-VA retry toggle is profile-specific and not an official Spike gate.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_to_cacheable_retry_toggle_corner);
// Manual/nongate: vector PBMT=IO active/inactive lane access-class identity is profile-specific and not an official Spike gate.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_vector_pbmt_io_active_fault_masked_inactive_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_same_va_remap_store_generation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_fault_same_pc_retry_once_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_fault_repair_log_ring_sequence_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_wrong_path_store_handler_load_no_leak_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_fault_handler_block_copy_retry_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_ssip_handler_stack_fault_retry_owner_corner);
TEST_REGISTER(ai_interrupt_memory_hs_ssip_nested_handler_fault_preserves_main_retry_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_fetch_repair_code_patch_retry_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_mixed_width_post_handler_data_select_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_store_fault_handler_store_younger_mask_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_amo_queue_pop_handler_log_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_handler_cbozero_main_store_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_handler_cbozero_return_first_memory_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_cboinval_same_line_handler_store_profile_contract_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_cbom_clean_flush_handler_read_visibility_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_wfi_store_fence_wakeup_marker_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_release_acquire_payload_handler_ack_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_time_read_store_drain_marker_corner);
// Manual/nongate: CLINT/TLTimer MMIO source semantics need a platform responder; ordinary Spike is only a compile smoke here.
// TEST_REGISTER(ai_interrupt_memory_m_msip_mmio_source_write_handler_clear_corner);
// Manual/nongate: CLINT/TLTimer RegField.bytes lane-select semantics need a platform responder; ordinary Spike is only a compile smoke here.
// TEST_REGISTER(ai_interrupt_memory_m_msip_mmio_lane_select_handler_clear_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vector_handler_context_save_restore_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_vlm_vsm_handler_mask_restore_corner);
TEST_REGISTER(ai_interrupt_memory_hs_stip_superpage_demotion_promotion_rewalk_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_fault_then_post_handler_dense_lsq_burst_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pmp_overlap_reconfig_retry_priority_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_same_block_partial_store_storm_generation_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_same_line_handler_store_merge_image_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_same_block_load_prefetch_result_matrix_corner);
// Manual/nongate: spfctl/SMS prefetch effect-level behavior is a Nanhu custom path; official Spike/current ordinary LinkNan are not semantic gates.
// TEST_REGISTER(ai_interrupt_memory_m_stip_spfctl_disable_reenable_stream_owner_corner);
// Manual/nongate: replay/freeList duplicate-free closure is a LinkNan RTL owner point; this is an interrupt-adjacent black-box symptom smoke only.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_replay_freelist_interrupt_flush_smoke_corner);
// Manual/nongate: fast replay valid/ready acceptance and source-priority arbitration are LinkNan RTL owner points; this is an interrupt-adjacent black-box progress smoke only.
// TEST_REGISTER(ai_interrupt_memory_hs_ssip_fast_replay_vector_priority_progress_smoke_corner);
// Manual/nongate: commit/difftest/MemTrace/L1MissTrace owner ordering needs LinkNan trace audit; this case only supplies a selfchecked event stimulus.
// TEST_REGISTER(ai_interrupt_memory_m_stip_trace_commit_event_order_audit_corner);
// Manual/nongate: PBMT=NC frontend fetch/data-response owner is a LinkNan frontend/uncache model boundary; this case supplies an audit stimulus only.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_pbmt_nc_fetch_data_owner_audit_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_skipped_partial_store_post_handler_mask_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_wrong_path_load_result_quarantine_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_same_pc_helper_load_trace_identity_smoke_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pmp_4kb_split_retry_owner_corner);
TEST_REGISTER(ai_interrupt_memory_m_stip_pmpcfg_partial_update_interrupt_snapshot_corner);
// Manual/nongate: PMA CSR/cacheability/Atomic routing is profile-specific and not modeled by official Spike/current ordinary LinkNan.
// TEST_REGISTER(ai_interrupt_memory_m_stip_pma_cache_atomic_retry_generation_corner);
// Manual/Spike-gap: current official/community Spike does not fault this high-PPN leaf as intended.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_high_ppn_fault_retry_priority_corner);
// Manual/nongate: satp root switch plus sfence/stale-translation visibility is a TLB model boundary for official Spike.
// TEST_REGISTER(ai_interrupt_memory_hs_stip_satp_root_switch_new_context_corner);
// ai_test_cases/interrupt/ai_interrupt_memory_vmodule_cross_cases.c
// Manual VModule-only interrupt/memory cross; current LinkNan/Spike do not run VModule injection.
// Keep for future VModule-capable AP-IT/special-run with whole-ELF VMODULE=1, not for current default gate.
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_mei_load_store_source_clear_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_mei_store_drain_post_store_image_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_mei_load_refill_same_rd_owner_audit_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_mei_descriptor_doorbell_payload_ack_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_mei_descriptor_ring_two_entry_clear_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_chi_error_rnmi_mei_marker_owner_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_axi_error_rnmi_store_marker_owner_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_mei_source_clear_interrupted_by_rnmi_owner_corner);
// TEST_REGISTER(ai_interrupt_memory_m_vmodule_rnmi_mnret_pending_mei_pmp_retry_corner);
// TEST_REGISTER(ai_interrupt_memory_hs_vmodule_sei_clear_store_fault_retry_once_corner);

// ai_test_cases/interrupt/ai_interrupt_breakpoint_cross_cases.c
TEST_REGISTER(ai_interrupt_m_stip_pending_disabled_ebreak_then_enable_interrupt_corner);
// VModule/AP-IT manual interrupt+breakpoint cross cases; require a confirmed VModule-capable AP-IT/special-run.
// Current ordinary LinkNan/Spike runners do not run the VModule injection path.
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_pending_disabled_ebreak_then_enable_corner);
TEST_REGISTER(ai_interrupt_m_stip_pending_disabled_c_ebreak_then_enable_interrupt_corner);
TEST_REGISTER(ai_interrupt_m_stip_enabled_store_trigger_no_side_effect_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_delayed_mei_execute_trigger_event_set_corner);
TEST_REGISTER(ai_interrupt_m_stip_handler_then_store_trigger_state_preserved_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_mei_handler_then_store_trigger_state_preserved_corner);
TEST_REGISTER(ai_interrupt_m_stip_pending_execute_trigger_then_access_fault_then_interrupt_corner);
TEST_REGISTER(ai_interrupt_m_stip_pending_chain_mismatch_then_interrupt_then_chain_closed_corner);
TEST_REGISTER(ai_interrupt_m_stip_enable_csr_gap_ebreak_then_global_enable_corner);
TEST_REGISTER(ai_interrupt_m_mret_target_ebreak_pending_stip_pc_precise_corner);
TEST_REGISTER(ai_interrupt_m_stip_csr_write_window_ebreak_preserves_pending_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_stip_write_window_ebreak_preserves_both_pending_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_back_to_back_clear_source_reinject_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_chain_reprogram_no_stale_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_execute_trigger_delay_sweep_event_contract_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_redirect_trigger_metadata_fresh_pc_va_corner);
TEST_REGISTER(ai_interrupt_m_wfi_stip_wakeup_then_ebreak_precise_corner);
TEST_REGISTER(ai_interrupt_m_breakpoint_handler_opens_stip_rvi_rvc_pc_corner);
TEST_REGISTER(ai_interrupt_m_breakpoint_handler_reads_pending_stip_preserves_delivery_corner);
TEST_REGISTER(ai_interrupt_m_stip_self_modifying_code_fence_i_execute_trigger_new_image_corner);
TEST_REGISTER(ai_interrupt_m_mideleg_ssip_switch_breakpoint_final_hs_delivery_corner);
TEST_REGISTER(ai_interrupt_m_mideleg_ssip_pending_address_trigger_priv_isolation_corner);
TEST_REGISTER(ai_interrupt_m_mideleg_ssip_m_only_trigger_privilege_no_cross_fire_corner);
TEST_REGISTER(ai_interrupt_hs_sum_mxr_breakpoint_delegated_ssip_permission_window_corner);
TEST_REGISTER(ai_interrupt_m_breakpoint_handler_repairs_pmp_stip_sees_final_permission_corner);
TEST_REGISTER(ai_interrupt_m_stip_address_trigger_clear_order_no_cross_clear_corner);
TEST_REGISTER(ai_interrupt_m_pmp_update_window_ebreak_stip_final_permission_corner);
// Manual nongate: satp root switch plus sfence/stale-translation visibility is a TLB model boundary for official Spike.
// TEST_REGISTER(ai_interrupt_hs_satp_switch_ebreak_stip_sfence_new_context_corner);
// Manual nongate: sfence.vma stale-translation plus S/HS VA address trigger metadata is a TLB/trigger model boundary for official Spike.
// TEST_REGISTER(ai_interrupt_hs_sfence_breakpoint_handler_stip_no_stale_trigger_va_corner);
TEST_REGISTER(ai_interrupt_m_ssip_stip_priority_after_breakpoint_corner);
TEST_REGISTER(ai_interrupt_m_vectored_mtvec_ebreak_direct_stip_vector_slot_corner);
TEST_REGISTER(ai_interrupt_m_breakpoint_handler_switch_mtvec_pending_stip_new_vector_corner);
TEST_REGISTER(ai_interrupt_m_stip_handler_ebreak_same_source_no_reentry_corner);
TEST_REGISTER(ai_interrupt_m_stip_clear_sequence_nested_breakpoint_no_reentry_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_write_protocol_interrupted_no_half_state_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_storm_breakpoint_rearm_clear_once_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_branch_redirect_execute_trigger_winner_unique_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_lsq_younger_store_flush_on_breakpoint_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_status_epc_back_to_back_no_mix_corner);
// TEST_REGISTER(ai_interrupt_hs_vmodule_meip_breakpoint_priv_stack_restore_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_trap_csr_owner_back_to_back_no_stale_return_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_trigger_csr_read_slot_stability_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_predecode_illegal_execute_trigger_lane_no_swap_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_frontend_ebreak_lane_length_no_swap_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_indirect_predictor_wrong_path_no_ghost_breakpoint_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_bkp_mei_back_to_back_difftest_marker_contract_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_nmi31_breakpoint_csr_owner_no_cross_write_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_tdata2_half_config_no_ghost_breakpoint_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_tselect_tdata_half_config_no_ghost_breakpoint_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_exceptionbuffer_metadata_same_source_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_vector_vstart_pending_mei_metadata_preserved_corner);
TEST_REGISTER(ai_interrupt_m_cross_page_store_execute_trigger_stip_no_partial_side_effect_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_rvc_rvi_breakpoint_return_length_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_masked_breakpoint_mei_no_half_record_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_mnscratch_mscratch_log_isolation_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_breakpoint_mei_source_clear_order_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_breakpoint_preserves_mie_control_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_mei_breakpoint_winner_unique_replay_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_mei_clear_source_rnmi_no_reassert_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_mtvec_switch_vector_isolation_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_fetch_block_no_ghost_breakpoint_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_wfi_execute_trigger_wakeup_exactly_once_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_rnmi_wfi_breakpoint_first_commit_corner);
// TEST_REGISTER(ai_interrupt_m_unexpected_trap_rnmi_breakpoint_handler_fault_owner_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_vsetvl_breakpoint_mei_final_vector_metadata_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_vector_dirty_breakpoint_mei_no_pollution_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_store_address_trigger_late_cancel_no_payload_corner);
// TEST_REGISTER(ai_interrupt_hs_vmodule_pbmt_nc_store_breakpoint_mei_profile_guard_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_frontend_icache_refill_old_block_no_ghost_breakpoint_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_load_address_trigger_late_response_no_rd_leak_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_meip_stip_csr_write_arbitration_no_cross_clear_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_pmp_reconfig_flush_store_trigger_generation_contract_corner);
TEST_REGISTER(ai_interrupt_m_stip_amo_execute_trigger_rmw_atomic_contract_corner);
TEST_REGISTER(ai_interrupt_m_stip_lrsc_sc_execute_trigger_status_memory_consistent_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_amo_aqrl_address_trigger_suppresses_order_side_effect_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_lrsc_sc_breakpoint_no_stale_success_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_lrsc_sc_blocked_no_stale_store_intent_corner);
// TEST_REGISTER(ai_interrupt_m_vmodule_two_slot_chain_partial_match_no_cross_episode_leak_corner);
TEST_REGISTER(ai_interrupt_m_stip_pending_trigger_slot_rearm_no_ghost_corner);
TEST_REGISTER(ai_interrupt_m_store_address_trigger_stip_retry_no_duplicate_uop_corner);
TEST_REGISTER(ai_interrupt_m_stip_handler_clears_trigger_slot_rearm_breakpoint_corner);
TEST_REGISTER(ai_interrupt_m_breakpoint_handler_clears_stip_preserves_trigger_corner);
TEST_REGISTER(ai_interrupt_m_counter_csr_logging_breakpoint_stip_trap_state_corner);
TEST_REGISTER(ai_interrupt_m_nested_handler_log_scratch_preserved_corner);
TEST_REGISTER(ai_interrupt_m_illegal_breakpoint_pending_stip_priority_corner);
TEST_REGISTER(ai_interrupt_m_mcountinhibit_toggle_counter_oracle_corner);
TEST_REGISTER(ai_interrupt_m_cbozero_execute_trigger_stip_ordering_no_lost_zero_corner);
TEST_REGISTER(ai_interrupt_hs_fetch_page_fault_execute_trigger_stip_no_stale_block_corner);
TEST_REGISTER(ai_interrupt_m_rvc_rvi_mixed_width_pending_stip_pc_corner);
TEST_REGISTER(ai_interrupt_m_back_to_back_trap_log_no_stale_record_corner);
TEST_REGISTER(ai_interrupt_m_csrrs_csrrc_mie_window_ebreak_old_value_corner);

// ai_test_cases/memblock/exception_trap/ai_micro_fence_i_trigger_cases.c
// Manual RTL-focused: execute trigger on the fence.i instruction itself, plus fence.i blocked side-effect oracle.
// TEST_REGISTER(ai_micro_hs_fence_i_self_execute_trigger_blocks_fence_effects_corner);
// Manual RTL-focused: execute trigger on the sfence.vma instruction itself, plus stale-TLB preservation oracle.
// TEST_REGISTER(ai_micro_hs_sfence_vma_self_execute_trigger_preserves_old_tlb_entry_corner);



// //当支持double trap时
// blocked missing-source: m_double_trap_6; 要修改manual_test_cases/interrupt/xtvec_test_asm.S文件中mhandler退出用mnret指令
// TEST_REGISTER(m_double_trap_10);     //要修改manual_test_cases/interrupt/xtvec_test_asm.S文件中mhandler退出用mnret指令
// TEST_REGISTER(m_double_trap_12);     //要修改manual_test_cases/interrupt/xtvec_test_asm.S文件中mhandler退出用mnret指令
// TEST_REGISTER(m_double_trap_13);
// TEST_REGISTER(m_double_trap_14);
// TEST_REGISTER(m_double_trap_15);
// TEST_REGISTER(m_double_trap_16);
// TEST_REGISTER(m_double_trap_17);
// TEST_REGISTER(s_double_trap_1);
// TEST_REGISTER(s_double_trap_2);
// TEST_REGISTER(s_double_trap_3);
// TEST_REGISTER(s_double_trap_4);
// TEST_REGISTER(s_double_trap_5);     //要修改manual_test_cases/interrupt/xtvec_test_asm.S文件中mhandler退出用mnret指令
// TEST_REGISTER(s_double_trap_6);
// TEST_REGISTER(s_double_trap_7);
// TEST_REGISTER(s_double_trap_8);
// TEST_REGISTER(s_double_trap_9);
// TEST_REGISTER(s_double_trap_10);
// TEST_REGISTER(s_double_trap_11);
// TEST_REGISTER(s_double_trap_12);
// TEST_REGISTER(s_double_trap_13);
// TEST_REGISTER(s_double_trap_14);
// TEST_REGISTER(s_double_trap_15);
// TEST_REGISTER(s_double_trap_16);

// // //当支持pointer mask时
// TEST_REGISTER(ssnpm_test_1);
// TEST_REGISTER(ssnpm_test_2);
// TEST_REGISTER(ssnpm_test_3);
// TEST_REGISTER(ssnpm_test_4);
// TEST_REGISTER(ssnpm_test_5);
// TEST_REGISTER(ssnpm_test_6);
// TEST_REGISTER(ssnpm_test_7);
// TEST_REGISTER(ssnpm_test_8);
// TEST_REGISTER(ssnpm_test_9);
// TEST_REGISTER(ssnpm_test_10);
// TEST_REGISTER(ssnpm_test_11);
// TEST_REGISTER(ssnpm_test_12);
// TEST_REGISTER(ssnpm_test_13);
// TEST_REGISTER(ssnpm_test_14);
// TEST_REGISTER(ssnpm_test_15);
// TEST_REGISTER(ssnpm_test_16);
// TEST_REGISTER(ssnpm_test_17);
// TEST_REGISTER(ssnpm_test_18);
// TEST_REGISTER(ssnpm_test_19);
// TEST_REGISTER(ssnpm_test_20);
// TEST_REGISTER(ssnpm_test_21);
// TEST_REGISTER(ssnpm_test_22);
// TEST_REGISTER(ssnpm_test_23);
// TEST_REGISTER(ssnpm_test_24);
// TEST_REGISTER(ssnpm_test_25);
// TEST_REGISTER(ssnpm_test_26);
// TEST_REGISTER(ssnpm_test_27);
// TEST_REGISTER(ssnpm_test_28);
// TEST_REGISTER(ssnpm_test_29);
// TEST_REGISTER(ssnpm_test_30);
// TEST_REGISTER(ssnpm_test_31);
// TEST_REGISTER(ssnpm_test_32);
// TEST_REGISTER(ssnpm_test_33);
// TEST_REGISTER(ssnpm_test_34);
// TEST_REGISTER(ssnpm_test_35);
// TEST_REGISTER(ssnpm_test_36);
// TEST_REGISTER(ssnpm_test_37);
// TEST_REGISTER(ssnpm_test_38);
// TEST_REGISTER(ssnpm_test_39);
// TEST_REGISTER(ssnpm_test_40);
// TEST_REGISTER(ssnpm_test_41);
// TEST_REGISTER(ssnpm_test_333);
// TEST_REGISTER(smmpm_test_1);
// TEST_REGISTER(smmpm_test_2);
// TEST_REGISTER(smmpm_test_3);
// TEST_REGISTER(smmpm_test_4);
// TEST_REGISTER(smmpm_test_5);
// TEST_REGISTER(smnpm_test_1);
// TEST_REGISTER(smnpm_test_2);
// TEST_REGISTER(smnpm_test_3);
// TEST_REGISTER(smnpm_test_4);
// TEST_REGISTER(smnpm_test_5);



// TEST_REGISTER(stateen_C_test_1);
// TEST_REGISTER(stateen_C_test_2);
// TEST_REGISTER(stateen_C_test_3);
// TEST_REGISTER(stateen_C_test_4);
// TEST_REGISTER(stateen_C_test_5);
// TEST_REGISTER(stateen_C_test_6);
// TEST_REGISTER(stateen_SE0_test_1);
// TEST_REGISTER(stateen_SE0_test_2);
// TEST_REGISTER(stateen_SE0_test_3);
// TEST_REGISTER(stateen_SE0_test_4);
// TEST_REGISTER(stateen_ENVCFG_test_1);
// TEST_REGISTER(stateen_ENVCFG_test_2);
// TEST_REGISTER(stateen_ENVCFG_test_3);
// TEST_REGISTER(stateen_test_1);

// TEST_REGISTER(cycle_test_1);
// TEST_REGISTER(cycle_test_2);
// TEST_REGISTER(cycle_test_3);
// TEST_REGISTER(cycle_test_4);
// TEST_REGISTER(cycle_test_5);
// TEST_REGISTER(cycle_test_6);
// TEST_REGISTER(instret_test_1);
// TEST_REGISTER(instret_test_2);
// TEST_REGISTER(instret_test_3);
// TEST_REGISTER(instret_test_4);
// TEST_REGISTER(instret_test_5);
// TEST_REGISTER(instret_test_6);
// TEST_REGISTER(time_test_1);
// TEST_REGISTER(time_test_2);
// TEST_REGISTER(time_test_3);
// TEST_REGISTER(time_test_4);
// TEST_REGISTER(time_test_5);
// TEST_REGISTER(time_test_6);
// TEST_REGISTER(HPM_test_1);
// TEST_REGISTER(HPM_test_2);
// TEST_REGISTER(HPM_test_3);
// TEST_REGISTER(HPM_test_4);
// TEST_REGISTER(HPM_test_5);
// TEST_REGISTER(HPM_test_6);
// TEST_REGISTER(timecmp_test_1);
// TEST_REGISTER(timecmp_test_2);
// TEST_REGISTER(timecmp_test_3);
// TEST_REGISTER(timecmp_test_4);
// TEST_REGISTER(timecmp_test_5);
// TEST_REGISTER(timecmp_test_6);

// ai_test_cases/backend/privilege/ai_arch_privileged_counter_cases.c
TEST_REGISTER(ai_arch_mcounteren_disable_does_not_stop_mcycle_minstret_corner);
// TEST_REGISTER(ai_arch_mtime_mtimecmp_mmio_mtip_shadow_corner);
// ai_test_cases/backend/privilege/ai_arch_privileged_csr_cases.c
// Commented cases below require manual/special-run coverage or nongate review.
// TEST_REGISTER(ai_arch_mconfigptr_ro_mtval_warl_mseccfg_rw_corner);
// TEST_REGISTER(ai_arch_counter_write_and_shadow_access_matrix_corner);
TEST_REGISTER(ai_arch_mstateen_se0_controls_sstateen_access_corner);
TEST_REGISTER(ai_arch_mstatus_vs_field_writable_and_off_illegal_corner);
// TEST_REGISTER(ai_arch_mstatus_xs_sdt_dte_ro_zero_corner);
// TEST_REGISTER(ai_arch_menvcfg_lpe_sse_ro_zero_corner);
TEST_REGISTER(ai_arch_svinval_tvm_and_fence_pair_privilege_corner);
TEST_REGISTER(ai_arch_xret_preserves_lr_reservation_nhv51_corner);
TEST_REGISTER(ai_arch_svnapot_bit63_leaf_pte_page_fault_corner);
// ai_test_cases/backend/privilege/ai_linknan_privileged_csr_audit_cases.c
TEST_REGISTER(ai_linknan_priv_aia_extension_csr_write_illegal_matrix_corner);
// LinkNan-specific no-AIA/no-H negative CSR behavior; Spike configurations
// that expose H legitimately allow HVIP and cannot serve as this case's gate.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_linknan_priv_mvip_hvip_write_illegal_without_aia_h_corner);
TEST_REGISTER(ai_linknan_priv_unsupported_machine_csr_illegal_matrix_corner);
TEST_REGISTER(ai_linknan_priv_unsupported_supervisor_csr_illegal_matrix_corner);
TEST_REGISTER(ai_linknan_priv_h_vs_csr_illegal_matrix_corner);
TEST_REGISTER(ai_linknan_priv_envcfg_stateen_ro0_warl_matrix_corner);
// LinkNan current RTL exposes mseccfg as RO0; keep manual because reference
// models may instead treat mseccfg as absent without Smepmp/Zkr/Zicfilp/Smmpm.
// TEST_REGISTER(ai_linknan_priv_mseccfg_ro0_current_rtl_observation_corner);
// LinkNan custom CSR permission is controlled by mstateen0.C; official Spike
// does not provide the sbpctl/spfctl/smblockctl/srnctl semantic oracle.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_linknan_priv_mstateen0_c_controls_smode_custom_csr_rw_corner);

// TEST_REGISTER(mstatus_csr_tests_1);
// TEST_REGISTER(smrnmi_csr_tests_1);


    

// TEST_REGISTER(priv_change_8);
// TEST_REGISTER(priv_change_7);
// TEST_REGISTER(priv_change_6);
// TEST_REGISTER(priv_change_5);
// TEST_REGISTER(priv_change_4);
// TEST_REGISTER(priv_change_3);
// TEST_REGISTER(priv_change_2);
// TEST_REGISTER(priv_change_1);
// TEST_REGISTER(illegal_except_1);
// TEST_REGISTER(illegal_except_2);
// TEST_REGISTER(illegal_except_3);
// TEST_REGISTER(illegal_except_4);
// TEST_REGISTER(illegal_except_5);
// TEST_REGISTER(illegal_except_6);
// TEST_REGISTER(illegal_except_7);
// TEST_REGISTER(illegal_except_8);
// TEST_REGISTER(illegal_except_9);
// TEST_REGISTER(illegal_except_10);
// TEST_REGISTER(illegal_except_11);
// TEST_REGISTER(illegal_except_12);
// TEST_REGISTER(illegal_except_13);
// TEST_REGISTER(illegal_except_14);
// TEST_REGISTER(illegal_except_15);
// TEST_REGISTER(illegal_except_16);
// TEST_REGISTER(illegal_except_17);

// TEST_REGISTER(wfi_exception_tests_1);
// TEST_REGISTER(wfi_exception_tests_2);
// TEST_REGISTER(wfi_exception_tests_3);
// TEST_REGISTER(wfi_exception_tests_4);
// TEST_REGISTER(wfi_exception_tests_5);
// TEST_REGISTER(wfi_exception_tests_6);
// TEST_REGISTER(wfi_exception_tests_7);
// TEST_REGISTER(wfi_exception_tests_8);
// TEST_REGISTER(wfi_exception_tests_9);
// TEST_REGISTER(wfi_exception_tests_10);
// TEST_REGISTER(wfi_exception_tests_11);
// TEST_REGISTER(wfi_exception_tests_12);
// TEST_REGISTER(wfi_exception_tests_13);
// TEST_REGISTER(wfi_exception_tests_14);
// TEST_REGISTER(wfi_exception_tests_15);
// TEST_REGISTER(wfi_exception_tests_16);


// svpbmt相关
// //l1-cache 用的写回法：cache写命中则不立即写入主存（除非此块被换出）
// TEST_REGISTER(svpbmt_test_1);
// TEST_REGISTER(svpbmt_test_2);
// TEST_REGISTER(svpbmt_test_3);
// TEST_REGISTER(svpbmt_test_4);

// TEST_REGISTER(svpbmt_test_5);
// TEST_REGISTER(svpbmt_test_6);
// TEST_REGISTER(svpbmt_test_7);
// TEST_REGISTER(svpbmt_test_8);
// TEST_REGISTER(svpbmt_test_9);
// TEST_REGISTER(svpbmt_test_10);
// TEST_REGISTER(svpbmt_test_11);
// TEST_REGISTER(svpbmt_test_12);
// TEST_REGISTER(svpbmt_test_13);
// TEST_REGISTER(svpbmt_test_14);
// TEST_REGISTER(svpbmt_test_15);
// TEST_REGISTER(svpbmt_test_16);
// TEST_REGISTER(svpbmt_test_17);
// TEST_REGISTER(svpbmt_test_18);




//页表动态satp.mode相关:用例在 manual_test_cases/special/dynamic_page_table_test.c；默认 Makefile 排除该文件，单跑时使用 ENABLE_DYNAMIC_PT=1 切到 src/dynamic_page_tables.c 并纳入该用例文件
// TEST_REGISTER(test_sv39_mode);
// TEST_REGISTER(test_sv48_mode);
// TEST_REGISTER(test_sv39_to_sv48_switch);
// TEST_REGISTER(test_sv48_to_sv39_switch);
// TEST_REGISTER(test_multiple_switches_s_mode);
// TEST_REGISTER(test_multiple_switches_u_mode);
// TEST_REGISTER(satp_mode_test);   //涉及tlb，单独跑rtl(代码待设计fence前后取指变化)
// TEST_REGISTER(satp_ppn_test);   //涉及tlb，单独跑rtl
// TEST_REGISTER(satp_ppn_asid_limit_test);     //逻辑太简单，后续需完善
// TEST_REGISTER(ai_micro_hs_satp_sv48_to_sv39_next_pc_noncanonical_ifetch_page_fault_corner); // manual: SATP flush first-refetch RTL path
// TEST_REGISTER(ai_micro_linknan_sv39_mret_bad_pa_ifetch_flash_alias_access_fault_corner); // manual: LinkNan low-PA fetch alias path


//access fault相关
// TEST_REGISTER(instruction_access_fault_21);
// TEST_REGISTER(instruction_access_fault_20);
// TEST_REGISTER(instruction_access_fault_19);
// TEST_REGISTER(instruction_access_fault_18);
// TEST_REGISTER(instruction_access_fault_17);
// TEST_REGISTER(instruction_access_fault_16);
// TEST_REGISTER(instruction_access_fault_15);
// TEST_REGISTER(instruction_access_fault_14);
// TEST_REGISTER(instruction_access_fault_13);
// TEST_REGISTER(instruction_access_fault_12);
// TEST_REGISTER(instruction_access_fault_11);
// TEST_REGISTER(instruction_access_fault_10);
// TEST_REGISTER(instruction_access_fault_9);
// TEST_REGISTER(instruction_access_fault_8);
// TEST_REGISTER(instruction_access_fault_7);
// TEST_REGISTER(instruction_access_fault_6);
// TEST_REGISTER(instruction_access_fault_5);
// TEST_REGISTER(instruction_access_fault_4);
// TEST_REGISTER(instruction_access_fault_3);
// TEST_REGISTER(instruction_access_fault_2);
// TEST_REGISTER(instruction_access_fault_1);

// TEST_REGISTER(store_access_fault_29);
// TEST_REGISTER(store_access_fault_28);
// TEST_REGISTER(store_access_fault_27);
// TEST_REGISTER(store_access_fault_26);
// TEST_REGISTER(store_access_fault_25);
// TEST_REGISTER(store_access_fault_24);
// TEST_REGISTER(store_access_fault_23);
// TEST_REGISTER(store_access_fault_22);
// TEST_REGISTER(store_access_fault_21);
// TEST_REGISTER(store_access_fault_20);
// TEST_REGISTER(store_access_fault_19);
// TEST_REGISTER(store_access_fault_18);
// TEST_REGISTER(store_access_fault_17);
// TEST_REGISTER(store_access_fault_16);
// TEST_REGISTER(store_access_fault_15);
// TEST_REGISTER(store_access_fault_14);
// TEST_REGISTER(store_access_fault_13);
// TEST_REGISTER(store_access_fault_12);
// TEST_REGISTER(store_access_fault_11);
// TEST_REGISTER(store_access_fault_10);
// TEST_REGISTER(store_access_fault_9);
// TEST_REGISTER(store_access_fault_8);
// TEST_REGISTER(store_access_fault_7);
// TEST_REGISTER(store_access_fault_6);
// TEST_REGISTER(store_access_fault_5);
// TEST_REGISTER(store_access_fault_4);
// TEST_REGISTER(store_access_fault_3);
// TEST_REGISTER(store_access_fault_2);
// TEST_REGISTER(store_access_fault_1);

// TEST_REGISTER(amo_access_fault_28);
// TEST_REGISTER(amo_access_fault_27);
// TEST_REGISTER(amo_access_fault_26);
// TEST_REGISTER(amo_access_fault_25);
// TEST_REGISTER(amo_access_fault_24);
// TEST_REGISTER(amo_access_fault_23);
// TEST_REGISTER(amo_access_fault_22);
// TEST_REGISTER(amo_access_fault_21);
// TEST_REGISTER(amo_access_fault_20);
// TEST_REGISTER(amo_access_fault_19);
// TEST_REGISTER(amo_access_fault_18);
// TEST_REGISTER(amo_access_fault_17);
// TEST_REGISTER(amo_access_fault_16);
// TEST_REGISTER(amo_access_fault_15);
// TEST_REGISTER(amo_access_fault_14);
// TEST_REGISTER(amo_access_fault_13);
// TEST_REGISTER(amo_access_fault_12);
// TEST_REGISTER(amo_access_fault_11);
// TEST_REGISTER(amo_access_fault_10);
// TEST_REGISTER(amo_access_fault_9);
// TEST_REGISTER(amo_access_fault_8);
// TEST_REGISTER(amo_access_fault_7);
// TEST_REGISTER(amo_access_fault_6);
// TEST_REGISTER(amo_access_fault_5);
// TEST_REGISTER(amo_access_fault_4);
// TEST_REGISTER(amo_access_fault_3);
// TEST_REGISTER(amo_access_fault_2);
// TEST_REGISTER(amo_access_fault_1);

// TEST_REGISTER(load_access_fault_28);
// TEST_REGISTER(load_access_fault_27);
// TEST_REGISTER(load_access_fault_26);
// TEST_REGISTER(load_access_fault_25);
// TEST_REGISTER(load_access_fault_24);
// TEST_REGISTER(load_access_fault_23);
// TEST_REGISTER(load_access_fault_22);
// TEST_REGISTER(load_access_fault_21);
// TEST_REGISTER(load_access_fault_20);
// TEST_REGISTER(load_access_fault_19);
// TEST_REGISTER(load_access_fault_18);
// TEST_REGISTER(load_access_fault_17);
// TEST_REGISTER(load_access_fault_16);
// TEST_REGISTER(load_access_fault_15);
// TEST_REGISTER(load_access_fault_14);
// TEST_REGISTER(load_access_fault_13);
// TEST_REGISTER(load_access_fault_12);
// TEST_REGISTER(load_access_fault_11);
// TEST_REGISTER(load_access_fault_10);
// TEST_REGISTER(load_access_fault_9);
// TEST_REGISTER(load_access_fault_8);
// TEST_REGISTER(load_access_fault_7);
// TEST_REGISTER(load_access_fault_6);
// TEST_REGISTER(load_access_fault_5);
// TEST_REGISTER(load_access_fault_4);
// TEST_REGISTER(load_access_fault_3);
// TEST_REGISTER(load_access_fault_2);
// TEST_REGISTER(load_access_fault_1);



//page fault 相关
// TEST_REGISTER(tinst_tests_pf);
// TEST_REGISTER(random_instruction_tests);
// TEST_REGISTER(ebreak_tests_1);
// TEST_REGISTER(ebreak_tests_2);

// TEST_REGISTER(amo_page_fault_1);
// TEST_REGISTER(amo_page_fault_2);
// TEST_REGISTER(amo_page_fault_3);
// TEST_REGISTER(amo_page_fault_4);
// TEST_REGISTER(amo_page_fault_5);
// TEST_REGISTER(amo_page_fault_6);
// TEST_REGISTER(amo_page_fault_7);
// TEST_REGISTER(amo_page_fault_8);
// TEST_REGISTER(amo_page_fault_9);
// TEST_REGISTER(amo_page_fault_10);
// TEST_REGISTER(amo_page_fault_11);
// TEST_REGISTER(amo_page_fault_12);
// TEST_REGISTER(amo_page_fault_13);
// TEST_REGISTER(amo_page_fault_14);
// TEST_REGISTER(amo_page_fault_15);
// TEST_REGISTER(amo_page_fault_16);
// TEST_REGISTER(amo_page_fault_17);    //sv48时跑，sv39时不跑
// TEST_REGISTER(amo_page_fault_18);    //sv48时跑，sv39时不跑
// TEST_REGISTER(amo_page_fault_19);    //sv48时跑，sv39时不跑
// TEST_REGISTER(store_page_fault_1);
// TEST_REGISTER(store_page_fault_2);
// TEST_REGISTER(store_page_fault_3);
// TEST_REGISTER(store_page_fault_4);
// TEST_REGISTER(store_page_fault_5);
// TEST_REGISTER(store_page_fault_6);
// TEST_REGISTER(store_page_fault_7);
// TEST_REGISTER(store_page_fault_8);
// TEST_REGISTER(store_page_fault_9);
// TEST_REGISTER(store_page_fault_10);
// TEST_REGISTER(store_page_fault_11);
// TEST_REGISTER(store_page_fault_12);
// TEST_REGISTER(store_page_fault_13);
// TEST_REGISTER(store_page_fault_14);
// TEST_REGISTER(store_page_fault_15);
// TEST_REGISTER(store_page_fault_16);
// TEST_REGISTER(store_page_fault_17);    //sv48时跑，sv39时不跑
// TEST_REGISTER(store_page_fault_18);    //sv48时跑，sv39时不跑
// TEST_REGISTER(store_page_fault_19);    //sv48时跑，sv39时不跑
// TEST_REGISTER(load_page_fault_1);
// TEST_REGISTER(load_page_fault_2);
// TEST_REGISTER(load_page_fault_3);
// TEST_REGISTER(load_page_fault_4);
// TEST_REGISTER(load_page_fault_5);
// TEST_REGISTER(load_page_fault_6);
// TEST_REGISTER(load_page_fault_7);
// TEST_REGISTER(load_page_fault_8);
// TEST_REGISTER(load_page_fault_9);
// TEST_REGISTER(load_page_fault_10);
// TEST_REGISTER(load_page_fault_11);
// TEST_REGISTER(load_page_fault_12);
// TEST_REGISTER(load_page_fault_13);    //sv48时跑，sv39时不跑
// TEST_REGISTER(load_page_fault_14);    //sv48时跑，sv39时不跑
// TEST_REGISTER(load_page_fault_15);    //sv48时跑，sv39时不跑
// TEST_REGISTER(instruction_page_fault_1);
// TEST_REGISTER(instruction_page_fault_2_1);
// TEST_REGISTER(instruction_page_fault_2_2);
// TEST_REGISTER(instruction_page_fault_2_3);
// TEST_REGISTER(instruction_page_fault_3);
// TEST_REGISTER(instruction_page_fault_4);
// TEST_REGISTER(instruction_page_fault_5);
// TEST_REGISTER(instruction_page_fault_6);
// TEST_REGISTER(instruction_page_fault_7);
// TEST_REGISTER(instruction_page_fault_8);
// TEST_REGISTER(instruction_page_fault_9);
// TEST_REGISTER(instruction_page_fault_10);
// TEST_REGISTER(instruction_page_fault_11);
// TEST_REGISTER(instruction_page_fault_12);
// TEST_REGISTER(instruction_page_fault_13);    //sv48时跑，sv39时不跑
// TEST_REGISTER(instruction_page_fault_14);    //sv48时跑，sv39时不跑
// TEST_REGISTER(instruction_page_fault_15);    //sv48时跑，sv39时不跑
// TEST_REGISTER(instruction_page_fault_16);    //sv48时跑，sv39时不跑
// TEST_REGISTER(instruction_page_fault_17);    //sv48时跑，sv39时不跑
// TEST_REGISTER(instruction_page_fault_18);
// TEST_REGISTER(instruction_page_fault_19);
// TEST_REGISTER(instruction_page_fault_20);
// TEST_REGISTER(instruction_page_fault_21);
// TEST_REGISTER(instruction_page_fault_22);
// TEST_REGISTER(instruction_page_fault_23);
// TEST_REGISTER(instruction_page_fault_24);
// TEST_REGISTER(instruction_page_fault_25);

// TEST_REGISTER(addr_unaligned_1);
// TEST_REGISTER(addr_unaligned_2);
// TEST_REGISTER(addr_unaligned_3);
// TEST_REGISTER(addr_unaligned_4);
// TEST_REGISTER(addr_unaligned_5);
// TEST_REGISTER(two_stage_translation_1);
// TEST_REGISTER(mtvec_test_1);
// TEST_REGISTER(mtvec_test_2);
// TEST_REGISTER(mtvec_test_3);
// TEST_REGISTER(mtvec_test_4);
// TEST_REGISTER(mtvec_test_5);
// TEST_REGISTER(mtvec_test_6);
// TEST_REGISTER(mtvec_test_7);
// TEST_REGISTER(stvec_test_1);
// TEST_REGISTER(stvec_test_2);
// TEST_REGISTER(stvec_test_3);
// TEST_REGISTER(mnret_test_1);     //要修改manual_test_cases/interrupt/xtvec_test_asm.S文件中mhandler退出用mnret指令
// TEST_REGISTER(m_double_trap_11);
// TEST_REGISTER(mnret_test_2);
// TEST_REGISTER(rnnmi_handler);
// TEST_REGISTER(script_test);
// TEST_REGISTER(pmp_granularity_test);
// TEST_REGISTER(smrnmi_test_1);    //要修改manual_test_cases/interrupt/xtvec_test_asm.S文件中mhandler退出用mnret指令
// TEST_REGISTER(smrnmi_test_2);
// TEST_REGISTER(pma_csr_test);

// TEST_REGISTER(menvcfg_cbze_tests_1);
// TEST_REGISTER(menvcfg_cbze_tests_2);
// TEST_REGISTER(senvcfg_cbze_tests_3);
// TEST_REGISTER(menvcfg_cbzfe_tests_1);
// TEST_REGISTER(menvcfg_cbzfe_tests_2);
// TEST_REGISTER(senvcfg_cbzfe_tests_3);
// TEST_REGISTER(menvcfg_cbie_tests_1);
// TEST_REGISTER(menvcfg_cbie_tests_2);
// TEST_REGISTER(senvcfg_cbie_tests_3);

// TEST_REGISTER(pmp_test_1);
// TEST_REGISTER(pmp_test_2);
// TEST_REGISTER(pmp_test_3);
// TEST_REGISTER(pmp_test_4);
// TEST_REGISTER(pmp_test_5);


// TEST_REGISTER(fence_test_1);
// TEST_REGISTER(fence_test_2);
// TEST_REGISTER(fence_test_3);
// TEST_REGISTER(fence_test_4);

// TEST_REGISTER(caogao);
// TEST_REGISTER(caogao_2);


// TEST_REGISTER(load_access_fault_11);     //要分别弄sv39、sv48跑
// TEST_REGISTER(store_access_fault_11);      //要分别弄sv39、sv48跑    
// TEST_REGISTER(amo_access_fault_11);      //要分别弄sv39、sv48跑


//vec测试用例 草稿
// TEST_REGISTER(vec_vle_vse_func);
// TEST_REGISTER(vec_test_1);
// TEST_REGISTER(vec_test_2);
// TEST_REGISTER(vec_test_3);
// TEST_REGISTER(vec_test_4);
// TEST_REGISTER(vec_test_5);
// TEST_REGISTER(vec_test_6);
// TEST_REGISTER(vec_test_7);
// TEST_REGISTER(vec_test_8);
// TEST_REGISTER(vec_test_9);
// TEST_REGISTER(vec_test_10);
// TEST_REGISTER(vec_test_11);
// TEST_REGISTER(vec_test_12);
// TEST_REGISTER(vec_test_13);
// TEST_REGISTER(vec_test_14);
// TEST_REGISTER(vec_test_15);
// TEST_REGISTER(vec_test_16);
// TEST_REGISTER(vec_test_17);
// TEST_REGISTER(vec_test_18);
// TEST_REGISTER(vec_test_19);
// TEST_REGISTER(vec_test_20);
// TEST_REGISTER(vec_test_21);

// AI case batch 1: architectural load/store correctness
// TEST_REGISTER(ai_arch_lb_sign_extension);
// TEST_REGISTER(ai_arch_lbu_zero_extension);
// TEST_REGISTER(ai_arch_lh_sign_extension);
// TEST_REGISTER(ai_arch_lhu_zero_extension);
// TEST_REGISTER(ai_arch_lw_sign_extension);
// TEST_REGISTER(ai_arch_lwu_zero_extension);
// TEST_REGISTER(ai_arch_ld_full_width_read);
TEST_REGISTER(ai_arch_load_x0_ea_and_fault_metadata_corner);
TEST_REGISTER(ai_arch_cross_page_scalar_stitch_and_store_byte_enable_corner);
TEST_REGISTER(ai_arch_cacheline_cross_scalar_lw_sw_visible_boundary_corner);
TEST_REGISTER(ai_arch_cross_page_scalar_load_pf_metadata_rd_suppression_corner);
TEST_REGISTER(ai_arch_cross_page_scalar_store_low_pf_no_partial_write_corner);
TEST_REGISTER(ai_arch_load_wakeup_cancel_dependent_store_owner_corner);
TEST_REGISTER(ai_arch_load_wakeup_cancel_compressed_store_owner_corner);
// Manual/nongate strict no-partial-write probe: official Spike exposes the legal low-half fragment
// before a high-half split-store page fault, so this is kept out of the default architecture gate.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_cross_page_scalar_pf_metadata_no_partial_write_corner);
TEST_REGISTER(ai_arch_little_endian_scalar_lane_matrix_corner);
TEST_REGISTER(ai_arch_cross_entry_memory_image_consistency_corner);
TEST_REGISTER(ai_arch_vector_unit_strided_lane_visible_matrix_corner);
TEST_REGISTER(ai_arch_maskreg_bytepack_consumer_owner_corner);
TEST_REGISTER(ai_arch_vsm_mask_store_tail_fault_repair_corner);
TEST_REGISTER(ai_arch_vlm_mask_load_fault_old_new_v0_merge_repair_corner);
TEST_REGISTER(ai_arch_indexed_vstore_lane_fault_repair_corner);
TEST_REGISTER(ai_arch_indexed_vload_fault_repair_prefix_corner);
TEST_REGISTER(ai_arch_indexed_vload_masked_vlmax_exception_owner_corner);
TEST_REGISTER(ai_arch_indexed_vector_emul_gt_lmul_load_index_register_boundary_corner);
TEST_REGISTER(ai_arch_indexed_vector_emul_gt_lmul_store_index_register_boundary_corner);
TEST_REGISTER(ai_arch_strided_mask_vstart_overlap_owner_corner);
TEST_REGISTER(ai_arch_vssplit_older_cross16_younger_vector_order_corner);
TEST_REGISTER(ai_arch_vector_segment_whole_register_visible_layout_corner);
TEST_REGISTER(ai_arch_vsegment_store_field_data_churn_corner);
TEST_REGISTER(ai_arch_indexed_vsegment_store_load_boundary_owner_corner);
TEST_REGISTER(ai_arch_vsegment_store_mask_tail_cross16_payload_owner_corner);
TEST_REGISTER(ai_arch_vector_vstart_success_path_owner_corner);
TEST_REGISTER(ai_arch_vector_mask_tail_vl_visible_matrix_corner);
TEST_REGISTER(ai_micro_unitstride_no_split_mask_then_cross16_split_store_byte_image_corner);
TEST_REGISTER(ai_micro_unitstride_cross16_sparse_store_then_vector_load_query_mask_corner);
TEST_REGISTER(ai_micro_hs_vector_store_misalign_prefix_then_late_page_fault_replay_keeps_completed_flow_corner);
TEST_REGISTER(ai_micro_hs_vector_store_first_page_fault_blocks_later_misalign_ghost_flow_corner);
TEST_REGISTER(ai_micro_hs_virtual_lq_near_full_wrong_path_redirect_recovery_no_ghost_full_corner);
// AP-101 manual: Nanhu/local Spike whole-register dirty-before-memory alignment policy, not an official Spike gate.
// TEST_REGISTER(ai_arch_vl1re8_element0_page_fault_dirty_vs_corner);
// TEST_REGISTER(ai_arch_vl1re8_element0_access_fault_dirty_vs_corner);
// TEST_REGISTER(ai_arch_vl8re64_element0_page_fault_dirty_vs_corner);
// TEST_REGISTER(ai_arch_vs1r_element0_page_fault_dirty_vs_corner);
// TEST_REGISTER(ai_arch_vs8r_element0_page_fault_dirty_vs_corner);
// TEST_REGISTER(ai_arch_vl1re8_vs_off_illegal_keeps_vs_off_corner);
// TEST_REGISTER(ai_arch_vl1re8_nonzero_vstart_page_fault_dirty_vs_corner);
// AP-110 manual: Nanhu VecExcpDataMerge whole-register exception merge boundary needs LinkNan/RTL closure.
// Black-box probes dump v8.. after later-register faults; internal sNoExcp_* signals still need waveform evidence.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_ap110_vl4re8_fault_second_register_preserves_old_tail_manual_corner);
// TEST_REGISTER(ai_arch_ap110_vl4re8_fault_third_fourth_register_boundaries_manual_corner);
// TEST_REGISTER(ai_arch_ap110_whole_load_width_matrix_tail_preservation_manual_corner);
// TEST_REGISTER(ai_arch_ap110_whole_register_nf_handle_count_full_copy_manual_corner);
TEST_REGISTER(ai_micro_ap7_vector_store_late_element_page_fault_dirty_vs_corner);
TEST_REGISTER(ai_micro_ap7_vector_load_late_element_page_fault_dirty_vs_corner);
TEST_REGISTER(ai_micro_ap7_scalar_load_page_fault_keeps_vs_clean_corner);
TEST_REGISTER(ai_micro_ap7_repeated_vector_load_faults_dirty_vs_after_clean_reset_corner);
// AP-7 manual RTL-focused: trigger/redirect and ROB-pressure variants are kept out of the default architecture gate.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap7_vector_load_fault_after_address_trigger_redirect_dirty_vs_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap7_vector_load_fault_after_scalar_rob_pressure_dirty_vs_manual_smoke_corner);
TEST_REGISTER(ai_arch_ap93_vsseg2e8_field1_fault_keeps_field0_store_corner);
TEST_REGISTER(ai_arch_ap93_vsseg2e32_later_segment_fault_keeps_prefix_fields_corner);
TEST_REGISTER(ai_arch_ap93_vssseg2e32_field1_fault_keeps_field0_store_corner);
TEST_REGISTER(ai_arch_ap93_vsuxseg2ei32_field1_fault_keeps_field0_store_corner);
// AP-93/AP-85 load-side Nanhu-aligned policy is not an official Spike gate.
// TEST_REGISTER(ai_arch_ap93_segment_load_fault_keeps_fault_segment_unwritten_corner);
// AP-85 manual: local Spike/Nanhu segment-load no-partial-destination alignment policy.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_ap85_vlseg2e32_later_segment_field1_fault_no_partial_dest_corner);
// TEST_REGISTER(ai_arch_ap85_vlseg4_vlseg8_middle_field_fault_no_partial_dest_corner);
// TEST_REGISTER(ai_arch_ap85_vluxseg2ei32_field1_fault_no_partial_dest_corner);
// TEST_REGISTER(ai_arch_ap85_masked_vlseg2e32_maskedoff_fault_no_trap_corner);
// TEST_REGISTER(ai_arch_ap85_vlsseg2e32_field1_cross_page_no_partial_dest_corner);
// TEST_REGISTER(ai_arch_ap85_vlseg2e32_tail_policy_after_fault_no_partial_dest_corner);
// AP-92 manual RTL-focused: VLMergeBuffer threshold/enq accounting needs LinkNan waveform/RTL evidence.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap92_vlmergebuffer_dense_unit_stride_progress_corner);
// TEST_REGISTER(ai_micro_ap92_vlmergebuffer_segment_burst_oldest_progress_corner);
// TEST_REGISTER(ai_micro_ap92_faulted_vector_load_then_repaired_burst_no_ghost_threshold_corner);
// TEST_REGISTER(ai_micro_ap92_storequeue_non_cross_unaligned_forwarding_no_overreplay_corner);
// TEST_REGISTER(ai_arch_lb_second_doubleword_sign_extension_corner);
// TEST_REGISTER(ai_arch_lbu_second_doubleword_zero_extension_corner);
// TEST_REGISTER(ai_arch_lh_second_doubleword_sign_extension_corner);
// TEST_REGISTER(ai_arch_lhu_second_doubleword_zero_extension_corner);
// TEST_REGISTER(ai_arch_lw_second_doubleword_sign_extension_corner);
// TEST_REGISTER(ai_arch_lwu_second_doubleword_zero_extension_corner);
// TEST_REGISTER(ai_arch_lb_second_doubleword_tail_byte_sign_extension_corner);
// TEST_REGISTER(ai_arch_lbu_second_doubleword_tail_byte_zero_extension_corner);
// TEST_REGISTER(ai_arch_lh_second_doubleword_upper_halfword_sign_extension_corner);
// TEST_REGISTER(ai_arch_lhu_second_doubleword_upper_halfword_zero_extension_corner);
// TEST_REGISTER(ai_arch_lw_second_doubleword_upper_word_sign_extension_corner);
// TEST_REGISTER(ai_arch_lwu_second_doubleword_upper_word_zero_extension_corner);
// TEST_REGISTER(ai_arch_ld_second_doubleword_full_width_corner);
// TEST_REGISTER(ai_arch_lb_sign_extension_tail_byte_corner);
// TEST_REGISTER(ai_arch_lbu_zero_extension_tail_byte_corner);
// TEST_REGISTER(ai_arch_lh_sign_extension_upper_halfword_corner);
// TEST_REGISTER(ai_arch_lhu_zero_extension_upper_halfword_corner);
// TEST_REGISTER(ai_arch_lw_sign_extension_upper_word_corner);
// TEST_REGISTER(ai_arch_lwu_zero_extension_upper_word_corner);
// TEST_REGISTER(ai_arch_sb_byte_writeback);
// TEST_REGISTER(ai_arch_sh_halfword_writeback);
// TEST_REGISTER(ai_arch_sw_word_writeback);
// TEST_REGISTER(ai_arch_sd_doubleword_writeback);
// TEST_REGISTER(ai_arch_sb_low_byte_preserves_second_doubleword_corner);
// TEST_REGISTER(ai_arch_sh_low_halfword_preserves_second_doubleword_corner);
// TEST_REGISTER(ai_arch_sw_low_word_preserves_second_doubleword_corner);
// TEST_REGISTER(ai_arch_sb_tail_byte_preserves_second_doubleword_corner);
// TEST_REGISTER(ai_arch_sh_upper_halfword_preserves_second_doubleword_corner);
// TEST_REGISTER(ai_arch_sw_upper_word_preserves_second_doubleword_corner);
// TEST_REGISTER(ai_arch_sd_second_doubleword_writeback_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sb_second_doubleword_low_byte_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sh_second_doubleword_low_halfword_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sw_second_doubleword_low_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sb_second_doubleword_tail_byte_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sh_second_doubleword_upper_halfword_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sw_second_doubleword_upper_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_sb_tail_byte_writeback_corner);
// TEST_REGISTER(ai_arch_sh_upper_halfword_writeback_corner);
// TEST_REGISTER(ai_arch_sw_lower_word_writeback_preserves_upper_word_corner);
// TEST_REGISTER(ai_arch_sw_upper_word_writeback_preserves_lower_word_corner);
TEST_REGISTER(ai_arch_store_source_lane_and_order_visibility_corner);
TEST_REGISTER(ai_arch_rvc_load_store_expansion_visible_matrix_corner);
TEST_REGISTER(ai_arch_rvc_fault_metadata_repair_visible_matrix_corner);
TEST_REGISTER(ai_arch_compressed_store_mab_pc2_crosspage_owner_corner);
TEST_REGISTER(ai_arch_rvc_reserved_encoding_illegal_isolation_matrix_corner);
TEST_REGISTER(ai_arch_rvc_fp_double_fault_repair_fs_dirty_corner);
TEST_REGISTER(ai_arch_zcb_narrow_fuop_forward_owner_corner);
TEST_REGISTER(ai_arch_zcb_narrow_fault_repair_metadata_corner);

// AI case batch 2: floating-point load path and AMO correctness
// TEST_REGISTER(ai_arch_flw_register_writeback);
// TEST_REGISTER(ai_arch_fld_register_writeback);
TEST_REGISTER(ai_arch_ap78_fp_load_fs_dirty_control_corner);
TEST_REGISTER(ai_arch_dirtyfs_wrong_path_fp_memory_squash_corner);
// TEST_REGISTER(ai_arch_ap78_mmio_fp_load_fs_dirty_manual_smoke_corner);
// TEST_REGISTER(ai_arch_flw_nan_payload_preserved_corner);
// TEST_REGISTER(ai_arch_flw_negative_zero_preserved_corner);
// TEST_REGISTER(ai_arch_flw_upper_word_negative_zero_preserved_corner);
// TEST_REGISTER(ai_arch_flw_upper_word_nan_payload_preserved_corner);
// TEST_REGISTER(ai_arch_flw_second_doubleword_negative_zero_preserved_corner);
// TEST_REGISTER(ai_arch_flw_second_doubleword_nan_payload_preserved_corner);
// TEST_REGISTER(ai_arch_flw_second_doubleword_upper_word_nan_payload_preserved_corner);
// TEST_REGISTER(ai_arch_flw_second_doubleword_upper_word_negative_zero_preserved_corner);
// TEST_REGISTER(ai_arch_fld_second_doubleword_register_writeback_corner);
// TEST_REGISTER(ai_arch_fld_negative_zero_preserved_corner);
// TEST_REGISTER(ai_arch_fld_nan_payload_preserved_corner);
// TEST_REGISTER(ai_arch_fld_second_doubleword_nan_payload_preserved_corner);
// TEST_REGISTER(ai_arch_fld_second_doubleword_negative_zero_preserved_corner);
TEST_REGISTER(ai_arch_fp_load_store_raw_bits_fflags_matrix_corner);
TEST_REGISTER(ai_arch_fp_load_selector_cross16_owner_corner);
TEST_REGISTER(ai_arch_fp_store_producer_forward_cross16_owner_corner);
TEST_REGISTER(ai_arch_fp_fs_off_illegal_no_fpr_memory_or_pte_side_effects_corner);
// TEST_REGISTER(ai_arch_amoswap_w_exchange);
// TEST_REGISTER(ai_arch_amoadd_w_accumulate);
// TEST_REGISTER(ai_arch_amoand_w_mask);
// TEST_REGISTER(ai_arch_amoor_w_merge);
// TEST_REGISTER(ai_arch_amoxor_w_toggle);
// TEST_REGISTER(ai_arch_amomin_w_signed);
// TEST_REGISTER(ai_arch_amomax_w_signed);
// TEST_REGISTER(ai_arch_amominu_w_unsigned);
// TEST_REGISTER(ai_arch_amomaxu_w_unsigned);
// TEST_REGISTER(ai_arch_amo_d_variants);
TEST_REGISTER(ai_arch_amo_rmw_op_width_visible_matrix_corner);
TEST_REGISTER(ai_arch_amoalu_cmd_width_signedness_churn_corner);
TEST_REGISTER(ai_arch_amo_aq_rl_rmw_semantics_corner);
// TEST_REGISTER(ai_arch_amoswap_w_sign_extended_old_value_corner);
// TEST_REGISTER(ai_arch_amoadd_w_wraparound_corner);
// TEST_REGISTER(ai_arch_amomaxu_w_sign_boundary_corner);
// TEST_REGISTER(ai_arch_amomin_w_signed_boundary_corner);
// TEST_REGISTER(ai_arch_amominu_w_zero_boundary_corner);
// TEST_REGISTER(ai_arch_amomax_w_signed_boundary_corner);
// TEST_REGISTER(ai_arch_amoswap_w_lower_word_preserves_upper_word_corner);
// TEST_REGISTER(ai_arch_amoadd_w_wraparound_upper_word_preserved_corner);
// TEST_REGISTER(ai_arch_amomin_w_signed_boundary_upper_word_preserved_corner);
// TEST_REGISTER(ai_arch_amomaxu_w_sign_boundary_upper_word_preserved_corner);
// TEST_REGISTER(ai_arch_amominu_w_zero_boundary_upper_word_preserved_corner);
// TEST_REGISTER(ai_arch_amomax_w_signed_boundary_upper_word_preserved_corner);
// TEST_REGISTER(ai_arch_amoand_w_lower_word_preserves_upper_word_corner);
// TEST_REGISTER(ai_arch_amoor_w_lower_word_preserves_upper_word_corner);
// TEST_REGISTER(ai_arch_amoxor_w_lower_word_preserves_upper_word_corner);
// TEST_REGISTER(ai_arch_amoswap_w_upper_word_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amoadd_w_upper_word_wraparound_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amomin_w_upper_word_signed_boundary_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amomaxu_w_upper_word_sign_boundary_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amominu_w_upper_word_zero_boundary_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amomax_w_upper_word_signed_boundary_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amoand_w_upper_word_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amoor_w_upper_word_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amoxor_w_upper_word_preserves_lower_word_corner);
// TEST_REGISTER(ai_arch_amoswap_w_second_doubleword_low_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoadd_w_second_doubleword_low_word_wraparound_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomin_w_second_doubleword_low_word_signed_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomaxu_w_second_doubleword_low_word_sign_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amominu_w_second_doubleword_low_word_zero_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomax_w_second_doubleword_low_word_signed_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoand_w_second_doubleword_low_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoor_w_second_doubleword_low_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoxor_w_second_doubleword_low_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoswap_w_second_doubleword_upper_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoadd_w_second_doubleword_upper_word_wraparound_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomin_w_second_doubleword_upper_word_signed_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomaxu_w_second_doubleword_upper_word_sign_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amominu_w_second_doubleword_upper_word_zero_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomax_w_second_doubleword_upper_word_signed_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoand_w_second_doubleword_upper_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoor_w_second_doubleword_upper_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoxor_w_second_doubleword_upper_word_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoadd_d_wraparound_corner);
// TEST_REGISTER(ai_arch_amomin_d_signed_boundary_corner);
// TEST_REGISTER(ai_arch_amominu_d_zero_boundary_corner);
// TEST_REGISTER(ai_arch_amomaxu_d_sign_boundary_corner);
// TEST_REGISTER(ai_arch_amomax_d_signed_boundary_corner);
// TEST_REGISTER(ai_arch_amoadd_d_wraparound_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amomin_d_signed_boundary_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amomaxu_d_sign_boundary_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amominu_d_zero_boundary_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amomax_d_signed_boundary_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amoand_d_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amoor_d_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amoxor_d_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_arch_amoswap_d_second_doubleword_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoadd_d_second_doubleword_wraparound_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomin_d_second_doubleword_signed_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amominu_d_second_doubleword_zero_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomax_d_second_doubleword_signed_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amomaxu_d_second_doubleword_sign_boundary_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoand_d_second_doubleword_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoor_d_second_doubleword_preserves_first_doubleword_corner);
// TEST_REGISTER(ai_arch_amoxor_d_second_doubleword_preserves_first_doubleword_corner);

// AI case batch 3: prefetch permission behavior and AMO fault class
TEST_REGISTER(ai_arch_prefetch_i_r_w_repeated_legal_no_arch_writeback_corner);
// TEST_REGISTER(ai_arch_prefetch_i_no_x_silent);
// TEST_REGISTER(ai_arch_prefetch_r_no_r_silent);
// TEST_REGISTER(ai_arch_prefetch_w_no_w_silent);
// Prefetch execute-trigger probes are generated and tracked, but remain out of the default Spike batch
// until hint-instruction breakpoint behavior is explicitly validated on the target Spike/RTL pair.
// TEST_REGISTER(ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_no_x_fetch_page_fault_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_no_x_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_redirect_kill_no_late_ifetch_prefetch_emit_corner);
// TEST_REGISTER(ai_arch_killed_prefetch_i_reused_load_slot_no_stale_frontend_prefetch_vaddr_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_noncanonical_valid_fetch_recovers_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_all_ones_valid_fetch_recovers_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_r_execute_trigger_then_no_r_load_page_fault_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_trigger_then_no_w_store_page_fault_corner);
// TEST_REGISTER(ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_no_r_load_page_fault_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_no_w_store_page_fault_corner);
// TEST_REGISTER(ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_grant_r_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_grant_r_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_grant_w_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_grant_w_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_r_address_trigger_silent_then_grant_r_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_address_trigger_silent_then_grant_r_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_w_address_trigger_silent_then_grant_w_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_address_trigger_silent_then_grant_w_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_r_execute_then_address_trigger_silent_grant_r_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_execute_then_address_trigger_silent_grant_r_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_then_address_trigger_silent_grant_w_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_execute_then_address_trigger_silent_grant_w_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_silent_then_fetch_still_faults);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_silent_then_fetch_still_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_silent_then_fetch_still_faults_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_silent_then_grant_x_executes_corner);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_silent_then_grant_x_executes_corner);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_silent_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_noncanonical_silent_then_fetch_page_faults);
// TEST_REGISTER(ai_arch_prefetch_i_noncanonical_repeated_silent_then_fetch_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_i_noncanonical_repeated_silent_then_fetch_page_faults_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_noncanonical_silent_then_valid_fetch_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_i_noncanonical_repeated_silent_then_valid_fetch_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_i_noncanonical_repeated_silent_then_valid_fetch_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_all_ones_silent_then_fetch_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_i_all_ones_repeated_silent_then_fetch_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_i_all_ones_repeated_silent_then_fetch_page_faults_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_all_ones_silent_then_valid_fetch_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_i_all_ones_repeated_silent_then_valid_fetch_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_i_all_ones_repeated_silent_then_valid_fetch_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_leaf_overflow_silent_then_fetch_access_faults);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fetch_access_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fetch_access_faults_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_i_leaf_overflow_silent_then_fix_leaf_executes_corner);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fix_leaf_executes_corner);
// TEST_REGISTER(ai_arch_prefetch_i_repeated_leaf_overflow_silent_then_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_prefetch_r_silent_then_load_still_faults);
// TEST_REGISTER(ai_arch_prefetch_r_repeated_silent_then_load_still_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_r_silent_then_grant_r_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_repeated_silent_then_grant_r_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_noncanonical_silent_then_load_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_r_noncanonical_repeated_silent_then_load_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_r_noncanonical_silent_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_noncanonical_repeated_silent_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_all_ones_silent_then_load_page_faults);
// TEST_REGISTER(ai_arch_prefetch_r_all_ones_repeated_silent_then_load_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_r_all_ones_silent_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_all_ones_repeated_silent_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_r_leaf_overflow_silent_then_load_access_faults);
// TEST_REGISTER(ai_arch_prefetch_r_repeated_leaf_overflow_silent_then_load_access_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_r_leaf_overflow_silent_then_fix_leaf_loads_corner);
// TEST_REGISTER(ai_arch_prefetch_r_repeated_leaf_overflow_silent_then_fix_leaf_loads_corner);
// TEST_REGISTER(ai_arch_prefetch_w_silent_then_store_still_faults);
// TEST_REGISTER(ai_arch_prefetch_w_repeated_silent_then_store_still_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_w_silent_then_grant_w_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_repeated_silent_then_grant_w_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_noncanonical_silent_then_store_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_w_noncanonical_repeated_silent_then_store_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_w_noncanonical_silent_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_noncanonical_repeated_silent_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_all_ones_silent_then_store_page_faults);
// TEST_REGISTER(ai_arch_prefetch_w_all_ones_repeated_silent_then_store_page_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_w_all_ones_silent_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_all_ones_repeated_silent_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_arch_prefetch_w_leaf_overflow_silent_then_store_access_faults);
// TEST_REGISTER(ai_arch_prefetch_w_repeated_leaf_overflow_silent_then_store_access_faults_corner);
// TEST_REGISTER(ai_arch_prefetch_w_leaf_overflow_silent_then_fix_leaf_stores_corner);
// TEST_REGISTER(ai_arch_prefetch_w_repeated_leaf_overflow_silent_then_fix_leaf_stores_corner);
// TEST_REGISTER(ai_arch_prefetch_pte_ad_no_update_then_real_access_rejudges_corner);
// TEST_REGISTER(ai_arch_amo_misaligned_fault);
TEST_REGISTER(ai_arch_amo_misaligned_fault_tval_raw_no_side_effect_corner);
TEST_REGISTER(ai_arch_amoswap_w_misaligned_fault_tval_raw_no_side_effect_corner);
// TEST_REGISTER(ai_arch_amoswap_w_misaligned_fault_then_lower_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoswap_w_misaligned_fault_then_upper_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoadd_w_misaligned_fault_then_lower_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoadd_w_misaligned_fault_then_upper_word_recovers_corner);
TEST_REGISTER(ai_arch_amo_misaligned_fault_then_aligned_recovers_corner);
TEST_REGISTER(ai_arch_amo_misaligned_repeated_faults_then_aligned_recovers_corner);
TEST_REGISTER(ai_arch_amo_misaligned_repeated_faults_tval_raw_no_side_effect_corner);
TEST_REGISTER(ai_arch_amo_permission_fault_metadata_no_side_effect_matrix_corner);
TEST_REGISTER(ai_arch_amoand_d_noncanonical_va_page_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_amoswap_w_repeated_misaligned_faults_tval_raw_no_side_effect_corner);
// TEST_REGISTER(ai_arch_amoswap_w_repeated_misaligned_fault_then_upper_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoswap_w_repeated_misaligned_fault_then_lower_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoadd_w_repeated_misaligned_fault_then_lower_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoadd_w_repeated_misaligned_fault_then_upper_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoswap_w_repeated_lower_lane_misaligned_fault_then_upper_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoswap_w_repeated_upper_lane_misaligned_fault_then_lower_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoadd_w_repeated_lower_lane_misaligned_fault_then_upper_word_recovers_corner);
// TEST_REGISTER(ai_arch_amoadd_w_repeated_upper_lane_misaligned_fault_then_lower_word_recovers_corner);
// These PBMT/PMA-dependent AMO fault cases are generated and tracked in test_point, but remain
// unregistered in the default Spike batch because PBMT/PMA CSR behavior is not a reliable Spike gate today.
// TEST_REGISTER(ai_arch_amo_io_nc_region_fault);
// TEST_REGISTER(ai_arch_amo_pma_device_region_fault);

// AI case batch 4: Svpbmt data-route correctness
// TEST_REGISTER(ai_arch_pbmt_pma_cacheable_data_correct);
// TEST_REGISTER(ai_arch_pbmt_nc_data_correct);
// TEST_REGISTER(ai_arch_pbmt_nc_narrow_load_store_mask_extension_corner);
// TEST_REGISTER(ai_arch_pbmt_nc_fault_repair_load_store_current_value_corner);
// TEST_REGISTER(ai_arch_cross_page_pbmt_half_attr_fault_repair_rejudge_corner);
// This PBMT=IO route case targets the LinkNan responsive 3T IO/uncache window.
// Keep it compiled and tracked in test_point, but out of the default Spike batch.
// TEST_REGISTER(ai_arch_pbmt_io_data_correct);
// TEST_REGISTER(ai_arch_pbmt_nc_over_pma_io_aligned_load_store_profile_result_corner);
// TEST_REGISTER(ai_arch_pbmt_pbmte_toggle_rejudges_nc_alias_corner);
// TEST_REGISTER(ai_arch_pbmt_reserved_leaf_faults_before_final_attr_route_corner);
// PBMT attribute switching uses MENVCFG.PBMTE, PMA reprogramming, Device/PBMT IO,
// and CBO final memory-attribute routing; keep it as a manual nongate case.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_pbmt_attr_switch_load_store_amo_cbo_rejudge_corner);

// AI case batch 5: exception priority and virtual-address boundary
TEST_REGISTER(ai_arch_all_ones_virtual_address_faults);
TEST_REGISTER(ai_arch_all_ones_store_page_fault_no_side_effect_corner);
TEST_REGISTER(ai_arch_all_ones_faults_then_valid_access_recovers_corner);
TEST_REGISTER(ai_arch_all_ones_faults_then_valid_access_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_all_ones_repeated_faults_stable_corner);
TEST_REGISTER(ai_arch_all_ones_repeated_faults_then_valid_access_recovers_corner);
TEST_REGISTER(ai_arch_all_ones_repeated_faults_then_valid_access_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_noncanonical_virtual_address_faults);
TEST_REGISTER(ai_arch_noncanonical_store_page_fault_no_side_effect_corner);
TEST_REGISTER(ai_arch_noncanonical_faults_then_valid_access_recovers_corner);
TEST_REGISTER(ai_arch_noncanonical_faults_then_valid_access_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_noncanonical_repeated_faults_stable_corner);
TEST_REGISTER(ai_arch_noncanonical_repeated_faults_then_valid_access_recovers_corner);
TEST_REGISTER(ai_arch_noncanonical_repeated_faults_then_valid_access_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_ialign16_rvc_jal_jalr_branch_halfword_targets_legal_corner);
TEST_REGISTER(ai_arch_ialign32_jal_jalr_branch_misaligned_targets_corner);
// TEST_REGISTER(ai_arch_fetch_all_ones_page_fault_tval_aligned_corner);
// TEST_REGISTER(ai_arch_fetch_all_ones_fault_then_canonical_executes_corner);
// TEST_REGISTER(ai_arch_fetch_all_ones_fault_then_canonical_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_fetch_all_ones_repeated_faults_stable_corner);
// TEST_REGISTER(ai_arch_fetch_all_ones_repeated_faults_then_canonical_executes_backing_preserved_corner);
// TEST_REGISTER(ai_arch_fetch_all_ones_repeated_faults_then_canonical_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_fetch_noncanonical_page_fault_tval_raw);
// TEST_REGISTER(ai_arch_fetch_noncanonical_fault_then_canonical_executes_corner);
// TEST_REGISTER(ai_arch_fetch_noncanonical_fault_then_canonical_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_fetch_noncanonical_repeated_faults_stable_corner);
// TEST_REGISTER(ai_arch_fetch_noncanonical_repeated_faults_then_canonical_executes_backing_preserved_corner);
// TEST_REGISTER(ai_arch_fetch_noncanonical_repeated_faults_then_canonical_executes_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_fetch_page_fault_masks_illegal_instruction_decode_corner);
TEST_REGISTER(ai_arch_fetch_page_fault_blocks_embedded_store_side_effect_corner);
TEST_REGISTER(ai_arch_fetch_data_fault_metadata_isolation_corner);
TEST_REGISTER(ai_arch_fetch_pf_af_priority_cross_page_corner);
TEST_REGISTER(ai_arch_amo_reserved_encoding_illegal_no_memory_side_effects_corner);
TEST_REGISTER(ai_arch_load_leaf_overflow_access_fault_rd_suppressed_corner);
// TEST_REGISTER(ai_micro_hs_load_pte_invalid_page_fault);
// TEST_REGISTER(ai_micro_hs_store_pte_invalid_page_fault);
// AI case AP46K: VS/two-stage guest fault metadata refresh, including VFOF silent guest-fault window.
// TEST_REGISTER(ai_micro_vs_vfof_nonzero_guest_fault_then_guest_load_fault_tval2_refresh_manual_corner);
// TEST_REGISTER(ai_arch_hyper_vs_amo_guest_page_fault_tval2_refreshes_to_next_gpa_corner);
// TEST_REGISTER(ai_micro_hs_load_noncanonical_page_fault_tval_raw);
// TEST_REGISTER(ai_micro_hs_load_tlb_hit_data_visible);
// TEST_REGISTER(ai_micro_hs_load_tlb_miss_replay_data_visible);
// TEST_REGISTER(ai_micro_hs_load_tlb_miss_then_hit_stable_corner);
// TEST_REGISTER(ai_micro_hs_load_tlb_reinit_then_cold_miss_stable_corner);
// TEST_REGISTER(ai_micro_hs_load_repeated_tlb_miss_no_side_effects_corner);
// TEST_REGISTER(ai_micro_hs_load_tlb_access_fault);
// AP-91 Spike high-PPN model-alignment regressions; keep commented/manual until the fixed LinkNan Spike and RTL policy are selected as gates.
// TEST_REGISTER(ai_micro_hs_leaf_high_ppn_access_fault_ad_unchanged_corner);
// TEST_REGISTER(ai_micro_hs_store_leaf_high_ppn_access_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_amo_leaf_high_ppn_access_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_fetch_leaf_high_ppn_access_fault_then_repair_executes_corner);
// TEST_REGISTER(ai_micro_vs_gstage_leaf_high_ppn_access_fault_not_guest_page_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_data_merge);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_hit_high_fill_data_merge);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_fill_high_hit_data_merge);
// TEST_REGISTER(ai_micro_hs_load_cross_page_1b7b_data_merge_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_7b1b_data_merge_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_load_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_cacheable_data_merge);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_tlb_miss_replay_merge);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_tlb_miss_then_hit_stable_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_7b1b_data_merge_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_load_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_stable_corner);
// TEST_REGISTER(ai_micro_hs_vector_aligned_load_adjacent_to_vector_cross_16b_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_vector_aligned_store_adjacent_to_vector_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_page_fault);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_access_fault);
// Manual RTL-only: current Spike does not expose scalar same-page internal cross-16B second-split PMP faults.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_samepage_second_split_pmp_access_fault_manual_rtl_only_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_page_fault_then_valid_cross_16b_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_page_fault_then_valid_cross_16b_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_access_fault_then_valid_cross_16b_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_access_fault_then_valid_cross_16b_load_adjacent_preserved_corner);
// These HS cross-16B trigger probes now pass on Spike after enabling mcontrol6
// for S/U contexts in addition to M-mode.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_trigger_breakpoint);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_cross_16b_samepage_second_element_address_trigger_vstart1_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_us_vector_trigger_fullva_matrix_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_masked_active_trigger_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_masked_off_trigger_ignored_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_vstart_restart_trigger_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_non_us_strided_trigger_gate_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_dual_trigger_priority_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_trigger_replay_rewalk_clear_stale_mask_manual_corner);
// TEST_REGISTER(ai_micro_ap41_ap81_store_trigger_replay_rewalk_clear_stale_mask_manual_corner);
// AP-49/AP-88 vector trigger/exception priority probes are kept manual because
// replay and ExceptionBuffer timing closure requires LinkNan/Nanhu evidence.
// TEST_REGISTER(ai_micro_ap49_vector_load_fault_vs_trigger_priority_manual_corner);
// TEST_REGISTER(ai_micro_ap49_vector_load_same_element_trigger_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap64_non_us_strided_load_same_element_trigger_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap49_vector_store_fault_vs_trigger_priority_manual_corner);
// TEST_REGISTER(ai_micro_ap49_vector_store_trigger_cutoff_manual_corner);
// TEST_REGISTER(ai_micro_ap49_ap88_vector_store_same_element_trigger_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap88_pure_cold_tlb_vector_store_no_bogus_exception_manual_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_priority_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_priority_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_access_fault);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_page_fault);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_dual_address_trigger_page_fault_priority_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_dual_address_trigger_access_fault_priority_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_page_fault_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_page_fault_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_priority_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_priority_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_access_fault_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_access_fault_then_valid_load_adjacent_preserved_corner);
// AP-35/AP-98 Spike PMP split-model regressions; keep commented/manual until local Spike revision and RTL policy are selected as gates.
// TEST_REGISTER(ai_micro_hs_load_cross_page_two_pmp_entries_allow_no_false_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_two_pmp_entries_allow_updates_both_entries_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_unitstride_cross_page_two_pmp_entries_allow_no_false_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_unitstride_cross_page_two_pmp_entries_allow_updates_split_element_corner);
// TEST_REGISTER(ai_micro_hs_vector_strided_load_multi_pmp_entries_allow_no_false_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_vector_strided_store_multi_pmp_entries_allow_updates_all_lanes_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_start_pmp_deny_tail_allow_tval_original_corner);
// TEST_REGISTER(ai_micro_hs_cross_page_overlap_pmp_start_priority_deny_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_pmp_reconfig_allow_deny_allow_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_pmp_reconfig_allow_deny_allow_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_tlb_hit_data_visible);
// TEST_REGISTER(ai_micro_hs_store_tlb_miss_replay_data_visible);
// TEST_REGISTER(ai_micro_hs_store_tlb_miss_then_hit_stable_corner);
// TEST_REGISTER(ai_micro_hs_store_tlb_miss_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_tlb_miss_then_reinit_load_latest_corner);
// ai_test_cases/memblock/translation/ai_micro_memblock_store_tlb_cases.c
TEST_REGISTER(ai_micro_hs_store_pf_then_same_va_remap_store_no_stale_tlb_paddr_corner);
TEST_REGISTER(ai_micro_hs_wrong_path_store_redirect_then_same_va_remap_no_ghost_tlb_request_corner);
// Indirect overlap coverage accepted by G1:
// same-VA and same-PA-alias store->load sequences both retire with the younger
// load observing the latest older store payload.
// TEST_REGISTER(ai_micro_hs_store_then_load_same_vaddr_latest);
// TEST_REGISTER(ai_micro_hs_store_alias_then_load_latest);
// TEST_REGISTER(ai_micro_hs_store_then_load_same_vaddr_partial_merge_latest);
// TEST_REGISTER(ai_micro_hs_same_vaddr_consecutive_partial_stores_merge_corner);
// TEST_REGISTER(ai_micro_hs_same_vaddr_partial_store_merge_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_same_vaddr_full_then_partial_merge_latest_corner);
// TEST_REGISTER(ai_micro_hs_same_vaddr_partial_then_full_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_partial_merge_latest);
// TEST_REGISTER(ai_micro_hs_store_alias_partial_merge_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_full_then_partial_merge_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_partial_then_full_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_hs_store_triple_alias_then_load_latest);
// TEST_REGISTER(ai_micro_hs_store_triple_alias_partial_merge_latest_corner);
// TEST_REGISTER(ai_micro_hs_triple_alias_multi_partial_store_merge_corner);
// TEST_REGISTER(ai_micro_hs_triple_alias_multi_partial_store_merge_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_triple_alias_full_then_partial_merge_latest_corner);
// TEST_REGISTER(ai_micro_hs_triple_alias_partial_then_full_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_hs_load_alias_pair_cold_consistent);
// TEST_REGISTER(ai_micro_hs_ld_st_ld_ld_distinct_vaddr_store_visible);
// TEST_REGISTER(ai_micro_hs_triple_alias_consecutive_stores_last_writer_wins_corner);
// Additional overlap corners:
// triple-alias partial-store merge and consecutive multi-alias last-writer-wins behavior.
// TEST_REGISTER(ai_micro_hs_store_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_store_noncanonical_page_fault_tval_raw);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_page_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_access_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_data_split);
// TEST_REGISTER(ai_micro_hs_store_cross_page_1b7b_data_split_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_7b1b_data_split_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_hit_high_fill_data_split);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_fill_high_hit_data_split);
// TEST_REGISTER(ai_micro_hs_store_cross_page_head_split_upper_half_paddr_and_sqdeq_correct_corner);
// TEST_REGISTER(ai_micro_hs_store_misalign_direct_wb_and_mabuf_wb_same_cycle_no_drop_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_cacheable_data_split);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_tlb_miss_replay_split);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_tlb_miss_then_hit_stable_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_7b1b_data_split_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_page_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_access_fault);
// Manual RTL-only: current Spike does not expose scalar same-page internal cross-16B second-split PMP faults.
// TEST_REGISTER(ai_micro_hs_store_cross_16b_samepage_second_split_pmp_access_fault_no_side_effect_manual_rtl_only_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_page_fault_then_valid_cross_16b_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_page_fault_then_valid_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_access_fault_then_valid_cross_16b_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_access_fault_then_valid_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_pmp_allow_data_visible);
// TEST_REGISTER(ai_micro_hs_store_pmp_write_access_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_trigger_breakpoint);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_cross_16b_samepage_second_element_address_trigger_first_element_only_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_access_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_page_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_dual_address_trigger_page_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_dual_address_trigger_access_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_exec_2nd_addr_slot_page_fault_repeated_bkp_no_side_effect_adjacent_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_exec_2nd_addr_slot_access_fault_repeated_bkp_no_side_effect_adjacent_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner);
// Manual/policy-sensitive: Spike exposes low-half bytes before high-half store page/access fault; keep out of default difftest until RTL policy is confirmed.
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_access_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_page_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_page_fault_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_page_fault_then_valid_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_access_fault_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_access_fault_then_valid_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_a_fence_store_b_load_b_latest);
// TEST_REGISTER(ai_micro_hs_store_a_fence_store_b_load_a_b_consistent_corner);
// TEST_REGISTER(ai_micro_hs_store_fence_store_same_addr_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_hs_store_fence_load_reads_latest);
// TEST_REGISTER(ai_micro_hs_store_fence_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_store_partial_fence_load_merged_latest_corner);
// TEST_REGISTER(ai_micro_hs_store_partial_fence_store_same_addr_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_hs_store_fence_partial_store_same_addr_merged_latest_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_fence_load_alias_latest_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_partial_fence_load_alias_merged_latest_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_partial_fence_partial_store_alias_merged_latest_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_fence_partial_store_alias_merged_latest_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_fence_store_alias_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_hs_store_alias_double_fence_loads_stable_corner);
// TEST_REGISTER(ai_micro_hs_back_to_back_fence_no_exception);
// TEST_REGISTER(ai_micro_hs_fence_variant_pred_succ_matrix_no_exception_corner);
// TEST_REGISTER(ai_micro_hs_fence_pred_succ_load_store_load_load_matrix_corner);
// TEST_REGISTER(ai_micro_hs_fence_pred_succ_full_rw_matrix_corner);
// TEST_REGISTER(ai_micro_hs_fence_single_hart_litmus_degenerate_matrix_corner);
// TEST_REGISTER(ai_arch_fence_amo_aqrl_pred_succ_overlay_corner);
// TEST_REGISTER(ai_arch_fence_lrsc_aqrl_pred_succ_overlay_corner);
// TEST_REGISTER(ai_micro_hs_fence_atomic_aqrl_single_hart_matrix_corner);
TEST_REGISTER(ai_arch_fence_fault_side_effect_boundary_corner);
TEST_REGISTER(ai_arch_atomic_aqrl_fault_side_effect_boundary_corner);
// TEST_REGISTER(ai_micro_hs_cbom_aligned_no_exception);
// TEST_REGISTER(ai_micro_hs_cbom_storequeue_head_data_visible);
// TEST_REGISTER(ai_micro_hs_cbom_storequeue_head_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_page_fault);
// TEST_REGISTER(ai_micro_hs_cbom_page_fault_then_valid_probe_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_repeated_page_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbom_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_cbom_tlb_access_fault_then_valid_probe_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_repeated_tlb_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_data_zeroed);
// TEST_REGISTER(ai_micro_hs_cbozero_page_fault);
// TEST_REGISTER(ai_micro_hs_cbozero_page_fault_then_valid_zero_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_repeated_page_fault_stable_corner);
// AP-131 Spike CBO.ZERO high-PPN tval alignment probes; local Spike/model metadata policy, not default gate.
// reason_code: D-MANUAL-SPIKE-GAP
// TEST_REGISTER(ai_micro_hs_cbozero_tlb_access_fault);
// TEST_REGISTER(ai_micro_vs_gstage_cbozero_high_ppn_tval_rs1_manual_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_tlb_access_fault_then_valid_zero_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_repeated_tlb_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbom_pmp_read_access_fault);
// TEST_REGISTER(ai_micro_hs_cbom_pmp_read_access_fault_then_valid_probe_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_repeated_pmp_read_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbom_mmio_access_fault_then_valid_probe_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_repeated_mmio_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_pmp_write_access_fault);
// TEST_REGISTER(ai_micro_hs_cbozero_pmp_write_access_fault_then_restore_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_repeated_pmp_write_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_mmio_access_fault);
// TEST_REGISTER(ai_micro_hs_cbozero_mmio_access_fault_then_valid_zero_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_repeated_mmio_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_storequeue_head_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cmo_last_instruction_page_fault_recovers);
// TEST_REGISTER(ai_micro_hs_cmo_last_instruction_page_fault_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cmo_first_instruction_page_fault_recovers);
// TEST_REGISTER(ai_micro_hs_cmo_first_instruction_page_fault_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cmo_last_instruction_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_cmo_last_instruction_mmio_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_cmo_last_instruction_access_fault_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cmo_last_instruction_mmio_access_fault_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cmo_first_instruction_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_cmo_first_instruction_mmio_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_cmo_first_instruction_access_fault_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cmo_first_instruction_mmio_access_fault_adjacent_block_preserved_corner);
// Diagnostic precise-exception ordering probes:
// capture the older HS load/store fault first, then explicitly re-arm exception capture
// in-sequence to observe the younger different fault without claiming internal ROB visibility.
// TEST_REGISTER(ai_micro_hs_load_exception_pair_oldest_page_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_load_exception_pair_oldest_access_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_store_exception_pair_oldest_page_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_store_exception_pair_oldest_access_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_load_store_exception_pair_oldest_load_page_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_load_store_exception_pair_oldest_load_access_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_store_load_exception_pair_oldest_store_page_fault_first_corner);
// TEST_REGISTER(ai_micro_hs_store_load_exception_pair_oldest_store_access_fault_first_corner);
// Spike-backed CMO sub-block-offset corner:
// ordinary cache-block-internal offsets do not fault for cbo.clean/flush/inval/zero.
// Tail-byte offsets also match this rule, and cbo.zero still does not spill into adjacent blocks.
// TEST_REGISTER(ai_micro_hs_cmo_subblock_offset_no_exception);
// TEST_REGISTER(ai_micro_hs_cbom_tail_subblock_offset_no_exception_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_tail_subblock_offset_adjacent_block_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_midblock_tail_offset_both_neighbors_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_midblock_tail_offset_both_neighbors_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbom_repeated_same_block_idempotent_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_repeated_same_block_idempotent_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_then_cbom_same_block_zero_persists_corner);
// Legacy raw CMO address probe is kept only for ad-hoc logging.
// TEST_REGISTER(ai_micro_hs_cmo_misaligned_probe);
// This HS CBOM MMIO case depends on PMA CSR behavior.
// Keep it compiled and tracked in test_point, but out of the default Spike regression.
// TEST_REGISTER(ai_micro_hs_cbom_mmio_access_fault);
// TEST_REGISTER(ai_micro_sfence_vma_privilege_interaction_illegal);
TEST_REGISTER(ai_micro_memblock_basic_privilege_csr_and_return_gate_corner);
TEST_REGISTER(ai_micro_memblock_mret_hs_data_access_rechecks_context_corner);
TEST_REGISTER(ai_micro_memblock_xret_status_restore_matrix_corner);
TEST_REGISTER(ai_micro_memblock_mprv_effective_privilege_data_access_matrix_corner);
TEST_REGISTER(ai_micro_memblock_sum_mxr_pteu_prefetch_silent_no_side_effect_corner);
TEST_REGISTER(ai_micro_memblock_sfence_tvm_illegal_preserves_translation_state_corner);
TEST_REGISTER(ai_micro_memblock_sfence_legal_retire_preserves_translation_state_corner);
TEST_REGISTER(ai_micro_memblock_u_mode_sfence_satp_illegal_preserves_satp_corner);
TEST_REGISTER(ai_micro_memblock_m_mode_tvm_satp_access_legal_corner);
TEST_REGISTER(ai_micro_memblock_satp_mode_warl_rejects_reserved_modes_corner);
TEST_REGISTER(ai_micro_memblock_sfence_repeated_flush_idempotent_corner);
TEST_REGISTER(ai_micro_memblock_sfence_unknown_asid_no_match_preserves_translation_corner);
TEST_REGISTER(ai_micro_memblock_superpage_leaf_alignment_matrix_corner);
TEST_REGISTER(ai_micro_memblock_satp_root_switch_refills_new_ptw_path_corner);
TEST_REGISTER(ai_micro_memblock_pte_structure_remap_sfence_rejudge_corner);
TEST_REGISTER(ai_micro_memblock_s_mode_4k_hit_miss_pte_consistent_corner);
TEST_REGISTER(ai_micro_memblock_legal_pte_permission_hit_matrix_corner);
TEST_REGISTER(ai_micro_memblock_u_mode_4k_hit_miss_pte_consistent_corner);
TEST_REGISTER(ai_micro_memblock_pte_ad_one_load_store_preserves_bits_corner);
TEST_REGISTER(ai_micro_memblock_asid_switch_refills_non_global_translation_corner);
// This global-PTE probe relies on a cached TLB entry crossing ASID changes without
// an intervening sfence.vma. Official Spike walks the current PTE and does not model
// this RTL TLB-residency behavior, so keep it compiled but out of default gate.
// reason_code: D-MANUAL-SPIKE-GAP
// TEST_REGISTER(ai_micro_memblock_global_pte_cross_asid_cached_hit_corner);
// This PTE.U update boundary probe checks the old TLB entry before explicit
// SFENCE.VMA and the refreshed entry after SFENCE.VMA. Official Spike does not
// model the stale TLB residency needed for the pre-SFENCE old-entry boundary.
// reason_code: D-MANUAL-SPIKE-GAP
// TEST_REGISTER(ai_micro_memblock_pte_u_update_requires_sfence_old_entry_boundary_corner);
TEST_REGISTER(ai_micro_memblock_pte_perm_matrix_faults_corner);
TEST_REGISTER(ai_micro_memblock_pte_invalid_leaf_reserved_faults_corner);
TEST_REGISTER(ai_micro_memblock_pte_u_sum_mode_mismatch_faults_corner);
TEST_REGISTER(ai_micro_memblock_pte_fault_repair_recovers_translation_corner);
TEST_REGISTER(ai_arch_pte_perm_valid_sfence_update_matrix_corner);
TEST_REGISTER(ai_arch_pte_ppn_leaflevel_sfence_rewalk_corner);
TEST_REGISTER(ai_arch_pte_ad_sfence_update_rejudge_corner);
TEST_REGISTER(ai_arch_pte_ppn_remap_fault_metadata_rebind_corner);
TEST_REGISTER(ai_arch_sfence_vma_data_fence_separation_corner);
TEST_REGISTER(ai_arch_sfence_vma_faulting_pte_store_keeps_old_translation_corner);
TEST_REGISTER(ai_arch_sfence_vma_same_ppn_alias_asid_refresh_corner);
TEST_REGISTER(ai_micro_hs_memblock_ptw_pmp_access_fault_repair_rewalks_translation_corner);
TEST_REGISTER(ai_micro_hs_memblock_amo_lrsc_ptw_pmp_access_fault_repair_rewalks_corner);
TEST_REGISTER(ai_micro_hs_memblock_fetch_ptw_pmp_access_fault_repair_rewalks_corner);
TEST_REGISTER(ai_micro_hs_memblock_leaf_g_rsw_pmp_access_faults_preserve_encoding_corner);
TEST_REGISTER(ai_micro_hs_memblock_pte_fault_repair_load_store_bidirectional_refreshes_exception_metadata_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_scalar_store_then_same_addr_load_sees_old_after_exception_return_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_narrow_store_masks_do_not_forward_after_exception_return_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_load_widths_do_not_clobber_rd_before_repair_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_fp_loads_do_not_clobber_frd_or_fflags_before_repair_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_fp_stores_do_not_update_memory_or_forward_after_exception_return_corner);
TEST_REGISTER(ai_micro_hs_memblock_cross_page_fp_load_store_second_half_fault_keeps_fp_and_precise_partial_memory_corner);
TEST_REGISTER(ai_micro_hs_memblock_fp_memory_clean_fs_faults_no_dirty_until_commit_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_amo_wd_do_not_clobber_rd_or_memory_before_repair_corner);
TEST_REGISTER(ai_micro_hs_memblock_faulting_lr_sc_do_not_clobber_rd_or_create_side_effects_corner);
TEST_REGISTER(ai_micro_hs_memblock_ad_faults_do_not_clobber_rd_pte_or_memory_before_repair_corner);
TEST_REGISTER(ai_micro_hs_memblock_bare_to_sv48_highhalf_perm_fault_tval_current_context_corner);
TEST_REGISTER(ai_micro_hs_memblock_bare_to_sv48_highhalf_cross_page_fault_tval_current_context_corner);
TEST_REGISTER(ai_micro_memblock_cross_page_low_half_pf_no_side_effect_corner);
TEST_REGISTER(ai_micro_hs_memblock_cross_page_privilege_load_fault_half_select_no_side_effect_corner);
// Cross-page split-store high-half fault no-partial-write is a project/RTL
// precise-side-effect probe. Official Spike does not enforce/model the same
// no-partial split-store guarantee; the existing high-half no-side-effect
// baseline also fails under Spike.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_memblock_cross_page_store_privilege_xonly_high_half_no_partial_write_matrix_corner);
TEST_REGISTER(ai_micro_memblock_svinval_supported_privilege_and_satp_stability_corner);
// Generated data-load sfence.vma probes target RTL / design-expected TLB+cache behavior.
// Spike currently does not model the needed TLB/cache data path here, so keep them compiled
// but out of the default Spike regression.
// TEST_REGISTER(ai_micro_hs_sfence_vma_store_preserves_data_before_flush);
// TEST_REGISTER(ai_micro_hs_sfence_vma_all_flushes_all_non_global);
// TEST_REGISTER(ai_micro_hs_sfence_vma_all_repeated_flush_stable_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_asid_flushes_matching_non_global);
// TEST_REGISTER(ai_micro_hs_sfence_vma_asid_repeated_flush_non_global_but_preserves_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_asid_other_asid_preserves_current_non_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_asid_flushes_non_global_but_preserves_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_rs2_scope_mixed_global_non_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_asid_preserves_global_translation);
// TEST_REGISTER(ai_micro_hs_sfence_vma_vaddr_all_asids_flushes_exact_non_global);
// TEST_REGISTER(ai_micro_hs_sfence_vma_vaddr_all_asids_repeated_flush_stable_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_other_vaddr_all_asids_preserves_current_non_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_vaddr_asid_flushes_exact_non_global);
// TEST_REGISTER(ai_micro_hs_sfence_vma_other_vaddr_asid_preserves_current_non_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_vaddr_other_asid_preserves_current_non_global_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_vaddr_asid_repeated_flush_stable_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_vaddr_asid_preserves_global_translation);
// TEST_REGISTER(ai_micro_hs_sfence_vma_store_preserves_data_across_repeated_flush_corner);
// TEST_REGISTER(ai_micro_hs_sfence_vma_all_store_preserves_data_across_repeated_flush_corner);
// AI case batch AP-74/AP-76: L2TlbPrefetch duplicate filtering and Decoupled payload stability manual probes.
// These depend on Nanhu L2TlbPrefetch input_valid/v/next_req/old_reqs/io.out.fire/ChiselDB observability; official Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap74_l2tlb_prefetch_pending_duplicate_manual_corner);
// TEST_REGISTER(ai_micro_ap74_l2tlb_prefetch_fire_handoff_manual_corner);
// TEST_REGISTER(ai_micro_ap74_l2tlb_prefetch_history_flush_manual_corner);
// TEST_REGISTER(ai_micro_ap76_l2tlb_prefetch_valid_hold_payload_manual_corner);
// TEST_REGISTER(ai_micro_ap76_l2tlb_prefetch_context_flush_manual_corner);
// These single-page PBMT/PBMTE-dependent probes are generated and tracked in test_point, but remain
// unregistered in the default Spike batch because PBMT/PMA CSR behavior is not a reliable Spike gate today.
// 2026-04-26 recheck: ai_micro_hs_load_pbmt_uncache_data_visible reaches an untested PMA/PBMT CSR path
// after the NC half passes, so keep these as manual PBMT/PMA environment checks.
// TEST_REGISTER(ai_micro_hs_load_pbmt_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_load_pbmt_io_access_fault);
// TEST_REGISTER(ai_micro_hs_store_pbmt_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_store_pbmt_io_access_fault);
// TEST_REGISTER(ai_micro_hs_load_pbmt_uncache_data_visible);
// TEST_REGISTER(ai_micro_hs_load_pbmt_uncache_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_load_pbmt_uncache_page_fault);
// TEST_REGISTER(ai_micro_hs_load_pbmt_uncache_pmp_read_access_fault);
// TEST_REGISTER(ai_micro_hs_load_uncache_bus_error_access_fault);
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_data_visible);
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_page_fault);
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_pmp_write_access_fault);
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_bus_error_access_fault);
// These PMA-dependent boundary cases are generated and tracked in test_point, but remain unregistered
// in the default Spike batch because PMA CSR behavior is not a reliable Spike gate today.
// TEST_REGISTER(ai_arch_io_boundary_atomic_faults);
// TEST_REGISTER(ai_arch_mem_boundary_data_correct);
// TEST_REGISTER(ai_arch_io_boundary_scalar_data_correct);
// TEST_REGISTER(ai_arch_pma_mem_io_cross_boundary_scalar_fault_no_partial_corner);
// mcontrol6 trigger coverage now validates both execute-PC and data-address
// matching, with breakpoint taking priority over AF/PF when the trigger hits.
// TEST_REGISTER(ai_arch_load_execute_trigger_breakpoint);
// TEST_REGISTER(ai_arch_load_execute_trigger_repeated_breakpoint_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_breakpoint);
// TEST_REGISTER(ai_arch_store_execute_trigger_repeated_breakpoint_no_side_effect_corner);
// TEST_REGISTER(ai_arch_amo_execute_trigger_breakpoint);
// TEST_REGISTER(ai_arch_amo_execute_trigger_repeated_breakpoint_no_side_effect_corner);
// TEST_REGISTER(ai_arch_load_address_trigger_breakpoint);
// TEST_REGISTER(ai_arch_load_address_trigger_repeated_breakpoint_corner);
// TEST_REGISTER(ai_arch_store_address_trigger_breakpoint);
// TEST_REGISTER(ai_arch_store_address_trigger_repeated_breakpoint_no_side_effect_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_repeated_breakpoint_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_repeated_breakpoint_no_side_effect_corner);
TEST_REGISTER(ai_arch_store_mcontrol6_double_execute_chain_breakpoint_corner);
TEST_REGISTER(ai_arch_trigger_mcontrol6_chain_mixed_access_bits_no_cross_fire_corner);
TEST_REGISTER(ai_arch_trigger_mcontrol6_chain_match_modes_store_addr_corner);
TEST_REGISTER(ai_arch_trigger_csr_privilege_warl_matrix_corner);
TEST_REGISTER(ai_arch_trigger_mcontrol6_chain_privilege_bits_no_cross_privilege_fire_corner);
TEST_REGISTER(ai_arch_trigger_hs_store_breakpoint_mstatus_mie0_still_sync_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_page_fault_priority_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_access_fault_priority);
// TEST_REGISTER(ai_arch_load_execute_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_access_fault_priority);
// TEST_REGISTER(ai_arch_store_execute_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_trigger_access_fault_priority);
// TEST_REGISTER(ai_arch_load_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_widths_trigger_access_fault_priority_manual_corner);
// TEST_REGISTER(ai_arch_store_trigger_access_fault_priority);
// TEST_REGISTER(ai_arch_store_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_page_fault_priority);
// TEST_REGISTER(ai_arch_load_execute_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_page_fault_priority);
// TEST_REGISTER(ai_arch_store_execute_trigger_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_load_trigger_page_fault_priority);
// TEST_REGISTER(ai_arch_store_trigger_page_fault_priority);
// TEST_REGISTER(ai_arch_load_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner);
// New PBMT=NC misalign + trigger batch extends the existing AF/PF trigger matrix.
// Keep them generated and tracked, but out of the default Spike batch until breakpoint-vs-misalign
// priority is explicitly validated on the target Spike/RTL pair.
// TEST_REGISTER(ai_arch_load_execute_trigger_misaligned_priority);
// TEST_REGISTER(ai_arch_load_execute_trigger_misaligned_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_trigger_misaligned_priority);
// TEST_REGISTER(ai_arch_load_trigger_misaligned_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_misaligned_priority);
// TEST_REGISTER(ai_arch_store_execute_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_trigger_misaligned_priority);
// TEST_REGISTER(ai_arch_store_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_misaligned_priority_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_store_adjacent_preserved_corner);
// Cross-page PBMT=NC low-half misalign + high-half PF combined with trigger:
// keep these probes generated and tracked, but out of the default Spike batch until the intended
// breakpoint-vs-low-half-misalign-vs-high-half-PF ordering is validated on the target pair.
// TEST_REGISTER(ai_arch_load_trigger_misaligned_page_fault_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_misaligned_page_fault_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_page_fault_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fixed_cross_page_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fixed_cross_page_load_adjacent_preserved_corner);
// Cross-page low-half-cacheable + high-half-PF combined with trigger:
// keep these probes generated and tracked, but out of the default Spike batch until the intended
// breakpoint-vs-second-half-PF ordering is validated on the target pair.
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner);
// These Store trigger+misalign+PF corners keep breakpoint priority while armed, then fall
// back to the project PBMT-NC low-half misaligned-store policy with no store side effect.
// TEST_REGISTER(ai_arch_store_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_repaired_cross_page_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_repaired_cross_page_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner);
// PBMT=NC-over-IO scalar misalign combined with trigger:
// keep these probes generated and tracked, but out of the default Spike batch until the intended
// breakpoint-vs-LAM/SAM ordering is validated on the target pair.
// TEST_REGISTER(ai_arch_load_trigger_pbmt_nc_over_io_misaligned_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_pbmt_nc_over_io_misaligned_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_priority_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_amo_address_trigger_breakpoint);
// TEST_REGISTER(ai_arch_amo_address_trigger_repeated_breakpoint_no_side_effect_corner);
// TEST_REGISTER(ai_arch_amo_execute_and_address_trigger_both_breakpoint_corner);
// TEST_REGISTER(ai_arch_amo_execute_and_address_trigger_access_fault_priority_corner);
// TEST_REGISTER(ai_arch_amo_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_amo_execute_and_address_trigger_page_fault_spike_corner);
// TEST_REGISTER(ai_arch_amo_execute_and_address_trigger_misaligned_access_fault_spike_corner);
// AMO trigger corners on Spike:
// execute-PC trigger still wins over AF/PF, and address trigger wins over direct-M AF.
// This batch now also probes AMO misaligned-driven AF intersections.
// But for "AMO address trigger + HS page fault" and "AMO address trigger + misaligned HS AMO",
// Spike currently reports the underlying fault directly, so keep those as Spike-corner
// recorders rather than generic breakpoint-priority proofs.
// TEST_REGISTER(ai_arch_amo_execute_trigger_access_fault_priority);
// TEST_REGISTER(ai_arch_amo_trigger_access_fault_priority);
// TEST_REGISTER(ai_arch_amo_trigger_access_fault_repeated_breakpoint_then_fault_corner);
// TEST_REGISTER(ai_arch_amo_execute_trigger_misaligned_access_fault_priority);
// TEST_REGISTER(ai_arch_amo_trigger_misaligned_access_fault_spike_corner);
// TEST_REGISTER(ai_arch_amo_trigger_misaligned_repeated_breakpoints_then_aligned_recovers_spike_corner);
// TEST_REGISTER(ai_arch_amo_execute_trigger_page_fault_priority);
// TEST_REGISTER(ai_arch_amo_trigger_page_fault_spike_corner);
// TEST_REGISTER(ai_arch_amo_trigger_page_fault_repeated_breakpoints_then_valid_access_recovers_spike_corner);
// TEST_REGISTER(ai_arch_load_execute_trigger_page_fault_rd_suppressed_corner);
// TEST_REGISTER(ai_arch_amo_address_trigger_rd_suppressed_no_rmw_then_clear_swaps_corner);
// TEST_REGISTER(ai_arch_cbozero_execute_trigger_breakpoint_blocks_zero_then_clear_trigger_zeroes_block_corner);
// These cross-page PBMT/PBMTE-dependent cases are generated and tracked in test_point, but remain
// unregistered in the default Spike batch because PBMT/PMA CSR behavior is not a reliable Spike gate today.
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_load_cross_page_low_half_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_store_cross_page_low_half_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_io_access_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_io_access_fault);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_access_fault);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_page_fault);
// These PMA-dependent cross-16B cases are generated and tracked in test_point, but remain unregistered
// in the default Spike batch because PMA CSR behavior is not a reliable Spike gate today.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_io_region_access_fault);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_pbmt_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_pbmt_io_access_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_io_region_access_fault);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_pbmt_nc_misaligned);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_pbmt_io_access_fault);
// These two baseline cases encode the target RTL expectation for a scalar PBMT=NC non-Device path.
// On current Spike, the single-page baseline does not reproduce load/store misaligned, so keep them
// compiled and tracked, but do not treat Spike as the gate for these two cases.
// TEST_REGISTER(ai_arch_load_pbmt_nc_misaligned_baseline);
// TEST_REGISTER(ai_arch_store_pbmt_nc_misaligned_baseline);
// The misalign+PF cases below are true cross-page constructions:
// lower half already lands in a PBMT=NC page, while the upper half faults in translation.
// For the project RTL policy, split accesses are checked half by half:
// if the low half already faults, stop there and report the low-half exception with low-half tval.
// The extra store corner remains only as a Spike-side observation and is not a project golden rule.
// TEST_REGISTER(ai_arch_load_misalign_pf_priority);
// TEST_REGISTER(ai_arch_load_misalign_pf_repeated_then_fixed_cross_page_load_recovers_corner);
// TEST_REGISTER(ai_arch_load_misalign_pf_repeated_then_fixed_cross_page_load_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_load_cross_page_high_half_page_fault_after_low_half_cacheable_corner);
TEST_REGISTER(ai_arch_load_cross_page_high_half_page_fault_repeated_then_fix_high_half_load_recovers_corner);
TEST_REGISTER(ai_arch_load_cross_page_high_half_page_fault_repeated_then_fix_high_half_load_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_load_cross_page_high_half_page_fault_repeated_then_other_aligned_load_recovers_corner);
TEST_REGISTER(ai_arch_load_cross_page_high_half_page_fault_repeated_then_other_aligned_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_misalign_pf_priority);
// TEST_REGISTER(ai_arch_store_misalign_pf_repeated_then_fixed_cross_page_store_recovers_corner);
// TEST_REGISTER(ai_arch_store_misalign_pf_repeated_then_fixed_cross_page_store_adjacent_preserved_corner);
// Manual/nongate strict no-partial-write probe: official Spike can expose the legal low-half fragment
// before the high-half store page fault, so this is not a default architecture gate.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_store_cross_page_high_half_page_fault_after_low_half_cacheable_corner);
TEST_REGISTER(ai_arch_store_cross_page_high_half_page_fault_repeated_then_fix_high_half_store_recovers_corner);
TEST_REGISTER(ai_arch_store_cross_page_high_half_page_fault_repeated_then_fix_high_half_store_adjacent_preserved_corner);
TEST_REGISTER(ai_arch_store_cross_page_high_half_page_fault_repeated_then_other_aligned_store_recovers_corner);
TEST_REGISTER(ai_arch_store_cross_page_high_half_page_fault_repeated_then_other_aligned_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_store_pbmt_nc_misaligned_store_fault_no_side_effect_corner);
// For PF+AF during address translation, follow the privileged-spec table: report the first encountered
// translation exception. The current synthetic setup resolves to PF first on Spike.
TEST_REGISTER(ai_arch_load_pf_af_priority);
TEST_REGISTER(ai_arch_store_pf_af_priority);
TEST_REGISTER(ai_arch_amo_pf_af_priority_no_rmw_side_effect_corner);
// These two PMA-dependent cases now encode the resolved D2 split:
// PBMT=NC-over-IO misaligned scalar access -> LAM/SAM.
// Keep them generated and tracked in test_point, but out of the default Spike flow.
// TEST_REGISTER(ai_arch_load_pbmt_nc_over_io_misaligned_priority);
// TEST_REGISTER(ai_arch_store_pbmt_nc_over_io_misaligned_priority);

// AI case batch 6: NC and device store-route distinction
// These store-route and MMIO-isolation cases depend on PBMT/PMA CSR setup and cache-path behavior.
// Keep them tracked in test_point, but out of the default Spike batch.
// Manual RTL-only: Spike has no matching D-cache/NC-uncache stale-line model, so cacheable-alias
// freshness after NC stores must be checked with standalone RTL selfcheck instead of Spike gating.
// TEST_REGISTER(ai_arch_nc_store_uncache_path);
// TEST_REGISTER(ai_arch_pma_device_store_mmio_path);
// TEST_REGISTER(ai_arch_device_store_mmio_path);
// TEST_REGISTER(ai_arch_pbmt_io_device_fence_quadrant_order_corner);
// TEST_REGISTER(ai_arch_device_ordinary_memory_cross_fence_order_corner);
// TEST_REGISTER(ai_arch_pbmt_io_pma_io_same_pa_order_consistency_corner);
// TEST_REGISTER(ai_arch_pbmt_io_load_store_load_order);
// TEST_REGISTER(ai_arch_mmio_store_program_order_indirect);
// TEST_REGISTER(ai_arch_pbmt_io_responder_lane_width_readback_corner);
// TEST_REGISTER(ai_arch_nc_vs_pma_device_store_path_distinguishable);
// TEST_REGISTER(ai_arch_nc_device_store_path_distinguishable);
// TEST_REGISTER(ai_micro_hs_nc_then_mmio_store_both_visible);
// TEST_REGISTER(ai_micro_hs_nc_store_fence_inval_cache_load_fresh);
// These MMIO/PMA-dependent microarchitectural cases are generated and tracked in test_point, but remain
// unregistered in the default Spike batch because PMA CSR behavior is not a reliable Spike gate today.
// TEST_REGISTER(ai_micro_hs_load_mmio_data_visible);
// TEST_REGISTER(ai_micro_hs_load_mmio_misaligned_access_fault);
// TEST_REGISTER(ai_micro_hs_load_mmio_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_load_mmio_page_fault);
// TEST_REGISTER(ai_micro_hs_load_mmio_pmp_read_access_fault);
// TEST_REGISTER(ai_micro_hs_store_mmio_data_visible);
// TEST_REGISTER(ai_micro_hs_store_mmio_misaligned_access_fault);
// TEST_REGISTER(ai_micro_hs_store_mmio_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_store_mmio_page_fault);
// TEST_REGISTER(ai_micro_hs_store_mmio_pmp_write_access_fault);
// TEST_REGISTER(ai_micro_hs_store_mmio_bus_error_access_fault);
// These replay-head and storequeue-head uncache visibility probes depend on PBMT/PMA CSR setup.
// Keep them tracked in test_point, but out of the default Spike batch.
// TEST_REGISTER(ai_micro_hs_load_uncache_replay_head_data_visible);
// TEST_REGISTER(ai_micro_hs_store_mmio_storequeue_head_data_visible);
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_storequeue_head_data_visible);

// AI case batch 7: CBO permission and PMP semantics follow the official CMO spec.
// Some Spike versions still report cache-block base for cbo.zero fault tval, so keep this group
// out of the default Spike batch unless the golden is intentionally switched back to Spike-observed.
// TEST_REGISTER(ai_arch_cbo_clean_rx_allowed);
// TEST_REGISTER(ai_arch_cbo_flush_rx_allowed);
// TEST_REGISTER(ai_arch_cbo_inval_rx_allowed);
// TEST_REGISTER(ai_arch_cbo_success_no_gpr_or_trap_csr_update_corner);
// TEST_REGISTER(ai_arch_cbo_clean_no_r_page_fault);
// TEST_REGISTER(ai_arch_cbo_clean_no_a_page_fault);
// TEST_REGISTER(ai_arch_cbo_flush_no_r_page_fault);
// TEST_REGISTER(ai_arch_cbo_flush_no_a_page_fault);
// TEST_REGISTER(ai_arch_cbo_inval_no_r_page_fault);
// TEST_REGISTER(ai_arch_cbo_inval_no_a_page_fault);
// TEST_REGISTER(ai_arch_cbozero_no_w_page_fault);
// TEST_REGISTER(ai_arch_cbozero_no_a_page_fault);
// TEST_REGISTER(ai_arch_cbozero_no_d_page_fault);
// TEST_REGISTER(ai_arch_cbo_clean_pmp_read_access_fault);
// TEST_REGISTER(ai_arch_cbo_flush_pmp_read_access_fault);
// TEST_REGISTER(ai_arch_cbo_inval_pmp_read_access_fault);
// TEST_REGISTER(ai_arch_cbozero_pmp_write_access_fault);
// TEST_REGISTER(ai_arch_cbom_access_fault_preserves_custom_csr_state_corner);
// TEST_REGISTER(ai_arch_cbo_clean_tail_offset_no_r_page_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_clean_tail_offset_no_a_page_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_clean_tail_offset_pmp_read_access_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_flush_tail_offset_no_r_page_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_flush_tail_offset_no_a_page_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_flush_tail_offset_pmp_read_access_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_inval_tail_offset_no_r_page_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_inval_tail_offset_no_a_page_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_inval_tail_offset_pmp_read_access_fault_tval_raw_rs1_spike_corner);
// TEST_REGISTER(ai_arch_cbo_envcfg_cbo_field_warl_privilege_matrix_corner);
// TEST_REGISTER(ai_arch_cbo_disabled_illegal_priority_tval_instruction_no_address_side_effect_corner);
// TEST_REGISTER(ai_arch_cbo_envcfg_fiom_does_not_gate_cbo_permissions_corner);
// TEST_REGISTER(ai_arch_cbo_mprv_effective_privilege_permission_path_corner);
// TEST_REGISTER(ai_arch_cbo_envcfg_illegal_then_scalar_memory_independent_corner);
// TEST_REGISTER(ai_arch_cbo_fault_then_followup_success_metadata_isolated_corner);
// TEST_REGISTER(ai_arch_cbo_fault_repair_reexecute_generates_fresh_metadata_corner);
// TEST_REGISTER(ai_arch_cbo_pmp_access_fault_repair_reexecute_success_corner);
// TEST_REGISTER(ai_arch_cbo_envcfg_pbmte_rejudges_pbmt_cbo_attr_corner);
// TEST_REGISTER(ai_arch_prefetch_silent_then_cbo_load_store_trap_csr_independent_corner);
// TEST_REGISTER(ai_arch_cbo_complete_then_repeated_load_store_amo_no_residual_corner);
// Historical Spike-deviation probes for cbo.zero tail-offset fault tval:
// TEST_REGISTER(ai_arch_cbozero_tail_offset_no_w_page_fault_tval_block_base_spike_corner);
// TEST_REGISTER(ai_arch_cbozero_tail_offset_no_a_page_fault_tval_block_base_spike_corner);
// TEST_REGISTER(ai_arch_cbozero_tail_offset_no_d_page_fault_tval_block_base_spike_corner);
// TEST_REGISTER(ai_arch_cbozero_tail_offset_pmp_write_access_fault_tval_block_base_spike_corner);

// AI case batch 7a: Nanhu custom CSR programmability and architectural diagnostic baselines
// These cases validate CSR programmability/readback and architectural no-regression only.
// The stronger effect-level smblockctl/spfctl scenarios in test_point still need Xiangshan-specific
// replay/prefetch observability and remain tracked separately.
// Legacy note: keep the historical smblockctl/spfctl names here for traceability only.
// Do not keep them in TEST_REGISTER(...) form; compile_elf.py --include-commented treats
// commented TEST_REGISTER lines as selectable build targets and will try to link stale names.
// Historical/superseded names:
//   ai_arch_smblockctl_soft_prefetch_cache_error_toggle
//   ai_arch_smblockctl_misaligned_bits_readback_corner
//   ai_arch_smblockctl_soft_cache_toggle_preserves_misaligned_readback_bits_corner
//   ai_arch_smblockctl_misaligned_readback_bits_preserve_soft_cache_bits_corner
//   ai_arch_smblockctl_misaligned_enable_bits_toggle_corner
//   ai_arch_smblockctl_toggle_preserves_misaligned_bits_corner
//   ai_arch_smblockctl_misaligned_enable_preserves_soft_cache_bits_corner
//   ai_arch_smblockctl_soft_prefetch_disable_prefetch_silent_corner
//   ai_arch_smblockctl_misaligned_toggle_aligned_access_unaffected_corner
//   ai_arch_spfctl_disable_prefetch_paths
//   ai_arch_spfctl_threshold_stride_programmable_corner
//   ai_arch_spfctl_disable_then_threshold_stride_preserves_disabled_paths_corner
//   ai_arch_spfctl_disable_prefetch_paths_explicit_access_correct_corner
//   ai_arch_spfctl_threshold_stride_stream_correct_corner
//   ai_arch_smblockctl_replay_escape_toggle_corner
//   ai_arch_smblockctl_replay_escape_threshold_programmable_corner
//   ai_arch_smblockctl_replay_escape_disable_then_threshold_preserved_corner
//   ai_arch_spfctl_agt_pht_train_on_hit_cross_readback_corner
//   ai_arch_spfctl_agt_disable_then_pht_threshold_stride_preserved_corner
//   ai_arch_spfctl_pht_disable_then_agt_threshold_stride_preserved_corner
//   ai_arch_spfctl_enable_stride_toggle_readback_no_regression_corner
//   ai_arch_spfctl_agt_pht_disable_explicit_prefetch_silent_corner
//   ai_arch_spfctl_train_on_hit_toggle_hit_stream_correct_corner
//   ai_arch_smblockctl_replay_escape_enable_small_threshold_ldld_replay_progress_corner
//   ai_arch_smblockctl_replay_escape_disable_small_threshold_dense_replay_no_regression_corner
//   ai_arch_spfctl_agt_pht_disable_repeated_page_fault_prefetch_silent_corner
//   ai_arch_spfctl_train_on_hit_toggle_prefetch_fault_then_valid_access_recovers_corner
//   ai_arch_spfctl_train_on_hit_toggle_prefetch_fault_then_valid_access_adjacent_preserved_corner
//   ai_arch_spfctl_agt_pht_disable_prefetch_access_fault_then_valid_access_recovers_corner
//   ai_arch_spfctl_agt_pht_disable_prefetch_access_fault_then_valid_access_adjacent_preserved_corner
//   ai_arch_spfctl_cross_reprogram_twice_stream_stable_corner
//   ai_arch_spfctl_sms_agt_pht_disable_then_reenable_stream_observable_corner

// AI case batch 8: LR/SC progress and nested-reservation behavior
TEST_REGISTER(ai_arch_lr_sc_fault_suppresses_reservation_and_status_side_effects_corner);
// TEST_REGISTER(ai_arch_lr_sc_pbmt_attr_fault_suppresses_side_effects_corner);
TEST_REGISTER(ai_arch_lr_sc_success_status_guard_x0_consumption_corner);
TEST_REGISTER(ai_arch_lr_sc_mismatch_fail_no_write_matrix_corner);
TEST_REGISTER(ai_arch_lr_sc_repeated_fail_then_success);
TEST_REGISTER(ai_arch_lr_sc_repeated_doubleword_fail_then_success_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_inner_only_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_doubleword_inner_only_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_inner_only_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_inner_success_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_doubleword_inner_only_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_doubleword_inner_success_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_word_then_doubleword_inner_only_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_word_then_doubleword_inner_success_then_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_word_then_doubleword_inner_success_then_doubleword_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_byte_fails);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_doubleword_range_fails_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_all_doubleword_lane_mismatch_matrix_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_sfence_refill_mismatch_preserves_full_address_corner);
TEST_REGISTER(ai_micro_lr_sc_cold_lr_after_previous_reservation_no_stale_full_address_success_corner);
// Spike also matches the current >64-cycle LR/SC timeout expectation.
// TEST_REGISTER(ai_arch_lr_sc_timeout_over_64_cycles_fails);
// TEST_REGISTER(ai_arch_lr_sc_timeout_over_64_cycles_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_arch_lr_sc_timeout_then_refresh_succeeds);
// TEST_REGISTER(ai_arch_lr_sc_timeout_then_refresh_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_arch_lr_sc_timeout_repeated_fails_stable_corner);
// TEST_REGISTER(ai_arch_lr_sc_timeout_repeated_fails_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_fails_spike_corner);
// TEST_REGISTER(ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_adjacent_doubleword_preserved_spike_corner);
// TEST_REGISTER(ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_then_refresh_succeeds_spike_corner);
// TEST_REGISTER(ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_then_refresh_adjacent_doubleword_preserved_spike_corner);
// TEST_REGISTER(ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_clears_reservation_spike_corner);
// TEST_REGISTER(ai_arch_lr_sc_gap_over_64_cycles_doubleword_timeout_second_sc_fail_then_refresh_succeeds_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_fails);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_refresh_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_refresh_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_timeout_fails_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_timeout_then_refresh_succeeds_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_timeout_clears_reservation_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_alias_retry_fails_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_adjacent_word_preserved_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_refresh_succeeds_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_refresh_adjacent_word_preserved_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_alias_timeout_then_original_refresh_succeeds_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_alias_retry_fails_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_refresh_succeeds_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_refresh_adjacent_doubleword_preserved_spike_corner);
// TEST_REGISTER(ai_micro_lr_sc_gap_over_64_cycles_doubleword_alias_timeout_then_original_refresh_succeeds_spike_corner);
// Spike matches the current LR/SC mixed-size / alias expectations; exact success/fail now depends on
// whether the latest live reservation is same-size, mixed-size, and whether the final sc returns to the same alias.
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_inner_success_then_outer_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_inner_success_then_inner_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_doubleword_inner_success_then_outer_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_doubleword_inner_success_then_inner_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_different_size_inner_only_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_different_size_inner_success_then_outer_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_different_size_inner_success_then_inner_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_word_then_doubleword_inner_only_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_word_then_doubleword_inner_success_then_outer_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_different_address_word_then_doubleword_inner_success_then_inner_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_different_size_fails);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_different_size_fail_then_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_nested_same_address_different_size_fail_then_word_refresh_succeeds_corner);
// AP-29 pure full-address/size LR/SC Spike oracle regressions.
TEST_REGISTER(ai_micro_lr_sc_same_block_different_byte_fail_then_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_byte_fail_then_doubleword_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_doubleword_range_fail_then_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_doubleword_range_fail_then_word_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_different_size_fails);
TEST_REGISTER(ai_micro_lr_sc_same_start_word_then_doubleword_fail_then_doubleword_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_word_then_doubleword_fail_then_word_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_doubleword_then_word_different_size_fails_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_doubleword_then_word_fail_then_word_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_doubleword_then_word_fail_then_doubleword_refresh_succeeds_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_byte_failed_sc_consumes_reservation_corner);
TEST_REGISTER(ai_micro_lr_sc_same_block_different_doubleword_range_failed_sc_consumes_reservation_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_word_then_doubleword_failed_sc_consumes_reservation_corner);
TEST_REGISTER(ai_micro_lr_sc_same_start_doubleword_then_word_failed_sc_consumes_reservation_corner);
// Same-paddr alias SC behavior follows Nanhu difftest semantics:
// LR/SC matches physical address plus access size; same-size cache-hit aliases can succeed,
// mixed-width aliases fail, and SC can also fail when the alias path misses in DCache.
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_fails);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_diff_setidx_doubleword_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_original_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_switched_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_execute_and_store_trigger_repeated_breakpoint_then_original_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_original_refresh_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_switched_refresh_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_execute_and_store_trigger_repeated_breakpoint_then_original_refresh_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_execute_and_store_trigger_repeated_breakpoint_then_switched_refresh_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_original_doubleword_refresh_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_store_trigger_repeated_breakpoint_then_switched_doubleword_refresh_adjacent_doubleword_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_original_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_fail_then_refresh_switched_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_original_word_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_switched_word_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_original_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_word_then_doubleword_fail_then_refresh_switched_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_original_word_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_switched_word_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_original_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_doubleword_then_word_fail_then_refresh_switched_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_original_word_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_refresh_switched_word_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_first_succeeds_second_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_first_succeeds_second_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_success_retry_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_success_retry_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_success_retry_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_success_retry_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_original_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_original_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_original_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_original_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_alias_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_cachehit_success_then_alias_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_cachehit_success_then_alias_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_doubleword_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_word_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_different_vaddr_cachehit_doubleword_success_then_alias_word_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_word_then_doubleword_size_mismatch_fail_then_alias_word_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_immediate_doubleword_then_word_size_mismatch_fail_then_alias_doubleword_retry_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_inner_only_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_inner_success_then_original_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_inner_success_then_switched_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_doubleword_inner_only_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_doubleword_inner_success_then_original_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_doubleword_inner_success_then_switched_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_only_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_original_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_switched_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_original_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_different_size_inner_success_then_switched_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_only_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_original_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_switched_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_original_doubleword_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_nested_word_then_doubleword_inner_success_then_switched_word_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_load_still_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_load_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_load_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_halfword_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_halfword_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_byte_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset1_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset2_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_tail_byte_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_halfword_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_halfword_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_byte_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset1_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset2_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_tail_byte_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_word_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_load_before_upper_word_sc_still_succeeds_corner);
// Doubleword same-address LR/SC corners, compiled individually when needed.
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_load_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_load_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_load_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_word_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte0_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte2_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte4_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte6_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_byte_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset1_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset2_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset3_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset4_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset5_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset6_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_tail_byte_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_load_before_low_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_store_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_store_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_store_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_store_before_low_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_store_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_store_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_word_store_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_tail_byte_store_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte0_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte2_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte4_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_halfword_at_byte6_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_byte_store_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset1_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset2_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset3_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset4_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset5_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset6_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_word_amo_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_amo_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_amo_before_low_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_amo_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_amo_sc_fails_preserves_amo_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_amo_failed_sc_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_amo_sc_fails_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_doubleword_store_adjacent_doubleword_preserved_corner);
// Same-paddr alias LR/SC intervening roots are now split by virtual set-idx:
// same-setidx keeps the existing still_succeeds family, while different-setidx roots
// are tracked separately as manual-reference / non-default-golden probes.
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_still_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_fails);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_diff_setidx_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_load_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_load_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_word_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_diff_setidx_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_word_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte0_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte2_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte4_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte6_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset3_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset4_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset5_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset6_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_load_before_doubleword_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_before_low_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_load_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_store_still_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_store_adjacent_word_preserved_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_store_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_store_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_halfword_store_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_halfword_store_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_byte_store_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset1_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset2_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_tail_byte_store_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_store_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_halfword_store_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_halfword_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_byte_store_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset1_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_byte_at_offset2_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_tail_byte_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_word_store_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_store_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_amo_sc_fails_preserves_amo);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_amo_failed_sc_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_amo_sc_fails_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_amo_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_low_word_amo_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_address_intervening_upper_word_amo_before_upper_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_preserves_store);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_fails);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_diff_setidx_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_preserves_store_and_adjacent_word_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_failed_sc_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_store_sc_fails_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_store_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_store_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_store_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_store_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_store_before_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_halfword_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_halfword_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_word_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_sc_fails_preserves_store_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_diff_setidx_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_failed_sc_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_sc_fails_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_sc_fails_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_before_low_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_store_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_store_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_word_store_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_tail_byte_store_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte0_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte2_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte4_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_halfword_at_byte6_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_byte_store_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset1_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset2_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset3_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset4_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset5_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_byte_at_offset6_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_word_amo_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_amo_before_doubleword_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_before_low_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_still_succeeds);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_fails);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_diff_setidx_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_success_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_amo_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_amo_before_word_sc_still_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_low_word_amo_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_upper_word_amo_before_upper_word_sc_fails_preserves_intervening_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_sc_fails_preserves_amo_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_failed_sc_consumes_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_diff_setidx_fail_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_failed_sc_clears_reservation_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_sc_fails_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_intervening_doubleword_amo_sc_fails_then_refresh_switched_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_second_sc_without_new_lr_fails);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_second_doubleword_sc_without_new_lr_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_second_sc_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_second_sc_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_second_doubleword_sc_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_same_paddr_alias_second_doubleword_sc_fail_then_refresh_original_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_second_sc_without_new_lr_fails);
// TEST_REGISTER(ai_micro_lr_sc_second_doubleword_sc_without_new_lr_fails_corner);
// TEST_REGISTER(ai_micro_lr_sc_second_sc_fail_then_refresh_succeeds_corner);
// TEST_REGISTER(ai_micro_lr_sc_second_doubleword_sc_fail_then_refresh_succeeds_corner);

// AI case batch 9: software prefetch remains architecturally silent across non-data-completion paths
// TEST_REGISTER(ai_micro_hs_prefetch_tlb_miss_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_repeated_tlb_miss_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_tlb_exception_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_repeated_same_address_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_page_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_page_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_repeated_page_fault_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_access_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_access_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_repeated_access_fault_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_dcache_miss_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_repeated_dcache_miss_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_preserves_latest_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_repeated_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_then_pmp_deny_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_then_pmp_resp_ld_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_then_pmp_resp_st_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_repeated_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_then_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_then_bus_error_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_then_pmp_deny_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_deny_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_deny_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_deny_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_bus_error_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_bus_error_repeated_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_bus_error_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_bus_error_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_deny_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_deny_repeated_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_deny_then_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_ld_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_ld_repeated_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_ld_then_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_then_pmp_resp_ld_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_resp_ld_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_st_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_st_repeated_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_st_then_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pmp_resp_st_repeated_then_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_then_pmp_resp_st_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_resp_st_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_then_pmp_resp_st_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_resp_st_restore_adjacent_preserved_corner);
// This PBMT/IO-routed prefetch case is generated and tracked in test_point, but remains out of the
// default Spike batch until the target MMIO-route semantics are validated in a PMA/PBMT-capable environment.
// TEST_REGISTER(ai_micro_hs_prefetch_mmio_silent);
// TEST_REGISTER(ai_micro_hs_prefetch_mmio_repeated_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_pbmt_nc_repeated_adjacent_preserved_corner);

// AI case batch 10: M-mode software prefetch remains silent across PMP and MPRV corner cases
// TEST_REGISTER(ai_micro_m_prefetch_pmp_allow_silent);
// TEST_REGISTER(ai_micro_m_prefetch_unlocked_pmp_u_bypass_silent_corner);
// These locked-PMP software-prefetch cases now pass on Spike after dropping the
// invalid in-case "restore locked PMP and reload" tail check.
// TEST_REGISTER(ai_micro_m_prefetch_locked_pmp_u_silent);
// TEST_REGISTER(ai_micro_m_prefetch_locked_pmp_u_repeated_silent_corner);
// TEST_REGISTER(ai_micro_m_prefetch_locked_pmp_m_silent);
// TEST_REGISTER(ai_micro_m_prefetch_locked_pmp_m_repeated_silent_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_pmp_deny_silent);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_pmp_deny_repeated_silent_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_same_page_deny_then_restore_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_same_page_deny_then_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_deny_then_allowed_translated_access_succeeds_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_repeated_deny_then_allowed_translated_access_succeeds_corner);
// These two M->S translated software-prefetch cases now pass on Spike after
// clearing MPRV before the trap handler touches memory-backed bookkeeping.
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_access_fault_silent);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_access_fault_then_valid_prefetch_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_access_fault_then_valid_prefetch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_silent_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_then_valid_prefetch_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_then_valid_prefetch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_page_fault_silent);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_page_fault_then_valid_prefetch_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_page_fault_then_valid_prefetch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_silent_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_then_valid_prefetch_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_then_valid_prefetch_adjacent_preserved_corner);

// AI case batch 11: M-mode load/store PMP handling across MPP and MPRV combinations
// TEST_REGISTER(ai_micro_m_load_pmp_allow_data_visible);
// TEST_REGISTER(ai_micro_m_load_unlocked_pmp_u_bypass_succeeds_corner);
// These locked-PMP direct-load cases now pass on Spike after moving the deny window
// into low-index PMP slots instead of the shadowed high-entry probe layout.
// TEST_REGISTER(ai_micro_m_load_locked_pmp_u_access_fault);
// TEST_REGISTER(ai_micro_m_load_locked_pmp_u_repeated_faults_stable_corner);
// TEST_REGISTER(ai_micro_m_load_locked_pmp_m_access_fault);
// TEST_REGISTER(ai_micro_m_load_locked_pmp_m_repeated_faults_stable_corner);
// TEST_REGISTER(ai_micro_m_load_mprv_u_pmp_read_access_fault);
// TEST_REGISTER(ai_micro_m_load_mprv_u_same_page_fault_then_restore_succeeds_corner);
// TEST_REGISTER(ai_micro_m_load_mprv_u_same_page_repeated_faults_then_restore_succeeds_corner);
// TEST_REGISTER(ai_micro_m_load_mprv_u_pmp_fault_then_allowed_translated_load_succeeds_corner);
// TEST_REGISTER(ai_micro_m_load_mprv_u_repeated_faults_then_allowed_translated_load_succeeds_corner);
// TEST_REGISTER(ai_micro_m_store_pmp_allow_data_visible);
// TEST_REGISTER(ai_micro_m_store_unlocked_pmp_u_bypass_succeeds_corner);
// These locked-PMP direct-store cases now pass on Spike after moving the deny window
// into low-index PMP slots instead of the shadowed high-entry probe layout.
// TEST_REGISTER(ai_micro_m_store_locked_pmp_u_access_fault);
// TEST_REGISTER(ai_micro_m_store_locked_pmp_u_repeated_faults_leave_data_unchanged_corner);
// TEST_REGISTER(ai_micro_m_store_locked_pmp_m_access_fault);
// TEST_REGISTER(ai_micro_m_store_locked_pmp_m_repeated_faults_leave_data_unchanged_corner);
// TEST_REGISTER(ai_micro_m_store_mprv_u_pmp_write_access_fault);
// TEST_REGISTER(ai_micro_m_store_mprv_u_same_page_fault_then_restore_succeeds_corner);
// TEST_REGISTER(ai_micro_m_store_mprv_u_same_page_repeated_faults_then_restore_succeeds_corner);
// TEST_REGISTER(ai_micro_m_store_mprv_u_pmp_fault_then_allowed_translated_store_succeeds_corner);
// TEST_REGISTER(ai_micro_m_store_mprv_u_repeated_faults_then_allowed_translated_store_succeeds_corner);
// Manual/commented: exercises PMP OFF/TOR/NAPOT/no-match/granularity/locked-write matrix; locked entries require reset-level cleanup.
// TEST_REGISTER(ai_micro_m_pmp_match_lock_granularity_matrix_corner);
// Manual/commented: exercises PMP X-only fetch allow plus locked no-permission load/store/fetch deny; locked entries require reset-level cleanup.
// TEST_REGISTER(ai_micro_m_pmp_fetch_xonly_and_locked_deny_matrix_corner);

// AI case batch 12: HS AMO paths with successful writeback and architecturally visible fault outcomes
// TEST_REGISTER(ai_micro_hs_amoadd_data_visible);
// TEST_REGISTER(ai_micro_hs_amo_back_to_back_same_address_accumulates_corner);
// TEST_REGISTER(ai_micro_hs_amo_back_to_back_same_address_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_amo_src2_operand_back_to_back_wd_chain_corner);
TEST_REGISTER(ai_micro_hs_amo_src2_operand_fault_success_isolation_corner);
// TEST_REGISTER(ai_micro_hs_amo_same_paddr_alias_back_to_back_accumulates_corner);
// TEST_REGISTER(ai_micro_hs_amo_same_paddr_alias_back_to_back_adjacent_preserved_corner);
// Manual RTL-only: D$/MSHR/refill behavior is the target; Spike has no matching cache/MSHR model.
// TEST_REGISTER(ai_micro_hs_amo_dcache_miss_data_visible);
// TEST_REGISTER(ai_micro_hs_amo_dcache_miss_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_mshr_full_retries_visible);
// TEST_REGISTER(ai_micro_hs_amo_mshr_full_retries_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_bus_error_access_fault_repeated_stable_corner);
// TEST_REGISTER(ai_micro_hs_amo_bus_error_access_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_bus_error_access_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_tlb_access_fault);
// TEST_REGISTER(ai_micro_hs_amo_repeated_tlb_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_amo_tlb_access_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_tlb_access_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_page_fault);
// TEST_REGISTER(ai_micro_hs_amo_repeated_page_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_amo_page_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_page_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_pmp_write_access_fault);
// TEST_REGISTER(ai_micro_hs_amo_pmp_write_access_fault_repeated_stable_corner);
// TEST_REGISTER(ai_micro_hs_amo_pmp_write_access_fault_then_restore_succeeds_corner);
// TEST_REGISTER(ai_micro_hs_amo_pmp_write_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_pmp_write_access_fault_then_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_bus_error_access_fault);
TEST_REGISTER(ai_micro_hs_amo_last_instruction_page_fault_recovers_corner);
TEST_REGISTER(ai_micro_hs_amo_last_instruction_page_fault_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_amo_first_instruction_page_fault_recovers_corner);
TEST_REGISTER(ai_micro_hs_amo_first_instruction_page_fault_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_amo_last_instruction_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_last_instruction_mmio_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_last_instruction_mmio_access_fault_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_amo_last_instruction_access_fault_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_amo_first_instruction_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_first_instruction_mmio_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_first_instruction_mmio_access_fault_adjacent_preserved_corner);
TEST_REGISTER(ai_micro_hs_amo_first_instruction_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_last_instruction_bus_error_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_last_instruction_bus_error_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_first_instruction_bus_error_access_fault_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_first_instruction_bus_error_access_fault_adjacent_preserved_corner);
// AP-16 manual RTL-only: DCache WritebackQueue pending / dirty-line release windows are not modeled by official Spike.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap16_dirty_evict_amo_w_offset_sweep_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap16_dirty_evict_amo_d_miss_pressure_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap16_dirty_evict_lrsc_w_d_offset_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap16_redirect_recovery_amo_after_dirty_pressure_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap16_cbo_flush_same_line_amo_owner_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap16_same_line_load_store_atomic_conflict_manual_smoke_corner);
// These HS AMO PBMT/MMIO cases depend on MENVCFG.PBMTE / PMA CSR behavior.
// Keep them compiled and tracked in test_point, but out of the default Spike regression.
// TEST_REGISTER(ai_micro_hs_amo_pbmt_nonzero_access_fault);
// TEST_REGISTER(ai_micro_hs_amo_mmio_access_fault);
// TEST_REGISTER(ai_micro_hs_amo_mmio_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_amo_mmio_access_fault_repeated_stable_corner);
// TEST_REGISTER(ai_micro_hs_amo_mmio_access_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_amo_mmio_access_fault_then_valid_access_adjacent_preserved_corner);

// AI case batch 13: M-mode direct-access and software-prefetch entry behavior on canonical vs high invalid addresses
// TEST_REGISTER(ai_micro_m_load_direct_paddr_hit_data_visible);
// TEST_REGISTER(ai_micro_m_load_direct_high_addr_access_fault_raw_tval);
// TEST_REGISTER(ai_micro_m_load_direct_high_addr_then_valid_paddr_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_direct_high_addr_then_valid_paddr_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_load_direct_repeated_high_addr_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_m_prefetch_direct_paddr_hit_silent);
// TEST_REGISTER(ai_micro_m_prefetch_direct_repeated_paddr_hit_silent_corner);
// TEST_REGISTER(ai_micro_m_prefetch_direct_high_addr_fault_silent);
// TEST_REGISTER(ai_micro_m_prefetch_direct_high_addr_then_valid_paddr_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_direct_high_addr_then_valid_paddr_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_direct_repeated_high_addr_silent_corner);
// TEST_REGISTER(ai_micro_m_store_direct_paddr_hit_data_visible);
// TEST_REGISTER(ai_micro_m_store_direct_repeated_paddr_last_writer_wins_corner);
// TEST_REGISTER(ai_micro_m_store_then_load_direct_paddr_latest_corner);
// TEST_REGISTER(ai_micro_m_store_direct_high_addr_access_fault_raw_tval);
// TEST_REGISTER(ai_micro_m_store_direct_high_addr_then_valid_paddr_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_direct_high_addr_then_valid_paddr_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_store_direct_repeated_high_addr_access_fault_stable_corner);

// AI case batch 14: fetch-side repeated execute-trigger permission flips after scalar/vector code-page rewrites
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);

// AI case batch 15: fetch-side repeated execute-trigger noncanonical/all-ones/leaf-overflow fallback chains after scalar/vector code-page rewrites
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_16b_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);

// AI case batch 16: fetch-side same-paddr alias code rewrites observed from an executable alias after fence.i
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);

// AI case batch 17: fetch-side same-paddr alias repeated execute-trigger fallback chains after code rewrites
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);

// AI case batch 18: fetch-side same-paddr alias mixed vector-then-scalar code rewrites observed from an executable alias
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_fetch_after_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner);

// AI architecture FENCE.I visibility follow-up: AMO/SC writers, prefetch.i old response, idempotent fence.i, SFENCE.VMA pairing, and CBO+FENCE.I.
TEST_REGISTER(ai_arch_fence_i_amo_sc_code_update_visibility_corner);
TEST_REGISTER(ai_arch_fence_i_sc_fail_and_store_fault_no_code_update_corner);
// TEST_REGISTER(ai_arch_fence_i_prefetch_i_old_response_no_override_corner);
// TEST_REGISTER(ai_arch_fence_i_repeated_idempotent_code_visibility_corner);
// TEST_REGISTER(ai_arch_fence_i_sfence_vma_translation_pair_visibility_corner);
// TEST_REGISTER(ai_arch_fence_i_cbo_clean_flush_code_visibility_corner);
// TEST_REGISTER(ai_arch_fence_i_cbo_inval_dirty_profile_boundary_corner);
// TEST_REGISTER(ai_arch_fence_i_data_order_requires_explicit_fence_boundary_corner);
TEST_REGISTER(ai_arch_fence_i_committed_code_fetch_fault_no_rollback_corner);

// AI case batch AP-52/AP-53: IFU/ITLB/PTW flush and duplicate-refill manual probes.
// These cover legal black-box fetch progress/stale-translation oracles only; the
// exact AP-53 duplicate refill skip/touch/debug semantics require RTL/FSDB evidence.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap52_legal_sfence_ifetch_rewalk_manual_corner);
// TEST_REGISTER(ai_micro_ap52_satp_context_ifetch_rewalk_manual_corner);
// TEST_REGISTER(ai_micro_ap52_svinval_ifetch_closeout_manual_corner);
// TEST_REGISTER(ai_micro_ap53_largepage_ifetch_flush_stress_manual_corner);

// Frontend IT MMIO/PBMT.NC fetch manual probes from the frontend IT decomposition table.
// These depend on PMA CSR, PBMT=NC, frontend uncache/MMIO routing, and LinkNan waveform/coverage
// confirmation; official Spike is not a semantic gate for these PMA/PBMT/cacheability paths.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_frontend_mmio_fetch_pma_uncache_single_line_executes_manual_corner);
// TEST_REGISTER(ai_micro_frontend_mmio_fetch_pma_uncache_cross_64b_refetch_manual_corner);
// TEST_REGISTER(ai_micro_frontend_mmio_fetch_pma_uncache_locked_pmp_pass_manual_corner);
// TEST_REGISTER(ai_micro_frontend_mmio_fetch_pma_fail_then_pmp_fail_manual_corner);
// TEST_REGISTER(ai_micro_frontend_mmio_fetch_wait_last_commit_manual_corner);
// TEST_REGISTER(ai_micro_frontend_mmio_fetch_mem_attr_cross_fault_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_hit_miss_executes_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_cross_64b_refetch_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_pmp_fault_priority_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_itlb_miss_pmp_fail_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_page_fault_priority_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_access_fault_priority_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_wait_last_commit_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_bus_corrupt_special_run_manual_corner);
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_fetch_mem_attr_cross_fault_manual_corner);
// TEST_REGISTER(ai_micro_hs_pbmt_nc_mixed_cjr_cebreak_call_ret_fetch_manual_corner);

// First-seen conditional branches are actually not-taken while LinkNan's
// default prediction is taken. Architectural selfchecks validate exception
// sources and no-ghost side effects; exact BPU/FTQ/ROB residency is RTL-only.
// reason_code: D-MANUAL-RTL-ONLY
// TEST_REGISTER(ai_micro_m_first_taken_prediction_wrong_path_illegal_ebreak_ecall_squash_corner);
// TEST_REGISTER(ai_micro_hs_first_taken_prediction_wrong_path_privileged_exception_squash_corner);
// TEST_REGISTER(ai_micro_hs_first_taken_prediction_wrong_path_fetch_ipf_iaf_squash_corner);
// TEST_REGISTER(ai_micro_hs_first_taken_prediction_wrong_path_load_store_pf_af_squash_corner);

// P1-215 manual/nongate: PBMT=NC instruction fetch uses InstrUncache/MmioFsm
// paths that official Spike does not model; LinkNan path markers must close
// late wrong-path response quarantine and resend owner metadata.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_hs_instr_uncache_pbmt_nc_branch_resend_quarantine_manual_corner);

// AI case batch 19: same-paddr alias mixed vector/scalar execute-and-address-trigger load/store probes
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);

// AI case batch 20: same-paddr alias mixed vector/scalar execute-and-address-trigger fallback to PF/AF then recover
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);

// AI case batch 18 supplement: same-paddr alias mixed vector/scalar same-op stable baselines
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_adjacent_preserved_corner);

// AI case batch 18 supplement: same-paddr alias mixed vector/scalar same-op dual-address-trigger Spike probes
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);

// AI case batch 19 supplement: same-paddr alias mixed vector/scalar same-op execute-trigger plus second-half-address-slot Spike completion corners
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);

// AI case batch 19 supplement: same-paddr alias mixed vector/scalar same-op execute-trigger plus second-half-address-slot Spike PF/AF fallback then recovery
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner);

// AI case batch 20 supplement: same-paddr alias mixed vector/scalar cross-op stable baselines
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_stable_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_adjacent_preserved_corner);

// AI case batch 20 supplement: same-paddr alias scalar-store then vector-load stable baselines
// TEST_REGISTER(ai_micro_hs_vector_same_paddr_alias_load_after_scalar_same_paddr_alias_store_stable_corner);
// TEST_REGISTER(ai_micro_hs_vector_same_paddr_alias_load_after_scalar_same_paddr_alias_store_adjacent_preserved_corner);

// AI case batch 21: same-paddr alias mixed vector/scalar cross-op execute-and-address-trigger probes
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);

// AI case batch 22: same-paddr alias mixed vector/scalar cross-op execute-and-address-trigger fallback to PF/AF then recover
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);

// AI case batch 23: cross-16B mixed vector/scalar cross-op execute-and-address-trigger probes
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);

// AI case batch 24: cross-16B mixed vector/scalar cross-op execute-and-address-trigger fallback to PF/AF then recover
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_load_after_vector_cross_16b_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_16b_store_after_vector_cross_16b_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);

// AI case batch 25: cross-page mixed vector/scalar cross-op execute-and-address-trigger probes
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner);

// AI case batch 26: cross-page mixed vector/scalar cross-op execute-and-address-trigger fallback to PF/AF then recover
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner);

// AI case batch 27: cross-page dual-address-trigger Spike corners for high-half PF/AF recovery
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner);

// AI case batch 28: cross-page dual-address-trigger Spike corners for high-half PF/AF recovery with adjacent preservation
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_spike_corner);

// AI case batch 29: cross-16B dual-address-trigger Spike corners for PF/AF fallback then recovery
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_spike_corner);

// AI case batch 30: same-paddr alias mixed vector/scalar dual-address-trigger Spike corners for PF/AF fallback then recovery
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner);

// AI case batch 31: same-paddr alias mixed vector/scalar dual-address-trigger Spike completion corners
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_second_half_address_trigger_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_only_original_matches_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_dual_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);

// AI case batch 32: same-paddr alias mixed vector/scalar execute-trigger plus second-half-address-slot Spike completion corners
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_ignored_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner);

// AI case batch 33: same-paddr alias mixed vector/scalar execute-trigger plus second-half-address-slot Spike PF/AF fallback then recovery
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_load_after_vector_same_paddr_alias_store_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_recovers_spike_corner);
// TEST_REGISTER(ai_micro_hs_scalar_same_paddr_alias_store_after_vector_same_paddr_alias_load_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_spike_corner);

// AI case batch 49: memblock suspected bug corners, load-forward epoch binding subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_forward_valid_and_invalid_cause_same_epoch_no_cross_cycle_mixup_corner);
// TEST_REGISTER(ai_micro_hs_load_replay_reports_real_blocking_sqidx_not_next_cycle_stale_index_corner);

// AI case batch 50: memblock suspected bug corners, sbuffer tag-mismatch query-epoch subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_sbuffer_tag_mismatch_decision_stays_on_original_query_epoch_corner);
// TEST_REGISTER(ai_micro_hs_alias_matchinvalid_not_created_or_lost_by_one_cycle_entry_retag_corner);

// AI case batch 51: memblock suspected bug corners, sbuffer evict-valid payload-stability subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_sbuffer_evict_valid_payload_stable_under_ready_backpressure_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_same_entry_update_after_evict_valid_no_mutated_dcache_payload_corner);

// AI case batch 52: memblock suspected bug corners, freelist prealloc free-then-realloc subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_freelist_prealloc_free_then_realloc_ready_and_slot_stay_consistent_corner);
TEST_REGISTER(ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner);

// AI case batch 53: memblock suspected bug corners, delayed misalign replay-slot no-rebind subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_replayq_delayed_misalign_request_not_rebound_to_reused_entry_corner);
// TEST_REGISTER(ai_micro_hs_replayq_delayed_misalign_request_not_rebound_to_reused_entry_adjacent_preserved_corner);

// AI case batch 54: memblock suspected bug corners, vector merge-buffer reused-entry delayed-writeback subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vload_mergebuffer_delayed_wb_not_applied_to_reused_mbindex_corner);
// TEST_REGISTER(ai_micro_hs_vload_mergebuffer_inactive_wb_metadata_not_read_from_reused_entry_corner);

// AI case batch 55: memblock suspected bug corners, LSQ-plus-AMO single-step exception oldest-selection subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_single_step_exception_multi_source_same_cycle_oldest_must_win_corner);
// TEST_REGISTER(ai_micro_hs_lsq_amo_same_cycle_exception_no_mux1h_age_loss_corner);

// AI case batch 56: memblock suspected bug corners, resident-exception redirect replacement subset
// Both redirect-replacement probes are Spike-clean today. The second one was narrowed to a Spike-stable
// older-store-AF then younger-load-PF approximation so it still checks stale resident-entry replacement
// without depending on PBMT=NC scalar misaligned-trap behavior.
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_mbexceptionbuffer_flushed_resident_replaced_by_same_cycle_new_exception_corner);
// TEST_REGISTER(ai_micro_hs_redirect_flush_old_exception_no_stale_resident_entry_after_new_enqueue_corner);

// AI case batch 57: memblock suspected bug corners, AtomicsUnit redirect-src2 lifetime subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_atomic_redirect_midflight_no_stale_src2_carryover_corner);
// TEST_REGISTER(ai_micro_hs_atomic_redirect_then_new_atomic_no_rereceive_or_wrong_result_corner);

// AI case batch 58: memblock suspected bug corners, AtomicsUnit early-feedback ordering subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_atomic_early_feedback_no_false_success_under_late_fault_corner);
// TEST_REGISTER(ai_micro_hs_atomic_early_feedback_no_order_break_under_replay_corner);

// AI case batch 59: memblock suspected bug corners, killed-store stale-rollback subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_loadqueue_raw_flushed_store_no_stale_rollback_redirect_corner);
// TEST_REGISTER(ai_micro_hs_stld_violation_store_killed_mid_select_does_not_flush_younger_load_corner);

// AI case batch 60: memblock suspected bug corners, dense-store replay-release boundary subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner);
TEST_REGISTER(ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner);

// ai_test_cases/memblock/forward_alias/ai_arch_newmdp_update_conflict_cases.c
TEST_REGISTER(ai_arch_newmdp_same_entry_update_collision_no_stale_payload_corner);

// AI case batch 61: memblock suspected bug corners, vector merge-buffer pressure subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vector_load_merge_buffer_pressure_no_hang_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_merge_buffer_pressure_with_redirect_or_exception_corner);

// AI case batch 62: memblock suspected bug corners, replay-head misalign under repeated younger-direct pressure subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_older_replayq_misalign_not_starved_by_repeated_younger_direct_requests_corner);
// TEST_REGISTER(ai_micro_hs_older_replayq_misalign_not_starved_by_repeated_younger_direct_requests_adjacent_preserved_corner);

// AI case batch 63: memblock suspected bug corners, ld-ld replay plus nearby writeback subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner);
// TEST_REGISTER(ai_micro_hs_ldld_replay_fetch_flush_no_duplicate_writeback_corner);
// Manual RTL-only AP-133 smoke: zero-threshold replay_escape + reversed same-PA alias load pair.
// TEST_REGISTER(ai_micro_hs_load_replay_escape_zero_threshold_reversed_alias_pair_no_stale_corner);

// AI case batch 64: memblock suspected bug corners, dual misalign replay staged-recovery subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_robhead_two_misalign_replay_entries_same_row_follow_uopidx_order_corner);
// TEST_REGISTER(ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner);

// AI case batch 65: memblock suspected bug corners, PBMT=NC vector-load feedback-path probes
// 2026-04-04 individual Spike recheck: 0/2 passed. The current expectations assume PBMT=NC vector loads
// take the older unsupported-path fault route, but the official Spike/zicclsm runner disagrees, so these probes need
// semantics cleanup before they can be used as a stable backfill for the feedback-reason item.
// 2026-04-26 recheck: manual RTL-only. This is not a Spike TLB/cache-model mismatch; Spike executes
// the PBMT=NC misaligned vector access under the current default runner, while the target RTL probe expects the
// vector unsupported-path access-fault route and scalar same-alias recovery.
// Withdrawn: the aligned PBMT=NC vector-load preserves-register variant over-constrained legal aligned vector access.
// TEST_REGISTER(ai_micro_hs_vector_load_pbmt_nc_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner);

// AI case batch 66: memblock suspected bug corners, dual PBMT=NC load same-window approximation
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// 2026-04-26 recheck: still Spike-passing, but target remains RTL replay/uncache-buffer ownership.
// TEST_REGISTER(ai_micro_hs_robhead_two_mmio_replay_entries_same_row_serialize_by_uopidx_corner);
// TEST_REGISTER(ai_micro_hs_same_rob_row_dual_uncache_load_replay_no_popcount_assert_corner);

// AI case batch 67: memblock suspected bug corners, mixed misalign-fault + PBMT=NC replay-service coexistence subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// 2026-04-26 recheck: still Spike-passing, but target remains RTL replay/uncache-buffer ownership.
// TEST_REGISTER(ai_micro_hs_robhead_mmio_and_misalign_replay_same_window_no_assert_or_drop_corner);
// TEST_REGISTER(ai_micro_hs_replayq_stale_mmio_plus_fresh_misalign_head_service_serializes_correctly_corner);

// AI case batch 67A: memblock suspected bug corners, shared uncache-buffer MMIO arbitration RTL-only subset
// PMA/MMIO coverage must stay on PBMT=IO + responsive 3T IO backing; Spike is not a reliable gate for PMA.
// TEST_REGISTER(ai_micro_hs_replayq_head_mmio_not_freed_when_direct_exu_mmio_wins_uncachebuffer_corner);
// TEST_REGISTER(ai_micro_hs_shared_uncachebuffer_mmio_mux_no_false_schedule_or_free_of_replay_head_corner);

// Manual RTL-only: directed TLB/cache residency of @/#/indexed-vector-load flow is not Spike-gatable.
// TEST_REGISTER(ai_micro_hs_load_store_then_vloxei_fault_dep_flow_corner);
// TEST_REGISTER(ai_micro_hs_load_store_then_vloxei_elem1_dmiss_replay_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_store_then_vloxei_no_fault_dep_flow_probe);
// TEST_REGISTER(ai_micro_hs_raw_store_vloxei_fault_then_reuse_probe_corner);
// TEST_REGISTER(ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_same_uop_escape_corner);
// TEST_REGISTER(ai_micro_hs_load_store_then_vloxei_elem0_dmiss_fault1_min_escape_corner);
// TEST_REGISTER(ai_micro_hs_vloxei_elem0_first_page_fault_vstart0_corner);

// AI case batch 67B: memblock suspected bug corners, delayed MMIO replay-entry ownership RTL-only subset
// Keep the stimulus on PBMT=IO + responsive 3T IO backing instead of replacing it with a PBMT=NC proxy.
// 2026-04-26 recheck: still Spike-passing, but target remains RTL replay-entry ownership.
// TEST_REGISTER(ai_micro_hs_replayq_delayed_mmio_request_not_rebound_to_reused_entry_corner);

// AI case batch 68: memblock suspected bug corners, segment-store + normal vector-store feedback-port coexistence subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_and_vsmerge_feedback_port0_same_cycle_no_mux1h_corruption_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_feedback_priority_segment_vs_merge_is_deterministic_corner);

// AI case batch 68A: memblock suspected bug corners, scalar-store + vsegment-store shared sbuffer-port approximation subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_sbuffer_port0_scalar_and_vsegment_same_cycle_no_mux1h_drop_corner);
// TEST_REGISTER(ai_micro_hs_shared_sbuffer_port0_lsqueue_vsegment_no_mixed_addr_data_mask_corner);

// AI case batch 68C: memblock suspected bug corners, scalar-load0 + vsegment shared dcache-port approximation subset
// 2026-05-25 source review + direct object compile: 2/2 passed.
TEST_REGISTER(ai_micro_hs_vsegment_scalar_load0_dcache_handoff_no_response_misroute_corner);
TEST_REGISTER(ai_micro_hs_vsegment_start_finish_boundary_no_scalar_load_starvation_corner);

// ai_test_cases/memblock/vector/ai_micro_memblock_vsegment_success_cases.c
TEST_REGISTER(ai_micro_hs_vsegment_success_then_scalar_load_store_no_preempt_leak_corner);
TEST_REGISTER(ai_micro_hs_vsegment_then_normal_vector_load_no_writeback_drop_corner);
TEST_REGISTER(ai_micro_hs_normal_vector_load_then_vsegment_no_writeback_preempt_drop_corner);
TEST_REGISTER(ai_micro_hs_vsegment_store_then_lrsc_no_stale_segment_writeback_or_reservation_loss_corner);

// AP-22 LoadPipe data_delayed visible vldsegment payload coverage.
TEST_REGISTER(ai_micro_ap22_vlseg_nf_eew_visible_payload_matrix_corner);
TEST_REGISTER(ai_micro_ap22_vlseg2e8_low4_offset_cross16_roundtrip_corner);
TEST_REGISTER(ai_micro_ap22_back_to_back_vlseg2e8_no_stale_data_corner);
TEST_REGISTER(ai_micro_ap22_masked_and_vstart_vlseg2e8_no_stale_lane_corner);
TEST_REGISTER(ai_micro_ap22_cross_page_fault_repair_vlseg2e8_roundtrip_corner);

// AI case batch 68D: memblock suspected bug corners, vsegment fault cleanup and prefix writeback subset
TEST_REGISTER(ai_micro_hs_vsegment_fault_clears_memblock_flag_followup_scalar_load_progress_corner);

// AI case batch 68E: memblock suspected bug corners, scalar-load and vsegment exception metadata isolation subset
TEST_REGISTER(ai_micro_hs_meb_loadport_vsegment_fault_switch_no_stale_tval_corner);
TEST_REGISTER(ai_micro_hs_meb_storeport_vsegment_fault_switch_no_stale_cause_tval_corner);

// AI case batch 68F: memblock suspected bug corners, sbuffer backpressure scalar-store valid vs faulting vsseg2 store segment.
TEST_REGISTER(ai_micro_hs_sbuffer_backpressure_sd_vsseg2_valid_not_dropped_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_backpressure_loop_sd_vsseg2_commit_not_dropped_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_backpressure_loop_vsseg2_then_sd_commit_not_dropped_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_backpressure_loop_vsseg2_then_sd_stride58_commit_not_dropped_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_late_fault_vssseg2_then_handler_sd_stride58_repro_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_indexed_vsegment_fill_then_sd_drop_repro_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_indexed_vsegment_pending_then_sd_drop_difftest_repro_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_indexed_vsegment_fault_handler_head_sd_drop_difftest_repro_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_indexed_vseg_pending_fault_head_sd_drop_repro_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_indexed_vsegment_late_pending_then_sd_drop_difftest_repro_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_probe_long_vsseg2_scalar_overlap_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_probe_cross16_sd_vsseg2_scalar_overlap_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_probe_strided_vssseg2_scalar_overlap_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_probe_long_vssseg2e8_stride58_scalar_overlap_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_probe_final_vssseg2e8_stride64_scalar_overlap_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_probe_stride58_tail_vssseg2e8_scalar_difftest_overlap_corner);

// AI case batch 68B: memblock suspected bug corners, queued-vsegment distinct-vl/context and burst-overwrite approximation subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_two_instructions_queued_no_instmicroop_context_bleed_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_back_to_back_many_uops_ready_must_not_hide_full_condition_corner);

// AI case batch 69: memblock suspected bug corners, VFOF writeback-port arbitration subset
// 2026-04-04 source review + individual Spike recheck: 4/4 passed.
TEST_REGISTER(ai_micro_hs_vfof_single_writeback_not_duplicated_across_two_vldu_ports_corner);
TEST_REGISTER(ai_micro_hs_vfof_writeback_ready_not_overwritten_by_dual_port_arbitration_corner);
TEST_REGISTER(ai_micro_hs_vfof_back_to_back_instructions_no_entry_context_overwrite_corner);
TEST_REGISTER(ai_micro_hs_younger_vfof_must_not_replace_older_live_vfof_entry_corner);

// AI case batch 69B: memblock suspected bug corners, VFOF partial-stop / recovery subset
// TEST_REGISTER(ai_micro_hs_vfof_partial_vl_then_followup_vector_load_no_stale_context_corner);
// TEST_REGISTER(ai_micro_hs_vfof_repeated_partial_stop_then_repaired_full_reload_recovers_corner);

// AI case batch 69C: memblock suspected bug corners, VFOF trap-driven redirect subset
// 2026-04-05 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vfof_partial_vl_then_trap_redirect_followup_vector_load_no_stale_context_corner);
// TEST_REGISTER(ai_micro_hs_vfof_repeated_partial_vl_then_trap_redirect_followup_vector_load_no_stale_context_corner);

// AI case batch 69C-AP31/AP46: VFOF first-fault and ExceptionBuffer stale-metadata regression subset
// TEST_REGISTER(ai_micro_hs_vfof_later_fault_candidates_keep_first_vl_once_corner);
// TEST_REGISTER(ai_micro_hs_vfof_masked_off_fault_skips_to_first_active_fault_corner);
// TEST_REGISTER(ai_micro_hs_vfof_e32_suffix_payload_not_valid_after_first_fault_corner);
// TEST_REGISTER(ai_micro_hs_vfof_vstart_nonzero_unmasked_first_fault_absolute_index_corner);
// TEST_REGISTER(ai_micro_hs_vfof_element0_fault_visible_exception_control_corner);
TEST_REGISTER(ai_micro_hs_vfof_nonzero_pf_then_scalar_load_fault_refreshes_tval_corner);
TEST_REGISTER(ai_micro_hs_vfof_nonzero_pf_then_scalar_store_fault_refreshes_tval_corner);
TEST_REGISTER(ai_micro_hs_vfof_exception_buffer_boundary_eew_matrix_corner);
TEST_REGISTER(ai_micro_hs_vfof_nonzero_pf_then_vector_and_amo_fault_refreshes_tval_corner);
TEST_REGISTER(ai_micro_hs_back_to_back_vfof_partial_stops_then_scalar_fault_refreshes_tval_corner);
TEST_REGISTER(ai_micro_hs_vfof_segment_nonzero_field_fault_then_scalar_fault_refreshes_tval_corner);
TEST_REGISTER(ai_micro_hs_vfof_repair_full_reload_then_scalar_fault_no_stale_metadata_corner);
// TEST_REGISTER(ai_micro_hs_vfof_nonzero_fault_then_address_trigger_breakpoint_priority_corner);
// TEST_REGISTER(ai_micro_hs_vector_fp_illegal_path_keeps_fs_clean_spike_corner);

// AI case batch FOF/US/BP cross: address-trigger + VFOF exact priority and
// trigger rearm/type-filter probes are LinkNan/RTL-oriented, not Spike default gate.
// TEST_REGISTER(ai_micro_fof_us_bp_quarantine_then_refresh_manual_corner);
// TEST_REGISTER(ai_micro_fof_masked_off_trigger_active_fault_manual_corner);
// TEST_REGISTER(ai_micro_fof_element0_same_address_bp_priority_manual_corner);
// TEST_REGISTER(ai_micro_us_nosplit_then_fof_second_flow_fault_manual_corner);
// TEST_REGISTER(ai_micro_us_store_bp_cutoff_then_fof_flag_isolation_manual_corner);
// TEST_REGISTER(ai_micro_us_bp_clear_rearm_then_fof_same_va_fault_manual_corner);
// TEST_REGISTER(ai_micro_nonzero_vstart_us_then_fof_bp_absolute_index_manual_corner);
// TEST_REGISTER(ai_micro_mask_hole_us_fof_active_only_oldest_manual_corner);
// TEST_REGISTER(ai_micro_cross_page_second_half_trigger_then_fof_fault_order_manual_corner);
// TEST_REGISTER(ai_micro_dual_address_trigger_earliest_then_fof_fallback_manual_corner);
// TEST_REGISTER(ai_micro_store_bp_prefix_to_fof_forward_boundary_manual_corner);
// TEST_REGISTER(ai_micro_fof_breakpoint_tail_mask_suffix_preserve_manual_corner);
// TEST_REGISTER(ai_micro_us_bp_pte_flip_then_fof_permission_refresh_manual_corner);
// TEST_REGISTER(ai_micro_wrong_path_us_fof_bp_kill_correct_path_fresh_manual_corner);
// TEST_REGISTER(ai_micro_vfof_entry_reuse_pressure_then_us_bp_owner_refresh_manual_corner);
// TEST_REGISTER(ai_micro_us_store_bp_suffix_cancel_then_new_store_full_commit_manual_corner);
// TEST_REGISTER(ai_micro_us_store_bp_sbuffer_pressure_then_fof_freshness_manual_corner);
// TEST_REGISTER(ai_micro_vfof_pte_rewalk_then_bp_reevaluate_manual_corner);
// TEST_REGISTER(ai_micro_zero_active_us_fof_then_bp_no_ghost_flow_manual_corner);
// TEST_REGISTER(ai_micro_vfof_v0_mask_then_us_bp_consumes_fresh_mask_manual_corner);
// TEST_REGISTER(ai_micro_segment_fof_us_bp_field_owner_resident_manual_corner);
// TEST_REGISTER(ai_micro_same_pc_loop_us_fof_bp_rearm_iteration_fresh_manual_corner);
// TEST_REGISTER(ai_micro_high_fullva_vfof_bp_tval_no_truncation_manual_corner);
// TEST_REGISTER(ai_micro_fof_us_bp_type_filter_nonzero_fault_same_va_manual_corner);
// TEST_REGISTER(ai_micro_fof_us_bp_rearm_same_va_type_filter_manual_corner);
// TEST_REGISTER(ai_micro_same_us_load_store_early_bp_late_fault_priority_manual_corner);
// TEST_REGISTER(ai_micro_wide_eew_partial_byte_trigger_element_index_manual_corner);
// TEST_REGISTER(ai_micro_vfof_vl_tail_boundary_trigger_active_only_manual_corner);
// TEST_REGISTER(ai_micro_segment_fof_vstart_field_trigger_absolute_index_manual_corner);
// TEST_REGISTER(ai_micro_high_only_mask_us_trigger_then_fof_split_clean_manual_corner);
// TEST_REGISTER(ai_micro_lmul_m2_vfof_group_boundary_bp_neighbor_manual_corner);
// TEST_REGISTER(ai_micro_vfof_final_vl_then_vsetvli_us_bp_order_manual_corner);
// TEST_REGISTER(ai_micro_segment_fof_nonzero_vstart_us_neighbor_owner_manual_corner);
// TEST_REGISTER(ai_micro_vfof_final_vl_wb_mask_no_clobber_manual_corner);
// TEST_REGISTER(ai_micro_vfof_redirect_kill_then_correct_path_bp_manual_corner);
// TEST_REGISTER(ai_micro_vtype_e8_e16_trigger_scale_refresh_manual_corner);
// TEST_REGISTER(ai_micro_store_vtype_e8_e16_trigger_scale_refresh_manual_corner);
// TEST_REGISTER(ai_micro_same_pa_alias_fullva_trigger_owner_manual_corner);
// TEST_REGISTER(ai_micro_mask_whole_neighbor_vl_isolation_manual_corner);
// TEST_REGISTER(ai_micro_amo_lrsc_trigger_neighbor_vfof_us_owner_manual_corner);
// TEST_REGISTER(ai_micro_priv_flip_us_bp_vfof_permission_refresh_manual_corner);
// TEST_REGISTER(ai_micro_vfof_us_visible_event_order_owner_manual_corner);
// TEST_REGISTER(ai_micro_dual_nonzero_fault_min_vl_then_us_bp_fresh_manual_corner);
// TEST_REGISTER(ai_micro_samepage_cross16_load_bp_then_vfof_split_fresh_manual_corner);
// TEST_REGISTER(ai_micro_vfof_then_scalar_fault_source_refresh_manual_corner);
// TEST_REGISTER(ai_micro_store_cross16_bp_suffix_mask_repair_manual_corner);
// TEST_REGISTER(ai_micro_strided_neighbor_then_us_fof_mask_owner_manual_corner);
// TEST_REGISTER(ai_micro_segment_store_bp_prefix_then_vfof_owner_manual_corner);
// TEST_REGISTER(ai_micro_fof_bp_trap_csr_logging_no_mixed_state_manual_corner);
// TEST_REGISTER(ai_micro_mixed_rvc_setup_us_fof_current_operands_manual_corner);
// TEST_REGISTER(ai_micro_vill_transient_recover_us_fof_retry_manual_corner);
// TEST_REGISTER(ai_micro_vs_off_illegal_suppresses_us_fof_trigger_manual_corner);
// TEST_REGISTER(ai_micro_trigger_reprogram_old_event_then_segment_fof_fresh_manual_corner);
// TEST_REGISTER(ai_micro_segment_fof_retry_field_owner_fullva_refresh_manual_corner);
// TEST_REGISTER(ai_micro_store_prefix_drained_then_segment_fof_reads_current_bytes_manual_corner);
// TEST_REGISTER(ai_micro_us_store_translation_fault_repair_then_bp_rewalk_manual_corner);
// TEST_REGISTER(ai_micro_same_mbindex_cross16_prefix_and_low_bp_mask_manual_corner);
// TEST_REGISTER(ai_micro_trigger_priv_filter_us_store_fof_owner_manual_corner);
// TEST_REGISTER(ai_micro_prefetch_address_trigger_silent_then_us_fof_fresh_manual_corner);
// TEST_REGISTER(ai_micro_pte_ad_us_vfof_bp_side_effect_boundary_manual_corner);
// TEST_REGISTER(ai_micro_pmp_us_vfof_bp_priority_matrix_manual_corner);
// TEST_REGISTER(ai_micro_cbozero_fencei_us_vfof_bp_fresh_owner_manual_corner);
// TEST_REGISTER(ai_micro_handler_vector_spill_between_us_vfof_bp_retry_manual_corner);
// TEST_REGISTER(ai_micro_vfof_elem0_fault_then_cross16_us_bp_mab_owner_manual_corner);
// TEST_REGISTER(ai_micro_wrong_path_store_bp_kill_then_vfof_no_ghost_forward_manual_corner);
// TEST_REGISTER(ai_micro_execute_slot_then_us_vfof_address_owner_manual_corner);
// TEST_REGISTER(ai_micro_handler_fault_between_us_vfof_bp_retry_manual_corner);
// TEST_REGISTER(ai_micro_same_va_sfence_remap_us_vfof_bp_owner_manual_corner);
// TEST_REGISTER(ai_micro_debug_action_warl_us_vfof_no_stale_manual_corner);

// AI case batch 69C-AP47: Spike/Nanhu VFOF mask-policy difftest oracle subset
TEST_REGISTER(ai_arch_vfof_mask_undisturbed_repeated_full_dump_corner);
// TEST_REGISTER(ai_micro_hs_vfof_mask_agnostic_suffix_full_dump_after_first_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_vfof_mask_undisturbed_suffix_full_dump_after_first_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_vfof_eew_matrix_mask_agnostic_suffix_after_first_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_vlseg2e8ff_mask_agnostic_suffix_fields_after_first_fault_spike_corner);
TEST_REGISTER(ai_arch_vsegment_fof_back_to_back_vl_prefix_owner_corner);
// TEST_REGISTER(ai_micro_hs_vfof_repeated_mask_policy_no_stale_suffix_context_spike_corner);
// TEST_REGISTER(ai_micro_hs_vfof_vstart_nonzero_mask_agnostic_suffix_after_first_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_vfof_second_fault_keeps_first_vl_and_mask_suffix_agnostic_spike_corner);
// TEST_REGISTER(ai_micro_hs_vfof_tail_agnostic_split_from_mask_suffix_after_first_fault_spike_corner);

// AI case batch 69D: memblock suspected bug corners, vector-store partial-commit / late-fault subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vector_store_partial_commit_then_late_page_fault_blocks_following_flows_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_repeated_late_page_fault_then_repaired_full_store_recovers_corner);

// AI case batch 69G: memblock suspected bug corners, unit-stride split-element precise-fault subset
// Target scenario: EEW=32, base=page_end-7 so element1 is split across flow0/flow1.
// Keep commented by default and compile individually with --include-commented.
// AP-11 manual RTL-focused: VLSU split-flow elemIdx/flowIdx ordering is internal MergeBuffer behavior; Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_hs_vector_load_unitstride_split_element_second_flow_page_fault_no_partial_dest_update_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_unitstride_split_element_second_flow_access_fault_no_partial_dest_update_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_unitstride_split_element_second_flow_page_fault_no_partial_faulting_element_commit_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_unitstride_split_element_second_flow_access_fault_no_partial_faulting_element_commit_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_unitstride_split_element_third_flow_page_fault_no_partial_faulting_element_commit_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_unitstride_split_fault_vstart_refreshes_from_second_to_third_flow_page_fault_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_unitstride_split_element_first_flow_page_fault_no_partial_dest_update_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_unitstride_split_element_first_flow_page_fault_no_partial_commit_corner);
// TEST_REGISTER(ai_micro_hs_vfof_unitstride_split_element_second_flow_fault_shortens_vl_no_partial_dest_corner);
// TEST_REGISTER(ai_micro_ap11_split_second_flow_fault_metadata_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap11_later_flow_replay_refresh_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap11_samepage_mask_fof_split_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap11_first_flow_and_oldest_exception_manual_smoke_corner);

// AI case batch 69H: AP-84/AP-90/AP-96/AP-97 StoreUnit/StoreQueue/StoreMisalignBuffer UnitStride split follow-up.
// Keep commented by default; these cases are black-box LinkNan/RTL stress probes for internal vstart, mask, RAW-forward, and cross-page high-paddr paths.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap84_vse32_samepage_misaligned_success_vstart_zero_corner);
// TEST_REGISTER(ai_micro_ap90_masked_vse16_inactive_first_cross_active_suffix_exact_once_corner);
// TEST_REGISTER(ai_micro_ap90_masked_vse32_inactive_first_cross_replay_suffix_refresh_corner);
TEST_REGISTER(ai_arch_unitstride_vstore_inactive_first_cross_fault_repair_corner);
// TEST_REGISTER(ai_micro_ap96_vse32_high_split_suffix_lw_gets_new_data_corner);
// TEST_REGISTER(ai_micro_ap96_scalar_unaligned_sw_forwarding_control_corner);
// TEST_REGISTER(ai_micro_ap97_cross_page_vector_store_nonadjacent_high_paddr_after_sq_pressure_corner);

// AI case batch AP-34/AP-79: VSMergeBuffer replay+exception mixed-feedback manual regression subset.
// AP-34/AP-79 are RTL timing/replay/StoreQueue-deqPtr sensitive; official Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap34_vsoxei32_exception_before_cold_replay_manual_corner);
// TEST_REGISTER(ai_micro_ap34_vsoxei32_cold_replay_before_exception_manual_corner);
// TEST_REGISTER(ai_micro_ap34_vsoxei32_same_element_cold_fault_no_replay_loop_manual_corner);
// TEST_REGISTER(ai_micro_ap34_masked_vsoxei32_inactive_fault_ignored_manual_corner);
// TEST_REGISTER(ai_micro_ap34_store_trigger_before_cold_replay_manual_corner);
// TEST_REGISTER(ai_micro_ap79_vsuxei32_misaligned_exception_before_later_cold_manual_corner);
// TEST_REGISTER(ai_micro_ap79_vsuxei32_cold_before_misaligned_exception_manual_corner);
// TEST_REGISTER(ai_micro_ap34_vsmerge_pressure_sqwrap_exception_replay_manual_corner);

// AI case batch AP-37: MisalignBuffer final-split fullVA/checkfullva manual regression subset.
// AP-37 depends on Nanhu Load/StoreMisalignBuffer final-split metadata and TLB checkfullva behavior; official Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap37_scalar_ld_final_split_sv39_sv48_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap37_scalar_sd_final_split_sv39_sv48_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap37_scalar_offset_matrix_final_split_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap37_low_split_adjusted_fullva_priority_manual_corner);
// TEST_REGISTER(ai_micro_ap37_leaf_overflow_final_split_access_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap37_vector_load_final_split_fullva_vstart_manual_corner);
// TEST_REGISTER(ai_micro_ap37_vector_store_final_split_fullva_vstart_manual_corner);
// TEST_REGISTER(ai_micro_ap37_masked_vector_final_split_fullva_ignored_manual_corner);
// TEST_REGISTER(ai_micro_ap37_tlb_warm_trigger_final_split_fullva_manual_corner);
// TEST_REGISTER(ai_micro_ap37_vs_gstage_final_split_high_ppn_manual_corner);

// AI case batch 69I: AP-100 StoreQueue non-aligned store forwarding follow-up.
// Keep commented by default; these cases are black-box LinkNan/RTL probes for StoreQueue dataInvalid/direct-forward policy.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap100_same16b_unaligned_sd_ld_overlap_no_direct_forward_corner);
// TEST_REGISTER(ai_micro_ap100_cross16b_unaligned_sd_ld_overlap_still_blocks_corner);
// TEST_REGISTER(ai_micro_ap100_aligned_sd_ld_forwarding_control_corner);
// TEST_REGISTER(ai_micro_ap100_vector_unitstride_same16b_unaligned_store_load_no_direct_forward_corner);
// TEST_REGISTER(ai_micro_ap100_unaligned_store_byte_lane_overlap_matrix_corner);
// TEST_REGISTER(ai_micro_ap100_unaligned_store_after_replay_pressure_no_stale_fast_forward_corner);

// AI case batch AP-65/AP-68/AP-82: StoreQueue full-vector generation and LsqEnqCtrl response pointer manual probes.
// These depend on Nanhu SQ/LQ internal pointer flag, commitVec, and same-cycle dispatch response timing; official Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap65_ap68_full_vector_store_marker_generation_manual_corner);
// TEST_REGISTER(ai_micro_ap65_ap68_full_vector_store_young_fault_redirect_marker_manual_corner);
// TEST_REGISTER(ai_micro_ap82_dual_strided_vector_store_dependent_load_sqidx_generation_manual_corner);
// TEST_REGISTER(ai_micro_ap82_prefill31_dual_strided_vector_store_wrap_value_manual_corner);
// TEST_REGISTER(ai_micro_ap82_dual_strided_vector_store_young_fault_redirect_manual_corner);
// TEST_REGISTER(ai_micro_ap82_dual_vector_load_lqidx_generation_manual_corner);

// AI case batch AP-69/AP-87: LoadMisalignBuffer vector load writeback and exception elemIdx manual probes.
// These depend on Nanhu MAB split-flow writeback ownership, ldVecActiveWb/orderPtr timing, and exception-buffer elemIdx metadata; official Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap69_ordered_indexed_crosspage_mab_writeback_manual_corner);
// TEST_REGISTER(ai_micro_ap69_mab_load_mode_matrix_final_writeback_manual_corner);
// TEST_REGISTER(ai_micro_ap69_aligned_vector_load_pipeline_contrast_manual_corner);
// TEST_REGISTER(ai_micro_ap87_mab_split_fault_elem1_tval_vstart_manual_corner);
// TEST_REGISTER(ai_micro_ap87_exception_buffer_mab_vs_normal_fault_priority_manual_corner);

// AI case batch 69D: memblock suspected bug corners, LoadQueueRAW oldest-uop selection approximation subset
// TEST_REGISTER(ai_micro_hs_loadqueue_raw_same_rob_two_loads_oldest_uop_selected_corner);
// TEST_REGISTER(ai_micro_hs_stld_violation_same_rob_diff_uopidx_redirect_target_correct_corner);

// AI case batch 69G-AP12: VirtualLoadQueue same-ROB vector RAR manual regression subset
// AP-12 direct RAR violation is an internal release/query timing bug; Spike is compile/basic-observation only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap12_unitstride_same_rob_multiflow_refresh_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap12_indexed_alias_ordering_mask_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap12_vector_scalar_true_younger_rar_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap12_replay_alias_exception_reuse_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap12_virtual_loadqueue_rar_direct_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap12_rar_existing_nearby_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap12_same_rob_replay_and_ignore_manual_smoke_corner);

// AI case batch 69F: memblock suspected bug corners, alias-vp-match plus ld-ld rollback-priority approximation subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_vpmatchfail_and_ldld_same_cycle_rollback_priority_correct_corner);
// TEST_REGISTER(ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner);

// AI case batch 69E: memblock suspected bug corners, vector-store earliest-element trigger-vs-access-fault subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vector_store_trigger_earlier_than_high_half_access_fault_repeated_breakpoint_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_low_half_access_fault_suppresses_later_high_half_trigger_corner);

// AI case batch 69E supplement: memblock suspected bug corners, vector-store trigger-vs-misalign subset
// 2026-04-05 source review + individual Spike recheck: same-element tie case 1/1 passed.
// The later-element misalign-suppresses-trigger probe remains commented as a target-RTL-only case:
// current Spike does not self-check unit-stride vector misalign fallback / later-element address-trigger match cleanly.
// TEST_REGISTER(ai_micro_hs_vector_store_same_elem_misalign_and_trigger_tie_breakpoint_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_earlier_misalign_suppresses_later_elem_trigger_corner);
// AP-17 manual RTL-focused: same vector element's internal second split flow must ignore address triggers.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap17_vector_load_split_element_second_flow_address_trigger_ignored_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap17_vector_store_split_element_second_flow_address_trigger_ignored_manual_smoke_corner);

// AI case batch 70: memblock suspected bug corners, store-writeback port0 scalar-vs-sqToStout arbitration subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_wb_port0_scalar_then_sq_mabuf_adjacent_window_corner);
// TEST_REGISTER(ai_micro_hs_store_wb_port0_sq_mabuf_then_scalar_no_lost_completion_corner);

// AI case batch 71: memblock suspected bug corners, LSQ multielem scalar/vector dense-window approximation subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_lsq_enq_multielem_offsets_near_full_no_overlap_or_skip_corner);
// TEST_REGISTER(ai_micro_hs_mixed_scalar_vector_memop_same_cycle_lq_sq_index_spacing_correct_corner);

// AI case batch 72: memblock suspected bug corners, shared vec-mem IQ mixed load/store feedback-binding subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_shared_vecmem_iq_vector_load_feedback_not_lost_to_neighbor_store_feedback_corner);
// TEST_REGISTER(ai_micro_hs_shared_vecmem_iq_mixed_unitstride_segment_memops_no_feedback_cross_binding_corner);

// AI case batch 73: memblock suspected bug corners, masked-all-inactive unit-stride vector memop subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_vsplit_masked_unitstride_load_all_inactive_no_fault_or_dest_clobber_corner);
TEST_REGISTER(ai_micro_hs_vsplit_masked_unitstride_store_all_inactive_no_fault_or_side_effect_corner);
TEST_REGISTER(ai_micro_hs_vsplit_vl0_load_store_no_ghost_mergebuffer_corner);

// AI case batch 74: memblock suspected bug corners, masked-all-inactive segment-store commit subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_store_all_inactive_masked_no_fault_or_side_effect_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_store_repeated_all_inactive_masked_then_live_store_recovers_corner);

// AI case batch 75: memblock suspected bug corners, sbuffer same-cacheline two-word vector-store subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_sbuffer_same_cacheline_unitstride_vector_store_two_words_no_drop_or_mix_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_same_cacheline_segment_store_two_words_no_drop_or_mix_corner);

// AI case batch 76: memblock suspected bug corners, sbuffer delayed hit-response mask-clear reuse subset
// 2026-04-04 source review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_hitresp_delayed_mask_clear_reused_entry_no_later_store_drop_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_hitresp_delayed_mask_clear_reused_entry_no_cross_line_mask_loss_corner);

// AI case batch 77: memblock suspected bug corners, alias partial-mask plus SQ-wrap approximation subset
// 2026-04-04 source review + individual Spike recheck: 3/3 passed.
// TEST_REGISTER(ai_micro_hs_store_load_alias_partial_mask_matchinvalid_sqidx_correct_corner);
// TEST_REGISTER(ai_micro_hs_store_load_alias_same_line_different_word_mask_no_wrong_forward_bytes_corner);
// TEST_REGISTER(ai_micro_hs_store_load_alias_sq_wrap_no_wrong_forward_bytes_corner);

// AI case batch 78: memblock suspected bug corners, recovered markdown registrations, arch/PBMT subset
// TEST_REGISTER(ai_arch_amo_pbmt_io_fault_then_cacheable_alias_amo_recovers_same_pa_corner);
// TEST_REGISTER(ai_arch_amo_pbmt_io_repeated_fault_then_scalar_load_store_recovers_corner);
TEST_REGISTER(ai_arch_amo_mprv_u_same_page_pmp_repeated_fault_then_restore_recovers_corner);
TEST_REGISTER(ai_arch_amo_mprv_u_same_page_pmp_repeated_fault_then_sfence_restore_recovers_corner);
// TEST_REGISTER(ai_arch_amo_pbmt_nc_fault_then_cacheable_alias_amo_recovers_same_pa_corner);
// TEST_REGISTER(ai_arch_amo_pbmt_nc_repeated_fault_then_scalar_load_store_recovers_corner);
// No-H target note: the earlier temporary ai_arch_hyper_* cross-16B probes have been withdrawn from ai_test_cases/ and are not maintained in the current Spike/Makefile configuration.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_cacheable_repeated_misaligned_then_aligned_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_cacheable_repeated_misaligned_then_aligned_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_pbmt_nc_repeated_misaligned_then_aligned_load_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_pbmt_io_repeated_access_fault_then_aligned_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_pbmt_nc_repeated_misaligned_then_aligned_store_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_pbmt_io_repeated_access_fault_then_aligned_access_recovers_corner);

// AI case batch 79: memblock suspected bug corners, recovered markdown registrations, scalar exception ordering subset
// TEST_REGISTER(ai_micro_hs_load_exception_pair_oldest_access_fault_then_misaligned_corner);
// TEST_REGISTER(ai_micro_hs_load_exception_pair_oldest_misaligned_then_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_store_exception_pair_oldest_access_fault_then_misaligned_corner);
// TEST_REGISTER(ai_micro_hs_store_exception_pair_oldest_misaligned_then_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_store_exception_pair_oldest_load_access_fault_then_store_misaligned_corner);
// TEST_REGISTER(ai_micro_hs_load_store_exception_pair_oldest_load_misaligned_then_store_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_store_load_exception_pair_oldest_store_access_fault_then_load_misaligned_corner);
// TEST_REGISTER(ai_micro_hs_store_load_exception_pair_oldest_store_misaligned_then_load_access_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_not_blocked_by_unrelated_older_unaligned_store_corner);
// TEST_REGISTER(ai_micro_hs_load_pbmt_io_repeated_bus_error_then_normal_scalar_recovers_corner);
// TEST_REGISTER(ai_micro_hs_load_pbmt_nc_repeated_bus_error_then_normal_scalar_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_io_repeated_bus_error_then_normal_scalar_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_nc_repeated_bus_error_then_normal_scalar_recovers_corner);
// TEST_REGISTER(ai_micro_hs_store_cross16_head_waits_for_two_databuffer_ports_without_hang_corner);
// TEST_REGISTER(ai_micro_hs_unaligned_store_false_datainvalid_sqidx_not_reported_corner);

// AI case batch 80: memblock suspected bug corners, recovered markdown registrations, sbuffer/cbo/fence subset
// 2026-04-26 recheck: the PBMT=NC members compile and pass Spike, but the target is RTL fence/drain
// ordering with PBMT=NC uncache traffic mixed with cacheable AMO/CMO/sbuffer activity.
// TEST_REGISTER(ai_micro_hs_atomic_boundary_store_drain_no_early_start_from_stale_empty_corner);
// TEST_REGISTER(ai_micro_hs_atomic_boundary_store_drain_repeated_no_false_empty_or_extra_wait_corner);
// TEST_REGISTER(ai_micro_hs_back_to_back_cbo_zero_busy_sbuffer_no_stuck_or_wrong_line_zero_corner);
// TEST_REGISTER(ai_micro_hs_cbo_zero_delayed_sbuffer_fire_not_rebound_to_new_sq_head_corner);
// TEST_REGISTER(ai_micro_hs_cbo_zero_lineflag_wildcard_no_wrong_forward_to_other_word_in_line_corner);
// TEST_REGISTER(ai_micro_hs_fence_waits_for_sbuffer_and_pbmt_io_uncache_empty_corner);
// TEST_REGISTER(ai_micro_hs_fence_waits_for_sbuffer_and_pbmt_nc_uncache_empty_corner);
// TEST_REGISTER(ai_micro_hs_fence_with_cacheable_amo_and_pbmt_io_store_same_window_corner);
// TEST_REGISTER(ai_micro_hs_fence_with_cacheable_amo_and_pbmt_nc_store_same_window_corner);
// TEST_REGISTER(ai_micro_hs_fence_with_cbo_zero_and_pbmt_io_store_same_window_corner);
// TEST_REGISTER(ai_micro_hs_fence_with_cbo_zero_and_pbmt_nc_store_same_window_corner);
// TEST_REGISTER(ai_micro_hs_multiple_cbo_zero_same_window_no_merged_or_lost_flush_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_partial_store_miss_full_overwrite_complement_line_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_evict_waits_for_same_entry_refill_write_completion_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_refill_and_evict_same_entry_no_stale_mask_or_data_to_dcache_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_refill_to_mp_resp_uses_coherent_line_snapshot_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_same_block_inflight_mask_clears_without_stuck_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_same_block_inflight_then_new_store_ordered_correctly_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_same_entry_update_during_refill_to_mp_no_mixed_epoch_data_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_same_pa_diff_va_alias_triggers_clean_drain_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_sameblock_inflight_replacement_no_hotline_starvation_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_sameblock_pressure_no_wrong_victim_or_evict_oscillation_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_synonym_alias_no_stale_forward_after_drain_corner);
// TEST_REGISTER(ai_micro_hs_sq_head_advance_after_cbo_zero_fire_no_false_or_missed_flush_trigger_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_cbom_sqdeq_not_corrupted_by_sbuffer_overlap_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_cbozero_sbuffer_adjacent_cycle_collision_corner);
// TEST_REGISTER(ai_micro_hs_uncache_load_store_back_to_back_response_not_swapped_corner);
// TEST_REGISTER(ai_micro_hs_uncache_load_store_same_window_no_starvation_corner);
// TEST_REGISTER(ai_micro_hs_pbmt_nc_burst_load_store_resource_pressure_recovers_corner);

// AI case batch 81: memblock suspected bug corners, recovered markdown registrations, load-queue and store-forward subset
// TEST_REGISTER(ai_micro_hs_loadqueue_raw_late_revoke_no_free_of_reallocated_slot_corner);
// TEST_REGISTER(ai_micro_hs_raw_tracker_slot_reuse_old_revoke_must_not_kill_new_load_corner);
// TEST_REGISTER(ai_micro_hs_sq_forward_one_cycle_late_result_matches_query_epoch_corner);
// TEST_REGISTER(ai_micro_hs_sq_forward_younger_full_store_no_next_epoch_data_leak_corner);
// TEST_REGISTER(ai_micro_hs_whole_line_store_and_partial_store_same_line_forward_by_byte_age_correctly_corner);

// AI case batch 82: memblock suspected bug corners, recovered markdown registrations, vector PBMT fault/recovery subset
// 2026-04-26 recheck: the PBMT=NC vector-store members are manual RTL-only for the same reason as
// batch 65: Spike can perform/split the vector store instead of taking the target RTL unsupported-path
// store-access-fault route.
// TEST_REGISTER(ai_micro_hs_vector_load_pbmt_io_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_pbmt_io_repeated_access_fault_preserves_register_corner);
// TEST_REGISTER(ai_micro_hs_vector_load_pbmt_io_repeated_access_fault_then_scalar_same_alias_recovers_corner);
// Withdrawn: the aligned PBMT=NC vector-load recovery variant over-constrained legal aligned vector access.
// TEST_REGISTER(ai_micro_hs_vector_store_pbmt_io_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_pbmt_io_repeated_access_fault_stable_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_pbmt_io_repeated_access_fault_then_scalar_same_alias_recovers_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_pbmt_nc_misaligned_repeated_access_fault_then_scalar_same_alias_recovers_corner);
// Withdrawn: the aligned PBMT=NC vector-store stable variant over-constrained legal aligned vector access.
// Withdrawn: the aligned PBMT=NC vector-store recovery variant over-constrained legal aligned vector access.
// TEST_REGISTER(ai_micro_ap77_vector_pbmt_macro_surface_manual_smoke_corner);
// AP-33/AP-56 manual: vector split PBMT/IO misalign cause matrix is NHV5.1AP profile and LinkNan RTL dependent.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap33_vector_load_second_split_pbmt_nc_access_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap33_vector_store_second_split_pbmt_nc_access_fault_no_side_effect_manual_corner);
// TEST_REGISTER(ai_micro_ap56_vector_load_second_split_pbmt_io_isvec_access_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap56_masked_off_second_split_pbmt_io_no_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap56_vector_load_second_split_pbmt_nc_fault_repair_success_manual_corner);
// TEST_REGISTER(ai_micro_ap33_segment_load_second_field_pbmt_nc_access_fault_manual_corner);
// TEST_REGISTER(ai_micro_ap33_first_split_pbmt_nc_priority_manual_corner);
// TEST_REGISTER(ai_micro_ap33_segment_store_second_field_pbmt_nc_no_side_effect_manual_corner);

// AI case batch 83: memblock suspected bug corners, recovered markdown registrations, MPRV prefetch subset
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_same_page_deny_then_sfence_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_same_page_deny_then_sfence_restore_recovers_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_same_page_repeated_deny_then_sfence_restore_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_prefetch_mprv_u_same_page_repeated_deny_then_sfence_restore_recovers_corner);

// AI case batch 84: memblock suspected bug corners, replay-head misalign false schedule/free under younger direct misalign pressure
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_replayq_head_misalign_not_freed_when_direct_exu_misalign_wins_misalignbuffer_corner);
TEST_REGISTER(ai_micro_hs_shared_misalignbuffer_misalign_mux_no_false_schedule_or_free_of_replay_head_corner);

// AI case batch 85: memblock suspected bug corners, replay-port cold-counter ownership handoff under direct misalign pressure
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_replay_port_cold_counter_not_inherited_by_next_misalign_replay_owner_corner);
TEST_REGISTER(ai_micro_hs_replay_port_cold_counter_switch_owner_adjacent_preserved_corner);

// AI case batch 86: memblock suspected bug corners, vector late-writeback owner binding versus younger vector store
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_vector_store_late_fault_vecwriteback_owner_not_rebound_to_younger_vector_store_corner);
TEST_REGISTER(ai_micro_hs_vector_store_late_fault_vecwriteback_owner_not_rebound_to_younger_vector_store_adjacent_preserved_corner);

// AI case batch 87: memblock suspected bug corners, partial-inactive segment-store tail ownership versus adjacent younger store
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_partial_inactive_tail_not_rebound_to_adjacent_younger_store_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_partial_inactive_tail_repeated_same_target_full_rewrite_recovers_corner);

// AI case batch 87A: memblock suspected bug corners, CBO zero/inval owner handoff to narrow scalar lane paths
// TEST_REGISTER(ai_micro_hs_cbozero_then_narrow_scalar_stores_no_stale_zero_owner_or_byte_mask_drop_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_then_narrow_scalar_loads_no_stale_zero_owner_or_signed_lane_leak_corner);
// R49/R105: SFENCE.VMA + same-VA CBO.INVAL then scalar load/read relies on CBO line/refill ownership.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_hs_cboinval_then_scalar_load_no_stale_inval_owner_or_refill_payload_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_scalar_store_no_stale_inval_owner_or_store_payload_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_narrow_scalar_loads_no_stale_inval_owner_or_signed_lane_leak_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_narrow_scalar_stores_no_stale_inval_owner_or_byte_mask_drop_corner);
// TEST_REGISTER(ai_micro_hs_narrow_scalar_stores_then_cbozero_no_stale_mask_or_zero_owner_corner);
// TEST_REGISTER(ai_micro_hs_narrow_scalar_loads_then_cbozero_no_lost_lane_or_zero_owner_corner);
// TEST_REGISTER(ai_micro_hs_narrow_scalar_stores_then_cboinval_no_stale_mask_or_inval_owner_corner);
// TEST_REGISTER(ai_micro_hs_narrow_scalar_loads_then_cboinval_no_lost_lane_or_inval_owner_corner);
// TEST_REGISTER(ai_micro_hs_cbom_then_narrow_scalar_stores_no_stale_cmo_owner_or_byte_mask_drop_corner);
// TEST_REGISTER(ai_micro_hs_cbom_then_narrow_scalar_loads_no_stale_cmo_owner_or_signed_lane_leak_corner);
// TEST_REGISTER(ai_micro_hs_narrow_scalar_stores_then_cbom_no_stale_mask_or_cmo_owner_corner);
// TEST_REGISTER(ai_micro_hs_narrow_scalar_loads_then_cbom_no_lost_lane_or_cmo_owner_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_cbom_no_stale_inval_or_cmo_owner_corner);
// TEST_REGISTER(ai_micro_hs_cbom_then_cboinval_no_stale_cmo_or_inval_owner_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_cbozero_no_stale_inval_or_zero_owner_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_then_cboinval_no_stale_zero_or_inval_owner_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_cbom_then_cbozero_no_stale_triple_cmo_owner_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_then_cbom_then_cboinval_no_stale_triple_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_cboinval_then_cbozero_then_cbom_no_stale_triple_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_cbom_then_cbozero_then_cboinval_no_stale_triple_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_cbozero_then_cboinval_then_cbom_no_stale_triple_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_cbom_then_cboinval_then_cbozero_no_stale_triple_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_two_block_cbozero_cboinval_cbom_no_cross_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_two_block_cboinval_cbom_cbozero_no_cross_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_two_block_dual_cbozero_cmo_no_lost_zero_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_two_block_cboinval_cbom_dual_preserve_no_cross_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_round_robin_cmo_no_cross_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_cmo_younger_scalar_store_no_lost_payload_corner);
TEST_REGISTER(ai_arch_cbozero_dispatch_younger_mixed_mem_owner_corner);
TEST_REGISTER(ai_micro_hs_cbom_two_block_finish_pending_no_cross_release_stores_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_cboinval_store_cbom_no_lost_payload_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_cmo_dual_younger_store_no_cross_payload_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_older_stores_then_cmo_no_cross_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_no_stale_middle_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_no_stale_zero_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_no_stale_finish_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_no_stale_zero_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_no_stale_inval_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_no_stale_clean_owner_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_three_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_post_zero_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_post_clean_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_mid_clean_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_final_zero_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_final_zero_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_store_inval_four_generation_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_store_cbom_four_generation_mid_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_store_inval_four_generation_mid_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_store_cbom_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_zero_store_cbom_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_inval_store_cbom_four_generation_mid_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_cbom_store_inval_four_generation_mid_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_zero_store_inval_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_store_inval_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_store_cbom_four_generation_post_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_store_cbom_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_store_inval_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_zero_store_inval_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_zero_store_cbom_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_inval_store_cbom_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_cbom_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_cbom_store_cbom_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_inval_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_inval_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_cbom_store_cbom_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_inval_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_cbom_store_cbom_four_generation_post_second_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_inval_store_inval_four_generation_post_second_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_cbom_store_inval_four_generation_post_second_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_inval_store_cbom_four_generation_post_second_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_zero_store_cbom_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_zero_store_inval_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_zero_store_cbom_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_zero_store_inval_four_generation_late_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_inval_store_cbom_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_cbom_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_inval_store_cbom_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_zero_store_inval_four_generation_post_third_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_zero_store_cbom_four_generation_post_third_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_inval_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_inval_store_inval_four_generation_post_first_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_inval_store_cbom_four_generation_post_first_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_store_cbom_four_generation_post_first_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_store_inval_four_generation_post_first_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_cbom_store_inval_four_generation_post_first_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_cbom_store_cbom_four_generation_post_first_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_zero_store_inval_four_generation_post_second_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_zero_store_cbom_four_generation_post_second_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_inval_store_inval_four_generation_mid_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_store_cbom_four_generation_mid_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_cbom_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_cbom_store_inval_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_cbom_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_cbom_store_cbom_four_generation_no_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_zero_store_zero_four_generation_final_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_zero_store_inval_four_generation_final_post_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_zero_store_cbom_four_generation_final_post_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_store_inval_four_generation_post_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_cbom_store_cbom_four_generation_post_zero_corner);
TEST_REGISTER(ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_store_zero_four_generation_final_zero_corner);

// AI case batch 87B: memblock high-risk table backfill, register real source cases that were only documented.
// Keep commented so default gate membership stays unchanged; use --include-commented for targeted compile.
// TEST_REGISTER(ai_micro_hs_cboinval_then_fence_no_stale_inval_owner_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_lrsc_no_stale_inval_owner_or_reservation_loss_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_masked_vector_load_all_inactive_no_dest_clobber_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_masked_vector_load_partial_active_no_lane_mix_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_masked_vector_store_all_inactive_no_side_effect_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_masked_vector_store_partial_active_no_lane_mix_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cboinval_then_normal_vector_no_stale_inval_owner_or_payload_corner);
// TEST_REGISTER(ai_micro_hs_cbom_then_fence_no_stale_cmo_flush_owner_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_then_masked_vector_load_all_inactive_no_dest_clobber_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_cbozero_then_masked_vector_load_partial_active_no_lane_mix_or_marker_drop_corner);
// TEST_REGISTER(ai_micro_hs_fence_then_cboinval_no_stale_fence_flush_owner_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_fence_then_cbom_no_stale_fence_flush_owner_or_cmo_drop_corner);
// TEST_REGISTER(ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner);
// TEST_REGISTER(ai_micro_hs_ldld_same_line_different_word_release_no_false_replay_corner);
// TEST_REGISTER(ai_micro_hs_loadqueue_raw_dual_store_violation_two_targets_no_cross_flush_corner);
// TEST_REGISTER(ai_micro_hs_lrsc_then_cboinval_no_stale_reservation_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_masked_vector_load_all_inactive_then_cboinval_no_dest_clobber_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_masked_vector_load_all_inactive_then_cbozero_no_dest_clobber_or_zero_drop_corner);
// TEST_REGISTER(ai_micro_hs_masked_vector_load_partial_active_then_cboinval_no_lane_mix_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_masked_vector_store_all_inactive_then_cboinval_no_side_effect_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_masked_vector_store_partial_active_then_cboinval_no_lane_mix_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_mebsingleport_amo_vsegment_fault_switch_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_hs_mebsingleport_cbo_amo_fault_switch_no_stale_cause_tval_corner);
// TEST_REGISTER(ai_micro_hs_mebsingleport_cbo_vsegment_fault_switch_no_stale_cause_tval_corner);
// TEST_REGISTER(ai_micro_hs_normal_vector_then_cboinval_no_stale_vector_writeback_or_inval_drop_corner);
// TEST_REGISTER(ai_micro_hs_replay_escape_unrelated_vector_wb_does_not_clear_scalar_escape_corner);
// TEST_REGISTER(ai_micro_hs_storedata_writeback_stream_interleaved_addr_no_std_ready_drop_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_dtlb_handoff_after_scalar_remap_no_stale_translation_corner);
// TEST_REGISTER(ai_micro_hs_wrong_path_amo_redirect_no_stale_atomic_state_or_spurious_fault_corner);
// TEST_REGISTER(ai_micro_hs_wrong_path_cbozero_redirect_no_stale_zero_or_store_wb_corner);
// TEST_REGISTER(ai_micro_hs_wrong_path_vector_load_store_redirect_no_spurious_fault_or_payload_leak_corner);
// TEST_REGISTER(ai_micro_hs_wrong_path_vsegment_redirect_no_stale_flag_or_payload_leak_corner);

// AI case batch 88: memblock suspected bug corners, LSQ enqueue redirect recovery under immediate mixed memop pressure
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_lsq_enq_redirect_recovery_no_duplicate_or_lost_lq_sq_index_corner);
// TEST_REGISTER(ai_micro_hs_lsq_enq_multicycle_redirect_near_wrap_accept_and_ptrs_stay_consistent_corner);

// AI case batch 89: memblock suspected bug corners, vsegment boundary store-drain stale-empty window
// 2026-04-04 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_boundary_store_drain_no_false_empty_or_extra_wait_corner);

// AI case batch 90: memblock suspected bug corners, load misalign-buffer writeback versus adjacent aligned-load shared ldout
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_load_cross_16b_mab_writeback_shared_ldout_no_drop_corner);
TEST_REGISTER(ai_micro_hs_load_cross_16b_mab_writeback_shared_ldout_no_cross_bind_adjacent_preserved_corner);

// AI case batch 91: memblock suspected bug corners, atomic entry versus nearby scalar-load writeback
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_atomic_entry_while_nearby_load_wb_no_lost_ldout_corner);
// TEST_REGISTER(ai_micro_hs_atomic_entry_while_nearby_load_wb_adjacent_preserved_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_memblock_ldaout_cases.c
// Same-cycle load0/AMO ldaOut arbitration needs scoped stress or RTL evidence; keep these direct black-box probes commented.
// TEST_REGISTER(ai_micro_hs_amo_then_scalar_load_no_stale_atomic_ldaout_or_load_payload_corner);
// TEST_REGISTER(ai_micro_hs_lrsc_then_scalar_load_no_stale_reservation_ldaout_or_load_payload_corner);
// TEST_REGISTER(ai_micro_hs_scalar_load_then_lrsc_no_lost_load0_or_stale_reservation_ldaout_corner);

// AI case batch 92: memblock suspected bug corners, replay-like load fallback plus nearby cross16 misalign writeback
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_replay_like_load_with_nearby_cross16_mab_wb_no_lost_completion_corner);
// TEST_REGISTER(ai_micro_hs_replay_like_load_with_nearby_cross16_mab_wb_adjacent_preserved_corner);

// AI case batch 93: memblock suspected bug corners, consecutive cross-page split-store upper-half paddr reuse
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_store_cross_page_reused_upper_half_paddr_not_rebound_to_next_pair_corner);
TEST_REGISTER(ai_micro_hs_store_cross_page_reused_upper_half_paddr_not_rebound_to_next_pair_adjacent_preserved_corner);

// AI case batch 94: memblock suspected bug corners, consecutive cross-16 load offset/width merge-template refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_load_cross_16b_offset_width_switch_no_stale_merge_template_corner);
TEST_REGISTER(ai_micro_hs_load_cross_16b_offset_width_switch_no_stale_merge_template_adjacent_preserved_corner);

// AI case batch 95: memblock suspected bug corners, consecutive cross-16 load signedness/width data-select refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_load_cross_16b_signedness_switch_no_stale_data_select_corner);
TEST_REGISTER(ai_micro_hs_load_cross_16b_signedness_switch_no_stale_data_select_adjacent_preserved_corner);

// AI case batch 96: memblock suspected bug corners, consecutive cross-16 store offset/width split-template refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_store_cross_16b_offset_width_switch_no_stale_split_template_corner);
TEST_REGISTER(ai_micro_hs_store_cross_16b_offset_width_switch_no_stale_split_template_adjacent_preserved_corner);

// AI case batch 97: memblock suspected bug corners, same-16B direct misaligned-store offset/width shift-template refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_store_same_16b_offset_width_switch_no_stale_direct_shift_mask_corner);
TEST_REGISTER(ai_micro_hs_store_same_16b_offset_width_switch_no_stale_direct_shift_mask_adjacent_preserved_corner);

// AI case batch 98: memblock suspected bug corners, same-16B direct misaligned-load offset/width data-select refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_load_same_16b_offset_width_switch_no_stale_direct_data_select_corner);
TEST_REGISTER(ai_micro_hs_load_same_16b_offset_width_switch_no_stale_direct_data_select_adjacent_preserved_corner);

// AI case batch 99: memblock suspected bug corners, same-16B direct misaligned-load signedness data-select refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_load_same_16b_signedness_switch_no_stale_direct_data_select_corner);
TEST_REGISTER(ai_micro_hs_load_same_16b_signedness_switch_no_stale_direct_data_select_adjacent_preserved_corner);

// AI case batch 100: memblock suspected bug corners, same-16B direct misaligned store/load overlap merged-byte binding
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_corner);
// TEST_REGISTER(ai_micro_hs_same_16b_store_load_overlap_no_wrong_forward_or_stale_merge_adjacent_preserved_corner);

// AI case batch 101: memblock suspected bug corners, same-16B direct store-then-load signedness data-select refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_same_16b_store_then_signed_unsigned_load_no_stale_data_select_corner);
// TEST_REGISTER(ai_micro_hs_same_16b_store_then_signed_unsigned_load_no_stale_data_select_adjacent_preserved_corner);

// AI case batch 102: memblock suspected bug corners, same-16B direct store/store overlap byte-age merge
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_corner);
// TEST_REGISTER(ai_micro_hs_same_16b_store_store_overlap_no_stale_shift_or_byte_age_merge_adjacent_preserved_corner);

// AI case batch 103: memblock suspected bug corners, same-16B direct/aligned store path switch merge
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_corner);
// TEST_REGISTER(ai_micro_hs_same_16b_store_path_switch_no_stale_direct_or_aligned_merge_adjacent_preserved_corner);

// AI case batch 104: memblock suspected bug corners, same-16B direct/aligned load path switch select
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_same_16b_load_path_switch_no_stale_direct_or_aligned_select_corner);
// TEST_REGISTER(ai_micro_hs_same_16b_load_path_switch_no_stale_direct_or_aligned_select_adjacent_preserved_corner);

// AI case batch 105: memblock suspected bug corners, same-16B direct/aligned store switch followed by immediate overlap load
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_corner);
// TEST_REGISTER(ai_micro_hs_same_16b_store_path_switch_then_overlap_load_no_stale_merge_or_select_adjacent_preserved_corner);

// AI case batch 106: memblock suspected bug corners, cbo.zero plus younger partial-store mixed-source small-load shaping
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_cbo_zero_partial_store_small_load_mixed_source_no_select_mixup_corner);
TEST_REGISTER(ai_micro_hs_cbo_zero_partial_store_small_load_mixed_source_adjacent_preserved_corner);

// AI case batch 107: memblock suspected bug corners, same-block inflight plus younger valid mixed-window forward
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_inflight_valid_mixed_load_forward_by_byte_age_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_inflight_valid_mixed_load_forward_by_byte_age_adjacent_preserved_corner);

// AI case batch 107 supplement: memblock suspected bug corners, same-16B direct cacheable misalign trigger cleanup plus trigger-vs-fault priority
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_execute_and_address_trigger_repeated_breakpoint_then_completion_corner);
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner);
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner);

// AI case batch 108: memblock suspected bug corners, same-block different-word inflight plus younger valid no cross-word bleed
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_diff_word_inflight_valid_no_cross_word_forward_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_diff_word_inflight_valid_no_cross_word_forward_adjacent_preserved_corner);

// AI case batch 109: memblock suspected bug corners, same mixed word back-to-back load query-epoch select
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_same_mixed_word_back_to_back_loads_no_stale_query_select_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_same_mixed_word_back_to_back_loads_no_stale_query_select_adjacent_preserved_corner);

// AI case batch 110: memblock suspected bug corners, same-block waiting-valid second merge cumulative byte age
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_waiting_valid_second_merge_preserves_byte_age_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_waiting_valid_second_merge_preserves_byte_age_adjacent_preserved_corner);

// AI case batch 111: memblock suspected bug corners, same-block waiting-valid multiword accumulation
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_waiting_valid_multiword_merge_keeps_all_words_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_same_block_waiting_valid_multiword_merge_keeps_all_words_adjacent_preserved_corner);

// AI case batch 112: memblock suspected bug corners, repeated same-block reinflight handoff with younger same-word merge
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_sbuffer_reinflight_handoff_then_younger_same_word_merge_preserves_generation_corner);
TEST_REGISTER(ai_micro_hs_sbuffer_reinflight_handoff_then_younger_same_word_merge_adjacent_preserved_corner);

// AI case batch 113: memblock suspected bug corners, PBMT=NC head request not rebound by next cacheable pointer advance
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// 2026-04-26 recheck: still Spike-passing, but target remains RTL StoreQueue PBMT/cacheable-entry ownership.
// TEST_REGISTER(ai_micro_hs_storequeue_pbmt_nc_req_not_rebound_when_next_cacheable_entry_advances_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_pbmt_nc_req_not_rebound_when_next_cacheable_cmo_advances_corner);

// AI case batch 114: memblock suspected bug corners, older scalar sbuffer handoff must not drop a later dense younger rewrite
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_older_sbuffer_handoff_then_younger_rewrite_not_dropped_corner);
TEST_REGISTER(ai_micro_hs_storequeue_older_sbuffer_handoff_then_younger_rewrite_adjacent_preserved_corner);

// AI case batch 115: memblock suspected bug corners, databuffer two-pair turnover must not swap or drop later stores
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_databuffer_two_pair_turnover_no_swap_or_drop_corner);
TEST_REGISTER(ai_micro_hs_databuffer_two_pair_turnover_adjacent_preserved_corner);

// AI case batch 116: memblock suspected bug corners, cross16 pseudo-second-half must not rebind the next aligned batch
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_pseudo_second_half_then_younger_aligned_batch_no_drop_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_pseudo_second_half_then_younger_aligned_batch_adjacent_preserved_corner);

// AI case batch 117: memblock suspected bug corners, cross16 split high half must not overwrite a younger same-word rewrite
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_high_half_then_younger_same_word_rewrite_keeps_youngest_data_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_high_half_then_younger_same_word_rewrite_adjacent_preserved_corner);

// AI case batch 118: memblock suspected bug corners, cross16 split low-half owner must not survive a younger same-word rewrite
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_low_half_then_younger_same_word_rewrite_keeps_youngest_data_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_low_half_then_younger_same_word_rewrite_adjacent_preserved_corner);

// AI case batch 119: memblock suspected bug corners, consecutive cross16 split-stores must not swap or mix boundary ownership during databuffer turnover
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_boundaries_no_cross_bind_or_drop_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_boundaries_adjacent_preserved_corner);

// AI case batch 120: memblock suspected bug corners, same-boundary consecutive cross16 split-stores must refresh to only the latest generation
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_keeps_latest_split_image_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_adjacent_preserved_corner);

// AI case batch 121: memblock suspected bug corners, same-boundary cross16 width-switch must refresh split template without stale older placement
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_width_switch_no_stale_template_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_width_switch_adjacent_preserved_corner);

// AI case batch 122: memblock suspected bug corners, same-boundary wide-then-narrow cross16 split-stores must merge by byte age without stale wide-mask carryover
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_narrower_switch_merges_by_byte_age_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_narrower_switch_adjacent_preserved_corner);

// AI case batch 123: memblock suspected bug corners, three consecutive same-boundary cross16 split-stores must fully refresh every generation without stale older template leakage
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_three_shape_refresh_no_stale_older_generation_corner);
// TEST_REGISTER(ai_micro_hs_storequeue_consecutive_cross16_same_boundary_three_shape_adjacent_preserved_corner);

// AI case batch 124: memblock suspected bug corners, vector local-misalign feedback and StoreMisalignBuffer vector writeback must not collide or lose completion identity
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vector_store_misalign_direct_feedback_and_mabuf_feedback_no_collision_bug_corner);
// TEST_REGISTER(ai_micro_hs_vector_store_misalign_direct_feedback_and_mabuf_feedback_adjacent_preserved_corner);

// AI case batch 125: memblock suspected bug corners, older aligned databuffer batch must not rebind a younger cross16 split-store during slot reuse
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_databuffer_aligned_pair_then_cross16_split_no_cross_bind_or_drop_corner);
TEST_REGISTER(ai_micro_hs_databuffer_aligned_pair_then_cross16_split_adjacent_preserved_corner);

// AI case batch 126: memblock suspected bug corners, older aligned low/high batch on one boundary must merge with a younger same-boundary cross16 split-store strictly by byte age during slot reuse
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_databuffer_aligned_pair_then_same_boundary_cross16_merges_by_byte_age_corner);
TEST_REGISTER(ai_micro_hs_databuffer_aligned_pair_then_same_boundary_cross16_adjacent_preserved_corner);

// AI case batch 127: memblock suspected bug corners, older same-boundary cross16 split-store must not survive a younger aligned low/high pair on that boundary during slot reuse
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_databuffer_same_boundary_cross16_then_aligned_pair_fully_refreshes_boundary_corner);
TEST_REGISTER(ai_micro_hs_databuffer_same_boundary_cross16_then_aligned_pair_adjacent_preserved_corner);

// AI case batch 128: memblock suspected bug corners, three same-boundary generations in split-aligned-split order must not leak the oldest split-store template past the middle aligned refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_databuffer_same_boundary_split_aligned_split_no_oldest_leak_corner);
TEST_REGISTER(ai_micro_hs_databuffer_same_boundary_split_aligned_split_adjacent_preserved_corner);

// AI case batch 129: memblock suspected bug corners, three same-boundary generations in aligned-split-aligned order must not let the middle split-store survive the final aligned refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_databuffer_same_boundary_aligned_split_aligned_no_middle_leak_corner);
TEST_REGISTER(ai_micro_hs_databuffer_same_boundary_aligned_split_aligned_adjacent_preserved_corner);

// AI case batch 130: memblock suspected bug corners, same-target non-unit-stride vector store generations in partial-full-partial order must not leak the oldest inactive tail past the middle full refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_full_partial_no_old_tail_leak_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_full_partial_adjacent_preserved_corner);

// AI case batch 131: memblock suspected bug corners, same-target non-unit-stride vector store generations in full-partial-full order must not let the middle inactive-tail state suppress the final full rewrite
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_full_partial_full_no_middle_tail_leak_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_full_partial_full_adjacent_preserved_corner);

// AI case batch 132: memblock suspected bug corners, same-target non-unit-stride vector store generations in partial-partial-full order must not accumulate stale inactive tails past the final full rewrite
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_partial_full_no_tail_accumulation_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_partial_full_adjacent_preserved_corner);

// AI case batch 133: memblock suspected bug corners, same-target non-unit-stride vector store generations in full-partial-partial order must preserve exact byte age across repeated inactive-tail windows
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_full_partial_partial_keeps_precise_byte_age_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_full_partial_partial_adjacent_preserved_corner);

// AI case batch 134: memblock suspected bug corners, same-target non-unit-stride vector store generations in partial-partial-partial order must preserve exact byte age across repeated inactive-tail owners
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_partial_partial_keeps_precise_byte_age_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_partial_partial_adjacent_preserved_corner);

// AI case batch 135: memblock suspected bug corners, same-target non-unit-stride vector store generations in partial-full-full order must not let an old inactive tail survive repeated full rewrites
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_full_full_no_stale_tail_after_double_full_rewrite_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_partial_full_full_adjacent_preserved_corner);

// AI case batch 136: memblock suspected bug corners, same-target non-unit-stride vector store generations in full-full-partial order must not let the oldest full image survive the middle full refresh
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_full_full_partial_no_oldest_full_leak_corner);
// TEST_REGISTER(ai_micro_hs_vsegment_same_target_full_full_partial_adjacent_preserved_corner);

// AI case batch 137: memblock suspected bug corners, cross16 scalar delayed dequeue must not consume an unrelated vector writeback as split completion
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_not_false_split_completion_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_not_false_split_completion_adjacent_preserved_corner);

// AI case batch 138: memblock suspected bug corners, cross16 scalar delayed completion must not be falsely freed by unrelated vector writeback before the next same-boundary split generation
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_next_split_keeps_latest_image_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_next_split_adjacent_preserved_corner);

// AI case batch 139: memblock suspected bug corners, cross16 scalar delayed completion must not be falsely freed by unrelated vector writeback and cross-bind the next boundary
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_no_cross_bind_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_adjacent_preserved_corner);

// AI case batch 140: memblock suspected bug corners, cross16 scalar delayed completion must not carry an old split template into a different-width next boundary after unrelated vector writeback
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_width_switch_no_stale_template_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_width_switch_adjacent_preserved_corner);

// AI case batch 141: memblock suspected bug corners, cross16 scalar delayed completion must not carry a wide split template into a narrower next boundary after unrelated vector writeback
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_narrower_switch_merges_by_byte_age_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_narrower_switch_adjacent_preserved_corner);

// AI case batch 142: memblock suspected bug corners, cross16 scalar delayed completion must not lose byte age across a same-boundary three-shape refresh after unrelated vector writeback
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_three_shape_refresh_no_stale_older_generation_corner);
TEST_REGISTER(ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_three_shape_adjacent_preserved_corner);

// AI case batch 143: memblock suspected bug corners, VirtualLoadQueue redirect-adjacent enqueue plus late MMIO paddr update must not corrupt surviving slots
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_virtual_lq_redirect_adjacent_enq_no_ptr_corruption_corner);
// TEST_REGISTER(ai_micro_hs_virtual_lq_mmio_late_paddr_update_after_redirect_hits_right_slot_corner);

// AI case batch 144: memblock suspected bug corners, older PBMT=NC store bus-error exception must keep its own virtual address instead of inheriting younger head state
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_pbmt_nc_bus_error_reports_older_va_under_younger_store_pressure_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_nc_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner);

// AI case batch 145: memblock suspected bug corners, a faulted PBMT=NC uncache store must not leak stale exception state into the next good PBMT=NC store episode
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_pbmt_nc_bus_error_then_good_pbmt_nc_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_nc_bus_error_then_repeated_good_pbmt_nc_store_no_false_fault_corner);

// AI case batch 145 supplement: memblock suspected bug corners, a faulted PBMT=NC/IO uncache load must not leak stale fault state into the next good same-alias load episode
// TEST_REGISTER(ai_micro_hs_load_pbmt_nc_bus_error_then_same_alias_good_pbmt_nc_load_clears_stale_fault_corner);
// TEST_REGISTER(ai_micro_hs_load_pbmt_io_bus_error_then_same_alias_good_pbmt_io_load_clears_stale_fault_corner);

// AI case batch 145 supplement 2: memblock suspected bug corners, a faulted PMA=Device load/store must not leak stale fault or exception state into the next good same-alias MMIO episode
// TEST_REGISTER(ai_micro_hs_load_mmio_bus_error_then_same_alias_good_mmio_load_clears_stale_fault_corner);
// TEST_REGISTER(ai_micro_hs_store_mmio_bus_error_then_same_alias_good_mmio_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_hs_store_mmio_bus_error_reports_older_va_under_younger_store_pressure_corner);
// TEST_REGISTER(ai_micro_hs_store_mmio_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner);
// TEST_REGISTER(ai_micro_hs_load_mmio_bus_error_reports_older_va_under_younger_load_pressure_corner);
// TEST_REGISTER(ai_micro_hs_load_mmio_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner);

// AI case batch 145 supplement 3: memblock suspected bug corners, MMIO bus-error precise exception ordering must survive redirect replacement against younger scalar access faults
// TEST_REGISTER(ai_micro_hs_load_mmio_bus_error_then_load_access_fault_oldest_exception_first_corner);
// TEST_REGISTER(ai_micro_hs_load_access_fault_then_load_mmio_bus_error_redirect_replacement_corner);
// TEST_REGISTER(ai_micro_hs_store_mmio_bus_error_then_store_access_fault_oldest_exception_first_corner);
// TEST_REGISTER(ai_micro_hs_store_access_fault_then_store_mmio_bus_error_redirect_replacement_corner);

// AI case batch 146: memblock suspected bug corners, same-page vector cross16 store must not leak fake cross-page owner state into the next boundary
// 2026-04-04 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_vector_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner);
TEST_REGISTER(ai_micro_hs_vector_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner);

// AI case batch 147: memblock suspected bug corners, AMO miss/replay forward progress should survive a redirect and still retire only the successful same-line atomics
// 2026-04-05 source review + individual Spike recheck: 2/2 passed.
// Manual RTL-only: redirect plus D$/MSHR replay behavior needs RTL selfcheck; Spike pass only checks architectural fallback.
// TEST_REGISTER(ai_micro_hs_amo_redirect_then_cold_miss_replay_forward_progress_corner);
// TEST_REGISTER(ai_micro_hs_amo_redirect_then_miss_pressure_replay_no_duplicate_completion_corner);

// AI case batch 148: memblock suspected bug corners, VirtualLoadQueue late uncache paddr update must preserve narrow scalar byte-lane selection after a redirect-adjacent replay
// 2026-04-06 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_virtual_lq_uncache_late_paddr_narrow_scalar_lane_matches_offset_corner);

// AI case batch 149: memblock suspected bug corners, StoreQueue head-side narrow NC stores must preserve exact byte-lane selection from the current low address bits
// 2026-04-06 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_storequeue_head_narrow_scalar_lane_matches_offset_corner);

// AI case batch 150: memblock suspected bug corners, same-16B cacheable direct-misalign narrow load/store paths must preserve width-dependent lane selection and byte merge across the internal doubleword boundary
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_narrow_cross_dw_signed_unsigned_lane_select_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_narrow_cross_dw_byte_merge_corner);

// AI case batch 151: memblock suspected bug corners, same-16B cacheable direct-misalign store address triggers must beat same-access page/access faults and still recover cleanly
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner);

// AI case batch 152: memblock suspected bug corners, same-16B cacheable direct-misalign dual store triggers must beat same-access page/access faults and still recover cleanly
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner);

// AI case batch 153: memblock suspected bug corners, same-16B cacheable direct-misalign dual load triggers must beat same-access page/access faults and still recover cleanly
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_completion_corner);
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_completion_corner);

// AI case batch 154: memblock suspected bug corners, cross-16B middle-ratio scalar ld/sd split templates must preserve their own 4B+4B, 3B+5B, and 2B+6B ownership
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_4b4b_3b5b_2b6b_data_merge_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_4b4b_3b5b_2b6b_data_split_corner);

// AI case batch 155: memblock suspected bug corners, cross-16B scalar fault episodes must not leak stale split templates into the next different-shape recovery access
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_fault_then_different_shape_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_fault_then_different_shape_recovery_no_stale_split_template_corner);

// AI case batch 156: memblock suspected bug corners, narrow PBMT scalar fault episodes must refresh replay/storequeue lane metadata before same-alias different-shape recovery
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_pbmt_narrow_bus_error_then_different_shape_same_alias_lane_refresh_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_narrow_bus_error_then_different_shape_same_alias_mask_refresh_corner);

// AI case batch 157: memblock suspected bug corners, repeated narrow PBMT fault aliases must refresh precise tval across different-shape fault episodes
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_pbmt_narrow_bus_error_tval_refreshes_across_different_shape_same_alias_faults_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_narrow_bus_error_tval_refreshes_across_different_shape_same_alias_faults_corner);

// AI case batch 158: memblock suspected bug corners, PBMT load bus-error episodes must preserve older precise VA and refresh to the next bad alias across trap boundaries
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_pbmt_bus_error_reports_older_va_under_younger_load_pressure_corner);
// TEST_REGISTER(ai_micro_hs_load_pbmt_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner);

// AI case batch 159: memblock suspected bug corners, PBMT=IO store bus-error episodes must preserve older precise VA and refresh to the next bad alias across trap boundaries
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_pbmt_io_bus_error_reports_older_va_under_younger_store_pressure_corner);
// TEST_REGISTER(ai_micro_hs_store_pbmt_io_bus_error_tval_refreshes_to_next_bad_alias_after_trap_corner);

// AI case batch 160: memblock suspected bug corners, same-16B direct narrow scalar fault episodes must refresh load lanes and store masks before different-shape recovery
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_access_fault_then_different_shape_recovery_lane_refresh_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_access_fault_then_different_shape_recovery_mask_refresh_corner);

// AI case batch 161: memblock suspected bug corners, same-16B direct narrow scalar page-fault episodes must refresh load lanes and store masks before different-shape recovery
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_same_16b_cacheable_page_fault_then_different_shape_recovery_lane_refresh_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_page_fault_then_different_shape_recovery_mask_refresh_corner);

// AI case batch 162: memblock suspected bug corners, cross-16B mirrored middle-ratio scalar ld/sd split templates must preserve their own 5B+3B and 6B+2B ownership
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_5b3b_6b2b_data_merge_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_5b3b_6b2b_data_split_corner);

// AI case batch 163: memblock suspected bug corners, faulted cross-16B scalar split templates must not leak into mirrored 5B+3B and 6B+2B recovery accesses
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_fault_then_5b3b_6b2b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_fault_then_5b3b_6b2b_recovery_no_stale_split_template_corner);

// AI case batch 164: memblock suspected bug corners, faulted cross-16B scalar split templates must not leak into low-half-narrow 3B+5B and 2B+6B recovery accesses
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_fault_then_3b5b_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_fault_then_3b5b_2b6b_recovery_no_stale_split_template_corner);

// AI case batch 165: memblock suspected bug corners, scalar cross-16B load second-half address trigger follows Spike and is ignored before later memory faults
// 2026-04-27 selfcheck update: second-half-start address trigger does not match; expect original misaligned VA fault.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_before_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_second_half_address_trigger_ignored_before_access_fault_spike_corner);

// AI case batch 166: memblock suspected bug corners, same-page scalar cross-16B stores must not leak fake cross-page state into the next boundary
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner);
TEST_REGISTER(ai_micro_hs_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner);

// AI case batch 167: memblock suspected bug corners, same-16B direct scalar stores must not inherit stale split-store state from an older cross-16B store
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_cross_16b_then_same_16b_direct_store_old_split_state_not_rebound_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_then_same_16b_direct_store_old_split_state_adjacent_preserved_corner);

// AI case batch 168: memblock suspected bug corners, scalar cross-16B store second-half address trigger follows Spike and is ignored before later memory faults
// 2026-04-27 selfcheck update: second-half-start address trigger does not match; expect original misaligned VA fault.
// TEST_REGISTER(ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_before_page_fault_spike_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_before_access_fault_spike_corner);

// AI case batch 169: memblock suspected bug corners, same-page scalar cross-16B width-switch stores must not reuse fake-crosspage owner or stale split template across boundaries
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
TEST_REGISTER(ai_micro_hs_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner);
TEST_REGISTER(ai_micro_hs_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner);

// AI case batch 170: memblock suspected bug corners, faulted scalar cross-16B stores must clear fake-crosspage owner before later same-page width-switch recovery
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_cross_16b_fault_then_samepage_width_switch_no_stale_fake_crosspage_rebind_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_fault_then_samepage_width_switch_adjacent_preserved_corner);

// AI case batch 171: memblock suspected bug corners, faulted same-16B direct scalar sd must not leave stale cross16 queue state before SQ-wrap recovery
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_fault_then_sq_wrap_direct_sd_recovery_no_fake_cross16_state_corner);
// TEST_REGISTER(ai_micro_hs_store_same_16b_cacheable_fault_then_sq_wrap_direct_sd_recovery_adjacent_preserved_corner);

// AI case batch 172: memblock suspected bug corners, faulted scalar cross-16B loads must clear primed merge/data-select state before signedness-switch recovery
// 2026-04-06 RTL review + individual Spike recheck: 2/2 passed.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery_no_stale_state_corner);
// TEST_REGISTER(ai_micro_hs_load_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery_adjacent_preserved_corner);

// AI case batch 173: memblock suspected bug corners, replay-head uncache narrow loads must refresh current signed/unsigned decode instead of inheriting stale sign extension
// 2026-04-06 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_virtual_lq_uncache_late_paddr_narrow_scalar_signedness_matches_current_uop_corner);

// AI case batch 174: memblock suspected bug corners, StoreQueue head-side narrow NC stores must refresh shifted payload/mask on every width-switch and preserve exact byte-age merge
// 2026-04-06 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_store_pbmt_uncache_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner);

// AI case batch 175: memblock suspected bug corners, StoreQueue head-side narrow MMIO stores must refresh shifted payload/mask on every width-switch and preserve exact byte-age merge
// 2026-04-06 RTL review + Spike compatibility check: current default Spike still hits the same early untested-exception boundary as the older full-width MMIO head-store probe.
// TEST_REGISTER(ai_micro_hs_store_mmio_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner);

// AI case batch 176: memblock suspected bug corners, PBMT narrow load bus-error recovery must refresh same-width signed/unsigned decode on the retargeted alias
// 2026-04-06 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_load_pbmt_narrow_bus_error_then_same_width_signedness_switch_recovery_corner);

// AI case batch 177: memblock suspected bug corners, PBMT=IO store bus-error recovery must clear stale exception state before same-alias good IO-style stores
// 2026-04-06 RTL review + individual Spike recheck: 1/1 passed.
// TEST_REGISTER(ai_micro_hs_store_pbmt_io_bus_error_then_same_alias_good_pbmt_io_store_clears_stale_exception_corner);

// AI case batch 178: memblock suspected bug corners, PBMT cross-16B load faults must clear primed merge and signedness-select state before later cacheable signedness-switch recovery
// 2026-04-06 RTL review + Spike compatibility check: current default Spike still hits the same early untested-exception boundary as the older PBMT cross-16B region probes.
// TEST_REGISTER(ai_micro_hs_load_cross_16b_pbmt_fault_then_cacheable_signedness_switch_recovery_no_stale_mab_state_corner);

// AI case batch 179: memblock suspected bug corners, no-H scalar split load/store low-half->high-half page-fault probes must refresh fullVa/tval instead of reusing the old low-half address
// TEST_REGISTER(ai_micro_m_load_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner);

// AI case batch 179A: memblock suspected bug corners, no-H scalar split-load low->high->low page-fault round-trip must refresh tval back to the newest low-half address after the high-half episode
// TEST_REGISTER(ai_micro_m_load_cross_page_page_fault_tval_refreshes_back_to_low_half_after_high_half_corner);

// AI case batch 180: memblock suspected bug corners, no-H shared scalar load/store cross-page page faults must refresh both cause and tval to the newer producer
// TEST_REGISTER(ai_micro_m_cross_page_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_cross_page_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);

// AI case batch 181: memblock suspected bug corners, no-H scalar split load/store low-half->high-half access-fault probes must refresh fullVa/tval instead of reusing the old low-half address
// TEST_REGISTER(ai_micro_m_load_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner);

// AI case batch 182: memblock suspected bug corners, no-H shared scalar load/store cross-page access faults must refresh both cause and tval to the newer producer
// TEST_REGISTER(ai_micro_m_cross_page_access_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_cross_page_access_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
//
// AI case batch 183: memblock suspected bug corners, no-H same-producer split load/store fault-class switches must refresh both cause and tval when page fault turns into access fault
// TEST_REGISTER(ai_micro_m_load_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner);
//
// AI case batch 184: memblock suspected bug corners, no-H same-producer split load/store fault-class switches must refresh both cause and tval when access fault turns into page fault
// TEST_REGISTER(ai_micro_m_load_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner);
//
// AI case batch 185: memblock suspected bug corners, no-H high-half split load/store faults must clear stale exception state before same-address page-fault recovery completes
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_page_fault_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_page_fault_then_same_address_valid_store_recovers_corner);
//
// AI case batch 186: memblock suspected bug corners, no-H high-half split load/store faults must clear stale exception state before same-address access-fault recovery completes
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_access_fault_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_access_fault_then_same_address_valid_store_recovers_corner);
//
// AI case batch 187: memblock suspected bug corners, no-H repeated high-half split load/store page faults must not retain stale exception state after same-address repair
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_page_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_page_fault_repeated_then_same_address_valid_store_recovers_corner);
//
// AI case batch 188: memblock suspected bug corners, no-H repeated high-half split load/store access faults must not retain stale exception state after same-address repair
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_access_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_access_fault_repeated_then_same_address_valid_store_recovers_corner);
// 2026-04-11 no-H Spike recheck: 20/20 passed across the base refresh/switch set, the PF<->AF same-producer supplement, the same-address recovery supplement, and the repeated high-half fault recovery supplement.
// Logs: .tmp/result_log/spike/spike_batch_result_20260411_001306_no_h_memblock.log
//       .tmp/result_log/spike/spike_batch_result_20260411_002857_no_h_memblock_pf_af_switch.log
//       .tmp/result_log/spike/spike_batch_result_20260411_004028_no_h_memblock_same_address_recovery.log
//       .tmp/result_log/spike/spike_batch_result_20260411_005824_no_h_memblock_repeated_recovery.log

// AI case batches 189-200 from the earlier H/guest exploration are withdrawn for the current no-H target.
// The temporary ai_arch_hyper_* / guest-fault files are no longer kept in ai_test_cases/ under the default Spike/Makefile configuration.

// AI case batch 201: memblock suspected bug corners, no-H same-page cross-16B split load/store second-split PMP access faults must clear stale exception state before same-address recovery
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_then_same_address_valid_store_recovers_corner);
//
// AI case batch 202: memblock suspected bug corners, no-H repeated same-page cross-16B split load/store second-split PMP access faults must not retain stale exception state after same-address repair
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner);
//
// AI case batch 203: memblock suspected bug corners, no-H same-page scalar cross-16B stores must not let fake cross-page owner state rebound into the next boundary
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner);
//
// AI case batch 204: memblock suspected bug corners, no-H same-page scalar cross-16B width-switch stores must not reuse fake cross-page owner or stale split templates across boundaries
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner);
//
// AI case batch 205: memblock suspected bug corners, no-H repeated same-page cross-16B split load second-split PMP access faults on address A must not poison the first valid different-address load on address B after repair
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner);
//
// AI case batch 206: memblock suspected bug corners, no-H repeated same-page cross-16B split store second-split PMP access faults on address A must not poison the first valid different-address store on address B after repair
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner);
//
// AI case batch 207: memblock suspected bug corners, no-H same-page scalar cross-16B store fault episodes must not let fake cross-page owner state rebound into the next boundary after repair
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_state_not_rebound_to_next_boundary_corner);
//
// AI case batch 208: memblock suspected bug corners, no-H same-page scalar cross-16B store fault episodes must preserve adjacent regions when the next boundary store executes after repair
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_adjacent_preserved_corner);
//
// AI case batch 209: memblock suspected bug corners, no-H same-page scalar cross-16B store fault episodes must refresh width-specific split state before the repaired next-boundary width-switch probe
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_no_fake_crosspage_rebind_corner);
//
// AI case batch 210: memblock suspected bug corners, no-H same-page scalar cross-16B store fault episodes must preserve adjacent regions during the repaired next-boundary width-switch probe
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_adjacent_preserved_corner);
//
// AI case batch 211: memblock suspected bug corners, no-H repeated same-page cross-16B split-load second-split access faults on address A must not poison the first different-address width/signedness-switched misaligned load on address B after repair
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_width_signedness_switch_recovers_corner);
//
// AI case batch 212: memblock suspected bug corners, no-H repeated same-page cross-16B split-load second-split access faults must still refresh merge/decode state when the repaired next probe switches width/signedness on the same misaligned address
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_width_signedness_switch_recovers_corner);
//
// AI case batch 213: memblock suspected bug corners, no-H repeated same-page cross-16B split-load faults on address A must not poison the first different-address store producer on address B after repair
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner);
//
// AI case batch 214: memblock suspected bug corners, no-H repeated same-page cross-16B split-store faults on address A must not poison the first different-address load producer on address B after repair
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner);
//
// AI case batch 215: memblock suspected bug corners, no-H repeated same-page cross-16B split-load faults on address A must not poison the first same-address store producer on address A after repair
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner);
//
// AI case batch 216: memblock suspected bug corners, no-H repeated same-page cross-16B split-store faults on address A must not poison the first same-address load producer on address A after repair
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner);
//
// AI case batch 217: memblock suspected bug corners, no-H same-page cross-16B load fault followed by store fault must refresh cause/tval to the newer store producer
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
//
// AI case batch 218: memblock suspected bug corners, no-H same-page cross-16B store fault followed by load fault must refresh cause/tval to the newer load producer
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
//
// AI case batch 219: memblock suspected bug corners, no-H same-page cross-16B same-address load fault followed by store fault must refresh cause/tval to the newer store producer
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_address_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
//
// AI case batch 220: memblock suspected bug corners, no-H same-page cross-16B same-address store fault followed by load fault must refresh cause/tval to the newer load producer
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_address_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
//
// AI case batch 221: memblock suspected bug corners, no-H repeated same-page cross-16B load faults followed by same-address different-width store recovery must not retain stale load-side exception metadata or stale store template
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_store_recovers_corner);
//
// AI case batch 222: memblock suspected bug corners, no-H repeated same-page cross-16B store faults followed by same-address different-width signedness-switched load recovery must not retain stale store-side exception metadata or stale load decode
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_signedness_load_recovers_corner);
//
// AI case batch 223: memblock suspected bug corners, no-H same-page same-width scalar cross-16B stores must refresh exact split template when the next boundary uses a different template of the same width
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_no_stale_template_corner);
//
// AI case batch 224: memblock suspected bug corners, no-H same-page same-width scalar cross-16B stores must preserve adjacent regions when the next boundary uses a different template of the same width
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_adjacent_preserved_corner);
//
// AI case batch 225: memblock suspected bug corners, no-H same-page same-width scalar cross-16B different-template store faults must refresh the current template on the repaired follow-up
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_no_stale_template_corner);
//
// AI case batch 226: memblock suspected bug corners, no-H same-page same-width scalar cross-16B different-template store faults must preserve adjacent regions on the repaired follow-up
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_adjacent_preserved_corner);
//
// AI case batch 227: memblock suspected bug corners, no-H same-page same-width cross-16B loads must refresh exact merge template when the next boundary uses a different template of the same width
// TEST_REGISTER(ai_micro_m_load_cross_16b_same_width_template_switch_no_stale_merge_corner);
//
// AI case batch 228: memblock suspected bug corners, no-H same-page same-width cross-16B different-template load faults must refresh the current merge template on the repaired follow-up
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_no_stale_merge_corner);
//
// AI case batch 229: memblock suspected bug corners, no-H same-page same-width cross-16B different-template load faults must preserve adjacent regions on the repaired follow-up
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_adjacent_preserved_corner);
//
// AI case batch 230: memblock suspected bug corners, no-H same-page same-width different-template load fault-switch must refresh tval to the newer template-specific address
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_load_keeps_latest_tval_corner);

// AI case batch 230A: memblock suspected bug corners, no-H same-page same-width different-template load fault-path repair followed by a valid next-boundary template-switch load must still let a younger next-boundary different-template load fault refresh tval to the current template-specific address
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_same_boundary_new_fault_refreshes_current_template_tval_corner);

// AI case batch 230B: memblock suspected bug corners, no-H same-page same-width different-template load fault-path repair followed by a valid next-boundary template-switch load must still let a returned original-boundary different-template load fault refresh tval back to the current template-specific address
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_original_boundary_new_fault_refreshes_back_template_tval_corner);

// AI case batch 230C: memblock suspected bug corners, no-H same-page same-width different-template load fault-path repair followed by a valid next-boundary template-switch load must still let a cross-producer next-boundary store fault refresh cause+tval to the current template-specific address
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_cross_producer_store_fault_refreshes_current_cause_tval_corner);

// AI case batch 230D: memblock suspected bug corners, no-H same-page same-width different-template load fault-path repair followed by a valid next-boundary template-switch load must still let a returned original-boundary cross-producer store fault refresh cause+tval back to the current template-specific address
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_returned_cross_producer_store_fault_refreshes_back_cause_tval_corner);
//
// AI case batch 231: memblock suspected bug corners, no-H repeated same-page same-width different-template load faults followed by different-address store recovery must not retain stale load-side metadata or stale store template
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_store_recovers_corner);
//
// AI case batch 232: memblock suspected bug corners, no-H repeated same-page same-width different-template store faults followed by different-address load recovery must not retain stale store-side metadata or stale load template
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_load_recovers_corner);
//
// AI case batch 233: memblock suspected bug corners, no-H same-page same-width different-template store fault-switch must refresh tval to the newer template-specific address
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_store_keeps_latest_tval_corner);
//
// AI case batch 234: memblock suspected bug corners, no-H same-address scalar cross-16B store fault episodes must refresh the current width template before the repaired same-address width-switch probe
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_refreshes_current_template_corner);
//
// AI case batch 235: memblock suspected bug corners, no-H same-address scalar cross-16B store fault episodes must preserve adjacent regions during the repaired same-address width-switch probe
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_adjacent_preserved_corner);

// AI case batch 235A: memblock suspected bug corners, no-H same-page scalar cross-16B store fault-path repair followed by a valid next-boundary store must clear stale fake-crosspage state before a younger next-boundary store fault refreshes tval
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_new_fault_refreshes_tval_corner);

// AI case batch 235B: memblock suspected bug corners, no-H same-page scalar cross-16B store fault-path repair followed by a valid next-boundary store must still let a returned younger original-boundary store fault refresh tval back to the current faulting boundary
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_refreshes_back_tval_corner);

// AI case batch 235E: memblock suspected bug corners, no-H same-page scalar cross-16B store fault-path repair followed by a valid next-boundary store must still preserve adjacent guard windows when a returned younger original-boundary store fault refreshes tval back to the current faulting boundary
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_adjacent_preserved_corner);

// AI case batch 235C: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by a valid next-boundary template-switch store must still let a returned original-boundary store fault refresh tval back to the current template-specific address
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_original_boundary_new_fault_refreshes_back_template_tval_corner);

// AI case batch 235D: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by a valid next-boundary template-switch store must still let a younger next-boundary different-template store fault refresh tval to the current template-specific address
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_new_fault_refreshes_current_template_tval_corner);

// AI case batch 235F: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by a valid next-boundary template-switch store must still let a returned original-boundary cross-producer load fault refresh cause+tval back to the current producer and template-specific address
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_cross_producer_load_fault_refreshes_back_cause_tval_corner);

// AI case batch 235G: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by a valid next-boundary template-switch store must still let a same-boundary cross-producer load fault on that next boundary refresh cause+tval to the current producer and template-specific address
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_cross_producer_load_fault_refreshes_current_cause_tval_corner);

// AI case batch 235H: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by a valid next-boundary template-switch store must still let a second returned cross-producer load fault override the first same-boundary load fault metadata with the latest cause+tval
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_two_cross_producer_load_faults_refresh_latest_cause_tval_corner);

// AI case batch 235I: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by a valid next-boundary template-switch store must still let a second same-boundary cross-producer load fault override the first returned load fault metadata with the latest cause+tval
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_refreshes_latest_cause_tval_corner);

// AI case batch 235J: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored valid template load clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_valid_template_load_clears_stale_exception_corner);

// AI case batch 235K: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by same-boundary-then-returned cross-producer load faults must still let the next restored returned valid template load clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_valid_template_load_clears_stale_exception_corner);

// AI case batch 235L: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by same-boundary-then-returned cross-producer load faults must still let the next restored next-boundary valid template load clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_next_boundary_valid_template_load_clears_stale_exception_corner);

// AI case batch 235M: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored returned valid template load clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_template_load_clears_stale_exception_corner);

// AI case batch 235N: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored returned valid template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_template_store_clears_stale_exception_corner);

// AI case batch 235O: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by same-boundary-then-returned cross-producer load faults must still let the next restored next-boundary valid template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_next_boundary_valid_template_store_clears_stale_exception_corner);

// AI case batch 235P: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by same-boundary-then-returned cross-producer load faults must still let the next restored returned valid template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_returned_valid_template_store_clears_stale_exception_corner);

// AI case batch 235Q: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored next-boundary valid template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_next_boundary_valid_template_store_clears_stale_exception_corner);

// AI case batch 235R: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored same-boundary valid different-template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_same_boundary_valid_different_template_store_clears_stale_exception_corner);


// AI case batch 235S: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored returned valid different-template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_different_template_store_clears_stale_exception_corner);

// AI case batch 235T: memblock suspected bug corners, no-H same-page same-width different-template store fault-path repair followed by returned-then-same-boundary cross-producer load faults must still let the next restored next-boundary valid different-template store clear stale exception metadata
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_next_boundary_valid_different_template_store_clears_stale_exception_corner);
//
// AI case batch 236: memblock suspected bug corners, no-H replay-head PBMT=NC narrow scalar loads must refresh the current signed/unsigned decode after a redirect-driving fault
// 2026-04-11 RTL-oriented no-H supplement: compile-only/manual, not folded into the default Spike-closed set because PBMT/uncache/cache effects are not fully represented there.
// TEST_REGISTER(ai_micro_m_pbmt_nc_replay_head_narrow_scalar_signedness_matches_current_uop_corner);
//
// AI case batch 237: memblock suspected bug corners, no-H StoreQueue head-side PBMT=NC narrow width-switch stores must refresh shifted payload/mask and preserve exact byte-age merge
// 2026-04-11 RTL-oriented no-H supplement: compile-only/manual, not folded into the default Spike-closed set because PBMT/uncache/cache effects are not fully represented there.
// TEST_REGISTER(ai_micro_m_store_pbmt_nc_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner);
//
// AI case batch 238: memblock suspected bug corners, no-H PBMT=IO bad-alias store faults must clear stale exception state before later same-alias good stores
// 2026-04-11 RTL-oriented no-H supplement: compile-only/manual, not folded into the default Spike-closed set because Spike-side PMA/PBMT bus-error behavior is not a stable acceptance criterion here.
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bus_error_then_same_alias_good_pbmt_io_store_clears_stale_exception_corner);
//
// AI case batch 239: memblock suspected bug corners, no-H PBMT narrow load bus-error recovery must refresh same-width signed/unsigned decode on the retargeted alias
// 2026-04-11 RTL-oriented no-H supplement: compile-only/manual, not folded into the default Spike-closed set because Spike-side PMA/PBMT bus-error behavior is not a stable acceptance criterion here.
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bus_error_then_same_width_signedness_switch_recovery_corner);
//
// AI case batch 240: memblock suspected bug corners, no-H same-page cross-16B cross-producer same-width different-template load->store fault-switch must refresh the newer producer/template-specific cause+tval
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
//
// AI case batch 241: memblock suspected bug corners, no-H same-page cross-16B cross-producer same-width different-template store->load fault-switch must refresh the newer producer/template-specific cause+tval
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_repeated_access_fault_then_same_address_valid_atomic_recovers_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_repeated_access_fault_then_same_address_valid_lr_recovers_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_repair_then_different_address_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_repair_then_different_address_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_repair_then_different_address_load_fault_refreshes_new_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_repair_then_different_address_atomic_fault_refreshes_new_cause_tval_corner);
//
// AI case batch 242: memblock suspected bug corners, no-H same-page translated atomic fault followed by translated load fault must refresh MemExceptionBuffer cause+tval from the single-port producer to the load producer
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_load_keeps_latest_cause_and_tval_corner);
//
// AI case batch 243: memblock suspected bug corners, no-H same-page translated load fault followed by translated atomic fault must refresh MemExceptionBuffer cause+tval from the load producer to the single-port producer
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_load_then_amo_keeps_latest_cause_and_tval_corner);
//
// AI case batch 244: memblock suspected bug corners, no-H same-page translated atomic fault followed by translated store fault must refresh tval from the single-port producer to the newer store producer even when both sides report store access fault
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_store_keeps_latest_tval_corner);
//
// AI case batch 245: memblock suspected bug corners, no-H same-page translated store fault followed by translated atomic fault must refresh tval from the store producer to the newer single-port producer even when both sides report store access fault
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_store_then_amo_keeps_latest_tval_corner);
//
// AI case batch 246: memblock suspected bug corners, no-H same-page translated lr.d fault followed by translated load fault must refresh tval from the single-port load producer to the newer load producer even when both sides report load access fault
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_load_keeps_latest_tval_corner);
//
// AI case batch 247: memblock suspected bug corners, no-H same-page translated load fault followed by translated lr.d fault must refresh tval from the load producer to the newer single-port load producer even when both sides report load access fault
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_load_then_lr_keeps_latest_tval_corner);
//
// AI case batch 248: memblock suspected bug corners, no-H same-page translated lr.d fault followed by translated store fault must refresh cause+tval from the single-port load producer to the newer store producer
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_store_keeps_latest_cause_and_tval_corner);
//
// AI case batch 249: memblock suspected bug corners, no-H same-page translated store fault followed by translated lr.d fault must refresh cause+tval from the store producer to the newer single-port load producer
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_store_then_lr_keeps_latest_cause_and_tval_corner);
//
// AI case batch 250: memblock suspected bug corners, no-H same-page translated lr.d fault followed by translated atomic fault must refresh cause+tval inside the shared single-port producer
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_amo_keeps_latest_cause_and_tval_corner);
//
// AI case batch 251: memblock suspected bug corners, no-H same-page translated atomic fault followed by translated lr.d fault must refresh cause+tval inside the shared single-port producer
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_lr_keeps_latest_cause_and_tval_corner);

// AI case batch 252: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should not retain stale exception metadata before the next valid load after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_load_no_stale_exception_after_mab_path_corner);
//
// AI case batch 252: memblock suspected bug corners, no-H same-page translated lr.d fault followed by translated lr.d fault at a different address must refresh tval inside the shared single-port load-side producer
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_lr_keeps_latest_tval_corner);
//
// AI case batch 253: memblock suspected bug corners, no-H same-page translated atomic fault followed by translated atomic fault at a different address must refresh tval inside the shared single-port store-side producer
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_amo_keeps_latest_tval_corner);
//
// AI case batch 254: memblock suspected bug corners, no-H LSQ-plus-AMO diagnostic pair must preserve the older precise exception ordering across translated load/atomic fault rearm windows
// TEST_REGISTER(ai_micro_m_single_step_exception_multi_source_same_cycle_oldest_must_win_diagnostic_corner);
//
// AI case batch 255: memblock suspected bug corners, no-H PBMT=IO bad-alias load/store diagnostic pair should preserve the older precise exception ordering across translated fault rearm windows
// TEST_REGISTER(ai_micro_m_pbmt_io_load_store_oldest_wins_diagnostic_corner);
//
// AI case batch 256: memblock suspected bug corners, no-H PBMT=IO same-alias bad-good-next-bad store episodes must refresh the new bad-alias tval after a successful middle recovery
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 257: memblock suspected bug corners, no-H PBMT=IO same-alias bad-good-next-bad load episodes must refresh the new bad-alias tval after a successful middle recovery
// TEST_REGISTER(ai_micro_m_load_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 258: memblock suspected bug corners, no-H PBMT narrow same-alias bad-good-next-bad load episodes must refresh the new bad-alias tval after a successful middle load
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 259: memblock suspected bug corners, no-H PBMT=IO same-alias bad-load -> good-store -> next-bad-store episodes must refresh cause+tval to the newer bad alias after the successful middle recovery
// TEST_REGISTER(ai_micro_m_load_pbmt_io_bad_good_next_bad_store_fault_refreshes_to_new_alias_corner);
//
// AI case batch 260: memblock suspected bug corners, no-H PBMT=IO same-alias bad-store -> good-load -> next-bad-load episodes must refresh cause+tval to the newer bad alias after the successful middle recovery
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_next_bad_load_fault_refreshes_to_new_alias_corner);
//
// AI case batch 261: memblock suspected bug corners, no-H PBMT narrow same-alias bad-good-next-bad load episodes must refresh the new bad-alias tval after a successful middle width/decode switch
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bad_good_next_bad_width_switch_refreshes_new_alias_corner);
//
// AI case batch 262: memblock suspected bug corners, no-H PBMT store same-alias bad-good-next-bad episodes must refresh the new bad-alias tval after a successful middle width switch
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_next_bad_width_switch_refreshes_new_alias_corner);
//
// AI case batch 263: memblock suspected bug corners, no-H PBMT bad-load -> good-store -> next-bad-load episodes must refresh cause+tval to the newer bad alias after the successful middle recovery
// TEST_REGISTER(ai_micro_m_load_pbmt_bad_good_store_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 264: memblock suspected bug corners, no-H PBMT bad-store -> good-load -> next-bad-store episodes must refresh cause+tval to the newer bad alias after the successful middle recovery
// TEST_REGISTER(ai_micro_m_store_pbmt_bad_good_load_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 265: memblock suspected bug corners, no-H PBMT=NC same-alias bad-good-next-bad store episodes must refresh the new bad-alias tval after a successful middle recovery
// TEST_REGISTER(ai_micro_m_store_pbmt_nc_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 266: memblock suspected bug corners, no-H repeated same-page cross-16B load/store fault episodes that have already been repaired on address A must refresh to a younger same-producer new fault on address B instead of reusing stale exception metadata
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_load_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_store_fault_refreshes_new_tval_corner);
//
// AI case batch 267: memblock suspected bug corners, no-H repeated same-page cross-16B load/store fault episodes that have already been repaired on address A must refresh cause+tval to a younger cross-producer new fault on address B
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_store_fault_refreshes_new_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_load_fault_refreshes_new_cause_tval_corner);
//
// AI case batch 268: memblock suspected bug corners, no-H mixed PBMT=NC load and PBMT=IO store diagnostic pair should preserve older precise exception ordering across loadQueue/storeQueue-like rearm windows
// TEST_REGISTER(ai_micro_m_pbmt_mixed_nc_load_io_store_oldest_wins_diagnostic_corner);
//
// AI case batch 269: memblock suspected bug corners, no-H mixed PBMT=NC load and PBMT=IO store bad-good-next-bad cross-producer episodes should refresh cause+tval to the newer bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_fault_refreshes_to_new_alias_corner);
//
// AI case batch 270: memblock suspected bug corners, no-H mixed PBMT=NC narrow-load and PBMT=IO width-switched store bad-good-next-bad episodes should refresh cause+tval to the newer bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_width_switch_refreshes_new_alias_corner);
//
// AI case batch 271: memblock suspected bug corners, no-H mixed PBMT width/decode switch episodes that cross through the opposite producer and then return to the original producer side should still refresh tval to the newer bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_load_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_store_width_switch_refreshes_new_alias_corner);
//
// AI case batch 272: memblock suspected bug corners, no-H mixed PBMT width/decode switch episodes whose younger final bad fault itself switches to a third shape should still refresh cause+tval to the current bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_store_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_load_width_switch_refreshes_current_alias_corner);
//
// AI case batch 273: memblock suspected bug corners, no-H mixed PBMT episodes that cross through the opposite producer but then return to the same bad alias on the original producer side with a different switched shape should still refresh tval to the current bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner);
//
// AI case batch 274: memblock suspected bug corners, no-H same-alias mixed PBMT episodes that cross through the opposite producer and finally flip to the opposite producer side with a different switched shape should still refresh cause+tval to the current bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner);
//
// AI case batch 275: memblock suspected bug corners, no-H mixed PBMT width/decode oldest-wins diagnostic pair should preserve older precise exception ordering across NC narrow-load and IO width-switched store rearm windows
// TEST_REGISTER(ai_micro_m_pbmt_mixed_nc_narrow_load_io_width_switch_store_oldest_wins_diagnostic_corner);

// AI case batch 276: memblock suspected bug corners, no-H same-producer PBMT narrow-load bad-good-then-same-alias-bad episodes should refresh tval to the current load width/decode on the original bad alias
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner);

// AI case batch 277: memblock suspected bug corners, no-H same-producer PBMT store bad-good-then-same-alias-bad episodes should refresh tval to the current store width on the original bad alias
// TEST_REGISTER(ai_micro_m_store_pbmt_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner);

// AI case batch 278: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-boundary different-template success store clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_no_stale_exception_after_mab_path_corner);

// AI case batch 279: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a two-step same-boundary template switch clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_then_original_template_store_no_stale_exception_after_mab_path_corner);

// AI case batch 280: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a different-template success store followed by a reused fault-template success store clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_then_reuse_fault_template_store_no_stale_exception_after_mab_path_corner);

// AI case batch 281: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-template 4B+4B success retry clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_no_stale_exception_after_mab_path_corner);

// AI case batch 282: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-template 1B+7B double-success retry clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_twice_no_stale_exception_after_mab_path_corner);

// AI case batch 283: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-fault-template 4B+4B double-success retry clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_twice_no_stale_exception_after_mab_path_corner);

// AI case batch 284: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a different-template 1B+7B double-success retry clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_twice_no_stale_exception_after_mab_path_corner);

// AI case batch 285: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a 4B+4B double-success retry after a 1B+7B fault clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_4b4b_store_twice_after_1b7b_fault_no_stale_exception_after_mab_path_corner);

// AI case batch 286: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-template success followed by a 4B+4B different-template success clear stale exception metadata after a 1B+7B fault path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_then_different_template_store_no_stale_exception_after_1b7b_mab_path_corner);

// AI case batch 287: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-fault-template success followed by a 1B+7B different-template success clear stale exception metadata after a 4B+4B fault path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_then_different_template_store_no_stale_exception_after_4b4b_mab_path_corner);

// AI case batch 288: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct 1B+7B different-template success clear stale exception metadata after a 4B+4B fault path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_no_stale_exception_after_4b4b_mab_path_corner);

// AI case batch 289: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct 3B+5B different-template success clear stale exception metadata after a 2B+6B fault path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_3b5b_store_no_stale_exception_after_2b6b_mab_path_corner);

// AI case batch 290: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-boundary aligned success store clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_no_stale_exception_after_mab_path_corner);

// AI case batch 291: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a same-boundary aligned success store and the following aligned load both clear stale exception metadata after the MAB path
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_same_boundary_aligned_load_no_stale_exception_after_mab_path_corner);

// AI case batch 292: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a low-half aligned success store be followed by an untouched upper-half aligned load that still preserves the seeded high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_load_preserves_seeded_high_half_no_stale_exception_after_mab_path_corner);

// AI case batch 293: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a low-half aligned success store be followed by an upper-half aligned success store that still preserves the committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_store_preserves_low_half_no_stale_exception_after_mab_path_corner);

// AI case batch 294: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a low-half aligned load that still preserves the seeded low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_load_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner);

// AI case batch 295: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a low-half aligned success store that still preserves the committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_store_preserves_high_half_no_stale_exception_after_mab_path_corner);

// AI case batch 296: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by an upper-half aligned load that still reads the committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_upper_half_aligned_load_reads_committed_high_half_no_stale_exception_after_mab_path_corner);

// AI case batch 297: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a repeated upper-half aligned success store that still preserves the seeded low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_repeat_upper_half_aligned_store_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner);

// AI case batch 298: memblock suspected bug corners, no-H page-tail cross-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by low-half and upper-half aligned loads; the faulting split store reports split tval but leaves the low byte invisible, and no stale exception metadata may leak
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_upper_load_consistent_no_stale_exception_corner);

// AI case batch 299: memblock suspected bug corners, no-H page-tail cross-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by upper-half and low-half aligned loads; the faulting split store reports split tval but leaves the low byte invisible, and no stale exception metadata may leak
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_load_consistent_no_stale_exception_corner);

// AI case batch 300: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by an upper-half aligned load and then a low-half aligned store that still preserves the committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_store_preserves_high_half_corner);

// AI case batch 301: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a low-half aligned store and then an upper-half aligned load that still preserves the committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_load_preserves_high_half_corner);

// AI case batch 302: memblock suspected bug corners, no-H page-tail cross-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a low-half aligned load and then a low-half aligned store; the faulting split store reports split tval but leaves the low byte invisible, and no stale exception metadata may leak
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_low_store_preserves_high_half_corner);

// AI case batch 303: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a low-half aligned success store and a final upper-half aligned success store that still preserves the committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_store_preserves_low_half_no_stale_exception_corner);

// AI case batch 304: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a low-half aligned success store and a repeated low-half aligned success store that still preserves the committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_repeat_low_store_preserves_high_half_no_stale_exception_corner);

// AI case batch 305: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by a repeated upper-half aligned success store and a final low-half aligned success store that still preserves the latest committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner);

// AI case batch 306: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct low-half aligned success store be followed by a repeated low-half aligned success store and a final upper-half aligned success store that still preserves the latest committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner);

// AI case batch 307: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct low-half aligned success store be followed by an upper-half aligned success store and a repeated upper-half aligned success store that still preserves the latest committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_repeat_upper_store_preserves_latest_low_half_no_stale_exception_corner);

// AI case batch 308: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct low-half aligned success store be followed by an upper-half aligned success store and a returned low-half aligned success store that still preserves the committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_low_store_preserves_high_half_no_stale_exception_corner);

// AI case batch 309: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct low-half aligned success store be followed by a low-half aligned load and an upper-half aligned load that still observe a consistent boundary image without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_low_load_upper_load_consistent_no_stale_exception_corner);

// AI case batch 310: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct low-half aligned success store be followed by a repeated low-half aligned success store that still preserves the seeded high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner);

// AI case batch 311: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct upper-half aligned success store be followed by two repeated upper-half aligned success stores that still preserve the seeded low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner);

// AI case batch 312: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let a direct low-half aligned success store be followed by two repeated low-half aligned success stores that still preserve the seeded high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner);

// AI case batch 313: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let three low-half aligned success stores be followed by a final upper-half aligned success store that still preserves the latest committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner);

// AI case batch 314: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let three upper-half aligned success stores be followed by a final low-half aligned success store that still preserves the latest committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner);

// AI case batch 315: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores still preserve the seeded low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner);

// AI case batch 316: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores still preserve the seeded high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner);

// AI case batch 317: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores be followed by a final upper-half aligned success store that still preserves the latest committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner);

// AI case batch 318: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores be followed by a final low-half aligned success store that still preserves the latest committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner);

// AI case batch 319: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores be followed by a final low-half aligned load that still preserves the seeded low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_preserves_seeded_low_half_no_stale_exception_corner);

// AI case batch 320: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores be followed by a final upper-half aligned load that still preserves the seeded high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_preserves_seeded_high_half_no_stale_exception_corner);

// AI case batch 321: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores be followed by a final upper-half aligned load that still reads the latest committed high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_reads_latest_high_half_no_stale_exception_corner);

// AI case batch 322: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores be followed by a final low-half aligned load that still reads the latest committed low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_reads_latest_low_half_no_stale_exception_corner);

// AI case batch 323: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores be followed by low-load then upper-load dual consumers that still observe the latest low half and seeded high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_then_upper_load_observes_latest_low_and_seeded_high_no_stale_exception_corner);

// AI case batch 324: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores be followed by upper-load then low-load dual consumers that still observe the latest high half and seeded low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_then_low_load_observes_latest_high_and_seeded_low_no_stale_exception_corner);

// AI case batch 325: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores be followed by low-load then upper-load dual consumers that still observe the seeded low half and latest high half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_then_upper_load_observes_seeded_low_and_latest_high_no_stale_exception_corner);

// AI case batch 326: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores be followed by upper-load then low-load dual consumers that still observe the seeded high half and latest low half without stale exception metadata
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_then_low_load_observes_seeded_high_and_latest_low_no_stale_exception_corner);

// AI case batch 327: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores be followed by a restored second-split fault that still refreshes precise fault metadata without clobbering the latest committed high half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_reports_precise_fault_and_preserves_latest_high_half_corner);

// AI case batch 328: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores be followed by a restored second-split fault that still refreshes precise fault metadata without clobbering the untouched seeded high half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_reports_precise_fault_and_preserves_seeded_high_half_corner);

// AI case batch 329: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned load that clears stale refault exception metadata and still reads the seeded high half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_upper_load_clears_stale_exception_and_reads_seeded_high_half_corner);

// AI case batch 330: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned load that clears stale refault exception metadata and still reads the latest committed high half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner);

// AI case batch 331: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores and a restored second-split fault be followed by a repaired low-half aligned load that clears stale refault exception metadata and reads the refault-updated current low half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_load_clears_stale_exception_and_reads_refault_updated_low_half_corner);

// AI case batch 332: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four low-half aligned success stores and a restored second-split fault be followed by a repaired low-half aligned store that clears stale refault exception metadata and becomes the latest committed low half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_store_clears_stale_exception_and_commits_latest_low_half_corner);

// AI case batch 333: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store that clears stale refault exception metadata and becomes the latest committed high half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_clears_stale_exception_and_commits_latest_high_half_corner);

// AI case batch 334: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a low-half aligned load that still observes the refault-updated current low half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_low_load_clears_stale_exception_and_observes_refault_updated_low_half_corner);

// AI case batch 335: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then an upper-half aligned load that reads the latest committed high half
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner);

// AI case batch 336: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a tail-byte translated load that still observes the refault-updated byte7
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_byte_load_clears_stale_exception_and_reads_refault_updated_byte7_corner);

// AI case batch 337: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a first-upper-byte translated load that reads the latest high-half byte0
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_byte_load_clears_stale_exception_and_reads_latest_high_half_byte0_corner);

// AI case batch 338: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a tail-halfword translated load that still observes seeded byte6 plus the refault-updated byte7
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_halfword_load_clears_stale_exception_and_reads_seeded_byte6_plus_refault_updated_byte7_corner);

// AI case batch 339: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a first-upper-halfword translated load that reads the latest high-half bytes0-1
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_halfword_load_clears_stale_exception_and_reads_latest_high_half_bytes01_corner);

// AI case batch 340: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a tail-word translated load that still observes seeded bytes4-6 plus the refault-updated byte7
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_word_load_clears_stale_exception_and_reads_seeded_bytes4to6_plus_refault_updated_byte7_corner);

// AI case batch 341: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a repaired upper-half aligned store and then a first-upper-word translated load that reads the latest high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_word_load_clears_stale_exception_and_reads_latest_high_half_bytes0to3_corner);

// AI case batch 342: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then a tail-word translated load that still observes seeded bytes4-6 plus the retried byte7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_word_corner);

// AI case batch 343: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then a first-upper-word translated load that reads the retried high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_corner);

// AI case batch 344: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then a first-upper-halfword translated load that reads the retried high-half bytes0-1
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_corner);

// AI case batch 345: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then an upper-half aligned translated load that reads the retried high-half image
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_load_corner);

// AI case batch 346: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then a first-upper-byte translated load that reads the retried high-half byte0
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_corner);

// AI case batch 347: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then a mid-upper-halfword translated load that reads the retried high-half bytes2-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_halfword_corner);

// AI case batch 348: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then a second-upper-byte translated load that reads the retried high-half byte1
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_corner);

// AI case batch 349: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then two head-byte translated loads that individually read the retried high-half bytes0-1
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_corner);

// AI case batch 350: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, two head-byte translated loads, and then a first-upper-halfword translated load that still reads the combined retried high-half bytes0-1
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_corner);

// AI case batch 351: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, two head-byte translated loads, a first-upper-halfword translated load, and then a first-upper-word translated load that still reads the combined retried high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_then_first_upper_word_corner);

// AI case batch 352: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, two head-byte translated loads, and then a first-upper-word translated load that still reads the combined retried high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_word_corner);

// AI case batch 353: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, one head-byte translated load, and then a first-upper-word translated load that still reads the combined retried high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_single_byte_then_first_upper_word_corner);

// AI case batch 354: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a second-upper-byte translated load, and then a first-upper-word translated load that still reads the combined retried high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_first_upper_word_corner);

// AI case batch 355: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-word translated load, and then a first-upper-word translated load that still reads the combined retried high-half bytes0-3
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_first_upper_word_corner);

// AI case batch 356: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-byte translated load, and then an upper-tail-word translated load that still reads the combined retried high-half bytes4-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_word_corner);

// AI case batch 357: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword translated load, and then an upper-tail-word translated load that still reads the combined retried high-half bytes4-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_word_corner);

// AI case batch 358: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-last-halfword translated load, and then an upper-tail-word translated load that still reads the combined retried high-half bytes4-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_then_upper_tail_word_corner);

// AI case batch 359: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-last-halfword-low-byte translated load, and then an upper-tail-word translated load that still reads the combined retried high-half bytes4-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_word_corner);

// AI case batch 360: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-last-halfword-high-byte translated load, and then an upper-tail-word translated load that still reads the combined retried high-half bytes4-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_word_corner);

// AI case batch 361: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-last-halfword-low-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);

// AI case batch 362: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-last-halfword-high-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);

// AI case batch 363: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_last_halfword_corner);

// AI case batch 364: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-second-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_second_byte_then_upper_tail_last_halfword_corner);

// AI case batch 365: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 366: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_then_upper_tail_last_halfword_corner);

// AI case batch 367: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a second-upper-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_upper_tail_last_halfword_corner);

// AI case batch 368: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-halfword translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 369: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a mid-upper-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_byte_then_upper_tail_last_halfword_corner);

// AI case batch 370: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a mid-upper-second-byte translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_second_byte_then_upper_tail_last_halfword_corner);

// AI case batch 371: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a mid-upper-halfword translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 372: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a head-middle-bridge halfword translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_middle_bridge_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 373: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge halfword translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 374: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);

// AI case batch 375: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_corner);

// AI case batch 376: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-word translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_upper_tail_last_halfword_corner);

// AI case batch 377: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a tail-adjacent-bridge word translated load, and then an upper-tail-last-halfword translated load that still reads the combined retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_corner);

// AI case batch 378: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, and then an upper-tail-halfword translated load that still reads the combined retried high-half bytes4-5
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_corner);

// AI case batch 379: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, and then an upper-tail-halfword translated load that still reads the combined retried high-half bytes4-5
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_halfword_corner);

// AI case batch 380: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a tail-adjacent-bridge word translated load, and then an upper-tail-halfword translated load that still reads the combined retried high-half bytes4-5
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_halfword_corner);

// AI case batch 381: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, and then an upper-tail-bridge-halfword translated load that still reads the combined retried high-half bytes5-6
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_bridge_halfword_corner);

// AI case batch 382: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, and then an upper-tail-bridge-halfword translated load that still reads the combined retried high-half bytes5-6
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_bridge_halfword_corner);

// AI case batch 383: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a tail-adjacent-bridge word translated load, and then an upper-tail-bridge-halfword translated load that still reads the combined retried high-half bytes5-6
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_bridge_halfword_corner);

// AI case batch 384: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, and then an upper-tail-last-halfword-high-byte translated load that still reads retried high-half byte7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_high_byte_corner);

// AI case batch 385: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, and then an upper-tail-last-halfword-high-byte translated load that still reads retried high-half byte7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_high_byte_corner);

// AI case batch 386: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a tail-adjacent-bridge word translated load, and then an upper-tail-last-halfword-high-byte translated load that still reads retried high-half byte7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_high_byte_corner);

// AI case batch 387: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, and then an upper-tail-last-halfword-low-byte translated load that still reads retried high-half byte6
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_low_byte_corner);

// AI case batch 388: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, and then an upper-tail-last-halfword-low-byte translated load that still reads retried high-half byte6
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_low_byte_corner);

// AI case batch 389: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a tail-adjacent-bridge word translated load, and then an upper-tail-last-halfword-low-byte translated load that still reads retried high-half byte6
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_low_byte_corner);

// AI case batch 390: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-last-halfword-low-byte warmup load, and then an upper-tail-last-halfword translated load that now reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);

// AI case batch 391: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, an upper-tail-last-halfword-low-byte warmup load, and then an upper-tail-last-halfword translated load that now reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);

// AI case batch 392: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-last-halfword-high-byte warmup load, and then an upper-tail-last-halfword translated load that now reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);

// AI case batch 393: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, and then an upper-tail-last-halfword translated load that now reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 394: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a middle-tail-bridge word translated load, an upper-tail-halfword warmup load, and then an upper-tail-last-halfword translated load that now reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_halfword_then_upper_tail_last_halfword_corner);

// AI case batch 395: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-bridge-halfword warmup load, and then an upper-tail-last-halfword translated load that now reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 396: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword prewarm load, a first-upper-word translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner);
// AI case batch 397: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-bridge-halfword prewarm load, a first-upper-word translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner);
// AI case batch 398: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword prewarm load, a middle-tail-bridge-word translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// AI case batch 399: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword prewarm load, a first-upper-word translated load, a middle-tail-bridge-word translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// AI case batch 400: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-bridge-halfword prewarm load, a first-upper-word translated load, a middle-tail-bridge-word translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// AI case batch 401: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a middle-tail-bridge-word translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// AI case batch 402: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword prewarm load, a first-upper-word translated load, a mid-upper-halfword translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 403: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-bridge-halfword prewarm load, a first-upper-word translated load, a mid-upper-halfword translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 404: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-halfword translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 405: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, an upper-tail-halfword prewarm load, a first-upper-word translated load, a mid-upper-byte translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_mid_upper_byte_then_upper_tail_last_halfword_corner);
// AI case batch 406: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_corner);
// AI case batch 407: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-second-byte translated load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_corner);
// AI case batch 408: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte warmup load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// AI case batch 409: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-high-byte warmup load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);
// AI case batch 410: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-bridge-halfword warmup load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 411: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 412: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-halfword rewarm load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// AI case batch 413: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, a first-upper-word rewarm load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_first_upper_word_rewarm_then_upper_tail_last_halfword_corner);
// AI case batch 414: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);
// AI case batch 415: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-last-halfword-low-byte follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// AI case batch 416: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 417: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// AI case batch 418: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-bridge-halfword follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 419: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 420: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, an upper-tail-bridge-halfword follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 421: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 422: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-bridge-halfword follow-up load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 423: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, an upper-tail-bridge-halfword follow-up load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 424: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, an upper-tail-word translated follow-up load, an upper-tail-bridge-halfword follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_word_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 425: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-bridge-halfword follow-up load, an upper-tail-word translated follow-up load, an upper-tail-halfword rewarm load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// AI case batch 426: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-second-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, an upper-tail-bridge-halfword follow-up load, an upper-tail-word translated follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// AI case batch 427: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-second-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-halfword rewarm load, an upper-tail-word translated follow-up load, an upper-tail-bridge-halfword follow-up load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_word_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// AI case batch 428: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store, a first-upper-word translated load, an upper-tail-halfword warmup load, a mid-upper-second-byte translated load, an upper-tail-last-halfword-low-byte follow-up load, an upper-tail-last-halfword-high-byte follow-up load, an upper-tail-bridge-halfword follow-up load, an upper-tail-word translated follow-up load, an upper-tail-halfword rewarm load, and then an upper-tail-last-halfword translated load that still reads retried high-half bytes6-7
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// AI case batch 429: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then an immediate same-address same-template translated store that still commits with the latest overlay only
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_template_store_corner);
// AI case batch 430: memblock suspected bug corners, no-H same-page cross-16B store fault-repair path should let four upper-half aligned success stores and a restored second-split fault be followed by a retried original-template success store and then an immediate same-address width-switched translated store that refreshes only the current narrow overlay
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_width_switch_store_corner);

// AI case batch 632: memblock suspected bug corners, with requested AVL=33 but active vl=32 on e8,m2, a late31 translated vector strided store fault on page31 should let the younger same-parameter vector observer see active committed-prefix bytes while keeping fault element31 old and leaving inactive idx32 untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_same_parameter_strided_load_sees_committed_prefix_and_old_suffix_corner);
// AI case batch 633: memblock suspected bug corners, with requested AVL=33 but active vl=32 on e8,m2, a late31 translated vector strided store fault on page31 should let scalar probes see prefix byte30 new while keeping fault element31 and inactive idx32 old
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_scalar_probes_see_committed_prefix_and_old_suffix_corner);
// AI case batch 634: memblock suspected bug corners, requested AVL=33 on e8,m2 should be pinned by a no-fault boundary guard that proves same-parameter vector observation only touches active elements0-31 and leaves idx32 untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_req33_e8m2_active32_same_parameter_observer_leaves_idx32_untouched_corner);
// AI case batch 635: memblock suspected bug corners, after a late31 e8,m2 translated vector strided store fault on page31, repairing write permission and exact replay should update active elements0-31 to the new image while still leaving inactive idx32 untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_repair_exact_replay_same_parameter_strided_load_sees_full_new_image_corner);
// AI case batch 644: memblock suspected bug corners, with requested AVL=33 but active vl=32 on e8,m2, a late31 HS leaf no-write store page fault should let the younger same-parameter vector observer see active committed-prefix bytes while keeping fault element31 old and leaving inactive idx32 untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner);
// AI case batch 645: memblock suspected bug corners, after a late31 PMP deny-write strided store fault, both a same-parameter vector observer and a following scalar-probe observer should still see fault element31 and inactive idx32 old
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_vector_observer_and_scalar_probes_both_see_old_fault_and_idx32_corner);
// AI case batch 646: memblock suspected bug corners, a late31 requested-33 e8,m2 strided store fault should let the trap-entry handler immediately issue the same-parameter strided load and still see only committed-prefix data while keeping fault element31 and inactive idx32 old
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner);
// AI case batch 647: memblock suspected bug corners, a late31 requested-33 e8,m2 HS leaf no-write strided store fault should let the trap-entry handler immediately issue the same-parameter strided load and still see only committed-prefix data while keeping fault element31 and inactive idx32 old
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner);
// AI case batch 648: memblock suspected bug corners, a late29 requested-33 e8,m2 PMP deny-write strided store fault should let trap entry observe committed prefix bytes0-28 while also proving active fault-plus-suffix elements29-31 stay old and inactive idx32 stays untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_active_suffix_corner);
// AI case batch 649: memblock suspected bug corners, a late29 requested-33 e8,m2 HS leaf no-write strided store fault should let trap entry observe committed prefix bytes0-28 while also proving active fault-plus-suffix elements29-31 stay old and inactive idx32 stays untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_active_suffix_corner);
// AP-8 manual RTL-focused: older scalar fault redirects around a younger vector store; target memory must not see wrong-path vector payload.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap8_older_scalar_load_fault_skips_younger_vector_store_no_wrongpath_leak_manual_smoke_corner);
// AP-8 manual RTL-focused: masked inactive, adjacent vector, wrap-pressure, and post-fault CBO/marker variants target StoreQueue timing.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap8_masked_even_late28_fault_preserves_inactive_and_suffix_old_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap8_adjacent_vector_store_after_late29_fault_is_skipped_no_wrongpath_payload_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap8_prior_full_vector_store_wrap_pressure_late31_fault_prefix_survives_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap8_late31_fault_then_cbo_zero_and_marker_no_stale_exception_state_manual_smoke_corner);
// AP-9 manual RTL-focused: old late31 vector-store lastFlow exception must not swallow the first post-redirect store/CBO/vector store.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap9_late31_lastflow_then_first_scalar_two_store_same_and_different_line_visible_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap9_late31_lastflow_then_first_cbo_zero_marker_and_cbom_owner_visible_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap9_late31_lastflow_then_narrow_and_cross16_store_halves_visible_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap9_late31_lastflow_then_first_vector_store_visible_under_repeated_recovery_manual_smoke_corner);
// AP-54 manual RTL-focused: stale vecExceptionFlag must not suppress post-redirect vector markers, and true non-lastFlow exceptions must keep suffix old.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap54_late29_nonlastflow_fault_then_post_redirect_vector_marker_visible_under_pressure_manual_corner);
// TEST_REGISTER(ai_micro_ap54_repeated_late29_fault_repair_vector_marker_loop_no_stale_flag_manual_corner);
// TEST_REGISTER(ai_micro_ap54_late31_lastflow_then_new_late29_exception_keeps_suffix_old_and_next_marker_visible_manual_corner);
// TEST_REGISTER(ai_micro_ap54_unitstride_first_fault_then_marker_vector_store_no_stale_inherit_manual_corner);
// AP-10 manual RTL-focused: MemBlock OldestRedirectSelector must compare redirect level via compRobPtr for same/near ROB mixed-level requests.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap10_vpmatch_ldld_raw_mixed_level_marker_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap10_vector_same_rob_flow_and_scalar_raw_marker_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap10_wrap_pressure_mixed_level_replay_boundary_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap10_mixed_redirect_then_cbo_zero_amo_side_effect_manual_smoke_corner);
// AI case batch 650: memblock suspected bug corners, after wave debug showed trap-entry same-parameter vector observation starts too late, a late29 PMP deny-write fault should let the earliest trap-entry scalar byte probe on the fault element still see the old image
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner);
// AI case batch 651: memblock suspected bug corners, after wave debug showed trap-entry same-parameter vector observation starts too late, a late29 PMP deny-write fault should let the earliest trap-entry scalar byte probe on the last active suffix element still see the old image
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_last_active_suffix_byte_probe_sees_old_image_corner);
// AI case batch 652: memblock suspected bug corners, after wave debug showed trap-entry same-parameter vector observation starts too late, a late29 HS leaf no-write fault should let the earliest trap-entry scalar byte probe on the fault element still see the old image
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner);
// AI case batch 653: memblock suspected bug corners, after wave debug showed trap-entry same-parameter vector observation starts too late, a late29 HS leaf no-write fault should let the earliest trap-entry scalar byte probe on the last active suffix element still see the old image
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_last_active_suffix_byte_probe_sees_old_image_corner);
// AI case batch 654: memblock suspected bug corners, the late31 HS leaf no-write follow-up matrix should also include scalar probes that independently confirm prefix byte30 committed while fault element31 and inactive idx32 stay old
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_scalar_probes_see_committed_prefix_and_old_suffix_corner);
// AI case batch 655: memblock suspected bug corners, after a late31 HS leaf no-write fault, repairing the leaf permission and exact replay should let the same-parameter observer recover the full active new image while leaving idx32 untouched
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_repair_exact_replay_same_parameter_strided_load_sees_full_new_image_corner);
// AI case batch 656: memblock suspected bug corners, after a late31 HS leaf no-write fault, both a same-parameter vector observer and later scalar probes should agree that fault element31 and inactive idx32 remain old
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_vector_observer_and_scalar_probes_both_see_old_fault_and_idx32_corner);
// AI case batch 657: memblock suspected bug corners, the late29 PMP deny-write earliest fault-byte trap-entry probe should also survive dual dense translated-store pressure bursts before fault and handler-tail pressure after probe
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_fault_byte_probe_under_dense_store_pressure_sees_old_fault_image_corner);
// AI case batch 658: memblock suspected bug corners, the late29 PMP deny-write earliest last-active-suffix trap-entry probe should also survive dual dense translated-store pressure bursts before fault and handler-tail pressure after probe
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_last_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner);
// AI case batch 659: memblock suspected bug corners, the late29 HS leaf no-write earliest fault-byte trap-entry probe should also survive dual dense translated-store pressure bursts before fault and handler-tail pressure after probe
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_fault_byte_probe_under_dense_store_pressure_sees_old_fault_image_corner);
// AI case batch 660: memblock suspected bug corners, the late29 HS leaf no-write earliest last-active-suffix trap-entry probe should also survive dual dense translated-store pressure bursts before fault and handler-tail pressure after probe
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_last_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner);
// AI case batch 661: memblock suspected bug corners, the late29 PMP deny-write matrix should also pin the immediate active suffix element30 with a dedicated trap-entry byte probe instead of only observing fault element29 and tail element31
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_next_active_suffix_byte_probe_sees_old_image_corner);
// AI case batch 662: memblock suspected bug corners, the late29 HS leaf no-write matrix should also pin the immediate active suffix element30 with a dedicated trap-entry byte probe instead of only observing fault element29 and tail element31
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_next_active_suffix_byte_probe_sees_old_image_corner);
// AI case batch 663: memblock suspected bug corners, the late29 PMP deny-write immediate active suffix element30 probe should also survive dual dense translated-store pressure bursts before fault and handler-tail pressure after probe
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_next_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner);
// AI case batch 664: memblock suspected bug corners, the late29 HS leaf no-write immediate active suffix element30 probe should also survive dual dense translated-store pressure bursts before fault and handler-tail pressure after probe
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_next_active_suffix_byte_probe_under_dense_store_pressure_sees_old_image_corner);
// AI case batch 665: memblock suspected bug corners, the late31 PMP deny-write path should also pin the fault element itself with a handler-head earliest byte probe instead of only relying on later same-parameter or post-fault observers
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner);
// AI case batch 666: memblock suspected bug corners, the late31 HS leaf no-write path should also pin the fault element itself with a handler-head earliest byte probe instead of only relying on later same-parameter or post-fault observers
// TEST_REGISTER(ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_trap_entry_fault_byte_probe_sees_old_fault_image_corner);
// AP-116 manual RTL-focused: wrong-path misalign-buffer requests must not escape redirect as spurious fault, marker write, or partial store.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_hs_load_cross_page_wrong_path_redirect_no_spurious_lpf_or_marker_write_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_16b_wrong_path_redirect_no_misalign_buffer_leak_corner);
// TEST_REGISTER(ai_micro_hs_store_cross_page_wrong_path_redirect_no_spurious_spf_or_partial_write_corner);
// AP-59/AP-86 manual RTL-focused: repeated misalign-buffer redirect/retry windows must not leak stale split responses, partial stores, or vector feedback.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap59_load_cross_page_wrong_path_repeated_redirect_then_repaired_retry_no_stale_mab_state_manual_corner);
// TEST_REGISTER(ai_micro_ap59_store_cross_page_wrong_path_repeated_redirect_then_repaired_retry_no_partial_or_stale_mab_state_manual_corner);
// TEST_REGISTER(ai_micro_ap59_vector_store_cross_page_wrong_path_repeated_redirect_then_repaired_retry_no_suffix_leak_manual_corner);
// TEST_REGISTER(ai_micro_ap86_load_replay_wrong_path_split_response_then_clean_retry_marker_no_stale_vecwb_manual_corner);

// AI case batch 701: trap-entry guard cases currently in active debug focus
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_vector_load_sees_old_16b_image_corner);
// Rewritten from the invalid PBMT=NC old-image observer: handler-head vector load must retrap with access fault.
// TEST_REGISTER(ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_vector_load_retrap_access_fault_no_ghost_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_vector_load_sees_latest_16b_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_load_sees_old_idx_and_idx_plus_one_image_corner);
// AI active register sync: cases added/updated in current workspace after last registration pass
// ai_test_cases/memblock/csr/ai_arch_custom_csr_cases.c
// TEST_REGISTER(ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner);
// TEST_REGISTER(ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_page_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_page_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_repeated_page_fault_silent_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_access_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_micro_hs_prefetch_access_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_same_block_cbo_zero_zeroes_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_head_cross16b_store_overlays_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_vector_third_element_page_fault_preserves_old_fault_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_mshr_pressure_then_same_block_cbo_zero_zeroes_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_mshr_pressure_then_head_cross16b_store_overlays_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_mshr_pressure_then_vector_third_element_page_fault_preserves_old_fault_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_same_block_cbo_zero_zeroes_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_head_cross16b_store_overlays_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_vector_third_element_page_fault_preserves_old_fault_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_same_paddr_alias_same_block_cbo_zero_zeroes_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_same_paddr_alias_head_cross16b_store_overlays_refilled_block_corner);
// TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_same_paddr_alias_vector_third_element_page_fault_preserves_old_fault_block_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_lrsc_no_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cold_lr_refill_replaces_previous_full_address_reservation_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_lrsc_no_stale_zero_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_lrsc_no_stale_zero_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_lrsc_cbo_zero_no_stale_sc_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_lrsc_cbo_zero_no_stale_sc_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_amo_no_stale_zero_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_amo_no_stale_zero_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_amo_cbo_zero_no_stale_amo_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_amo_cbo_zero_no_stale_amo_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_amo_no_stale_old_value_or_lost_update_corner);
// ai_test_cases/memblock/atomic_lrsc/ai_micro_atomic_release_pressure_cases.c
TEST_REGISTER(ai_micro_hs_atomic_load0_vector_lrsc_pressure_no_early_release_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_partial_store_no_stale_zero_or_byte_mask_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_partial_store_no_stale_zero_or_byte_mask_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_partial_store_forward_merge_no_stale_bytes_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_partial_store_cbo_zero_no_stale_mask_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_partial_store_cbo_zero_no_stale_mask_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_cross16_store_no_stale_zero_or_split_payload_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cross16_store_cbo_zero_no_stale_split_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_store_cbo_zero_no_stale_split_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_cross16_load_no_stale_zero_or_merge_payload_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cross16_load_no_stale_zero_or_merge_payload_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cross16_load_cbo_zero_no_stale_merge_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_load_cbo_zero_no_stale_merge_or_zero_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_cross16_load_no_stale_owner_or_merge_payload_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cross16_load_cbom_no_stale_merge_or_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_load_cbom_no_stale_merge_or_cmo_owner_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_cross16_load_no_stale_owner_or_merge_payload_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_lrsc_no_stale_owner_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_lrsc_no_stale_owner_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_lrsc_cbom_no_stale_sc_or_cmo_owner_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_lrsc_cbom_no_stale_sc_or_cmo_owner_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_amo_no_stale_owner_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_amo_no_stale_owner_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_amo_cbom_no_stale_amo_or_cmo_owner_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_amo_cbom_no_stale_amo_or_cmo_owner_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_fence_post_store_no_stale_cmo_fence_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_fence_cbom_post_store_no_stale_fence_cmo_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_fence_post_store_no_stale_cmo_fence_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_fence_cbom_post_store_no_stale_fence_cmo_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_fence_post_store_no_stale_zero_fence_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_fence_cbo_zero_post_store_no_stale_prior_zero_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbom_post_store_no_stale_zero_cmo_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_cbo_zero_post_store_no_stale_cmo_zero_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_inval_post_store_no_stale_zero_inval_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_inval_cbo_zero_post_store_no_stale_inval_zero_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_post_store_no_stale_repeated_zero_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cbom_post_store_no_stale_repeated_zero_cmo_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cbo_inval_post_store_no_stale_repeated_zero_inval_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_fence_post_store_no_stale_repeated_zero_fence_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_lrsc_no_stale_repeated_zero_reservation_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_amo_no_stale_repeated_zero_amo_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_partial_store_no_stale_repeated_zero_byte_mask_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_no_stale_repeated_zero_split_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_no_stale_repeated_zero_split_merge_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_amo_no_stale_repeated_zero_merge_or_atomic_owner_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_lrsc_no_stale_repeated_zero_merge_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_lrsc_no_stale_repeated_zero_split_payload_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_amo_no_stale_repeated_zero_split_payload_or_atomic_owner_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_fence_post_store_no_stale_repeated_zero_split_fence_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_fence_post_store_no_stale_repeated_zero_split_fence_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_cbom_post_store_no_stale_repeated_zero_split_cmo_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_normal_vector_no_stale_zero_vector_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_vsegment_no_stale_zero_segment_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_vfof_no_stale_zero_fof_payload_or_vl_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_all_inactive_masked_vstore_no_source_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_all_inactive_masked_vload_no_dest_clobber_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_partial_active_masked_vload_no_lane_mix_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_partial_active_masked_vstore_no_lane_mix_or_marker_drop_corner);
TEST_REGISTER(ai_micro_memblock_lr_load_trigger_breakpoint_does_not_create_reservation_corner);
TEST_REGISTER(ai_micro_memblock_lr_store_trigger_does_not_fire_on_lr_then_fresh_lrsc_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_load_trigger_breakpoint_suppresses_rmw_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_store_trigger_breakpoint_suppresses_rmw_then_clear_succeeds_corner);
// TEST_REGISTER(ai_micro_memblock_sc_store_trigger_breakpoint_blocks_commit_then_fresh_lrsc_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_sc_load_trigger_does_not_fire_on_sc_then_fresh_lrsc_succeeds_corner);
// TEST_REGISTER(ai_micro_memblock_amoadd_load_store_chain_breakpoint_suppresses_rmw_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_upper_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_upper_word_load_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_sc_w_upper_word_load_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_upper_word_store_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_upper_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
// TEST_REGISTER(ai_micro_memblock_sc_w_upper_word_store_trigger_breakpoint_blocks_commit_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_lower_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_lower_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_lower_word_load_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_lower_word_load_store_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_upper_word_load_store_trigger_no_reservation_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_lr_d_load_store_trigger_no_reservation_then_fresh_lrsc_updates_doubleword_corner);
TEST_REGISTER(ai_micro_memblock_lr_d_load_store_trigger_repeated_breakpoint_no_reservation_then_fresh_lrsc_updates_doubleword_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_lower_word_store_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_sc_w_lower_word_load_trigger_does_not_fire_then_fresh_lrsc_updates_lane_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_lower_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_lower_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_upper_word_load_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_upper_word_store_trigger_breakpoint_suppresses_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_upper_word_load_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_upper_word_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_lower_word_load_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_lower_word_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_upper_word_store_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_upper_word_load_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_lower_word_load_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_lower_word_store_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_upper_word_load_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_upper_word_store_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_lower_word_load_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_lower_word_store_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_upper_word_load_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_upper_word_store_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_lower_word_load_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_lower_word_store_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_lower_word_load_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_lower_word_store_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_upper_word_load_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_upper_word_store_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_lower_word_load_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_lower_word_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_upper_word_load_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_upper_word_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_upper_word_store_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_upper_word_load_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_lower_word_load_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_lower_word_store_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_d_load_trigger_breakpoint_suppresses_logic_and_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_d_store_trigger_breakpoint_suppresses_logic_and_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_d_load_trigger_breakpoint_suppresses_logic_or_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_d_store_trigger_breakpoint_suppresses_logic_or_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_d_load_trigger_breakpoint_suppresses_logic_xor_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_d_store_trigger_breakpoint_suppresses_logic_xor_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_d_load_trigger_breakpoint_suppresses_signed_min_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_d_store_trigger_breakpoint_suppresses_signed_min_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_d_load_trigger_breakpoint_suppresses_unsigned_min_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_d_store_trigger_breakpoint_suppresses_unsigned_min_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_d_load_trigger_breakpoint_suppresses_signed_max_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_d_store_trigger_breakpoint_suppresses_signed_max_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_d_load_trigger_breakpoint_suppresses_unsigned_max_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_d_store_trigger_breakpoint_suppresses_unsigned_max_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_d_load_trigger_breakpoint_suppresses_swap_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_d_store_trigger_breakpoint_suppresses_swap_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_d_load_store_trigger_breakpoint_suppresses_swap_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_d_load_store_trigger_breakpoint_suppresses_unsigned_max_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_d_load_store_trigger_breakpoint_suppresses_signed_min_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_d_load_store_trigger_breakpoint_suppresses_unsigned_min_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_d_load_store_trigger_breakpoint_suppresses_signed_max_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_d_load_store_trigger_breakpoint_suppresses_logic_and_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_d_load_store_trigger_breakpoint_suppresses_logic_or_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_d_load_store_trigger_breakpoint_suppresses_logic_xor_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_d_load_store_trigger_breakpoint_suppresses_add_doubleword_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_lower_word_load_store_trigger_breakpoint_suppresses_add_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_d_load_store_trigger_repeated_breakpoint_suppresses_add_then_clear_succeeds_corner);
// TEST_REGISTER(ai_micro_memblock_amoadd_d_chain_slot0_mismatch_suppresses_sibling_slot1_match_corner);
TEST_REGISTER(ai_micro_memblock_sc_d_chain_slot0_mismatch_suppresses_sibling_slot1_commit_corner);
TEST_REGISTER(ai_micro_memblock_lr_d_chain_slot0_mismatch_suppresses_sibling_slot1_load_reservation_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_lr_d_chain_slot0_store_access_mismatch_suppresses_sibling_slot1_load_reservation_corner);
TEST_REGISTER(ai_micro_memblock_sc_d_chain_slot0_load_access_mismatch_suppresses_sibling_slot1_store_commit_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_upper_chain_slot0_store_access_mismatch_suppresses_sibling_slot1_load_reservation_corner);
TEST_REGISTER(ai_micro_memblock_sc_w_upper_chain_slot0_load_access_mismatch_suppresses_sibling_slot1_store_commit_corner);
TEST_REGISTER(ai_micro_memblock_sc_w_lower_chain_slot0_load_access_mismatch_suppresses_sibling_slot1_store_commit_corner);
TEST_REGISTER(ai_micro_memblock_lr_w_lower_chain_slot0_store_access_mismatch_suppresses_sibling_slot1_load_reservation_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_logic_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_logic_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_logic_or_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_logic_xor_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_logic_or_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_signed_max_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_min_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_max_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_signed_min_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_d_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_max_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomin_d_chain_slot0_mismatch_suppresses_sibling_slot1_signed_min_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomax_d_chain_slot0_mismatch_suppresses_sibling_slot1_signed_max_commit_corner);
TEST_REGISTER(ai_micro_memblock_amominu_d_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_min_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_signed_max_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_max_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_upper_chain_slot0_mismatch_suppresses_sibling_slot1_logic_xor_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_signed_min_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_lower_chain_slot0_mismatch_suppresses_sibling_slot1_unsigned_min_lane_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoand_d_chain_slot0_mismatch_suppresses_sibling_slot1_logic_and_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoor_d_chain_slot0_mismatch_suppresses_sibling_slot1_logic_or_commit_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_d_chain_slot0_mismatch_suppresses_sibling_slot1_logic_xor_commit_corner);
// AI case batch P15CJ: memblock suspected bug corner, same-VA partial-store narrow-load forwarding signedness subset
TEST_REGISTER(ai_micro_hs_same_vaddr_partial_store_forward_narrow_load_sign_zero_extension_corner);
// AI case batch P15CK: memblock suspected bug corner, overlapping partial-store byte-age halfword forwarding subset
TEST_REGISTER(ai_micro_hs_same_vaddr_overlapping_partial_store_halfword_forward_byte_age_sign_zero_extension_corner);
// AI case batch P15CL: memblock suspected bug corner, overlapping partial-store byte-age word forwarding subset
TEST_REGISTER(ai_micro_hs_same_vaddr_overlapping_partial_store_word_forward_byte_age_sign_zero_extension_corner);
// AI case batch P15CM: memblock suspected bug corner, youngest byte clears forwarded word sign bit subset
TEST_REGISTER(ai_micro_hs_same_vaddr_overlapping_partial_store_word_forward_youngest_byte_clears_sign_extension_corner);
// AI case batch P15CN: memblock suspected bug corner, disjoint partial-store no-false-forward narrow-load subset
TEST_REGISTER(ai_micro_hs_same_vaddr_partial_store_disjoint_narrow_load_no_false_forward_sign_zero_extension_corner);
TEST_REGISTER(ai_micro_memblock_amoadd_w_upper_word_load_store_trigger_breakpoint_suppresses_add_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_lower_word_load_store_trigger_breakpoint_suppresses_replacement_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoswap_w_upper_word_load_store_trigger_breakpoint_suppresses_replacement_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_lower_word_load_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomin_w_upper_word_load_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_lower_word_load_store_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amominu_w_upper_word_load_store_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_lower_word_load_store_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomax_w_upper_word_load_store_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_lower_word_load_store_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amomaxu_w_upper_word_load_store_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_lower_word_load_store_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoand_w_upper_word_load_store_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_lower_word_load_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoor_w_upper_word_load_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_lower_word_load_store_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_memblock_amoxor_w_upper_word_load_store_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_fence_post_store_no_stale_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_store_cbom_no_stale_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_store_no_stale_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_partial_store_no_stale_owner_or_byte_mask_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_partial_store_no_stale_owner_or_byte_mask_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_cross16_store_no_stale_owner_or_split_payload_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_cross16_store_no_stale_owner_or_split_payload_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_store_cbom_no_stale_split_or_cmo_owner_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_lrsc_no_stale_forward_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_amo_no_stale_old_value_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_forward_merge_no_stale_bytes_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_lrsc_no_stale_merged_old_value_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_amo_no_stale_merged_old_value_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_fence_post_store_no_stale_merged_prior_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_cbom_no_stale_merged_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_fence_post_store_no_stale_prior_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_lrsc_no_stale_forward_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_amo_no_stale_old_value_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_forward_merge_no_stale_bytes_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_lrsc_no_stale_merged_old_value_or_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_amo_no_stale_merged_old_value_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_fence_post_store_no_stale_merged_prior_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_cbom_no_stale_merged_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_fence_post_store_no_stale_prior_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_lrsc_no_reservation_loss_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_amo_no_stale_old_value_or_lost_update_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_partial_store_forward_merge_no_stale_bytes_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_fence_post_store_no_stale_owner_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_store_cbom_no_stale_payload_or_marker_drop_corner);
TEST_REGISTER(ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_store_no_stale_owner_or_marker_drop_corner);
// Manual/nongate: non-replay_escape smblockctl fields are WARL/unimplemented.
// AP-20 manual/nongate: SPFctl AGT request generation is an RTL/perf/trace topic;
// these cases cover CSR readback, field isolation, and architectural no-regression only.
// TEST_REGISTER(ai_arch_smblockctl_non_replay_escape_warl_fields_probe_corner);
// TEST_REGISTER(ai_arch_memblock_custom_csr_privilege_wpri_warl_matrix_corner);
// TEST_REGISTER(ai_arch_memblock_custom_csr_trap_handler_snapshot_restore_corner);
// TEST_REGISTER(ai_arch_spfctl_smblockctl_reset_default_readback_matrix_corner);
// TEST_REGISTER(ai_arch_scec_sceic_baseline_field_privilege_matrix_corner);
// TEST_REGISTER(ai_arch_spfctl_disable_prefetch_paths);
// TEST_REGISTER(ai_arch_spfctl_threshold_stride_programmable_corner);
// TEST_REGISTER(ai_arch_spfctl_disable_then_threshold_stride_preserves_disabled_paths_corner);
// TEST_REGISTER(ai_arch_smblockctl_soft_prefetch_bit_clear_no_regression_corner);
// TEST_REGISTER(ai_arch_smblockctl_misaligned_readback_programming_aligned_access_unaffected_corner);
// TEST_REGISTER(ai_arch_spfctl_disable_prefetch_paths_explicit_access_correct_corner);
// TEST_REGISTER(ai_arch_spfctl_threshold_stride_stream_correct_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_toggle_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_threshold_programmable_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_disable_then_threshold_preserved_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_fault_metadata_recovery_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_amo_cbo_no_regression_corner);
// TEST_REGISTER(ai_arch_smblockctl_legacy_bits_amo_cbo_fault_no_regression_corner);
// TEST_REGISTER(ai_arch_spfctl_agt_pht_train_on_hit_cross_readback_corner);
// TEST_REGISTER(ai_arch_spfctl_agt_disable_then_pht_threshold_stride_preserved_corner);
// TEST_REGISTER(ai_arch_spfctl_pht_disable_then_agt_threshold_stride_preserved_corner);
// TEST_REGISTER(ai_arch_spfctl_enable_stride_toggle_readback_no_regression_corner);
// TEST_REGISTER(ai_arch_spfctl_agt_pht_disable_explicit_prefetch_silent_corner);
// TEST_REGISTER(ai_arch_spfctl_train_on_hit_toggle_hit_stream_correct_corner);
// TEST_REGISTER(ai_arch_spfctl_cross_reprogram_cbo_fence_i_fetch_no_regression_corner);
// TEST_REGISTER(ai_arch_spfctl_pbmt_nc_io_prefetch_silent_no_side_effect_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_small_threshold_ldld_replay_progress_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_disable_small_threshold_dense_replay_no_regression_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_small_threshold_dense_replay_progress_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_disable_small_threshold_ldld_replay_no_regression_corner);
// Manual/long-run: keep out of default selfcheck; these pass only under extended-timeout runs.
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_threshold_sweep_replay_wrapper_matrix_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_disable_small_threshold_replay_queue_multi_round_stress_no_regression_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_disable_threshold_sweep_replay_wrapper_matrix_no_regression_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_small_threshold_replay_queue_multi_round_stress_progress_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_toggle_between_wrapper_and_stress_rounds_no_state_leak_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_zero_threshold_pair_round_progress_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_zero_threshold_ldld_flushafter_store_progress_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_zero_threshold_flushafter_freelist_no_dup_free_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_mab_redirect_adjacent_port_manual_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_enable_max_threshold_replay_queue_stress_progress_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_disable_zero_threshold_replay_queue_stress_no_regression_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_toggle_zero_and_max_threshold_between_modes_no_state_leak_corner);
// TEST_REGISTER(ai_arch_smblockctl_replay_escape_head_service_paths_remain_serialized_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_load_miss_lqidx_followup_cases.c
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_miss_then_ldld_replay_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_miss_then_dense_replay_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_miss_then_replay_queue_wrap_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_miss_then_raw_replay_alloc_free_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_mixed_miss_sources_then_replay_bundle_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_cross16_miss_then_dual_replay_round_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_triple_round_miss_replay_interleave_progress_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_alias_miss_then_replay_no_stuck_regression_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_load_miss_lqidx_keyword_cases.c
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_0_then_32_noise_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_32_then_0_noise_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_wrap_pressure_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_dual_line_merge_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_pingpong_noise_storm_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_dual_line_interleave_storm_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_same_line_consumer_storm_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_age_pressure_consumer_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_wrap_window_0_32_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_wrap_window_32_0_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_store_forward_after_miss_data_corner);
// TEST_REGISTER(ai_micro_hs_load_miss_lqidx_keyword_dual_block_wrap_pressure_data_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_ap25_fast_replay_assert_cases.c
// TEST_REGISTER(ai_micro_ap25_fast_replay_vector_issue_progress_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap25_fast_replay_prefetch_dcache_pressure_manual_smoke_corner);
TEST_REGISTER(ai_micro_ap25_fast_replay_redirect_kills_wrong_path_cold_load_no_marker_poison_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_dcache_refill_forward_owner_cases.c
TEST_REGISTER(ai_micro_hs_missqueue_multisource_refill_vector_window_no_halfline_cross_source_corner);
TEST_REGISTER(ai_micro_hs_same_set_way_training_dependent_store_no_wrong_way_payload_corner);
TEST_REGISTER(ai_micro_hs_dcache_refill_tagwrite_backpressure_held_load_owner_no_wrong_line_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_dcache_banked_read_owner_cases.c
TEST_REGISTER(ai_micro_hs_bankeddata_three_read_replay_pressure_no_starvation_or_cross_payload_corner);
TEST_REGISTER(ai_micro_hs_bankeddata_amo_readline_vs_load_vector_no_wrong_line_or_halfwindow_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mainpipe_replace_quarantine_cases.c
TEST_REGISTER(ai_micro_hs_mainpipe_amo_after_dirty_pressure_no_ghost_replace_release_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_load_firstissue_quarantine_cases.c
TEST_REGISTER(ai_micro_hs_load_firstissue_scalar_vector_retry_no_stale_source_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_lsq_bug_manual_smoke_cases.c
// Manual RTL-only AP-106/AP-122/AP-125/AP-129 LSQ bug smoke. These cases stress architectural symptoms only;
// waveform/assertion checks are still required for ldld_nuke_query/updatePaddrValid,
// StoreQueue redirect cancel count/sqEmpty/StExcpBlockUnit, prefetch ignoreRARCheck lifetime, and RAWQ lifetime closure.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_micro_ap106_vecst_then_scalar_crosspage_store_fault_cancel_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap106_scalar_fault_redirect_then_dense_store_enqueue_recovers_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap106_repeated_scalar_store_fault_no_stale_sq_nonempty_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap106_vector_committed_entry_survives_scalar_redirect_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap106_mixed_vector_scalar_redirect_wrap_pressure_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap122_non_aligned_old_load_rar_query_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap122_mab_split_alias_reuse_no_paddr_pollution_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap125_prefetch_then_same_line_demand_load_rar_ignore_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap125_prefetch_repeated_faulting_alias_ignore_lifetime_reuse_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap129_vluxei64_fault1_delayed_store_rawq_manual_smoke_corner);
// TEST_REGISTER(ai_micro_ap129_vluxei64_fault2_dual_prefix_rawq_manual_smoke_corner);
// TEST_REGISTER(ai_micro_lsq_mab_prefetch_vector_multiflow_rar_manual_corner);
// TEST_REGISTER(ai_arch_spfctl_agt_pht_disable_repeated_page_fault_prefetch_silent_corner);
// TEST_REGISTER(ai_arch_spfctl_train_on_hit_toggle_prefetch_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_arch_spfctl_train_on_hit_toggle_prefetch_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_spfctl_agt_pht_disable_prefetch_access_fault_then_valid_access_recovers_corner);
// TEST_REGISTER(ai_arch_spfctl_agt_pht_disable_prefetch_access_fault_then_valid_access_adjacent_preserved_corner);
// TEST_REGISTER(ai_arch_spfctl_cross_reprogram_twice_stream_stable_corner);
	// TEST_REGISTER(ai_arch_spfctl_sms_agt_pht_disable_then_reenable_stream_observable_corner);
	// TEST_REGISTER(ai_arch_spfctl_sms_agt_pht_stride_all_on_off_observable_corner);
	// ai_test_cases/memblock/exception_trap/ai_micro_cache_error_nmi_cases.c
	// LinkNan frontend ICache data-ECC injection uses custom SCEIC/SCEC and RTL cache arrays; official Spike has no cache/ECC model.
	// reason_code: D-MANUAL-NONGATE
	// TEST_REGISTER(ai_micro_m_icache_data_ecc_inject_fetch_progress_reports_scec_corner);
	// TEST_REGISTER(ai_micro_m_icache_data_ecc_multi_rearm_doubleline_fetch_reports_scec_corner);
	// TEST_REGISTER(ai_micro_m_dcache_data_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner);
	// TEST_REGISTER(ai_micro_m_dcache_data_ecc_1bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner);
	// TEST_REGISTER(ai_micro_m_dcache_data_ecc_2bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_ecc_nmi31_interleaved_hs_tlb_load_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_ecc_nmi31_head_load_younger_marker_progress_corner);
// TEST_REGISTER(ai_micro_m_dcache_tag_ecc_inject_interleaved_hs_tlb_load_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_tag_ecc_1bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_tag_ecc_2bit_inject_interleaved_hs_tlb_load_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_tag_ecc_dense_dual_inject_interleaved_hs_tlb_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_tag_ecc_dense_dual_1bit_inject_interleaved_hs_tlb_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_tag_ecc_dense_dual_2bit_inject_interleaved_hs_tlb_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_tag_dense_dual_nmi31_interleaved_hs_tlb_progress_no_stuck_corner);
// TEST_REGISTER(ai_micro_m_dcache_data_ecc_nmi31_rearm_same_line_multi_hit_progress_corner);
// TEST_REGISTER(ai_micro_m_cbo_flush_dirty_line_dcache_data_ecc_nmi31_converges_corner);
// TEST_REGISTER(ai_micro_m_l2tlb_l1_ecc_inject_repeated_hs_page_walk_debug_manual_corner);
// TEST_REGISTER(ai_micro_m_l2tlb_l1_ecc_non_l2tlb_source_hs_page_walk_not_consumed_manual_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_aligned_recovery_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_same_boundary_aligned_load_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_load_preserves_seeded_high_half_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_aligned_store_then_valid_upper_half_aligned_store_preserves_low_half_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_load_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_low_half_aligned_store_preserves_high_half_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_valid_upper_half_aligned_load_reads_committed_high_half_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_upper_half_aligned_store_then_repeat_upper_half_aligned_store_preserves_seeded_low_half_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_upper_load_consistent_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_load_consistent_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_upper_load_low_store_preserves_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_load_preserves_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_load_low_store_preserves_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_upper_store_preserves_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_low_store_repeat_low_store_preserves_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_repeat_upper_store_preserves_latest_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_upper_store_low_store_preserves_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_low_load_upper_load_consistent_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_aligned_recovery_ext_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_preserves_seeded_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_preserves_seeded_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_store_preserves_latest_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_store_preserves_latest_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_preserves_seeded_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_preserves_seeded_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_reads_latest_low_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_low_load_then_upper_load_observes_latest_low_and_seeded_high_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_reads_latest_high_half_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_upper_load_then_low_load_observes_latest_high_and_seeded_low_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_low_load_then_upper_load_observes_seeded_low_and_latest_high_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_upper_load_then_low_load_observes_seeded_high_and_latest_low_no_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_reports_precise_fault_and_preserves_latest_high_half_corner);
// ai_test_cases/memblock/mix/ai_micro_mmode_memblock_cases.c
// TEST_REGISTER(ai_micro_m_load_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_page_fault_tval_refreshes_back_to_low_half_after_high_half_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner);
// TEST_REGISTER(ai_micro_m_cross_page_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_cross_page_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner);
// TEST_REGISTER(ai_micro_m_cross_page_access_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_cross_page_access_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_page_fault_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_access_fault_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_page_fault_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_access_fault_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_page_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_page_high_half_access_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_page_fault_repeated_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_page_high_half_access_fault_repeated_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_single_step_exception_multi_source_same_cycle_oldest_must_win_diagnostic_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_repeated_access_fault_then_same_address_valid_atomic_recovers_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_repeated_access_fault_then_same_address_valid_lr_recovers_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_repair_then_different_address_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_repair_then_different_address_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_repair_then_different_address_load_fault_refreshes_new_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_repair_then_different_address_atomic_fault_refreshes_new_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_load_then_amo_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_store_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_store_then_amo_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_load_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_load_then_lr_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_store_then_lr_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_amo_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_lr_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_lr_fault_switch_lr_then_lr_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_amo_fault_switch_amo_then_amo_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_load_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_store_fault_refreshes_new_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_store_fault_refreshes_new_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_load_fault_refreshes_new_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_address_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_width_signedness_switch_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_width_signedness_switch_recovers_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_same_width_template_switch_no_stale_merge_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_no_stale_merge_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_same_boundary_new_fault_refreshes_current_template_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_original_boundary_new_fault_refreshes_back_template_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_returned_cross_producer_store_fault_refreshes_back_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_cross_producer_store_fault_refreshes_current_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_signedness_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_load_recovers_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_address_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_load_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_store_keeps_latest_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_no_stale_template_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_no_stale_template_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_state_not_rebound_to_next_boundary_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_no_fake_crosspage_rebind_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_refreshes_current_template_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_new_fault_refreshes_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_refreshes_back_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_adjacent_preserved_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_original_boundary_new_fault_refreshes_back_template_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_new_fault_refreshes_current_template_tval_corner);
// TEST_REGISTER(ai_micro_m_pbmt_nc_replay_head_narrow_scalar_signedness_matches_current_uop_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_nc_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bus_error_then_same_alias_good_pbmt_io_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bus_error_then_same_width_signedness_switch_recovery_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bad_good_next_bad_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_bad_good_store_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_bad_good_load_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_nc_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_next_bad_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_io_bad_good_next_bad_store_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_next_bad_load_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_pbmt_io_load_store_oldest_wins_diagnostic_corner);
// TEST_REGISTER(ai_micro_m_pbmt_mixed_nc_load_io_store_oldest_wins_diagnostic_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_fault_refreshes_to_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_load_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_store_width_switch_refreshes_new_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_store_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_load_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_load_pbmt_narrow_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_store_pbmt_bad_good_then_same_alias_bad_width_switch_refreshes_current_alias_corner);
// TEST_REGISTER(ai_micro_m_pbmt_mixed_nc_narrow_load_io_width_switch_store_oldest_wins_diagnostic_corner);
// ai_test_cases/memblock/mix/ai_micro_mmode_memblock_followup_cases.c
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_cross_producer_load_fault_refreshes_back_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_next_boundary_valid_different_template_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_different_template_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_cross_producer_load_fault_refreshes_current_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_two_cross_producer_load_faults_refresh_latest_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_refreshes_latest_cause_tval_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_valid_template_load_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_valid_template_load_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_next_boundary_valid_template_load_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_template_load_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_returned_valid_template_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_next_boundary_valid_template_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_then_returned_cross_producer_load_fault_then_returned_valid_template_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_next_boundary_valid_template_store_clears_stale_exception_corner);
// TEST_REGISTER(ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_returned_then_same_boundary_cross_producer_load_fault_then_same_boundary_valid_different_template_store_clears_stale_exception_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_mab_back_to_back_cases.c
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_load_fault_repair_back_to_back_load_no_stale_merge_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_fault_then_4b4b_recovery_no_stale_merge_template_corner);
// Manual/nongate: policy-sensitive partial-store repair diagnostic.
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_repair_back_to_back_store_no_stale_split_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_load_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_load_fault_then_other_pair_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_fault_then_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_5b3b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_6b2b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_7b1b_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_7b1b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_6b2b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_5b3b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_5b3b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_6b2b_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_split_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner);
// TEST_REGISTER(ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_mab_flush_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_load_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_boundary_different_template_store_then_original_template_store_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_then_reuse_fault_template_store_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_twice_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_twice_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_twice_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_4b4b_store_twice_after_1b7b_fault_no_stale_exception_after_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_template_store_then_different_template_store_no_stale_exception_after_1b7b_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_same_fault_template_store_then_different_template_store_no_stale_exception_after_4b4b_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_different_template_store_no_stale_exception_after_4b4b_mab_path_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_valid_3b5b_store_no_stale_exception_after_2b6b_mab_path_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_refault_ext_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_reports_precise_fault_and_preserves_seeded_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_upper_load_clears_stale_exception_and_reads_seeded_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_load_clears_stale_exception_and_reads_refault_updated_low_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_low_store_repeat_low_store_repeat_low_store_repeat_low_store_refault_then_low_store_clears_stale_exception_and_commits_latest_low_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_clears_stale_exception_and_commits_latest_high_half_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_refault_followup_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_low_load_clears_stale_exception_and_observes_refault_updated_low_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_upper_load_clears_stale_exception_and_reads_latest_high_half_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_byte_load_clears_stale_exception_and_reads_refault_updated_byte7_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_byte_load_clears_stale_exception_and_reads_latest_high_half_byte0_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_halfword_load_clears_stale_exception_and_reads_seeded_byte6_plus_refault_updated_byte7_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_halfword_load_clears_stale_exception_and_reads_latest_high_half_bytes01_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_tail_word_load_clears_stale_exception_and_reads_seeded_bytes4to6_plus_refault_updated_byte7_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross_16b_store_fault_repair_then_upper_store_repeat_upper_store_repeat_upper_store_repeat_upper_store_refault_then_upper_store_then_first_upper_word_load_clears_stale_exception_and_reads_latest_high_half_bytes0to3_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_byte4_exact_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_bridge_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_second_upper_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_center_semantic_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_tail_bridge_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_adjacent_bridge_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_high_half_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_same_template_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_tail_last_halfword_low_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_tail_last_halfword_high_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_low_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_high_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_tail_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_tail_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_tail_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_tail_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_tail_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_half_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_first_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_second_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_first_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_second_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_last_halfword_high_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_last_halfword_low_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_last_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_tail_bridge_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_adjacent_bridge_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_bridge_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_tail_adjacent_bridge_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_head_middle_bridge_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_word_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_second_upper_byte_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_head_middle_bridge_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_bridge_halfword_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_first_byte_load_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_byte5_internal_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_center_semantic_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_head_side_semantic_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_head_middle_bridge_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_first_upper_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_first_upper_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_second_upper_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_center_semantic_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_near_tail_semantic_byte_store_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_byte5_tail_edge_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_first_out_of_range_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_second_out_of_range_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_third_out_of_range_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_out_of_range_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_upper_tail_overlapping_out_of_range_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_first_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_exact_upper_tail_word_store_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_byte6_tail_edge_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_first_out_of_range_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_second_out_of_range_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_out_of_range_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_overlapping_out_of_range_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_upper_tail_bridge_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_exact_upper_tail_first_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_first_upper_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_semantic_word_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_semantic_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_middle_tail_bridge_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_center_semantic_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_center_semantic_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_near_tail_semantic_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_2b6b_store_then_second_upper_byte_store_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_template_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_width_switch_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_halfword_width_switch_store_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_tail_byte_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_same_address_tail_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_first_upper_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_last_halfword_low_byte_store_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_followup_upper_tail_store_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_last_halfword_high_byte_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_halfword_store_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_word_store_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_cross_window_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_middle_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_cross_window_ext_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_bridge_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_bridge_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_bridge_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_tail_byte_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_high_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_high_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_high_byte_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_tail_halfword_prewarm_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_tail_halfword_rewarm_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_tail_halfword_unlock_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_tail_low_byte_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_last_halfword_low_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_middle_tail_bridge_word_then_upper_tail_last_halfword_low_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_tail_adjacent_bridge_word_then_upper_tail_last_halfword_low_byte_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_bridge_word_chain_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_middle_tail_bridge_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_bridge_halfword_then_first_upper_word_then_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_first_upper_word_then_mid_upper_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_first_upper_word_rewarm_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_word_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_halfword_rewarm_then_upper_tail_word_then_upper_tail_bridge_halfword_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_word_then_upper_tail_halfword_then_mid_upper_second_byte_then_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_high_byte_then_upper_tail_bridge_halfword_then_upper_tail_word_then_upper_tail_halfword_rewarm_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_consumer_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_load_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_consumer_ext_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_halfword_then_first_upper_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_dual_byte_then_first_upper_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_head_single_byte_then_first_upper_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_first_upper_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_word_then_first_upper_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_then_upper_tail_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_word_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_low_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_last_halfword_high_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_second_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_upper_tail_halfword_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_cross_window_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_second_upper_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_first_upper_halfword_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/replay_retry/ai_micro_mmode_memblock_retry_high_mid_cross_window_cases.c
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_second_byte_then_upper_tail_last_halfword_corner);
// TEST_REGISTER(ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_mid_upper_halfword_then_upper_tail_last_halfword_corner);
// ai_test_cases/memblock/exception_trap/ai_micro_trap_entry_store_fault_cases.c
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_byte_load_sees_old_value_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_doubleword_load_sees_old_value_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_flush_preserves_old_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_doubleword_store_only_handler_8b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_clean_preserves_old_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_inval_preserves_old_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_zero_only_handler_zero_visible_corner);
// Manual/nongate: old PBMT=NC scalar misaligned trap-entry visibility diagnostics.
// TEST_REGISTER(ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_byte_load_sees_old_value_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner);
// Invalid old-image observer: PBMT=NC/IO misaligned vector store should fault instead of becoming a visible handler write.
// TEST_REGISTER(ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_doubleword_load_retrap_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_doubleword_store_retrap_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_doubleword_load_sees_latest_committed_value_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_flush_preserves_latest_committed_block_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_byte_load_sees_latest_value_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_doubleword_store_only_handler_8b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_clean_preserves_latest_committed_block_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_inval_preserves_latest_committed_block_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_zero_only_handler_zero_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_byte_load_sees_old_value_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_load_sees_old_idx_and_idx_plus_one_8b_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_flush_preserves_old_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_clean_preserves_old_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_inval_preserves_old_image_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_zero_only_handler_zero_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_byte_store_only_handler_byte_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_store_only_handler_16b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_store_only_handler_8b_image_visible_corner);

// Boundary suite: ordinary ISA selfchecks use Spike; IT windows require the boundary monitor.
// TEST_REGISTER(ai_micro_boundary_301_flush_addi);
// TEST_REGISTER(ai_micro_boundary_302_violation_replay_load);
// TEST_REGISTER(ai_micro_boundary_303_flush_illegal);
// TEST_REGISTER(ai_micro_boundary_304_flush_noncanonical);
// TEST_REGISTER(ai_micro_boundary_305_flush_fetch_pmp);
// TEST_REGISTER(ai_micro_boundary_306_flush_old_fetch_error);
// TEST_REGISTER(ai_micro_boundary_401_flush_first_load);
// TEST_REGISTER(ai_micro_boundary_402_flush_first_store);
// TEST_REGISTER(ai_micro_boundary_403_flush_first_amo);
// TEST_REGISTER(ai_micro_boundary_404_flush_first_load_pf);
// TEST_REGISTER(ai_micro_boundary_405_flush_first_store_pf);
// TEST_REGISTER(ai_micro_boundary_406_flush_squashed_store);
TEST_REGISTER(ai_micro_boundary_501_handler_first_addi);
TEST_REGISTER(ai_micro_boundary_502_handler_first_jal);
TEST_REGISTER(ai_micro_boundary_503_handler_first_illegal);
TEST_REGISTER(ai_micro_boundary_504_handler_first_fetch_pmp);
TEST_REGISTER(ai_micro_boundary_505_handler_first_fetch_pf);
TEST_REGISTER(ai_micro_boundary_506_handler_masked_irq);
TEST_REGISTER(ai_micro_boundary_601_handler_first_load);
TEST_REGISTER(ai_micro_boundary_602_handler_first_store);
// TEST_REGISTER(ai_micro_boundary_603_handler_first_unaligned_load);
TEST_REGISTER(ai_micro_boundary_604_handler_first_load_pf);
TEST_REGISTER(ai_micro_boundary_605_handler_first_store_pmp);
TEST_REGISTER(ai_micro_boundary_606_handler_first_load_pf_masked_irq);
TEST_REGISTER(ai_micro_boundary_701_mret_first_addi);
TEST_REGISTER(ai_micro_boundary_703_mret_first_illegal);
TEST_REGISTER(ai_micro_boundary_704_mret_first_noncanonical);
TEST_REGISTER(ai_micro_boundary_705_mret_first_fetch_pmp);
// TEST_REGISTER(ai_micro_boundary_706_sret_pending_irq);
// TEST_REGISTER(ai_micro_boundary_801_mret_first_load_mprv);
TEST_REGISTER(ai_micro_boundary_804_mret_first_load_pf);
TEST_REGISTER(ai_micro_boundary_805_mret_first_store_pf);
// TEST_REGISTER(ai_micro_boundary_806_xret_irq_before_store);
// TEST_REGISTER(ai_micro_boundary_901_bare_to_sv39_first_fetch);
// TEST_REGISTER(ai_micro_boundary_902_bare_to_sv48_first_fetch);
// TEST_REGISTER(ai_micro_boundary_903_sv39_to_sv48_first_fetch);
// TEST_REGISTER(ai_micro_boundary_904_sv48_to_sv39_noncanonical);
// TEST_REGISTER(ai_micro_boundary_906_satp_first_fetch_no_x);
TEST_REGISTER(ai_micro_boundary_1001_satp_first_load);
TEST_REGISTER(ai_micro_boundary_1002_satp_first_store);
TEST_REGISTER(ai_micro_boundary_1003_satp_first_load_pf);
TEST_REGISTER(ai_micro_boundary_1004_satp_first_store_pf);
TEST_REGISTER(ai_micro_boundary_1005_satp_first_load_pmp);

// ai_test_cases/mix/stress/ai_directed_random_stress_cases.c
TEST_REGISTER(ai_stress_directed_random_branch_mem_cleanup_signature_corner);
TEST_REGISTER(ai_stress_directed_random_vector65_redirect_signature_corner);
TEST_REGISTER(ai_stress_directed_random_three_source_dram_scoreboard_corner);
// TLB/PTW high-watermarks require LinkNan coverage or FSDB; official Spike is architecture smoke only.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_stress_directed_random_mmu_sfence_recovery_signature_corner);
// ai_test_cases/mix/stress/ai_it_random_instruction_stream_cases.c
// Software stimulus and scoreboards are default-safe; RTL high-watermark closure still requires LinkNan coverage/FSDB.
TEST_REGISTER(ai_stress_it_randomized_instruction_schedule_scoreboard_corner);

// ai_test_cases/backend/micro/ai_micro_backend_condition_stimulus_cases.c
TEST_REGISTER(ai_micro_backend_bku_zero_clz_ctz_cpop_matrix_corner);
TEST_REGISTER(ai_micro_backend_crypto_aes_sm4_sbox_exhaustive_matrix_corner);
// ai_test_cases/backend/micro/ai_micro_backend_bku_count_tree_cases.c
TEST_REGISTER(ai_micro_backend_bku_count_tree_asymmetric_zero_subblocks_corner);
// ai_test_cases/frontend/fetch/ai_micro_canonical_boundary_jump_cases.c
TEST_REGISTER(ai_micro_hs_sv39_sv48_boundary_jal_to_noncanonical_ipf_corner);
TEST_REGISTER(ai_micro_hs_sv48_boundary_taken_branch_to_noncanonical_backend_ipf_corner);
// ai_test_cases/frontend/fetch/ai_micro_sv48_upper_non_cfi_jump_cases.c
TEST_REGISTER(ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_corner);
TEST_REGISTER(ai_micro_hs_sv48_upper_canonical_jump_to_non_cfi_block_x_deny_corner);
// ai_test_cases/frontend/fetch/ai_micro_frontend_it_followup_cases.c
TEST_REGISTER(ai_micro_frontend_cross_page_fetch_fault_attribution_corner);
TEST_REGISTER(ai_micro_frontend_split_page_pmp_first_second_fault_corner);
TEST_REGISTER(ai_micro_frontend_ftq_redirect_and_pointer_pressure_corner);
TEST_REGISTER(ai_micro_frontend_refill_fencei_repeated_fetch_corner);
TEST_REGISTER(ai_micro_frontend_delayed_store_redirect_fetch_pressure_corner);
TEST_REGISTER(ai_micro_frontend_same_pa_different_vset_fetch_corner);
TEST_REGISTER(ai_micro_frontend_new_asid_root_no_sfence_fetch_corner);
TEST_REGISTER(ai_micro_frontend_split_ret_ras_return_corner);
TEST_REGISTER(ai_micro_frontend_split_branch_train_taken_then_not_taken_corner);
TEST_REGISTER(ai_micro_frontend_split_lui_fake_cjr_cebreak_then_rvc_corner);
TEST_REGISTER(ai_micro_frontend_mshr_redirect_same_line_revisit_corner);
// MEM+PBMT.NC: ordinary DRAM responder; uncached fetch needs RTL observation.
// reason_code: D-COMPILE-ONLY-ENV
// TEST_REGISTER(ai_micro_frontend_pbmt_nc_rvc_at_8byte_end_corner);
// ai_test_cases/frontend/fetch/ai_micro_frontend_it_injection_cases.c
// IT injection + fcov required; each ELF must run alone with its scenario driver.
// reason_code: D-COMPILE-ONLY-ENV
// TEST_REGISTER(ai_micro_frontend_it_second_page_corrupt_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_split_first_line_ecc_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_split_second_line_ecc_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_single_line_ecc_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_first_beat_corrupt_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_last_beat_corrupt_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_second_line_corrupt_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_redirect_late_corrupt_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_fencei_late_corrupt_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_doubleline_ecc_stall_injected_corner);
// TEST_REGISTER(ai_micro_frontend_it_corrupt_then_clean_injected_corner);
// Existing split-fetch cases reused for frontend IT rows 382, 383, 385, 386, 388.
// ai_test_cases/frontend/fetch/ai_micro_frontend_last_half_cases.c
TEST_REGISTER(ai_micro_frontend_last_half_cross_32b_branch_refetch_no_lane_drop_corner);
TEST_REGISTER(ai_micro_frontend_last_half_following_rvi_lane1_not_dropped_corner);
TEST_REGISTER(ai_micro_frontend_last_half_split_taken_branch_no_stale_fallthrough_corner);
TEST_REGISTER(ai_micro_frontend_last_half_split_jal_fixed_taken_link_no_fallthrough_corner);
TEST_REGISTER(ai_micro_frontend_last_half_split_jalr_indirect_target_link_no_fallthrough_corner);
TEST_REGISTER(ai_micro_frontend_last_half_false_midblock_notcfi_stale_ftq_seq_repair_corner);
// ai_test_cases/backend/privilege/ai_arch_privileged_condition_stimulus_cases.c
// LinkNan keeps the previous legal MPP value for reserved encoding 2; official Spike legalizes it to U.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_mstatus_mpp_reserved_encoding_no_update_corner);
// PMA CSRs are LinkNan-specific and the lock is reset-only; compile and run as an isolated ELF.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_linknan_priv_pma_next_locked_tor_blocks_previous_addr_write_corner);
// PMP lock bits are reset-only; compile and run as an isolated ELF.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_pmpcfg2_locked_entries_ignore_reserved_bits_write_corner);
TEST_REGISTER(ai_arch_pmpcfg_reserved_rw_write_warl_zero_corner);
// LinkNan applies no-effect to reserved CBIE=2; official Spike legalizes it to Off.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_senvcfg_cbie_reserved_encoding_no_update_corner);
// ai_test_cases/mix/debug_trigger/ai_arch_trigger_b2b_cases.c
TEST_REGISTER(ai_arch_trigger_b2b_execute_enable);
TEST_REGISTER(ai_arch_trigger_b2b_execute_disable);
TEST_REGISTER(ai_arch_trigger_b2b_execute_retarget);
TEST_REGISTER(ai_arch_trigger_b2b_execute_set_clear);
TEST_REGISTER(ai_arch_trigger_b2b_load_enable_disable);
TEST_REGISTER(ai_arch_trigger_b2b_store_enable_disable);
TEST_REGISTER(ai_arch_trigger_b2b_address_retarget);
TEST_REGISTER(ai_arch_trigger_b2b_access_type_switch);
TEST_REGISTER(ai_arch_trigger_b2b_tselect_write);
TEST_REGISTER(ai_arch_trigger_b2b_last_write_wins);
TEST_REGISTER(ai_arch_trigger_b2b_machine_enable);
TEST_REGISTER(ai_arch_trigger_b2b_execute_chain_update);
TEST_REGISTER(ai_arch_trigger_b2b_write_then_tselect);
// ai_test_cases/mix/debug_trigger/ai_arch_trigger_chain_warl_cases.c
// LinkNan limits trigger chains to two levels while official Spike accepts longer chains.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_trigger_adjacent_chain_warl_corner);
// ai_test_cases/backend/privilege/ai_arch_linknan_warl_difference_cases.c
// LinkNan keeps the previous legal MNPP value for reserved encoding 2.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_mnstatus_mnpp_reserved_encoding_no_update_corner);
// LinkNan applies no-effect to reserved MENVCFG.CBIE=2.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_menvcfg_cbie_reserved_encoding_no_update_corner);
// PMA CSRs are LinkNan-specific and the entry lock is reset-only; run as an isolated ELF.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_linknan_priv_pmacfg_locked_entry_ignores_config_and_address_writes_corner);
// LinkNan mcontrol6 supports only EQ/GE/LT and ACTION 0 or Debug-Mode ACTION 1.
// reason_code: D-MANUAL-NONGATE
// TEST_REGISTER(ai_arch_trigger_mcontrol6_illegal_match_action_warl_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_store_only_handler_16b_image_visible_corner);
// TEST_REGISTER(ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_store_only_handler_8b_image_visible_corner);
