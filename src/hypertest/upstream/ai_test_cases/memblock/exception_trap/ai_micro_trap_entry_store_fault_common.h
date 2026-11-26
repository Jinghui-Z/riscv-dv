#ifndef AI_MICRO_TRAP_ENTRY_STORE_FAULT_COMMON_H
#define AI_MICRO_TRAP_ENTRY_STORE_FAULT_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>
#include <string.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_byte_load_sees_old_value_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_doubleword_load_sees_old_value_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_vector_load_sees_old_16b_image_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_flush_preserves_old_image_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_addr_doubleword_store_only_handler_8b_image_visible_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_clean_preserves_old_image_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_inval_preserves_old_image_corner();
bool ai_micro_hs_sbuffer_full_store_page_fault_then_trap_entry_same_block_cbo_zero_only_handler_zero_visible_corner();
bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_byte_load_sees_old_value_corner();
bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_vector_load_retrap_access_fault_no_ghost_image_corner();
bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner();
bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner();
bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_doubleword_load_retrap_corner();
bool ai_micro_hs_sbuffer_full_pbmt_nc_misaligned_store_then_trap_entry_same_addr_doubleword_store_retrap_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_doubleword_load_sees_latest_committed_value_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_byte_store_only_handler_byte_visible_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_vector_load_sees_latest_16b_image_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_flush_preserves_latest_committed_block_image_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_byte_load_sees_latest_value_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_doubleword_store_only_handler_8b_image_visible_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_addr_vector_store_only_handler_16b_image_visible_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_clean_preserves_latest_committed_block_image_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_inval_preserves_latest_committed_block_image_corner();
bool ai_micro_hs_sbuffer_full_breakpoint_then_trap_entry_same_block_cbo_zero_only_handler_zero_visible_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_byte_load_sees_old_value_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_load_sees_old_idx_and_idx_plus_one_8b_image_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_load_sees_old_idx_and_idx_plus_one_image_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_flush_preserves_old_image_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_clean_preserves_old_image_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_inval_preserves_old_image_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_block_cbo_zero_only_handler_zero_visible_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_byte_store_only_handler_byte_visible_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_vector_store_only_handler_16b_image_visible_corner();
bool ai_micro_hs_sbuffer_full_vector_store_third_element_page_fault_then_trap_entry_same_fault_doubleword_store_only_handler_8b_image_visible_corner();
bool ai_micro_hs_vector_strided_store_late31of33_pmp_st_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner();
bool ai_micro_hs_vector_strided_store_late31of33_hs_leaf_no_write_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_fault_image_corner();
bool ai_micro_hs_vector_strided_store_late29of33_pmp_st_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_active_suffix_corner();
bool ai_micro_hs_vector_strided_store_late29of33_hs_leaf_no_write_fault_then_trap_entry_same_parameter_strided_load_sees_committed_prefix_and_old_active_suffix_corner();
bool ai_micro_ap8_older_scalar_load_fault_skips_younger_vector_store_no_wrongpath_leak_manual_smoke_corner();
bool ai_micro_ap8_masked_even_late28_fault_preserves_inactive_and_suffix_old_manual_smoke_corner();
bool ai_micro_ap8_adjacent_vector_store_after_late29_fault_is_skipped_no_wrongpath_payload_manual_smoke_corner();
bool ai_micro_ap8_prior_full_vector_store_wrap_pressure_late31_fault_prefix_survives_manual_smoke_corner();
bool ai_micro_ap8_late31_fault_then_cbo_zero_and_marker_no_stale_exception_state_manual_smoke_corner();
bool ai_micro_ap9_late31_lastflow_then_first_scalar_two_store_same_and_different_line_visible_manual_smoke_corner();
bool ai_micro_ap9_late31_lastflow_then_first_cbo_zero_marker_and_cbom_owner_visible_manual_smoke_corner();
bool ai_micro_ap9_late31_lastflow_then_narrow_and_cross16_store_halves_visible_manual_smoke_corner();
bool ai_micro_ap9_late31_lastflow_then_first_vector_store_visible_under_repeated_recovery_manual_smoke_corner();
bool ai_micro_ap54_late29_nonlastflow_fault_then_post_redirect_vector_marker_visible_under_pressure_manual_corner();
bool ai_micro_ap54_repeated_late29_fault_repair_vector_marker_loop_no_stale_flag_manual_corner();
bool ai_micro_ap54_late31_lastflow_then_new_late29_exception_keeps_suffix_old_and_next_marker_visible_manual_corner();
bool ai_micro_ap54_unitstride_first_fault_then_marker_vector_store_no_stale_inherit_manual_corner();

#define AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT 64U
#define AI_MICRO_TRAP_ENTRY_FILL_STRIDE 0x40U
#define AI_MICRO_TRAP_ENTRY_VECTOR_BYTES 16U
#define AI_MICRO_TRAP_ENTRY_BLOCK_BYTES 64U
#define AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT 16U
#define AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL 33U
#define AI_MICRO_TRAP_ENTRY_STRIDED_ACTIVE_VL 32U
#define AI_MICRO_TRAP_ENTRY_STRIDED_FAULT_IDX 31U
#define AI_MICRO_TRAP_ENTRY_STRIDED_SUFFIX_IDX 32U
#define AI_MICRO_TRAP_ENTRY_STRIDED_OFFSET 0x120U
#define AI_MICRO_TRAP_ENTRY_STRIDED_SOURCE_OFFSET 0x200U
#define AI_MICRO_TRAP_ENTRY_STRIDED_OBSERVER_SENTINEL 0x5aU
#define AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT 4U
#define AI_MICRO_TRAP_ENTRY_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_TRAP_ENTRY_PMP_TOR_NO_W 0x0dULL

static volatile uintptr_t ai_micro_trap_entry_probe_addr = 0;
static volatile uintptr_t ai_micro_trap_entry_probe_block_addr = 0;
static volatile uint64_t ai_micro_trap_entry_probe_value = 0;
static volatile uint64_t ai_micro_trap_entry_probe_fired = 0;
static volatile uintptr_t ai_micro_trap_entry_handler_pressure_base_addr = 0;
static volatile uint64_t ai_micro_trap_entry_handler_pressure_fired = 0;
static volatile uint64_t
    ai_micro_trap_entry_handler_pressure_words[
        AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT]
    __attribute__((aligned(16)));
static volatile uint64_t ai_micro_trap_entry_trap_cause = 0;
static volatile uint64_t ai_micro_trap_entry_trap_tval = 0;
static volatile uint64_t ai_micro_trap_entry_trap_epc = 0;
static volatile uint64_t ai_micro_trap_entry_trap_vstart = 0;
static volatile uint64_t ai_micro_trap_entry_retrap_cause = 0;
static volatile uint64_t ai_micro_trap_entry_retrap_tval = 0;
static volatile uint64_t ai_micro_trap_entry_retrap_epc = 0;
static volatile uint64_t ai_micro_trap_entry_retrap_vstart = 0;
static volatile uintptr_t ai_micro_trap_entry_nested_resume_epc = 0;
static volatile uintptr_t ai_micro_trap_entry_store_fix_pte_addr = 0;
static volatile uint64_t ai_micro_trap_entry_store_fix_pte_value = 0;
static volatile uintptr_t ai_micro_trap_entry_strided_probe_base_addr = 0;
static volatile intptr_t ai_micro_trap_entry_strided_probe_stride = 0;
static volatile uint64_t ai_micro_trap_entry_strided_probe_vl = 0;
static volatile uint64_t ai_micro_trap_entry_early_probe_count = 0;
static volatile uintptr_t
    ai_micro_trap_entry_early_probe_addrs[AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT];
static volatile uint8_t ai_micro_trap_entry_probe_bytes[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES]
    __attribute__((aligned(16)));
static volatile uint8_t
    ai_micro_trap_entry_strided_probe_bytes[AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL]
    __attribute__((aligned(16)));
static volatile uint8_t
    ai_micro_trap_entry_early_probe_bytes[AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT]
    __attribute__((aligned(8)));
static volatile uint8_t
    ai_micro_trap_entry_vector_store_source[AI_MICRO_TRAP_ENTRY_VECTOR_BYTES]
    __attribute__((aligned(16), used));
static uint64_t ai_micro_trap_entry_trap_scratch[8] __attribute__((aligned(16)));
static uint64_t ai_micro_trap_entry_nested_trap_scratch[8]
    __attribute__((aligned(16), used));

static inline void ai_micro_trap_entry_ebreak32(void)
{
    asm volatile (".word 0x00100073" ::: "memory");
}

#define AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM \
"    la t0, ai_micro_trap_entry_handler_pressure_base_addr\n" \
"    ld t0, 0(t0)\n" \
"    beqz t0, 9f\n" \
"    la t1, ai_micro_trap_entry_handler_pressure_words\n" \
"    ld t2, 0(t1)\n" \
"    sd t2, 0(t0)\n" \
"    ld t2, 8(t1)\n" \
"    sd t2, 64(t0)\n" \
"    ld t2, 16(t1)\n" \
"    sd t2, 128(t0)\n" \
"    ld t2, 24(t1)\n" \
"    sd t2, 192(t0)\n" \
"    ld t2, 32(t1)\n" \
"    sd t2, 256(t0)\n" \
"    ld t2, 40(t1)\n" \
"    sd t2, 320(t0)\n" \
"    ld t2, 48(t1)\n" \
"    sd t2, 384(t0)\n" \
"    ld t2, 56(t1)\n" \
"    sd t2, 448(t0)\n" \
"    ld t2, 64(t1)\n" \
"    sd t2, 512(t0)\n" \
"    ld t2, 72(t1)\n" \
"    sd t2, 576(t0)\n" \
"    ld t2, 80(t1)\n" \
"    sd t2, 640(t0)\n" \
"    ld t2, 88(t1)\n" \
"    sd t2, 704(t0)\n" \
"    ld t2, 96(t1)\n" \
"    sd t2, 768(t0)\n" \
"    ld t2, 104(t1)\n" \
"    sd t2, 832(t0)\n" \
"    ld t2, 112(t1)\n" \
"    sd t2, 896(t0)\n" \
"    ld t2, 120(t1)\n" \
"    sd t2, 960(t0)\n" \
"    la t1, ai_micro_trap_entry_handler_pressure_fired\n" \
"    li t2, 1\n" \
"    sd t2, 0(t1)\n" \
"9:\n"

extern void ai_micro_trap_entry_same_addr_byte_load_handler(void);
extern void ai_micro_trap_entry_same_addr_doubleword_load_handler(void);
extern void ai_micro_trap_entry_same_addr_doubleword_load_retrap_handler(void);
extern void ai_micro_trap_entry_same_addr_byte_store_handler(void);
extern void ai_micro_trap_entry_same_addr_doubleword_store_handler(void);
extern void ai_micro_trap_entry_same_addr_byte_store_with_pte_fix_handler(void);
extern void ai_micro_trap_entry_same_addr_doubleword_store_with_pte_fix_handler(void);
extern void ai_micro_trap_entry_same_addr_doubleword_store_retrap_handler(void);
extern void ai_micro_trap_entry_same_addr_vector_load_handler(void);
extern void ai_micro_trap_entry_same_addr_vector_load_retrap_handler(void);
extern void ai_micro_trap_entry_same_addr_vector_store_handler(void);
extern void ai_micro_trap_entry_same_addr_vector_store_with_pte_fix_handler(void);
extern void ai_micro_trap_entry_same_parameter_strided_vector_load_handler(void);
extern void ai_micro_trap_entry_same_block_cbo_clean_handler(void);
extern void ai_micro_trap_entry_same_block_cbo_inval_handler(void);
extern void ai_micro_trap_entry_same_block_cbo_zero_with_pte_fix_handler(void);
extern void ai_micro_trap_entry_same_block_cbo_flush_handler(void);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_byte_load_handler\n"
"ai_micro_trap_entry_same_addr_byte_load_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    lbu t2, 0(t1)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    sd t2, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_doubleword_load_handler\n"
"ai_micro_trap_entry_same_addr_doubleword_load_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    ld t2, 0(t1)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    sd t2, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_byte_store_handler\n"
"ai_micro_trap_entry_same_addr_byte_store_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    ld t2, 0(t0)\n"
"    sb t2, 0(t1)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_doubleword_store_handler\n"
"ai_micro_trap_entry_same_addr_doubleword_store_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_byte_store_with_pte_fix_handler\n"
"ai_micro_trap_entry_same_addr_byte_store_with_pte_fix_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -40\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    sfence.vma zero, zero\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    ld t2, 0(t0)\n"
"    sb t2, 0(t1)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    addi sp, sp, 40\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_doubleword_store_with_pte_fix_handler\n"
"ai_micro_trap_entry_same_addr_doubleword_store_with_pte_fix_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -40\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    sfence.vma zero, zero\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    addi sp, sp, 40\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_retrap_second_level_handler\n"
"ai_micro_trap_entry_same_addr_retrap_second_level_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -48\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    sd t5, 40(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_retrap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_retrap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_retrap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_retrap_epc\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 2\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_nested_resume_epc\n"
"    ld t1, 0(t0)\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    ld t5, 40(sp)\n"
"    addi sp, sp, 48\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_doubleword_load_retrap_handler\n"
"ai_micro_trap_entry_same_addr_doubleword_load_retrap_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -64\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    sd t5, 40(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    csrr t5, sscratch\n"
"    sd t5, 48(sp)\n"
"    csrr t5, sstatus\n"
"    sd t5, 56(sp)\n"
"    la t0, ai_micro_trap_entry_nested_resume_epc\n"
"    la t1, ai_micro_trap_entry_same_addr_doubleword_load_retrap_handler_resume\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_same_addr_retrap_second_level_handler\n"
"    csrw stvec, t0\n"
"    la t0, ai_micro_trap_entry_nested_trap_scratch\n"
"    addi t0, t0, 64\n"
"    csrw sscratch, t0\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    ld t2, 0(t1)\n"
"ai_micro_trap_entry_same_addr_doubleword_load_retrap_handler_resume:\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 3\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    ld t1, 56(sp)\n"
"    csrw sstatus, t1\n"
"    ld t1, 48(sp)\n"
"    csrw sscratch, t1\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    ld t5, 40(sp)\n"
"    addi sp, sp, 64\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_doubleword_store_retrap_handler\n"
"ai_micro_trap_entry_same_addr_doubleword_store_retrap_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -64\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    sd t5, 40(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    csrr t5, sscratch\n"
"    sd t5, 48(sp)\n"
"    csrr t5, sstatus\n"
"    sd t5, 56(sp)\n"
"    la t0, ai_micro_trap_entry_nested_resume_epc\n"
"    la t1, ai_micro_trap_entry_same_addr_doubleword_store_retrap_handler_resume\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_same_addr_retrap_second_level_handler\n"
"    csrw stvec, t0\n"
"    la t0, ai_micro_trap_entry_nested_trap_scratch\n"
"    addi t0, t0, 64\n"
"    csrw sscratch, t0\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_probe_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"ai_micro_trap_entry_same_addr_doubleword_store_retrap_handler_resume:\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 3\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    ld t1, 56(sp)\n"
"    csrw sstatus, t1\n"
"    ld t1, 48(sp)\n"
"    csrw sscratch, t1\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    ld t5, 40(sp)\n"
"    addi sp, sp, 64\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_vector_load_retrap_handler\n"
"ai_micro_trap_entry_same_addr_vector_load_retrap_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -64\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    sd t5, 40(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    csrr t5, sscratch\n"
"    sd t5, 48(sp)\n"
"    csrr t5, sstatus\n"
"    sd t5, 56(sp)\n"
"    la t0, ai_micro_trap_entry_nested_resume_epc\n"
"    la t1, ai_micro_trap_entry_same_addr_vector_load_retrap_handler_resume\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_same_addr_retrap_second_level_handler\n"
"    csrw stvec, t0\n"
"    la t0, ai_micro_trap_entry_nested_trap_scratch\n"
"    addi t0, t0, 64\n"
"    csrw sscratch, t0\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    csrwi vstart, 0\n"
"    li t2, 1\n"
"    vsetvli t3, t2, e64, m1, ta, ma\n"
"    vle64.v v6, (t1)\n"
"ai_micro_trap_entry_same_addr_vector_load_retrap_handler_resume:\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 3\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    ld t1, 56(sp)\n"
"    csrw sstatus, t1\n"
"    ld t1, 48(sp)\n"
"    csrw sscratch, t1\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    ld t5, 40(sp)\n"
"    addi sp, sp, 64\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_vector_load_handler\n"
"ai_micro_trap_entry_same_addr_vector_load_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -40\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    la t0, ai_micro_trap_entry_probe_addr\n"
"    ld t1, 0(t0)\n"
"    csrr t2, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t2, 0(t0)\n"
"    csrwi vstart, 0\n"
"    li t2, 16\n"
"    vsetvli t3, t2, e8, m1, ta, ma\n"
"    vle8.v v6, (t1)\n"
"    la t4, ai_micro_trap_entry_probe_bytes\n"
"    vse8.v v6, (t4)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    addi sp, sp, 40\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_vector_store_handler\n"
"ai_micro_trap_entry_same_addr_vector_store_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -40\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    la t0, ai_micro_trap_entry_vector_store_source\n"
"    csrr t2, vstart\n"
"    la t1, ai_micro_trap_entry_trap_vstart\n"
"    sd t2, 0(t1)\n"
"    csrwi vstart, 0\n"
"    li t1, 16\n"
"    vsetvli t3, t1, e8, m1, ta, ma\n"
"    vle8.v v6, (t0)\n"
"    la t2, ai_micro_trap_entry_probe_addr\n"
"    ld t4, 0(t2)\n"
"    vse8.v v6, (t4)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    addi sp, sp, 40\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_parameter_strided_vector_load_handler\n"
"ai_micro_trap_entry_same_parameter_strided_vector_load_handler:\n"
"    csrrw t0, sscratch, t0\n"
"    sd t1, 0(t0)\n"
"    sd t2, 8(t0)\n"
"    sd t3, 16(t0)\n"
"    sd t4, 24(t0)\n"
"    sd t5, 32(t0)\n"
"    sd t6, 40(t0)\n"
"    csrr t3, vstart\n"
"    la t1, ai_micro_trap_entry_early_probe_count\n"
"    ld t2, 0(t1)\n"
"    la t4, ai_micro_trap_entry_early_probe_addrs\n"
"    la t5, ai_micro_trap_entry_early_probe_bytes\n"
"1:\n"
"    beqz t2, 2f\n"
"    ld t6, 0(t4)\n"
"    lbu t1, 0(t6)\n"
"    sb t1, 0(t5)\n"
"    addi t4, t4, 8\n"
"    addi t5, t5, 1\n"
"    addi t2, t2, -1\n"
"    j 1b\n"
"2:\n"
"    la t1, ai_micro_trap_entry_strided_probe_base_addr\n"
"    ld t1, 0(t1)\n"
"    la t2, ai_micro_trap_entry_strided_probe_stride\n"
"    ld t2, 0(t2)\n"
"    csrwi vstart, 0\n"
"    li t4, 33\n"
"    vsetvli t4, t4, e8, m2, ta, ma\n"
"    vlse8.v v8, (t1), t2\n"
"    la t1, ai_micro_trap_entry_strided_probe_bytes\n"
"    vse8.v v8, (t1)\n"
"    la t1, ai_micro_trap_entry_probe_fired\n"
"    li t2, 1\n"
"    sd t2, 0(t1)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    la t1, ai_micro_trap_entry_trap_vstart\n"
"    sd t3, 0(t1)\n"
"    la t1, ai_micro_trap_entry_strided_probe_vl\n"
"    sd t4, 0(t1)\n"
"    csrr t2, scause\n"
"    la t1, ai_micro_trap_entry_trap_cause\n"
"    sd t2, 0(t1)\n"
"    csrr t2, stval\n"
"    la t1, ai_micro_trap_entry_trap_tval\n"
"    sd t2, 0(t1)\n"
"    csrr t2, sepc\n"
"    la t1, ai_micro_trap_entry_trap_epc\n"
"    sd t2, 0(t1)\n"
"    addi t2, t2, 4\n"
"    csrw sepc, t2\n"
"    ld t1, 0(t0)\n"
"    ld t2, 8(t0)\n"
"    ld t3, 16(t0)\n"
"    ld t4, 24(t0)\n"
"    ld t5, 32(t0)\n"
"    ld t6, 40(t0)\n"
"    csrrw t0, sscratch, t0\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_addr_vector_store_with_pte_fix_handler\n"
"ai_micro_trap_entry_same_addr_vector_store_with_pte_fix_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -40\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    csrr t2, vstart\n"
"    la t1, ai_micro_trap_entry_trap_vstart\n"
"    sd t2, 0(t1)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    sfence.vma zero, zero\n"
"    la t0, ai_micro_trap_entry_vector_store_source\n"
"    csrwi vstart, 0\n"
"    li t1, 16\n"
"    vsetvli t3, t1, e8, m1, ta, ma\n"
"    vle8.v v6, (t0)\n"
"    la t2, ai_micro_trap_entry_probe_addr\n"
"    ld t4, 0(t2)\n"
"    vse8.v v6, (t4)\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    addi sp, sp, 40\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_block_cbo_clean_handler\n"
"ai_micro_trap_entry_same_block_cbo_clean_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    la t0, ai_micro_trap_entry_probe_block_addr\n"
"    ld t1, 0(t0)\n"
"    .insn i 0x0f, 0x2, x0, t1, 1\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_block_cbo_inval_handler\n"
"ai_micro_trap_entry_same_block_cbo_inval_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    la t0, ai_micro_trap_entry_probe_block_addr\n"
"    ld t1, 0(t0)\n"
"    .insn i 0x0f, 0x2, x0, t1, 0\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_block_cbo_zero_with_pte_fix_handler\n"
"ai_micro_trap_entry_same_block_cbo_zero_with_pte_fix_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -40\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    sd t3, 24(sp)\n"
"    sd t4, 32(sp)\n"
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_addr\n"
"    ld t1, 0(t0)\n"
"    la t0, ai_micro_trap_entry_store_fix_pte_value\n"
"    ld t2, 0(t0)\n"
"    sd t2, 0(t1)\n"
"    sfence.vma zero, zero\n"
"    la t0, ai_micro_trap_entry_probe_block_addr\n"
"    ld t1, 0(t0)\n"
"    .insn i 0x0f, 0x2, x0, t1, 4\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    ld t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    ld t3, 24(sp)\n"
"    ld t4, 32(sp)\n"
"    addi sp, sp, 40\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

asm(
".balign 0x4\n"
".global ai_micro_trap_entry_same_block_cbo_flush_handler\n"
"ai_micro_trap_entry_same_block_cbo_flush_handler:\n"
"    csrrw sp, sscratch, sp\n"
"    addi sp, sp, -24\n"
"    sd t0, 0(sp)\n"
"    sd t1, 8(sp)\n"
"    sd t2, 16(sp)\n"
"    la t0, ai_micro_trap_entry_probe_block_addr\n"
"    ld t1, 0(t0)\n"
"    .insn i 0x0f, 0x2, x0, t1, 2\n"
"    la t0, ai_micro_trap_entry_probe_fired\n"
"    li t1, 1\n"
"    sd t1, 0(t0)\n"
AI_MICRO_TRAP_ENTRY_HANDLER_PRESSURE_ASM
"    csrr t1, vstart\n"
"    la t0, ai_micro_trap_entry_trap_vstart\n"
"    sd t1, 0(t0)\n"
"    csrr t1, scause\n"
"    la t0, ai_micro_trap_entry_trap_cause\n"
"    sd t1, 0(t0)\n"
"    csrr t1, stval\n"
"    la t0, ai_micro_trap_entry_trap_tval\n"
"    sd t1, 0(t0)\n"
"    csrr t1, sepc\n"
"    la t0, ai_micro_trap_entry_trap_epc\n"
"    sd t1, 0(t0)\n"
"    addi t1, t1, 4\n"
"    csrw sepc, t1\n"
"    ld t0, 0(sp)\n"
"    ld t1, 8(sp)\n"
"    ld t2, 16(sp)\n"
"    addi sp, sp, 24\n"
"    csrrw sp, sscratch, sp\n"
"    sret\n"
);

static inline pte_t *ai_micro_trap_entry_hs_leaf(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_trap_entry_set_hs_leaf(enum test_page page, uint64_t flags)
{
    goto_priv(PRIV_M);
    *ai_micro_trap_entry_hs_leaf(page) = (phys_page_base(page) >> 2) | flags;
}

static inline void ai_micro_trap_entry_dense_store_tail_refresh(
    uintptr_t page_base_vaddr, uintptr_t first_line_index,
    uintptr_t line_count, uint64_t seed);

static inline void ai_micro_trap_entry_reset_local_state(uintptr_t probe_addr)
{
    __sync_synchronize();
    ai_micro_trap_entry_probe_addr = probe_addr;
    ai_micro_trap_entry_probe_block_addr =
        probe_addr & ~(uintptr_t)(AI_MICRO_TRAP_ENTRY_BLOCK_BYTES - 1U);
    ai_micro_trap_entry_probe_value = 0;
    ai_micro_trap_entry_probe_fired = 0;
    ai_micro_trap_entry_trap_cause = 0;
    ai_micro_trap_entry_trap_tval = 0;
    ai_micro_trap_entry_trap_epc = 0;
    ai_micro_trap_entry_trap_vstart = 0;
    ai_micro_trap_entry_retrap_cause = 0;
    ai_micro_trap_entry_retrap_tval = 0;
    ai_micro_trap_entry_retrap_epc = 0;
    ai_micro_trap_entry_retrap_vstart = 0;
    ai_micro_trap_entry_nested_resume_epc = 0;
    ai_micro_trap_entry_store_fix_pte_addr = 0;
    ai_micro_trap_entry_store_fix_pte_value = 0;
    ai_micro_trap_entry_handler_pressure_base_addr = 0;
    ai_micro_trap_entry_handler_pressure_fired = 0;
    ai_micro_trap_entry_strided_probe_base_addr = 0;
    ai_micro_trap_entry_strided_probe_stride = 0;
    ai_micro_trap_entry_strided_probe_vl = 0;
    ai_micro_trap_entry_early_probe_count = 0;
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_VECTOR_BYTES; ++i) {
        ai_micro_trap_entry_probe_bytes[i] = 0U;
    }
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL; ++i) {
        ai_micro_trap_entry_strided_probe_bytes[i] = 0U;
    }
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT; ++i) {
        ai_micro_trap_entry_handler_pressure_words[i] = 0ULL;
    }
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT; ++i) {
        ai_micro_trap_entry_early_probe_addrs[i] = 0U;
        ai_micro_trap_entry_early_probe_bytes[i] = 0U;
    }
    __sync_synchronize();
}

static inline uintptr_t ai_micro_trap_entry_prepare_pbmt_nc_alias(
    enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline void ai_micro_trap_entry_reset_pbmt_pbmte_only_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_micro_trap_entry_dense_store_pattern(uint64_t seed,
    uintptr_t line_index)
{
    return seed ^ ((0x9e3779b97f4a7c15ULL * (line_index + 1U)) +
        (line_index << 7));
}

static inline void ai_micro_trap_entry_dense_store_pressure_window(
    uintptr_t page_base_vaddr, uintptr_t line_count,
    uint64_t first_seed, uint64_t second_seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        *(volatile uint64_t *)(page_base_vaddr + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE) =
            ai_micro_trap_entry_dense_store_pattern(first_seed, i);
    }

    for (uintptr_t i = 0; i < line_count; ++i) {
        *(volatile uint64_t *)(page_base_vaddr + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE) =
            ai_micro_trap_entry_dense_store_pattern(second_seed, i);
    }

    if (line_count >= AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT) {
        uintptr_t first_tail_line = line_count -
            AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT;

        ai_micro_trap_entry_dense_store_tail_refresh(page_base_vaddr,
            first_tail_line, AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT,
            second_seed);

        ai_micro_trap_entry_handler_pressure_base_addr = page_base_vaddr +
            first_tail_line * AI_MICRO_TRAP_ENTRY_FILL_STRIDE;
        ai_micro_trap_entry_handler_pressure_fired = 0;
        for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT; ++i) {
            ai_micro_trap_entry_handler_pressure_words[i] =
                ai_micro_trap_entry_dense_store_pattern(second_seed,
                    first_tail_line + i);
        }
        __sync_synchronize();
    }
}

static inline void ai_micro_trap_entry_dense_store_tail_refresh(
    uintptr_t page_base_vaddr, uintptr_t first_line_index,
    uintptr_t line_count, uint64_t seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        uintptr_t line_index = first_line_index + i;
        *(volatile uint64_t *)(page_base_vaddr +
            line_index * AI_MICRO_TRAP_ENTRY_FILL_STRIDE) =
            ai_micro_trap_entry_dense_store_pattern(seed, line_index);
    }
}

static inline void ai_micro_trap_entry_refresh_final_pressure_tail(
    uintptr_t page_base_vaddr, uint64_t seed)
{
    ai_micro_trap_entry_dense_store_tail_refresh(page_base_vaddr,
        AI_MICRO_TRAP_ENTRY_PRESSURE_LINE_COUNT -
            AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT,
        AI_MICRO_TRAP_ENTRY_FINAL_PRESSURE_LINE_COUNT, seed);
}

static inline bool ai_micro_trap_entry_dense_store_window_match(uintptr_t page_base_paddr,
    uintptr_t line_count, uint64_t seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        if (read64(page_base_paddr + i * AI_MICRO_TRAP_ENTRY_FILL_STRIDE) !=
            ai_micro_trap_entry_dense_store_pattern(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline void ai_micro_trap_entry_write_bytes(uintptr_t base_paddr,
    const uint8_t *bytes, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        write8(base_paddr + i, bytes[i]);
    }
}

static inline bool ai_micro_trap_entry_bytes_match(uintptr_t base_paddr,
    const uint8_t *bytes, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        if (read8(base_paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_trap_entry_probe_bytes_match(const uint8_t *bytes,
    uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        if (ai_micro_trap_entry_probe_bytes[i] != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_trap_entry_strided_probe_bytes_match(
    const uint8_t *bytes, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        if (ai_micro_trap_entry_strided_probe_bytes[i] != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_trap_entry_early_probe_bytes_match(
    const uint8_t *bytes, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; ++i) {
        if (ai_micro_trap_entry_early_probe_bytes[i] != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_micro_trap_entry_prepare_vector_store_source(
    const uint8_t *bytes)
{
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_VECTOR_BYTES; ++i) {
        ai_micro_trap_entry_vector_store_source[i] = bytes[i];
    }
    __sync_synchronize();
}

static inline void ai_micro_trap_entry_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, 1ULL << 9);
    CSRS(CSR_MSTATUS, 1ULL << 10);
}

static inline void ai_micro_trap_entry_prepare_strided_probe(
    uintptr_t base_addr, intptr_t stride, uint8_t sentinel)
{
    __sync_synchronize();
    ai_micro_trap_entry_strided_probe_base_addr = base_addr;
    ai_micro_trap_entry_strided_probe_stride = stride;
    ai_micro_trap_entry_strided_probe_vl = 0;
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_STRIDED_REQ_VL; ++i) {
        ai_micro_trap_entry_strided_probe_bytes[i] = sentinel;
    }
    __sync_synchronize();
}

static inline void ai_micro_trap_entry_prepare_early_byte_probes(
    const uintptr_t *probe_addrs, uintptr_t count, uint8_t sentinel)
{
    if (count > AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT) {
        count = AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT;
    }

    __sync_synchronize();
    ai_micro_trap_entry_early_probe_count = count;
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_EARLY_BYTE_PROBE_COUNT; ++i) {
        ai_micro_trap_entry_early_probe_addrs[i] =
            (i < count) ? probe_addrs[i] : 0U;
        ai_micro_trap_entry_early_probe_bytes[i] = sentinel;
    }
    __sync_synchronize();
}

static inline void ai_micro_trap_entry_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_TRAP_ENTRY_PMP_TOR_RWX);
}

static inline void ai_micro_trap_entry_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_TRAP_ENTRY_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_TRAP_ENTRY_PMP_TOR_RWX << 16));
}

static inline enum test_page ai_micro_trap_entry_strided_target_page(unsigned index)
{
    return (enum test_page)((int)VSRWX_GURWX + (int)index);
}

static inline uintptr_t ai_micro_trap_entry_strided_target_vaddr(unsigned index)
{
    return hs_page_base(ai_micro_trap_entry_strided_target_page(index)) +
        AI_MICRO_TRAP_ENTRY_STRIDED_OFFSET;
}

static inline uintptr_t ai_micro_trap_entry_strided_target_phys(unsigned index)
{
    return phys_page_base(ai_micro_trap_entry_strided_target_page(index)) +
        AI_MICRO_TRAP_ENTRY_STRIDED_OFFSET;
}

static inline void ai_micro_trap_entry_strided_fill_patterns(
    uint8_t *old_image, uint8_t *new_image, uint8_t *expected_after_fault,
    unsigned vl, unsigned fault_idx)
{
    for (unsigned i = 0; i < vl; ++i) {
        old_image[i] = (uint8_t)(0x30U + i);
        new_image[i] = (uint8_t)(0xa0U + i);
        expected_after_fault[i] = (i < fault_idx) ? new_image[i] : old_image[i];
    }
}

static inline bool ai_micro_trap_entry_strided_target_window_matches(
    const uint8_t *expected, unsigned vl)
{
    for (unsigned i = 0; i < vl; ++i) {
        if (read8(ai_micro_trap_entry_strided_target_phys(i)) != expected[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_trap_entry_strided_vaddr_window_matches(
    uintptr_t base_vaddr, intptr_t stride, const uint8_t *expected, unsigned vl)
{
    for (unsigned i = 0; i < vl; ++i) {
        uintptr_t addr = base_vaddr + (uintptr_t)((intptr_t)i * stride);
        if (lbu(addr) != expected[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_micro_trap_entry_vle8_to_v8_m2(const uint8_t *src, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 1);
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

static inline void ai_micro_trap_entry_vsse8_from_v8_m2(
    uint8_t *dst, intptr_t stride, unsigned vl)
{
    int vtype = VTYPE(VSEW_FROM_BITS(8), 1);
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %2, %3\n\t"
        "vsse8.v v8, (%0), %1\n\t"
        ".option pop\n\t"
        :
        : "r"(dst), "r"(stride), "r"(vl), "r"(vtype)
        : "t0", "memory");
}

static inline void ai_micro_trap_entry_seed_block(uintptr_t base_paddr,
    uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_BLOCK_BYTES / sizeof(uint64_t); ++i) {
        write64(base_paddr + i * sizeof(uint64_t),
            ai_micro_trap_entry_dense_store_pattern(seed, i));
    }
}

static inline bool ai_micro_trap_entry_block_matches_seed(uintptr_t base_paddr,
    uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_BLOCK_BYTES / sizeof(uint64_t); ++i) {
        if (read64(base_paddr + i * sizeof(uint64_t)) !=
            ai_micro_trap_entry_dense_store_pattern(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_trap_entry_block_is_zero(uintptr_t base_paddr)
{
    for (uintptr_t i = 0; i < AI_MICRO_TRAP_ENTRY_BLOCK_BYTES / sizeof(uint64_t); ++i) {
        if (read64(base_paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

// Move the fault earlier than the last active element so trap-entry observation
// can directly check whether the fault element and later active suffix bytes are
// still being forwarded out of the SQ window.

#endif /* AI_MICRO_TRAP_ENTRY_STORE_FAULT_COMMON_H */
